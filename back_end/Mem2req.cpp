/*******************************************************/
/*! \file  Mem2req.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert Nanotube memory interface to requests
**   \date 2020-04-07
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
 * Purpose:
 *   This pass converts packet kernels from a load / store API to an
 *   explicit request API.  In more detail, accesses to Nanotube map and
 *   packet data via loads / stores are converted to explicit calls to
 *   {nanotube_map, nanotube_packet} x {read, write}.
 *
 * Input Expectations:
 *   * input code uses the existing NT functions for accessing packet data
 *     / data_end and map_lookup
 *   * existing map operations that use the request interface rather than
 *     memory are okay, as well
 *   * map values and packet data are accessed with loads / stores
 *   * memcpy / memset to / from / of packet and map data is supported
 *
 * Output Characteristics:
 *   * remaining loads / stores are only to the stack or parameter pointers
 *   * every access to a packet / map happens through the (simplified)
 *     request API
 *   * all remaining memcpy / memset calls are only to stack / heap memory
 *   * instead of packet pointers pointing into the packet, all packet
 *     accesses use a base packet and explicit offset in the reoquest
 *
 * Limitations:
 *   * no loop support
 *   * no support for "mixed mode" accesses where one load / store can
 *     access either map or packet depending on some condition
 *   * no support for select statements; as long as the pointer base is the
 *     same, the select will only ever select the offset it seems! (but see
 *     the point above!!)
 *   * no support for map accesses that can go to different maps
 *   * (in progress) using pieces of packet as key does not work
 *
 * Mode of operation:
 *   Phase 1 does a small instruction replacement, i.e., it replaces a
 *   single load / store with a short sequence of instructions.  It also
 *   identifies the latest packet pointer, and adds that as the packet
 *   parameter, but leaves the actual into-packet pointer as the offset.
 *
 *   * Phase 1a: convert loads / stores
 *     * find out which memory is accessed by the load / store
 *     * do a fairly mechanical translation with temporary buffer memory
 *     * establish the underlying packet / map which is a packet_data or
 *       map_lookup call
 *   * Phase 1b: translate memcpy / memset
 *     * classify cases and split memcpy into a read and a write phase
 *     * allocate temporary memory if necessary (neither "end" on stack)
 *     * convert read part into the right request if necessary (no-op if
 *       from stack)
 *     * convert write part in a similar fashion
 *
 *    Phase 2 cleans up the pointers and replaces the into-packet /
 *    into-map-value pointers used in phase 1 and replaces them with packet
 *    offsets.  For that, it walks computation chains originating from
 *    nanotube_packet_data & nanotube_packet_end / map_lookup and converts
 *    instructions traversing the DAG (no loops yet!) of information flow,
 *    converting consumers one by one; effectively pushing a frontier of
 *    inttoptr instructions away from the converted nanotube_packet_data
 *    origins, all the way into the nanotube_packet_read/writes.
 *
 */

#include "Mem2req.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/ADT/SmallSet.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"

#include "Intrinsics.h"
#include "Provenance.h"
#include "Dep_Aware_Converter.h"
#include "setup_func.hpp"
#include "utils.h"

#define DEBUG_TYPE "mem2req"
#define VERBOSE_DEBUG

using namespace llvm;
using namespace nanotube;


static Value *get_pointer_operand(Instruction *inst);


raw_ostream& operator<<(raw_ostream& os, const pointer_meta& meta);


Value *get_pointer_operand(Instruction *inst) {
  switch( inst->getOpcode() ) {
    case Instruction::Load: {
      //errs() << "|" << *inst << '\n';
      auto *ld = cast<LoadInst>(inst);
      return ld->getPointerOperand();
    }
    case Instruction::Store: {
      //errs() << "|" << *inst << '\n';
      auto *st = cast<StoreInst>(inst);
      return st->getPointerOperand();
    }
    default:
      return nullptr;
  }
}

static void cleanup(flow_conversion *fc, IntToPtrInst *i2p)
{
  if( (i2p != nullptr) && i2p->use_empty() ) {
    i2p->eraseFromParent();
    fc->val_to_meta.erase(i2p);
  }
}

/* Merge two program states for CFG convergence points.
 *
 * \param fc The conversion state for the pass.
 * \param dest The program state to be updated.
 * \param src The program state to be merged in.
 */
static void merge_prog_state(flow_conversion *fc,
                             prog_state *dest,
                             const prog_state *src) {
}

Value *convert_ptr(flow_conversion* fc,
                   IntToPtrInst* i2p,
                   uint64_t count,
                   bool is_write,
                   Instruction *inst)
{
  auto* mem_ty  = i2p->getType()->getPointerElementType();
  auto* offset  = i2p->getOperand(0);
  auto  it      = fc->val_to_meta.find(i2p);
  assert(it != fc->val_to_meta.end());
  auto* meta    = it->second;
  auto& dl      = fc->m.getDataLayout();
  uint64_t size = dl.getTypeStoreSize(mem_ty) * count;

  LLVM_DEBUG(dbgs() << " offset: " << *offset
                    << " size: " << size << '\n');

  IRBuilder<> ir(is_write ? inst->getNextNode() : inst);
  IRBuilder<> ir_entry(inst->getFunction()->getEntryBlock().getFirstNonPHI());

  ir.SetCurrentDebugLocation(inst->getDebugLoc());

  Constant* nt_function = nullptr;

  /* Common code: alloca + bitcast for data_in / data_out */
  auto* count_val = (count == 1 ? nullptr : ir_entry.getInt64(count));
  auto* alloca = ir_entry.CreateAlloca(mem_ty, count_val,
                                       inst->getName() + "_buffer");
  auto* bc     = ir_entry.CreateBitCast(alloca, ir.getInt8PtrTy(),
                                        inst->getName() + "_buffer.bc");

  /* Specific prefix + call */
  if( !is_write ) {
    /* Loads are just reads */
    nt_function = meta->is_map ? create_nt_map_read(fc->m) :
                                 create_nt_packet_read(fc->m);
  } else {
    /* Stores are just writes */
    nt_function = meta->is_map ? create_nt_map_write(fc->m) :
                                 create_nt_packet_write(fc->m);
  }

  /* Arguments: make sure to add key and key size for map access */
  std::vector<Value*> args;
  if( meta->is_map )
    args.push_back(inst->getFunction()->arg_begin());
  args.push_back(meta->base);
  if( meta->is_map ) {
    args.push_back(meta->key);
    args.push_back(meta->key_sz);
  }
  args.push_back(bc);
  args.push_back(offset);
  args.push_back(ir.getInt64(size));

#ifdef VERBOSE_DEBUG
  LLVM_DEBUG(
    auto* fty = cast<Function>(nt_function)->getFunctionType();
    for( unsigned i = 0; i < args.size(); ++i ) {
      dbgs() << "  Arg " << i << " " << *args[i] << " Type "
             << *args[i]->getType() << " Param type: "
             << *fty->getParamType(i) << '\n';
    }
  );
#endif //VERBOSE_DEBUG

  const char* suffix = is_write ? "_wr" : "_rd";
  auto* call    = ir.CreateCall(nt_function, args,
                                i2p->getName() + suffix);
  LLVM_DEBUG(dbgs() << *call << '\n');

  return alloca;
}

static Value* gep_to_arithmetic_zero(GetElementPtrInst *gep);
static Value* gep_to_arithmetic_base(GetElementPtrInst *gep, Value *base_int);


/* Convert the packet_data and packet_data_end calls:
 * packet_data => (u8*)0;
 * packet_data_end => (u8*)nanotube_packet_bounded_length();
 */

static
Value* gep_to_arithmetic_zero(GetElementPtrInst *gep) {
  auto *m  = gep->getModule();
  auto &dl = m->getDataLayout();

  /* Easy case first: all constant offsets
     NB: Run both code-paths for these cases for sanity checking! */
  APInt off(64, 0);
  bool only_const_off = gep->accumulateConstantOffset(dl, off);

  /* Multiply out and handle non-constants */
  IRBuilder<> ir(gep);
  unsigned const_off = 0;
  Value *chain = nullptr;

  LLVM_DEBUG(dbgs() << "  GEP Conversion:\n");
  for( auto it = gep_type_begin(gep); it != gep_type_end(gep); ++it ) {
    LLVM_DEBUG(dbgs() << "  Type: " << *it.getIndexedType()
                      << " Operand: " << *it.getOperand() << '\n');
    if( it.isStruct() ) {
      /* Struct fields must be accessed by constant */
      auto *layout = dl.getStructLayout(it.getStructType());
      auto *idx    = cast<ConstantInt>(it.getOperand());
      unsigned off = layout->getElementOffset(idx->getZExtValue());
      const_off += off;
      LLVM_DEBUG(dbgs() << "    Struct Off: " << off
                        << " Const Off: " << const_off << '\n');
    } else {
      /* Arrays can be accessed by constant or by variable */
      unsigned size = dl.getTypeStoreSize(it.getIndexedType());
      if( isa<ConstantInt>(it.getOperand()) ) {
        const_off += size *
                     cast<ConstantInt>(it.getOperand())->getZExtValue();
        LLVM_DEBUG(dbgs() << "    Const Off: " << const_off << '\n');
      } else {
        auto *mul = (size > 1) ?
                    ir.CreateMul(ir.getInt64(size), it.getOperand()) :
                    it.getOperand();
        LLVM_DEBUG(dbgs() << "    Adding: " << *mul << '\n');
        if( chain != nullptr ) {
          chain = ir.CreateAdd(chain, mul);
          LLVM_DEBUG(dbgs() << "    Adding: " << *chain << '\n');
        } else {
          chain = mul;
        }
      }
    }
  }

  /* Handly remaining accumulated constant offset */
  if( const_off != 0 ) {
    if( chain != nullptr )
      chain = ir.CreateAdd(chain, ir.getInt64(const_off),
                           Twine(gep->getName()) + ".addc");
    else
      chain = ir.getInt64(const_off);
    LLVM_DEBUG(dbgs() << "  Final: " << *chain << '\n');
  } else {
    if( chain == nullptr ) {
      chain = ir.getInt64(const_off);
      LLVM_DEBUG(dbgs() << "  Final: " << *chain << '\n');
    }
  }

  /* Sanity checking for those with all const fields */
  if( only_const_off && (off != const_off) ) {
    errs() << "FIXME: Offset difference for all-const GEP " << *gep
           << " homebrew: " << const_off << " vs. LLVM: " << off << '\n';
    assert(false /*njet!*/);
  }
  return chain;
}

static
Value* gep_to_arithmetic_base(GetElementPtrInst *gep, Value *base_int) {
  auto* val = gep_to_arithmetic_zero(gep);
  IRBuilder<> ir(gep);
  return ir.CreateAdd(base_int, val,
                      Twine(gep->getName()) + ".addb");
}

/**
 * Just a simple helper to get more meaningful names of values.
 */
std::string
get_proper_name(const Value* v, unsigned max_depth = 5) {
  if( v->hasName() )
    return v->getName();
  if( max_depth == 0 )
    return "";

  const auto* load = dyn_cast<LoadInst>(v);
  if( load != nullptr )
    return get_proper_name(load->getOperand(0), max_depth - 1) + ".val";

  const auto* cast = dyn_cast<CastInst>(v);
  if( cast != nullptr )
    return get_proper_name(cast->getOperand(0), max_depth - 1) + ".c";

  const auto* gep = dyn_cast<GetElementPtrInst>(v);
  if( gep != nullptr )
    return get_proper_name(gep->getPointerOperand(), max_depth - 1) + ".gep";

  return "";
}

/**
 * Checks if this function is a source of a pointer to map entry / packet
 */
bool is_map_pointer_source(const Instruction* inst) {
  auto i = get_intrinsic(inst);
  return (i == Intrinsics::map_lookup);
}

bool is_packet_pointer_source(const Instruction* inst) {
  auto i = get_intrinsic(inst);
  return (i == Intrinsics::packet_data) || (i == Intrinsics::packet_end);
}
bool is_nanotube_root(const Instruction& inst) {
  return is_map_pointer_source(&inst) || is_packet_pointer_source(&inst);
}

struct arg_info {
  llvm::ModRefInfo mod_ref;
  uint64_t size;
};

void get_read_write_unknown(CallInst* call, unsigned op_idx, arg_info* ai) {
  nt_api_call api(call);

  bool precise = false;
  ai->size = api.get_max_size(op_idx, &precise);

  if( precise )
    ai->mod_ref = get_nt_arg_info(api.get_intrinsic(), op_idx);
  else
    ai->mod_ref = ModRefInfo::ModRef;
}

void flow_conversion::flow(BasicBlock *bb) {
  LLVM_DEBUG(
    dbgs() << "Processing basic block: " << as_operand(bb) << "\n";
  );

  /* Set the current state for this basic block. */
  auto state_it = bb_states.find(bb);
  assert(state_it != bb_states.end());
  std::swap(current_state, state_it->second);
  bb_states.erase(state_it);

  /* Process the instructions. */
  for( auto it = bb->begin(); it != bb->end(); ) {
    auto cur = it++;
    Instruction &inst = *cur;

    /* Check whether the instruction should be processed.  A call
     * instruction will always be processed as is may produce a
     * tracked pointer.  Other instructions are processed if they use
     * a tracked pointer. */
    if( !isa<CallBase>(&inst) ) {
      bool any_tracked = false;
      for( Use &use: inst.operands() ) {
        auto it = val_to_meta.find(use.get());
        if( it != val_to_meta.end() ) {
          any_tracked = true;
          break;
        }
      }
      if (!any_tracked)
        continue;
    }
    flow(&inst);
  }

  /* Update the states for the successor basic blocks and mark them as
   * ready. */
  for( auto succ: successors(bb) ) {
    prog_state *new_state = &(bb_states[succ]);
    merge_prog_state(this, new_state, &current_state);
    dac.mark_dep_ready(succ);
  }
}

void flow_conversion::flow(Instruction* inst) {
  /**
   * Interesting cases:
   *   * constant => convert, convert users
   *   * inttoptr => remove, convert users
   *   * ptrtoint => remove, end recursion
   *   * bitcast  => inttoptr, convert users
   *   * gep      => convert to add, convert users
   *   * cmp      => convert to use integer arguments
   *   * select   => convert, convert users
   *   * phi      => convert, convert users
   *   * other    => panic (a snake!)
   */

  auto* op0  = inst->getOperand(0);
  auto* i2p  = dyn_cast<IntToPtrInst>(op0);

  pointer_meta* meta = nullptr;
  auto     it   = val_to_meta.find(i2p);

  if( it != val_to_meta.end() )
    meta = it->second;

  IRBuilder<> ir(inst);

#ifdef VERBOSE_DEBUG
  LLVM_DEBUG(
    dbgs() << "Val-to-meta:\n";
    for( auto v2m : val_to_meta ) {
      dbgs() << "  v: " << v2m.first << " " << *v2m.first
             << "  m: " << *v2m.second << '\n';
    }
    dbgs() << "\n\nMeta nodes:\n";
    for( auto meta : meta_nodes ) {
      dbgs() << "  m: " << *meta << '\n';
    }
    dbgs() << '\n';
  );
#endif //VERBOSE_DEBUG

  switch( inst->getOpcode() ) {
    case Instruction::Call:
      convert_call(cast<CallInst>(inst));
      break;

    case Instruction::IntToPtr:
      /* i2p is really the frontier of conversion => convert all users
       * which will clean up this i2p once done */
      LLVM_DEBUG(dbgs() << "Handling i2p" << *inst << '\n');
      break;

    case Instruction::PtrToInt: {
      /* p2i is an endpoint for the conversion process and once conversion
       * reaches it, it will get removed. */
      LLVM_DEBUG(dbgs() << "Handling p2i" << *inst << '\n');
      assert( meta != nullptr );

      /* i2p and p2i just anihilate each other */
      auto* orig_op = i2p->getOperand(0);
      auto* repl = ir.CreateZExtOrTrunc(orig_op, inst->getType());
      replace_and_cleanup(inst, repl, i2p);
      LLVM_DEBUG(dbgs() << "  Replacing with " << *orig_op << '\n');
      break;
    }

    case Instruction::BitCast: {
      /* i2p; bc just gets turned into i2p' with a different pointer type:
         %ptr = i2p %int; %foo = bc %ptr, <type>; %flark = op %foo
         %foo' = i2p %int, <type>; %flark = op %foo' */
      LLVM_DEBUG(dbgs() << "Handling bc" << *inst << '\n'
                        << "  Param: " << *op0 << '\n');
      assert( meta != nullptr );
      // NB: IRBuilder likes to create ConstExprs which make the parsing
      //     harder.  Therefore, manually force creation of an i2p
      //     instruction.
      //auto *i2p_new = ir.CreateIntToPtr(i2p->getOperand(0),
      //                                  inst->getType());
      auto *i2p_new = new IntToPtrInst(i2p->getOperand(0),
                                       inst->getType(), "", inst);
      i2p_new->setDebugLoc(inst->getDebugLoc());
      replace_and_cleanup(inst, i2p_new, i2p);
      LLVM_DEBUG(dbgs() << "  Converting to" << *i2p_new << '\n');
      val_to_meta.insert({i2p_new, meta});
      break;
    }

    case Instruction::GetElementPtr: {
      /* GEPs will be converted to a sequence of multiplications and adds;
       * some of which may be constant, some of which may be variable. */
      LLVM_DEBUG(dbgs() << "Handling gep" << *inst << '\n');
      auto* gep = cast<GetElementPtrInst>(inst);
      Value *conv;
      assert( meta != nullptr );
      conv = gep_to_arithmetic_base(gep, i2p->getOperand(0));
      // NB: see BitCast
      //auto *i2p_new = ir.CreateIntToPtr(conv, gep->getType());
      auto *i2p_new = new IntToPtrInst(conv, gep->getType(), "", gep);
      i2p_new->setDebugLoc(inst->getDebugLoc());
      LLVM_DEBUG(dbgs() << "  Converting to " << *i2p_new << '\n');
      val_to_meta.insert({i2p_new, meta});
      replace_and_cleanup(inst, i2p_new, i2p);
      break;
    }

    case Instruction::ICmp: {
      /* ICmp either compares a packet pointer to the end of packet, or
       * checks whether a map lookup found the key.
       */
      auto* icmp = cast<ICmpInst>(inst);
      auto* rhs  = icmp->getOperand(1);
      auto* i2p2 = dyn_cast<IntToPtrInst>(rhs);

      if( i2p2 != nullptr ) {
        /* This is the packet case, comparing the data pointer to the end
         * of the packet
         */
        auto it2 = val_to_meta.find(i2p2);
        assert(it2 != val_to_meta.end());
        assert(!meta->is_map && !it2->second->is_map);

        icmp->setOperand(0, i2p->getOperand(0));
        icmp->setOperand(1, i2p2->getOperand(0));
      } else {
        /* This is the map case, where the code compares the pointer to
         * NULL to see if the key was maybe not present.
         */
        assert(isa<ConstantPointerNull>(rhs));
        assert(meta->is_map);
        assert(meta->dummy_read_ret != nullptr);
        icmp->setOperand(0, meta->dummy_read_ret);
        icmp->setOperand(1, ir.getInt64(0));
      }
      LLVM_DEBUG(dbgs() << "Handling icmp" << *inst << '\n'
                        << "  Op0: " << *i2p << "\n  Op1: " << *rhs << '\n'
                        << "  Converting to " << *icmp << '\n');
      cleanup(this, i2p);
      cleanup(this, i2p2);
      break;
    }

    case Instruction::Select: {
      /* Select picks from either pointer; for packets, this means just
       * offset, but for maps, this could mean different maps and / or
       * different keys, as well. Therefore add a select for those, too. */
      auto* select = cast<SelectInst>(inst);
      LLVM_DEBUG(dbgs() << "Handling select" << *select << '\n');

      i2p        = cast<IntToPtrInst>(select->getTrueValue());
      auto* i2p2 = cast<IntToPtrInst>(select->getFalseValue());
      it         = val_to_meta.find(i2p);
      auto it2   = val_to_meta.find(i2p2);
      assert((it != val_to_meta.end()) && (it2 != val_to_meta.end()));

      meta        = it->second;
      auto* meta2 = it2->second;
      assert(meta->is_map == meta2->is_map);

      /* Create a new select for the offset data */
      auto* select_new = ir.CreateSelect(select->getCondition(),
                           i2p->getOperand(0), i2p2->getOperand(0),
                           select->getName());

      auto* meta_new = new pointer_meta();
      meta_nodes.push_back(meta_new);
      meta_new->is_map = meta->is_map;

      if( !meta->is_map ) {
        /* Packet needs the same base and no additional work */
        assert(meta->base == meta2->base);
        meta_new->base = meta->base;
      } else {
        /* Maps need to check whether additional selects are needed */
        /* Create select if needed for the key */
        if( meta->key != meta2->key ) {
          meta_new->key = ir.CreateSelect(select->getCondition(),
                                          meta->key, meta2->key);
        } else {
          meta_new->key = meta->key;
        }

        /* Create select if needed for the map */
        if( meta->base != meta2->base ) {
          meta_new->base = ir.CreateSelect(select->getCondition(),
                                           meta->base, meta2->base);
        } else {
          meta_new->base = meta->base;
        }

        /* Create select if needed for the dummy read */
        if( meta->dummy_read_ret != meta2->dummy_read_ret ) {
          meta_new->dummy_read_ret = ir.CreateSelect(
                                       select->getCondition(),
                                       meta->dummy_read_ret,
                                       meta2->dummy_read_ret);
        } else {
          meta_new->dummy_read_ret = meta->dummy_read_ret;
        }

        /* Key size has to be the same */
        assert(meta->key_sz == meta2->key_sz);
        meta_new->key_sz = meta->key_sz;
      }
      LLVM_DEBUG(dbgs() << "  New meta: " << *meta_new << '\n'
                        << "Replacing with: " << *select_new <<'\n');

      auto *i2p_new = new IntToPtrInst(select_new, select->getType(), "",
                                       select);
      i2p_new->setDebugLoc(select->getDebugLoc());
      replace_and_cleanup(select, i2p_new, i2p);
      cleanup(this, i2p2);
      val_to_meta[i2p_new] = meta_new;
      break;
    }

    case Instruction::PHI: {
      /* Instead of reading i2p-ed pointer values, PHIs simpy read ints,
       * and push the i2p to the other side.  For maps, also check where
       * these pointers come from, and create PHIs for the map / key if
       * necessary, and update the meta data */
      auto *phi     = cast<PHINode>(inst);
      LLVM_DEBUG(dbgs() << "Handling phi" << *phi << '\n');
      unsigned incoming = phi->getNumIncomingValues();

      auto *phi_offset = ir.CreatePHI(ir.getInt64Ty(), incoming);
      SmallSet<IntToPtrInst*, 8> input_i2ps;

      PHINode* phi_map = nullptr;
      PHINode* phi_key = nullptr;
      /* Get the type (map vs packet) from the first parameter */
      bool is_map    = val_to_meta[phi->getOperand(0)]->is_map;

      if( is_map) {
        phi_map = ir.CreatePHI(get_nt_map_id_type(*phi->getModule()),
                    incoming);
        phi_key = ir.CreatePHI(ir.getInt8PtrTy(), incoming);
      }

      pointer_meta* meta = new pointer_meta();
      meta_nodes.push_back(meta);

      bool base_same = true;
      bool keys_same = true;
      Value* base    = nullptr;
      Value* key     = nullptr;

      for( unsigned i = 0; i < phi->getNumIncomingValues(); ++i ) {
        auto *v  = cast<IntToPtrInst>(phi->getIncomingValue(i));
        auto *bb = phi->getIncomingBlock(i);
        auto it  = val_to_meta.find(v);
        assert(it != val_to_meta.end());

        /* We currently do not support mixed PHI nodes, i.e., where one
         * pointer comes from a packet, and another from a map */
        if( is_map != it->second->is_map ) {
          errs() << "Phi node" << *phi << " combines map & packets\n";
          for( auto& v : phi->incoming_values() ) {
            errs() << "  " << (val_to_meta[v]->is_map ? "Map" : "Pkt")
                   << ": " << *v << '\n';
          }
          errs() << "Users:\n";
          for( auto* u : phi->users() )
            errs() << "  " << *u << '\n';
          assert(is_map == it->second->is_map);
        }

        LLVM_DEBUG(dbgs() << "  Input " << i << " " << *v->getOperand(0)
                          << " meta: " << *it->second << '\n');

        phi_offset->addIncoming(v->getOperand(0), bb);
        input_i2ps.insert(v);

        if( i == 0 )
          base = it->second->base;

        if( it->second->base != base )
          base_same = false;

        if( !is_map )
          continue;

        /* More processing for map entries */
        if( i == 0 ) {
          key                  = it->second->key;
          meta->key_sz         = it->second->key_sz;
          meta->dummy_read_ret = it->second->dummy_read_ret;
        }

        phi_map->addIncoming(it->second->base, bb);
        phi_key->addIncoming(it->second->key, bb);
        if( it->second->key != key )
          keys_same = false;
      }

      /* Sanity check and clean up phi nodes */
      if( !is_map ) {
        /* Packets must have the same base */
        assert(base_same);
      } else {
        /* Clean up all identical maps / keys */
        if( base_same ) {
          phi_map->eraseFromParent();
        } else {
          base = phi_map;
        }
        if( keys_same ) {
          phi_key->eraseFromParent();
        } else {
          key = phi_key;
        }
        meta->key    = key;
        LLVM_DEBUG(dbgs() << "  Map: " << *base << '\n'
                          << "  Key: " << *key << '\n');
      }
      meta->base   = base;
      meta->is_map = is_map;

      LLVM_DEBUG(dbgs() << "  Replacing with " << *phi_offset
                        << " meta: " << *meta << '\n');

      //// NB: see above, BitCast
      //auto *i2p_new = ir.CreateIntToPtr(phi_new, phi->getType());
      auto *i2p_new = new IntToPtrInst(phi_offset, phi->getType(), "",
                                       inst);
      i2p_new->setDebugLoc(inst->getDebugLoc());
      replace_and_cleanup(phi, i2p_new, nullptr);
      for( auto *input_i2p : input_i2ps) {
        cleanup(this, input_i2p);
      }
      val_to_meta[i2p_new] = meta;
      break;
    }

    case Instruction::Store:
    case Instruction::Load:
      convert_mem(inst);
      break;
    default:
      errs() << "Cannot convert unknown instruction " << *inst << '\n';
      assert(false);
  };
}
void flow_conversion::convert_nanotube_root(CallInst* call) {
  LLVM_DEBUG(dbgs() << "Converting root " << *call << '\n');

  Value* replacement = nullptr;
  pointer_meta* meta = new pointer_meta();
  IRBuilder<> ir(call);

  if( is_packet_pointer_source(call) ) {
    switch( get_intrinsic(call) ) {
      case Intrinsics::packet_data:
        /* Lookups of packet data simply turn to zero offsets */
        replacement = ir.getInt64(0);
        break;
      case Intrinsics::packet_end: {
        /* Packet end pointer requests return the length of the packet; that
         * way all comparisons etc still work as expected. */
        auto* len = create_nt_packet_bounded_length(m);
        Value* args[] = {call->getOperand(0), ir.getInt64(32767)};
        replacement = ir.CreateCall(len, args);
        LLVM_DEBUG(dbgs() << *replacement << '\n');
        break;
      }
      default:
        assert(false);
    };

    meta->is_map = false;
    meta->base   = call->getOperand(0);

  } else if( is_map_pointer_source(call) ) {
    /* Map lookups do multiple things:
     *   * create a copy of the key, in case it is modified, later
     *   * a dummy map read with the same key and a single byte to
     *     understand whether the lookup missed
     *   * a simple zero as the pointer to offset base
     */
    assert(get_intrinsic(call) == Intrinsics::map_lookup);

    auto* ctx    = call->getOperand(0);
    auto* map_id = call->getOperand(1);
    auto* key    = call->getOperand(2);
    auto* key_sz = call->getOperand(3);

    IRBuilder<> ir(call);
    IRBuilder<> ir_entry(call->getFunction()->getEntryBlock().getFirstNonPHI());
    /* Copy the key to a new stack variable */
    auto* key_copy = ir_entry.CreateAlloca(ir.getInt8Ty(), key_sz,
                                           get_proper_name(key) + "_copy");
    auto* memcpy   = ir.CreateMemCpy(key_copy, 0, key, 0,
                       cast<ConstantInt>(key_sz)->getZExtValue());
    LLVM_DEBUG(dbgs() << *key_copy << '\n' << *memcpy << '\n');

    /* Perform the dummy read */
    auto* alloca = ir_entry.CreateAlloca(ir.getInt8Ty(), nullptr,
                                         call->getName() + "_buf");
    auto* map_rd = create_nt_map_read(*call->getModule());
    /* Copy or not does not matter here */
    // Value* args[] = { ctx, map_id, key, key_sz, alloca,
    //                   ir.getInt64(0), ir.getInt64(1) };
    Value* args[] = { ctx, map_id, key_copy, key_sz, alloca,
                      ir.getInt64(0), ir.getInt64(1) };
    auto* bytes_read = ir.CreateCall(map_rd, args, "key_check");
    LLVM_DEBUG(dbgs() << *alloca << '\n' << *bytes_read << '\n');

    replacement = ir.getInt64(0);

    meta->is_map         = true;
    meta->base           = map_id;
    meta->key            = key_copy;
    meta->key_sz         = key_sz;
    meta->dummy_read_ret = bytes_read;
  }

  auto* i2p = new IntToPtrInst(replacement, call->getType(),
                               call->getName() + "_",  call);
  LLVM_DEBUG(dbgs() << *i2p << '\n');

#ifdef VERBOSE_DEBUG
  LLVM_DEBUG( dbgs() << "Meta: " << *meta << '\n');
#endif //VERBOSE_DEBUG

  call->replaceAllUsesWith(i2p);
  call->eraseFromParent();

  meta_nodes.push_back(meta);
  val_to_meta[i2p] = meta;
}
void flow_conversion::convert_mem(Instruction* inst) {
  assert( isa<LoadInst>(inst) || isa<StoreInst>(inst) );

  bool is_write = isa<StoreInst>(inst);
  auto* ptr     = get_pointer_operand(inst);
  auto* i2p     = dyn_cast<IntToPtrInst>(ptr);

  LLVM_DEBUG(
    auto* mem_ty  = ptr->getType()->getPointerElementType();
    dbgs() << "Converting " << *inst << " with ptr: " << *ptr
           << " memory type: " << *mem_ty << '\n');

  ptr = convert_ptr(this, i2p, /*count*/1, is_write, inst);

  /* Suffix code and cleanup */
  inst->setOperand(is_write ? 1 : 0, ptr);
  cleanup(this, i2p);
}

bool flow_conversion::try_convert_special_case_call(CallInst* call) {
  switch( get_intrinsic(call) ) {
    case Intrinsics::llvm_memcpy:
      return try_convert_memcpy(call);
    default:
      return false;
  }
}

static
void convert_call_arg(flow_conversion *fc, CallInst *call, unsigned i,
                      Value *arg, val_meta_map::iterator it) {
  auto* meta = it->second;

  /* Pull out the integer offset */
  auto* i2p    = cast<IntToPtrInst>(arg);

  LLVM_DEBUG(
    dbgs() << "Converting arg " << i << " " << *arg << '\n'
    << "  Meta: " << *meta  << '\n';
  );

  arg_info ai;
  get_read_write_unknown(call, i, &ai);
  if( ai.mod_ref != ModRefInfo::MustRef && ai.mod_ref != ModRefInfo::MustMod ) {
    errs() << "Unknown read / write pointer argument " << i << *arg
           << " in call " << *call << '\n';
    assert(false);
  }

  bool is_write = ai.mod_ref == ModRefInfo::MustMod;
  assert(i2p->getType()->getPointerElementType()->isIntegerTy(8));

  auto* ptr = convert_ptr(fc, i2p, ai.size, is_write, call);
  call->setOperand(i, ptr);

  cleanup(fc, i2p);
}

void flow_conversion::convert_call(CallInst* call) {
  LLVM_DEBUG(dbgs() << "Converting call " << *call << '\n');

  /* Check special cases */
  if (try_convert_special_case_call(call))
    return;

  /* Deal with map / packet pointer arguments */
  for( unsigned i = 0; i < call->arg_size(); i++ ) {
    auto* arg  = call->getOperand(i);
    auto it    = val_to_meta.find(arg);
    if( it == val_to_meta.end() )
      continue;

    convert_call_arg(this, call, i, arg, it);
  }

  /* Convert function calls (if needed) */
  if( is_nanotube_root(*call) ) {
    convert_nanotube_root(call);
  }
}

bool flow_conversion::try_convert_memcpy(Instruction* inst) {
  auto* dst     = inst->getOperand(0);
  auto* src     = inst->getOperand(1);
  auto* size    = inst->getOperand(2);

  auto dst_meta_it = val_to_meta.find(dst);
  auto src_meta_it = val_to_meta.find(src);
  pointer_meta* dst_meta = (dst_meta_it != val_to_meta.end()) ?
                            dst_meta_it->second : nullptr;
  pointer_meta* src_meta = (src_meta_it != val_to_meta.end()) ?
                            src_meta_it->second : nullptr;

  auto* size_const = dyn_cast<ConstantInt>(size);
  if( size_const == nullptr )
    return false;

  uint64_t count = size_const->getZExtValue();

  IRBuilder<> ir(inst);
  IRBuilder<> ir_entry(inst->getFunction()->getEntryBlock().getFirstNonPHI());

  // No conversion needed for mem 2 mem transfer
  if( (dst_meta == nullptr) && (src_meta == nullptr) )
    return true;

  LLVM_DEBUG(
    dbgs() << "Memcpy Src: ";
    if( src_meta != nullptr )
      dbgs() << *src_meta;
    else
      dbgs() << "<stack>";
    dbgs() << " => Dst: ";
    if( dst_meta != nullptr )
      dbgs() << *dst_meta;
    else
      dbgs() << "<stack>";
    dbgs() << " for " << *inst << '\n';
  );

  /* Try to merge the memcpy with the source access. */
  if( (src_meta != nullptr) && !src_meta->is_map ) {
    /* Convert source to a packet read */
    auto* mem = dst;
    if( dst_meta != nullptr )
      mem = convert_ptr(this, cast<IntToPtrInst>(dst), count, true, inst);
    auto* offset  = cast<IntToPtrInst>(src)->getOperand(0);
    auto* pkt_rd  = create_nt_packet_read(*inst->getModule());
    Value* args[] = { src_meta->base, mem, offset, size };
    auto* call    = ir.CreateCall(pkt_rd, args, src->getName() + "_rd");

    LLVM_DEBUG(dbgs() << "Packet read: " << *call << '\n');
    inst->eraseFromParent();
    cleanup(this, dyn_cast<IntToPtrInst>(dst));
    cleanup(this, dyn_cast<IntToPtrInst>(src));
    return true;
  }

  /* Try to merge the memcpy with the destination access. */
  if( (dst_meta != nullptr) && !dst_meta->is_map ) {
    /* Convert destination to a packet write */
    auto* mem = src;
    if( src_meta != nullptr )
      mem = convert_ptr(this, cast<IntToPtrInst>(src), count, false, inst);
    auto* offset  = cast<IntToPtrInst>(dst)->getOperand(0);
    auto* pkt_wr  = create_nt_packet_write(*inst->getModule());
    Value* args[] = { dst_meta->base, mem, offset, size };
    auto* call    = ir.CreateCall(pkt_wr, args, dst->getName() + "_wr");
    LLVM_DEBUG(dbgs() << "Packet write: " << *call << '\n');
    inst->eraseFromParent();
    cleanup(this, dyn_cast<IntToPtrInst>(dst));
    cleanup(this, dyn_cast<IntToPtrInst>(src));
    return true;
  }

  return false;
}

flow_conversion::flow_conversion(Function& f) : m(*f.getParent()),
                                                c(m.getContext()),
                                                func(f)
{
}

void flow_conversion::replace_and_cleanup(Instruction* inst, Value* repl,
                                          IntToPtrInst* i2p) {
  if( inst->getType() != repl->getType() ) {
    errs() << "ERROR: Replacement " << *repl << " has different type ("
           << *repl->getType() << ")\nthan instruction " << *inst
           << " (" << *inst->getType() << ")\nAborting!\n";
    abort();
  }
  if( !repl->hasName() )
    repl->takeName(inst);
  inst->replaceAllUsesWith(repl);
  inst->eraseFromParent();
  cleanup(this, i2p);
}

/* Mini-flow: compute the number of input operands that need conversion for
** a specific instruction.
**
** The idea here is to start at the map / packet pointer sources, add them
** to a worklist, and for each entry in the worklist, check if they propagate
** map / packet pointer"-ness" from their arguments to their own value
** (casts do, GEPs do, ICmp does not, for example).  Then, if they
** propagate, increment the number of map /packet inputs for all users and
** add them (unless already done) to the worklist.
**/

raw_ostream& operator<<(raw_ostream& os, const pointer_meta& meta) {
  os << &meta << " ";
  os << (meta.is_map ? "map" : "packet") << " ";

  os << "base: " << meta.base << " ";
  if( meta.base != nullptr )
    os << *meta.base << " ";
  if( meta.is_map ) {
    os << "key: " << meta.key << " " << *meta.key << " ";
    os << "key_sz: " << meta.key_sz << " " << *meta.key_sz << " ";
    os << "check_read: " << *meta.dummy_read_ret;
  }
  return os;
}

static
bool convert_to_req(Function* f) {
  LLVM_DEBUG( dbgs() << "\nMem2req converting function '"
                     << f->getName() << "'\n";);
  flow_conversion fc(*f);

  BasicBlock *entry_block = &(f->getEntryBlock());
  fc.bb_states.emplace(entry_block, prog_state());

  fc.dac.init_forward(fc.func);
  fc.dac.execute(
    [&](flow_conversion::dac_t *dac, BasicBlock *bb) {
      fc.flow(bb);
    });

  if (!fc.dac.empty()) {
    errs() << "INTERNAL ERROR: Unconverted items after converting function '"
           << f->getName() << "'.\n";
    exit(1);
  }

  return true;
}

/*************************************************************************/

bool mem_to_req::runOnModule(Module& m) {
  setup_func setup(m);
  bool any_changes = false;

  for( auto &kernel: setup.kernels() ) {
    any_changes |= kernel.modify(convert_to_req);
  }

  return any_changes;
}

char mem_to_req::ID = 0;
static RegisterPass<mem_to_req>
    X("mem2req", "Convert Nanotube L1 ld/st API to the request-based L2 API",
      false,
      false
      );

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
