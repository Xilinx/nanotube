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

#define DEBUG_TYPE "mem2req"
#define VERBOSE_DEBUG

using namespace llvm;
using namespace nanotube;


static Value *get_pointer_operand(Instruction *inst);


raw_ostream& operator<<(raw_ostream& os, const nt_meta& meta);


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
      chain = ir.CreateAdd(chain, ir.getInt64(const_off));
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
  return ir.CreateAdd(base_int, val);
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

void replace_all_uses_but(Value* val, Value* repl, Value* except) {
  for( auto it = val->use_begin(); it != val->use_end(); ) {
    auto& u = *it;
    ++it;
    if( u.getUser() != except )
      u.set(repl);
  }
  assert(val->hasNUses(1));
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

void collect_roots(Function& f, std::vector<Instruction*>* roots) {
  std::vector<Instruction*> todo;
  for( auto& bb : f ) {
    for( auto& inst : bb) {
      if( is_nanotube_root(inst) ) {
        LLVM_DEBUG( dbgs() << "Instruction " << inst
                           << " is a map / packet pointer source.\n");
        roots->push_back(&inst);
      }
    }
  }
}

enum {
  READ    = 1,
  WRITE   = 2,
  UNKNOWN = 4,
};
struct arg_info {
  uint8_t rwu;
  Value*  size;
};

void get_read_write_unknown(CallInst* call, unsigned op_idx, arg_info* ai) {
  auto intrinsic = get_intrinsic(call);
  switch( intrinsic ) {
    case Intrinsics::llvm_memset:
      if( op_idx == 0 ) {
        ai->rwu  = WRITE;
        ai->size = call->getOperand(2);
        return;
      }
    case Intrinsics::llvm_memcpy:
      ai->size = call->getOperand(2);
      if( op_idx == 0 ) {
        ai->rwu = WRITE;
        return;
      } else if( op_idx == 1 ) {
        ai->rwu = READ;
        return;
      }
      break;
    case Intrinsics::map_lookup:
      ai->size = call->getOperand(3);
      if( op_idx == 2 ) {
        ai->rwu = READ;
        return;
      }
      break;
    default:
      ;/* fall out */
  };
  ai->size = nullptr;
  ai->rwu  = UNKNOWN;
}

void flow_conversion::flow(Value* v) {
  LLVM_DEBUG(dbgs() << "Flowing " << *v
                    << " forward through the program.\n");
  auto* inst = dyn_cast<Instruction>(v);
  if( inst == nullptr ) {
    errs() << "Unexpected non-instruction " << *v << '\n';
    assert(false);
  }

  flow(inst);
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

  nt_meta* meta = nullptr;
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

      one_dep_ready(inst);
      break;

    case Instruction::PtrToInt: {
      /* p2i is an endpoint for the conversion process and once conversion
       * reaches it, it will get removed. */
      LLVM_DEBUG(dbgs() << "Handling p2i" << *inst << '\n');
      assert( meta != nullptr );
      if( meta->is_start ) {
        /* Replace the zeroes with zero */
        auto* zero = Constant::getNullValue(inst->getType());
        LLVM_DEBUG(dbgs() << "  Replacing with " << *zero << '\n');
        replace_and_cleanup(inst, zero, nullptr);
      } else {
        /* i2p and p2i just anihilate each other */
        auto* orig_op = i2p->getOperand(0);
        auto* repl = ir.CreateZExtOrTrunc(orig_op, inst->getType());
        replace_and_cleanup(inst, repl, i2p);
        LLVM_DEBUG(dbgs() << "  Replacing with " << *orig_op << '\n');
      }
      break;
    }

    case Instruction::BitCast: {
      /* i2p; bc just gets turned into i2p' with a different pointer type:
         %ptr = i2p %int; %foo = bc %ptr, <type>; %flark = op %foo
         %foo' = i2p %int, <type>; %flark = op %foo' */
      LLVM_DEBUG(dbgs() << "Handling bc" << *inst << '\n'
                        << "  Param: " << *op0 << '\n');
      assert( meta != nullptr );
      if( meta->is_start ) {
        /* we are a root / zero base if our argument was, too */
        LLVM_DEBUG(dbgs() << "  We are a root!\n");
        val_to_meta.insert({inst, meta});
        one_dep_ready(inst);
      } else {
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
        one_dep_ready(i2p_new);
      }
      break;
    }

    case Instruction::GetElementPtr: {
      /* GEPs will be converted to a sequence of multiplications and adds;
       * some of which may be constant, some of which may be variable. */
      LLVM_DEBUG(dbgs() << "Handling gep" << *inst << '\n');
      auto* gep = cast<GetElementPtrInst>(inst);
      Value *conv;
      assert( meta != nullptr );
      if( meta->is_start ) {
        LLVM_DEBUG(dbgs() << "  We are relative to root!\n");
        conv = gep_to_arithmetic_zero(gep);
      } else {
        conv = gep_to_arithmetic_base(gep, i2p->getOperand(0));
      }
      // NB: see BitCast
      //auto *i2p_new = ir.CreateIntToPtr(conv, gep->getType());
      auto *i2p_new = new IntToPtrInst(conv, gep->getType(), "", gep);
      i2p_new->setDebugLoc(inst->getDebugLoc());
      LLVM_DEBUG(dbgs() << "  Converting to " << *i2p_new << '\n');
      auto* meta_new = new nt_meta(*meta);
      meta_new->is_start = false;
      meta_nodes.push_back(meta_new);
      val_to_meta.insert({i2p_new, meta_new});
      replace_and_cleanup(inst, i2p_new, i2p);
      one_dep_ready(i2p_new);
      break;
    }

    case Instruction::ICmp: {
      /* ICmp either compares a packet pointer to the end of packet, or
       * checkcks whether a map lookup found the key.
       */
      auto* icmp = cast<ICmpInst>(inst);
      auto* rhs  = icmp->getOperand(1);
      auto* i2p2 = dyn_cast<IntToPtrInst>(rhs);
      Value* icmp_new = nullptr;

      if( i2p2 != nullptr ) {
        /* This is the packet case, comparing the data pointer to the end
         * of the packet
         */
        auto it2 = val_to_meta.find(i2p2);
        assert(it2 != val_to_meta.end());
        assert(!meta->is_map && !it2->second->is_map);

        icmp_new = ir.CreateICmp(icmp->getPredicate(), i2p->getOperand(0),
                                 i2p2->getOperand(0));
      } else {
        /* This is the map case, where the code compares the pointer to
         * NULL to see if the key was maybe not present.
         */
        assert(isa<ConstantPointerNull>(rhs));
        assert(meta->is_map);
        assert(meta->dummy_read_ret != nullptr);
        icmp_new = ir.CreateICmp(icmp->getPredicate(),
                                 meta->dummy_read_ret, ir.getInt64(0));
      }
      LLVM_DEBUG(dbgs() << "Handling icmp" << *inst << '\n'
                        << "  Op0: " << *i2p << "\n  Op1: " << *rhs << '\n'
                        << "  Converting to " << *icmp_new << '\n');
      replace_and_cleanup(inst, icmp_new, i2p);

      if( (i2p2 != nullptr) && i2p2->use_empty() ) {
        i2p2->eraseFromParent();
        val_to_meta.erase(i2p2);
      }
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
      assert(meta->is_map == meta->is_map);

      /* Create a new select for the offset data */
      auto* select_new = ir.CreateSelect(select->getCondition(),
                           i2p->getOperand(0), i2p2->getOperand(0),
                           select->getName());

      auto* meta_new = new nt_meta();
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
      if( i2p2->use_empty() ) {
          i2p2->eraseFromParent();
          val_to_meta.erase(i2p2);
      }
      val_to_meta[i2p_new] = meta_new;
      one_dep_ready(i2p_new);
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

      nt_meta* meta = new nt_meta();
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
        if( input_i2p->use_empty() ) {
          input_i2p->eraseFromParent();
          val_to_meta.erase(input_i2p);
        }
      }
      val_to_meta[i2p_new] = meta;
      one_dep_ready(i2p_new);
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
  nt_meta* meta      = new nt_meta();
  IRBuilder<> ir(call);

  if( is_packet_pointer_source(call) ) {
    switch( get_intrinsic(call) ) {
      case Intrinsics::packet_data:
        /* Lookups of packet data simply turn to zero offsets */
        replacement = ir.getInt64(0);
        break;
      case Intrinsics::packet_end: {
        /* Packet end pointer requests return thelength of the packet; that
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
                                         "dummy_rd_data");
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
  //val_to_meta.insert({i2p, meta});
  val_to_meta[i2p] = meta;

  one_dep_ready(i2p);
}
void flow_conversion::convert_mem(Instruction* inst) {
  assert( isa<LoadInst>(inst) || isa<StoreInst>(inst) );

  bool is_write = isa<StoreInst>(inst);
  auto* ptr     = get_pointer_operand(inst);
  auto* mem_ty  = ptr->getType()->getPointerElementType();
  auto* i2p     = dyn_cast<IntToPtrInst>(ptr);

  LLVM_DEBUG(
    dbgs() << "Converting " << *inst << " with ptr: " << *ptr
           << " memory type: " << *mem_ty << '\n');

  auto* offset  = i2p->getOperand(0);
  auto  it      = val_to_meta.find(i2p);
  assert(it != val_to_meta.end());
  auto* meta    = it->second;
  auto& dl      = m.getDataLayout();
  unsigned size = dl.getTypeStoreSize(mem_ty);

  LLVM_DEBUG(dbgs() << " offset: " << *offset
                    << " size: " << size << '\n');

  IRBuilder<> ir(inst);
  IRBuilder<> ir_entry(inst->getFunction()->getEntryBlock().getFirstNonPHI());

  Constant* nt_function = nullptr;

  /* Common code: alloca + bitcast for data_in / data_out */
  auto* alloca = ir_entry.CreateAlloca(mem_ty, nullptr,
                                       inst->getName() + "_buffer");
  auto* bc     = ir_entry.CreateBitCast(alloca, ir.getInt8PtrTy());

  /* Specific prefix + call */
  if( !is_write) {
    /* Loads are just reads */
    nt_function = meta->is_map ? create_nt_map_read(m) :
                                 create_nt_packet_read(m);
  } else {
    /* Generate code for stores: store, nt_packet_write(..) */
    auto* val = cast<StoreInst>(inst)->getValueOperand();
    ir.CreateStore(val, alloca);
    nt_function = meta->is_map ? create_nt_map_write(m) :
                                 create_nt_packet_write(m);
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

  auto* call    = ir.CreateCall(nt_function, args);
  LLVM_DEBUG(dbgs() << *call << '\n');

  /* Suffix code and cleanup */
  if( !is_write ) {
    auto* ld = ir.CreateLoad(alloca);
    replace_and_cleanup(inst, ld, i2p);
  } else {
    inst->eraseFromParent();
    if( i2p->use_empty() ) {
      i2p->eraseFromParent();
      val_to_meta.erase(it);
    }
  }
}
void flow_conversion::convert_special_case_call(CallInst* call) {
  switch( get_intrinsic(call) ) {
    case Intrinsics::llvm_memcpy:
      convert_memcpy(call);
      return;
    default:
      assert(false);
  }
}


Instruction*
flow_conversion::typed_alloca(Type* ty, const Twine& name, Value* size,
                              IRBuilder<>* ir)
{
  auto* alloca = ir->CreateAlloca(Type::getInt8Ty(c), 0, size, name);
  if( ty == alloca->getType() )
    return alloca;
  else
    return cast<Instruction>(ir->CreateBitCast(alloca, ty, ""));
}

void flow_conversion::convert_call(CallInst* call) {
  LLVM_DEBUG(dbgs() << "Converting call " << *call << '\n');

  /* Check special cases */
  auto intrinsic = get_intrinsic(call);
  bool is_special_case = (intrinsic == Intrinsics::llvm_memcpy);
  if( is_special_case ) {
    convert_special_case_call(call);
    return;
  }

  /* Deal with map / packet pointer arguments */
  auto it = input_deps.find(call);
  if( it != input_deps.end() ) {
    /* Check read / write pointer usage */
    for( unsigned i = 0; i < call->arg_size(); i++ ) {
      auto* arg  = call->getOperand(i);
      auto it    = val_to_meta.find(arg);
      if( it == val_to_meta.end() )
        continue;
      auto* meta = it->second;

      /* Pull out the integer offset */
      auto* i2p    = cast<IntToPtrInst>(arg);
      auto* offset = i2p->getOperand(0);

      LLVM_DEBUG(
        dbgs() << "Converting arg " << i << " " << *arg << '\n'
               << "  Meta: " << *meta  << '\n';
      );

      arg_info ai;
      get_read_write_unknown(call, i, &ai);
      if( ai.rwu & UNKNOWN ) {
        errs() << "Unknown read / write pointer argument " << i << *arg
               << " in call " << *call << '\n';
        assert(false);
      }

      IRBuilder<> ir(call);
      IRBuilder<> ir_entry(call->getFunction()->getEntryBlock().getFirstNonPHI());
      /* Perform a read request into a local stack variable and feed that
      *  into the call */
      if( ai.rwu & READ ) {
        LLVM_DEBUG(dbgs() << "  Read ");
        auto* alloca = typed_alloca(arg->getType(), arg->getName() +
                                    "_in", ai.size, &ir_entry);

        CallInst* rd_op = nullptr;
        if( meta->is_map ) {
          LLVM_DEBUG(dbgs() << "map");
          /* Map read into the new variable */
          auto* map_rd  = create_nt_map_read(*call->getModule());
          Value* args[] = { call->getFunction()->arg_begin(), meta->base,
                            meta->key, meta->key_sz, alloca, offset, ai.size };
          rd_op         = ir.CreateCall(map_rd, args, arg->getName() + "_rd");
        } else {
          LLVM_DEBUG(dbgs() << "packet");
          /* Packet read into the new variable */
          auto* pkt_rd  = create_nt_packet_read(*call->getModule());
          Value* args[] = { meta->base, alloca, offset, ai.size };
          rd_op         = ir.CreateCall(pkt_rd, args, arg->getName() + "_rd");
        }

        call->setOperand(i, alloca);

        LLVM_DEBUG(
          dbgs() << '\n' << *alloca <<'\n'
                 << *rd_op << '\n'
                 << *call << '\n'
        );
      }

      /* Write the data to a stack variable, and then into the packet /
       * map. */
      if( ai.rwu & WRITE ) {
        LLVM_DEBUG(dbgs() << "  Write ");
        auto* alloca = typed_alloca(arg->getType(), arg->getName() +
                                    "_out", ai.size, &ir_entry);
        call->setOperand(i, alloca);

        /* Hack to allow inserting after the call */
        ir.SetInsertPoint(call->getParent(), ++call->getIterator());
        /* .. but make sure the debug information matches the call */
        ir.SetCurrentDebugLocation(call->getDebugLoc());

        CallInst* wr_op = nullptr;
        if( meta->is_map ) {
          /* Map write with new variable */
          LLVM_DEBUG(dbgs() << "map");
          auto* map_wr  = create_nt_map_write(*call->getModule());
          Value* args[] = { call->getFunction()->arg_begin(), meta->base,
                            meta->key, meta->key_sz, alloca, offset, ai.size };
          wr_op         = ir.CreateCall(map_wr, args, arg->getName() + "_wr");
        } else {
          /* Packet write with new variable */
          LLVM_DEBUG(dbgs() << "packet");
          auto* pkt_wr  = create_nt_packet_write(*call->getModule());
          Value* args[] = { meta->base, alloca, offset, ai.size };
          wr_op         = ir.CreateCall(pkt_wr, args, arg->getName() + "_wr");
        }

        LLVM_DEBUG(
          dbgs() << '\n' << *alloca <<'\n'
                 << *call << '\n'
                 << *wr_op << '\n'
        );
      }

      if (i2p->use_empty()) {
        i2p->eraseFromParent();
        val_to_meta.erase(it);
      }
    }
  }

  /* Convert function calls (if needed) */
  if( is_nanotube_root(*call) ) {
    convert_nanotube_root(call);
  }
}
void flow_conversion::one_dep_ready(Value *val) {
  for( auto u : val->users()) {
    if( dac.contains(u) ) {
      LLVM_DEBUG(dbgs() << "Marking one dep ready of " << *u << '\n');
      dac.mark_dep_ready(u);
      continue;
    }

    auto* inst = dyn_cast<Instruction>(u);
    if( inst == nullptr ) {
      errs() << "Unknown non-instruction " << *u
             << " in flow conversion!\n";
      assert( inst != nullptr);
    }

    unsigned inputs = input_deps[inst];
    assert( inputs > 0 );
    LLVM_DEBUG(dbgs() << "Adding " << *u << " with " << inputs
                      << " input deps.\n");
    /* Mark val as ready already */
    dac.insert(u, inputs - 1);
  }
}
void flow_conversion::convert_memcpy(Instruction* inst) {
  auto* dst     = inst->getOperand(0);
  auto* src     = inst->getOperand(1);
  auto* size    = inst->getOperand(2);

  auto dst_meta_it = val_to_meta.find(dst);
  auto src_meta_it = val_to_meta.find(src);
  nt_meta* dst_meta = (dst_meta_it != val_to_meta.end()) ?
                        dst_meta_it->second : nullptr;
  nt_meta* src_meta = (src_meta_it != val_to_meta.end()) ?
                        src_meta_it->second : nullptr;

  IRBuilder<> ir(inst);
  IRBuilder<> ir_entry(inst->getFunction()->getEntryBlock().getFirstNonPHI());

  // Ignore mem 2 mem transfer
  if( (dst_meta == nullptr) && (src_meta == nullptr) )
    return;

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

  // Pull out / add memory storage
  Value* mem = nullptr;
  if( src_meta == nullptr )
    mem = src;
  else if( dst_meta == nullptr )
    mem = dst;
  if( mem == nullptr )
    mem = ir_entry.CreateAlloca(ir.getInt8Ty(), size, "tmp_mem");
  if( mem->getType() != ir.getInt8PtrTy() )
    mem = ir.CreateBitCast(mem, ir.getInt8PtrTy(), mem->getName());

  LLVM_DEBUG(
    dbgs() << "Memory location: " << *mem << '\n';
  );

  /* Handle the source operands */
  if( (src_meta != nullptr) && !src_meta->is_map ) {
    /* Convert source to a packet read */
    auto* offset  = cast<IntToPtrInst>(src)->getOperand(0);
    auto* pkt_rd  = create_nt_packet_read(*inst->getModule());
    Value* args[] = { src_meta->base, mem, offset, size };
    auto* call    = ir.CreateCall(pkt_rd, args);

    LLVM_DEBUG(dbgs() << "Packet read: " << *call << '\n');
  } else if( (src_meta != nullptr) && src_meta->is_map ) {
    /* Convert source to a map read */
    auto* offset  = cast<IntToPtrInst>(src)->getOperand(0);
    auto* map_rd  = create_nt_map_read(*inst->getModule());
    Value* args[] = { inst->getFunction()->arg_begin(), src_meta->base,
                      src_meta->key, src_meta->key_sz, mem, offset, size };
    auto* call    = ir.CreateCall(map_rd, args);
    LLVM_DEBUG(dbgs() << "Map read: " << *call << '\n');
  }

  /* Handle destination operands */
  if( (dst_meta != nullptr) && !dst_meta->is_map ) {
    /* Convert destination to a packet write */
    auto* offset  = cast<IntToPtrInst>(dst)->getOperand(0);
    auto* pkt_wr  = create_nt_packet_write(*inst->getModule());
    Value* args[] = { dst_meta->base, mem, offset, size };
    auto* call    = ir.CreateCall(pkt_wr, args);
    LLVM_DEBUG(dbgs() << "Packet write: " << *call << '\n');
  } else if( (dst_meta != nullptr) && dst_meta->is_map ) {
    /* Convert destination to a map write */
    auto* offset  = cast<IntToPtrInst>(dst)->getOperand(0);
    auto* map_wr  = create_nt_map_write(*inst->getModule());
    Value* args[] = { inst->getFunction()->arg_begin(), dst_meta->base,
                      dst_meta->key, dst_meta->key_sz, mem, offset, size };
    auto* call    = ir.CreateCall(map_wr, args);
    LLVM_DEBUG(dbgs() << "Map write: " << *call << '\n');
  }
  inst->eraseFromParent();
  if( (dst != nullptr) && dst->use_empty() ) {
    cast<IntToPtrInst>(dst)->eraseFromParent();
    val_to_meta.erase(dst_meta_it);
  }
  if( (src != nullptr) && src->use_empty() ) {
    cast<IntToPtrInst>(src)->eraseFromParent();
    val_to_meta.erase(src_meta_it);
  }
}

flow_conversion::flow_conversion(Function& f) : m(*f.getParent()),
                                                c(m.getContext())
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
  inst->replaceAllUsesWith(repl);
  inst->eraseFromParent();
  if( i2p != nullptr && i2p->use_empty() ) {
    i2p->eraseFromParent();
    val_to_meta.erase(i2p);
  }
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

/* Check whether the given instruction propagates (or produces) map /
 * packet pointers. */
static
bool propagates_map_packet_ptr(Instruction* inst) {
  if( is_map_pointer_source(inst) || is_packet_pointer_source(inst) )
    return true;

  switch( inst->getOpcode() ) {
    case Instruction::Call: {
      auto in = get_intrinsic(inst);
      /* Handle known function calls */
      if( (in == Intrinsics::llvm_memset) ||
          (in == Intrinsics::llvm_memcpy) )
        return false;

      /* Bail for unknown call targets */
      errs() << "ERROR: Unkown map/pkt propagation for call\n" << *inst
             << "\ncalling: "
             << cast<CallInst>(inst)->getCalledFunction()->getName()
             << ".  Please inline / special case their handling.\n";
      abort();
    }

    /* Casts and pointer arithmetic that results in pointers keeps the map
     * / packet-ness */
    case Instruction::PtrToInt:
    case Instruction::IntToPtr:
    case Instruction::BitCast:
    case Instruction::GetElementPtr:
    case Instruction::PHI:
    case Instruction::Select:
    case Instruction::Trunc:
    case Instruction::Add:
      return true;

    /* The map / packet-ness does not propagate for these, because they are
     * not (logical) pointers anymore. */
    case Instruction::ICmp:
    case Instruction::Load:
    case Instruction::Store:
      return false;

    /* We can either subtract a non-pointer and get a pointer, meaning map
     * / packet-ness remains, or we subtract one pointer from another, thus
     * giving us an offset which is not map / packet anymore. */
    case Instruction::Sub:
      //XXX: Actually check the operands, here! NANO-263
      return false;

    default:
      errs() << "ERROR: Unkown map/pkt propagation for generic instruction"
             << *inst <<'\n';
      abort();
  }
  return false;
}

/* Compute the number of map / packet inputs per instruction */
static
void compute_map_packet_inputs(const std::vector<Instruction*>& roots,
    std::unordered_map<Instruction*, unsigned>* input_deps)
{
  std::unordered_set<Instruction*> todo;
  for( auto* i : roots)
    todo.insert(i);
  /* Propagate map / packet pointer-ness */
  std::unordered_set<Instruction*> done;
  while( !todo.empty() ) {
    auto it = todo.begin();
    auto* inst = *it;
    todo.erase(it);
    done.insert(inst);

    if( !propagates_map_packet_ptr(inst) )
      continue;

    /* Go through users of the map / packet pointer */
    LLVM_DEBUG(dbgs() << "Current: " << *inst <<'\n');
    for( auto* user : inst->users() ) {
      auto* ui = dyn_cast<Instruction>(user);
      if( ui == nullptr ) {
        LLVM_DEBUG(dbgs() << "Unexpected non-instruction user " << *ui << '\n');
        assert(ui);
      }
      LLVM_DEBUG(dbgs() << "  User: :" << *ui << '\n');
      /* Count the number of incident pointer flows to this instruction */
      (*input_deps)[ui]++;

      /* Continue if we have not already propagated this instruction */
      if( done.count(ui) == 0 )
        todo.insert(ui);
    }
  }

  LLVM_DEBUG(
    dbgs() << "\nInput deps:\n";
    for( auto& v : *input_deps )
      dbgs() << "  " << *v.first << " Deps: " << v.second << '\n';
  );
}

raw_ostream& operator<<(raw_ostream& os, const nt_meta& meta) {
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

bool mem_to_req::convert_to_req(Function& f) {
  flow_conversion fc(f);

  std::vector<Instruction*> roots;
  collect_roots(f, &roots);
  compute_map_packet_inputs(roots, &fc.input_deps);

  /* Find those roots that are ready to convert */
  LLVM_DEBUG( dbgs() << "\nStarting flow conversion with ready roots:\n" );
  for( auto* r : roots ) {
    if( fc.input_deps.find(r) == fc.input_deps.end() ) {
      fc.dac.insert_ready(r);
      LLVM_DEBUG(dbgs() << "  " << *r << '\n');
    }
  }

  fc.dac.execute(
    [&](dep_aware_converter<Value>* dac, Value* v) {
      fc.flow(v);
    });
  return true;
}
bool mem_to_req::runOnFunction(Function& f) {
  bool changes = false;

  changes |= convert_to_req(f);
  return changes;
}

char mem_to_req::ID = 0;
static RegisterPass<mem_to_req>
    X("mem2req", "Convert Nanotube L1 ld/st API to the request-based L2 API",
      false,
      false
      );

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
