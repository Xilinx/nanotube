/*******************************************************/
/*! \file Converge.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Merge map and packet accesses across basic blocks.
**   \date 2020-02-11
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/*!
** Converge merges map and packet accesses across basic blocks.  This is
** WIP, and should eventually be split into separate analysis and
** transformation passes.
**
** _Theory of Operation_
**
** The pass works by walking through the packet kernel and finds all packet
** / map accesses.  These are parsed and recorded.
**
** We then simplify the control flow graph (CFG) to only contain basic
** blocks (BBs) that contain a map / packet access while keeping the edges
** through other BBs intact by adding them in the reduced CFG.
**
** XXX: Stats
**
** Review the reduced CFG and construct a converge plan which tells us
** the order into which accesses will be converged.  Conceptually, the
** merge plan looks like this:
**
** ({packet_acc1, packet_acc2}, {map_acc1}, {map_acc2, map_acc3})
**
** It is a sequence of merge sets, in this example, the merge plan will
** result in a three access program: packet_acc, map_acc, map_acc.  The
** elements of each set are the accesses that will be merged together.
**
** The converge plan constructor ensures that accesses are compatible
** with one another, i.e., can actually be converged into a single
** access.
**
** XXX: How does the constructor work?
**
**/

//#define VISUALISE_CONVERGE_STEPS

#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CFGUpdate.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "../include/nanotube_api.h"
#include "../include/nanotube_utils.hpp"
#include "Dep_Aware_Converter.h"
#include "Intrinsics.h"
#include "Map_Packet_CFG.hpp"
#include "graphs.hpp"
#include "set_ops.hpp"


#define DEBUG_TYPE "converge"
using namespace llvm;
using namespace nanotube;

static cl::opt<bool> converge_stats("converge-stats",
    cl::desc("Print statistics for the converge pass"),
    cl::Hidden, cl::init(false));

namespace {
class converged_access_block;

static bool isMapAccess(const Instruction *inst) {
  if (!isa<CallInst>(inst))
    return false;
  nt_api_call c(const_cast<CallInst*>(cast<CallInst>(inst)));
  return c.is_map() && c.is_access();
}
static bool isPktAccess(const Instruction *inst) {
  if (!isa<CallInst>(inst))
    return false;
  nt_api_call c(const_cast<CallInst*>(cast<CallInst>(inst)));
  return c.is_packet();
}

static raw_ostream& operator<<(raw_ostream& os, MemoryLocation& mloc)
__attribute__ ((unused));
raw_ostream& operator<<(raw_ostream& os, MemoryLocation& mloc) {
  return os << "[Val: " << *mloc.Ptr << " Sz: " << mloc.Size <<"]";
}

struct Converge : public FunctionPass {
  static char ID;
  Converge() : FunctionPass(ID) {}

  /* Common Types */
  typedef Value* MapID;
  typedef std::map<BasicBlock*, std::vector<CallInst*>> bb_to_access_vec_t;
  typedef std::unordered_map<BasicBlock*, unsigned>     bb_to_unsigned_t;
  typedef std::map<MapID, std::vector<CallInst*>>       map_to_access_vec_t;
  typedef std::set<CallInst*>                           access_set_t;
  typedef dep_aware_converter<BasicBlock>               bb_dac_t;

  // Map helpers
  MapID getMapID(const CallInst *inst) {
    assert(isMapAccess(inst));
    assert(inst->getNumArgOperands() == 10);

    // XXX: The interface has changed here.. options:
    // * pull out the ID from the nanotube_map_create call for this
    // * or simply assume that different nanotube_map point to
    //   different maps
    Value *id = inst->getArgOperand(1);
    return id;

    /*
    assert(isa<ConstantInt>(id));
    auto *ci = cast<ConstantInt>(id);
    return ci->getZExtValue();
    */
  }
  enum map_access_t getMapAccessT(const CallInst *inst) {
    assert(isMapAccess(inst));
    assert(inst->getNumArgOperands() == 10);

    Value *type = inst->getArgOperand(2);

    assert(isa<ConstantInt>(type));
    auto *ci = cast<ConstantInt>(type);
    return (enum map_access_t)ci->getZExtValue();
  }
  void addMAP(Graph<MapID> &g, const CallInst *src,
              const CallInst *dst) {
    MapID sid = getMapID(src);
    MapID did = getMapID(dst);
    g.add(sid, did);
  }

  bool converge(Function& F);
  bool check_nanotube_api_level(Function& F);
  bool runOnFunction(Function &F) override {
    if( !check_nanotube_api_level(F) ) {
      errs() << "Please lower the Nanotube API level by linking to "
                "libnt/map_highlevel.bc and libnt/packet_highlevel.bc\n";
      assert(false);
    }
    //errs() << "Function: " << F.getName() << " is kernel: " << is_nt_packet_kernel(F) << '\n';
    if( !is_nt_packet_kernel(F) )
      return false;
    return converge(F);
  }
  bool record_map_packet_accesses(Function& f,
                                  bb_to_access_vec_t* bb_mas,
                                  bb_to_access_vec_t* bb_pktas,
                                  map_to_access_vec_t* map_mas);
  void construct_reduced_cfg(Function& f,
                             const bb_to_access_vec_t& bb_mas,
                             const bb_to_access_vec_t& bb_pktas,
                             LLGraph<BasicBlock>* reduced_cfg);
  void compute_max_access_tail(LLGraph<BasicBlock>& reduced_cfg,
         bb_to_unsigned_t* max_accesses_remaining);
  bool can_merge_accesses(const CallInst* acc1, const CallInst* acc2);
  void print_chain_stats(const LLGraph<BasicBlock>& reduced_cfg,
         const bb_to_unsigned_t& max_accesses_remaining);
  void pick_merge_set(access_set_t* result, const access_set_t& candidates,
                      const LLGraph<BasicBlock>& reduced_cfg,
                      bb_to_unsigned_t* max_accesses_remaining);
  bool can_converge(const CallInst* lhs, const CallInst* rhs);
  void construct_plan(std::vector<access_set_t>* converge_plan,
                      const LLGraph<BasicBlock>& reduced_cfg,
                      bb_to_unsigned_t* max_accesses_remaining);
  void execute_converge_plan(const std::vector<access_set_t>& converge_plan,
                             const LLGraph<BasicBlock>& reduced_cfg,
                             std::vector<converged_access_block*>* merge_blocks,
                             DominatorTree& dt, PostDominatorTree& pdt,
                             Function& f);
  BasicBlock* converge_into(converged_access_block* cab, CallInst* acc,
                            bool dummy, DominatorTree& dt,
                            PostDominatorTree& pdt);
  Instruction* find_dummy_insertion_place_before(CallInst* acc);


  enum dependence_type { NONE, RAW, OTHER, };
  dependence_type intra_bb_dep(Instruction* prod, Instruction* cons,
                               AliasAnalysis& aa, MemorySSA& mssa,
                               TargetLibraryInfo& tli);
  void merge_intra_bb_accesses(BasicBlock* bb, AliasAnalysis& aa,
                               MemorySSA& mssa, TargetLibraryInfo& tli)
                               __attribute__((unused));
  std::vector<Value*> get_outputs(Instruction* call);
  bool get_inputs(Value* val, std::vector<Value*>* input_vals);
  bool get_memory_inputs(Value* val,
                         std::vector<MemoryLocation>* input_mlocs,
                         std::vector<MemoryDef*>* input_mdefs,
                         AliasAnalysis& aa, MemorySSA& msaa,
                         TargetLibraryInfo& tli);

  std::unordered_set<BasicBlock*>
  backwards_reachable_between(BasicBlock* from_bb, BasicBlock* to_bb);
  void replace_targets(Instruction* term, BasicBlock* from,
                       BasicBlock* to);
  void weave_single_edge(Function& f, BasicBlock* src, BasicBlock* dst,
                         converged_access_block* out_hub,
                         DominatorTree* dt, PostDominatorTree* pdt,
                         std::unordered_set<BasicBlock*>* pads);

  BasicBlock *insertDummyMapAccess(BasicBlock *src, BasicBlock *dst,
                std::vector<MapID>::iterator ma_start,
                std::vector<MapID>::iterator ma_end, Function *map_op_fkt,
                std::map<MapID, std::vector<CallInst*>> *map_mas) {
    LLVMContext &C = src->getContext();
    Function  *F = src->getParent();

    LLVM_DEBUG(
      dbgs() << "Adding dummy map-accesses between BB " << src
             << " and " << dst << " to: { ";
      for (auto it = ma_start; it != ma_end; ++it)
        dbgs() << *it << " ";
      dbgs() << "}\n";
    );

    BasicBlock *bb = BasicBlock::Create(C, "dummy_ma", F);
    ConstantInt *type   = ConstantInt::get(IntegerType::get(C, 32),
                                           NANOTUBE_MAP_NOP, true);
    ConstantInt *length = ConstantInt::get(IntegerType::get(C, 64),
                                           0, true);
    Constant *null_ptr  = Constant::getNullValue(Type::getInt8PtrTy(C));
    SmallVector<Value *, 9> args = {nullptr, type, null_ptr, length,
                                    null_ptr, null_ptr, null_ptr,
                                    length, length};
    for (auto it = ma_start; it != ma_end; ++it) {
      // ConstantInt *map_id = ConstantInt::get(IntegerType::get(C, 32),
      //                    *it, true);
      args[0] = *it;
      CallInst *call = CallInst::Create(map_op_fkt, args,
                                        "call_mopnop", bb);
      (*map_mas)[*it].push_back(call);
    }
    BranchInst::Create(dst, bb);

    // Patch up the "receiving" PHI nodes
    // NOTE: Not available in LLVM 8
    //dst->replacePhiUsesWith(src, bb);
    for (auto &inst : *dst) {
      PHINode *phi = dyn_cast<PHINode>(&inst);
      if (!phi)
        break;
      int i;
      while ((i = phi->getBasicBlockIndex(src)) != -1)
        phi->setIncomingBlock(i, bb);
    }

    // Patch up the original call site
    Instruction *term = src->getTerminator();
    if (isa<BranchInst>(term)) {
      BranchInst *src_br = cast<BranchInst>(term);
      for (unsigned i = 0; i < src_br->getNumSuccessors(); i++) {
        if (src_br->getSuccessor(i) == dst)
          src_br->setSuccessor(i, bb);
      }
    } else if (isa<SwitchInst>(term)) {
        SwitchInst *src_sw = cast<SwitchInst>(term);
      for (unsigned i = 0; i < src_sw->getNumSuccessors(); i++) {
        if (src_sw->getSuccessor(i) == dst)
          src_sw->setSuccessor(i, bb);
      }
    } else {
      errs() << "Unknown terminator " << *term << " in BB " << src
             << '\n';
      assert(false && "Only handling branch and switch terminators "\
             "for now!");
    }

    LLVM_DEBUG(dbgs() << "New basic block: " << *bb);
    return bb;
  }

  template <typename BB, typename IT>
  IT next_map_packet_access(BB& bb, IT start) {
    for( auto ii = start; ii != bb.end(); ++ii ) {
      auto* inst = &(*ii);
      if( isMapAccess(inst) || isPktAccess(inst) )
        return ii;
    }
    return bb.end();
  }
  template <typename BB>
  typename BB::const_iterator next_map_packet_access(const BB& bb) {
    return next_map_packet_access(bb, bb.begin());
  }
  template <typename BB>
  typename BB::iterator next_map_packet_access(BB& bb) {
    return next_map_packet_access(bb, bb.begin());
  }

  void getAnalysisUsage(AnalysisUsage &info) const override {
    info.addRequired<DominatorTreeWrapperPass>();
    info.addRequired<PostDominatorTreeWrapperPass>();
    info.addRequired<MemorySSAWrapperPass>();
    info.addRequired<AAResultsWrapperPass>();
    info.addRequired<TargetLibraryInfoWrapperPass>();
    // TODO: Add things we preserve
  }

  void repair_def_dominance(Function&  f, DominatorTree& dt,
                            PostDominatorTree& pdt);
  void weave_bypassing_flows(Function& f,
                             const std::vector<converged_access_block*>& merge_blocks,
                             DominatorTree& dt, PostDominatorTree& pdt);
};

/**
 * The converged access block represents a convergence point (a special
 * basic block) that concentrates all control flow and has no edges going
 * past it.
 */
class converged_access_block {
public:
  converged_access_block(CallInst* example, unsigned fan_io);
  void add_access(BasicBlock* src, CallInst* access, BasicBlock* dst);
  void add_dummy_access(BasicBlock* src, BasicBlock* dst);
  BasicBlock* get_bb() const {return bb;}
  CallInst*   get_call() const {return call.get_call();}
  void simplify(DominatorTree& dt);
  bool remove_pointer_phis();
  bool convert_phi_of_addr(MemoryLocation& loc, bool is_input);
  BasicBlock* get_successor_bb_for(BasicBlock* pred);
private:
  BasicBlock*         bb;
  unsigned            fan_io;
  unsigned            cur_path;
  PHINode*            path_phi;
  std::vector<Value*> arg_phis;
  nt_api_call         call;
  SwitchInst*         switch_inst;
  BasicBlock*         unreachable_bb;
};

} // anonymous namespace

bool Converge::record_map_packet_accesses(Function& f,
                                          bb_to_access_vec_t* bb_mas,
                                          bb_to_access_vec_t* bb_pktas,
                                          map_to_access_vec_t* map_mas) {
  for (auto &bb : f) {
    LLVM_DEBUG(dbgs() << "BB: " << &bb << " " << bb << '\n');
    // Read block and record accesses
    for (auto &i : bb) {
      if (isMapAccess(&i)) {
        CallInst *ci = cast<CallInst>(&i);
        (*bb_mas)[&bb].push_back(ci);
        (*map_mas)[getMapID(ci)].push_back(ci);
      } else if (isPktAccess(&i)) {
        (*bb_pktas)[&bb].push_back(cast<CallInst>(&i));
      }
    }
  }
  return !(bb_mas->empty() && bb_pktas->empty());
}

void Converge::construct_reduced_cfg(Function& f,
                                     const bb_to_access_vec_t& bb_mas,
                                     const bb_to_access_vec_t& bb_pktas,
                                     LLGraph<BasicBlock>* reduced_cfg) {
  // Initialise the reduced CFG
  for (auto &bb : f)
    reduced_cfg->add(&bb);
  LLVM_DEBUG(dbgs() << "Full CFG\n" << reduced_cfg << '\n');

  // Reduce: remove those basic blocks that don't have a map access
  for (auto &bb : f) {
    auto has_ma   = bb_mas.find(&bb);
    auto has_pkta = bb_pktas.find(&bb);
    if (has_ma == bb_mas.end() && has_pkta == bb_pktas.end()) {
      // bb has no map accesses -> remove from CFG'
      reduced_cfg->unlink_remove(&bb);
    }
  }
  LLVM_DEBUG(dbgs() << "Reduced CFG\n" << reduced_cfg << '\n');
}

void Converge::compute_max_access_tail(LLGraph<BasicBlock>& reduced_cfg,
                 bb_to_unsigned_t* max_accesses_remaining) {
  dep_aware_converter<BasicBlock> dac;

  /* Go through the reduced CFG in reverse order */
  for( auto& bb_node : reduced_cfg.T_pss ) {
    auto* bb        = bb_node.first;
    auto  num_succs = bb_node.second.succs.size();

    dac.insert(bb, num_succs);
  }

  dac.execute(
    [&](dep_aware_converter<BasicBlock>* dac, BasicBlock* n) {
      LLVM_DEBUG(dbgs() << "Executing node: \n" << n << '\n');

      /* Count our own accesses */
      auto it = next_map_packet_access(*n);
      unsigned count = 0;
      while( it != n->end() ) {
          it = next_map_packet_access(*n, ++it);
          count++;
      }

      /* Find the maximum number of accesses across all our
       * successors */
      unsigned max = 0;
      for( auto succ : reduced_cfg.succs(n) ) {
          auto it = max_accesses_remaining->find(succ);
          assert(it != max_accesses_remaining->end());
          if( it->second > max)
              max = it->second;
      }
      LLVM_DEBUG(
        dbgs() << "Own map & packet accesses: " << count << '\n'
               << "Longest tail:              " << max   << '\n');
      (*max_accesses_remaining)[n] = count + max;

      /* Now process all our predecessors */
      for(auto pred : reduced_cfg.preds(n) )
          dac->mark_dep_ready(pred);
    });
}

bool Converge::can_merge_accesses(const CallInst* acc1, const CallInst* acc2) {
  auto int1 = get_intrinsic(acc1);
  auto int2 = get_intrinsic(acc2);
  bool p1 = isPktAccess(acc1);
  bool p2 = isPktAccess(acc2);
  if( p1 && p2 ) {
    if( int1 == Intrinsics::packet_read && int1 == int2 ) {
      //XXX: Some distance analysis
      return true;
    }
    if( int1 == Intrinsics::packet_write_masked && int1 == int2 ) {
      //XXX: Some distance analysis
      return true;
    }
    return false;
  } else if (!p1 && !p2) {
    if( int1 != Intrinsics::map_op || int1 != int2)
      return false;
    auto mi1 = getMapID(acc1);
    auto mi2 = getMapID(acc2);
    if( mi1 != mi2 )
      return false;
    auto t1 = getMapAccessT(acc1);
    auto t2 = getMapAccessT(acc2);
    if( t1 != t2 )
      return false;
    if( acc1->getOperand(3) != acc2->getOperand(3) )
      return false;
    return true;
  } else {
    return false;
  }

}
void Converge::print_chain_stats(const LLGraph<BasicBlock>& reduced_cfg,
                 const bb_to_unsigned_t& mar) {
  /* The root of the reduced CFG has the length of the longest chain */
  BasicBlock* root = nullptr;
  for( auto& bb_node : reduced_cfg.T_pss ) {
    if ( bb_node.second.preds.size() == 0 ) {
      root = bb_node.first;
      break;
    }
  }
  assert(root != nullptr);
  dbgs() << "Converge longest chain: " << mar.at(root) << '\n';

  /* Follow one of the longest chains and compute merge opportunities
   * there.  Note, this obviously is only a single chain, and another chain
   * of the same (or slightly lower) length may have much worse merge
   * potential.  But this is still useful to get an idea. */
  dbgs() << "Converge chain entries: ";
  unsigned mergeable       = 0;
  unsigned mergeable_in_bb = 0;
  unsigned min_len         = 0;
  unsigned min_len_bb      = 0;
  BasicBlock* chosen = root;
  CallInst* prev = nullptr;
  while( chosen != nullptr) {
    auto access = next_map_packet_access(*chosen);
    bool new_bb = true;
    /* Simply start and go through the basic block... */
    while( access != chosen->end() ) {
      dbgs() << *access << '\n';
      if( prev != nullptr && can_merge_accesses(prev, cast<CallInst>(&*access)) ) {
        mergeable++;
        if( !new_bb )
          mergeable_in_bb++;
        else
          min_len_bb++;
      } else {
        min_len++;
        min_len_bb++;
      }
      new_bb = false;
      prev = cast<CallInst>(&*access);
      access = next_map_packet_access(*chosen, ++access);
    }
    dbgs() << "--------\n";
    /* .. and then follow the successor with the longest chain. */
    unsigned n = 0;
    auto& succs = reduced_cfg.succs(chosen);
    chosen = nullptr;
    for( BasicBlock* succ : succs ) {
      if( mar.at(succ) > n ) {
        n = mar.at(succ);
        chosen = succ;
      }
    }
  }
  dbgs() << "Converge mergeable: " << mergeable << " / " << mergeable_in_bb
         << " (intra-BB)\n"
         << "Resulting length " << min_len << " / " << min_len_bb
         << " (intra-BB)\n";

}

/**
 * Gets a list of all values that (potentially) influence this value.
 *
 * This is all the values this instruction is evaluating (with an influence
 * on the output), and all the stores / calls that modify memory locations
 * this load / call is reading from.
 */
bool Converge::get_inputs(Value* val, std::vector<Value*>* input_vals) {
  assert(input_vals  != nullptr);

  auto& out = *input_vals;
  /* Special memory-sensitive instructions */
  /* Calls may read pointer arguments */
  if( isa<CallInst>(val) ) {
    auto* call = cast<CallInst>(val);

    /* Simply add all non-constant call arguments */
    std::vector<unsigned> call_ptr_ins;
    for( auto& u : call->args() ) {
      auto* arg = u.get();
      if( !isa<Constant>(arg) )
        out.push_back(arg);
    }

  } else if( isa<LoadInst>(val) ){
    auto* ld = cast<LoadInst>(val);
    // XXX: Check the pointer for aliases and add stores / calls
    errs() << "XXX: Converge::get_inputs() - Implement me: " << *ld
           << '\n';
    errs() << "This will be part of the work for NANO-52\n";
    return false;

  } /* Non-memory sensitive instructions */
  else if( isa<StoreInst>(val) ){
    /* Stores only depend on the data being stored */
    auto* st = cast<StoreInst>(val);
    out.push_back(st->getValueOperand());

  } else  if( isa<PHINode>(val) ){
    auto* phi = cast<PHINode>(val);
    for( auto& u : phi->incoming_values() )
      out.push_back(u.get());

  } else if( isa<SelectInst>(val) ){
    auto* sel = cast<SelectInst>(val);
    out.push_back(sel->getCondition());
    out.push_back(sel->getTrueValue());
    out.push_back(sel->getFalseValue());

  } else  if( isa<User>(val) ){
    auto* user = cast<User>(val);
    /* Generic user -> add all operands */
    for( auto* v : user->operand_values() )
      out.push_back(v);

  } else {
    errs() << "XXX: unknown value inputs for " << *val << '\n';
    return false;
  }
  return true;
}

/**
 * Returns the memory inputs that are consumed by this instruction /
 * function call.
 */
bool Converge::get_memory_inputs(Value* val,
                          std::vector<MemoryLocation>* input_mlocs,
                          std::vector<MemoryDef*>* input_mdefs,
                          AliasAnalysis& aa, MemorySSA& mssa,
                          TargetLibraryInfo& tli) {
  assert(input_mlocs != nullptr);
  assert(input_mdefs != nullptr);
  /* Special memory-sensitive instructions */
  /* Calls may read pointer arguments */
  if( isa<CallInst>(val) ) {
    auto* call = cast<CallInst>(val);

    /* Skip functions that don't read memory */
    auto fmrb = aa.getModRefBehavior(call);
    if( AAResults::doesNotReadMemory(fmrb) )
      return true;
    /* Give up on functions that are all over memory */
    if( !AAResults::onlyAccessesInaccessibleOrArgMem(fmrb) ) {
      errs() << "XXX: Converge::get_memory_inputs() - Not analysing "
             << "memory-crazy call: " << *call << '\n';
      errs() << "This will be needed for NANO-52\n";
      return false;
    }

    /* Record memory locations that this function reads from */
    for( auto& u : call->args() ) {
      auto* arg = u.get();
      if( !arg->getType()->isPointerTy() )
        continue;

      unsigned idx = u.getOperandNo();
      auto  mri = aa.getArgModRefInfo(call, idx);
      if( !isRefSet(mri) )
        continue;
      auto mloc = MemoryLocation::getForArgument(call, idx, tli);
      input_mlocs->emplace_back(mloc);
      LLVM_DEBUG(dbgs() << "    Arg: " << idx << " Mem: " << mloc << '\n');
    }

    /* We now have a sensible function that accesses memory.  Go forth and
     * try to make sense of the madness that is memory dataflow :) */
    auto* mem = mssa.getMemoryAccess(call);
    LLVM_DEBUG(
      dbgs() << "  Call: " << *call << " MemSSA: " << *mem << '\n');

    /* Go through all potential memory definitions for this and try to find
     * the previous writer */
    for( auto it = mem->defs_begin(); it != mem->defs_end(); ++it ) {
      auto* def = dyn_cast<MemoryDef>(*it);
      if( def == nullptr ) {
        errs() << "XXX: Converge::get_memory_inputs() - Unhandled memory "
               << "def " << **it << " for " << *call << '\n';
        errs() << "This will be needed for NANO-52\n";
        return false;
      }
      input_mdefs->push_back(def);
      LLVM_DEBUG(dbgs() << "    Def: " << **it << '\n');
    }

  } else if( isa<LoadInst>(val) ){
    auto* ld = cast<LoadInst>(val);

    auto mloc = MemoryLocation::get(ld);
    input_mlocs->emplace_back(mloc);
    dbgs() << "    Mem: " << mloc << '\n';

    auto* mem = mssa.getMemoryAccess(ld);
    LLVM_DEBUG(
      dbgs() << "  Load: " << *ld << " MemSSA: " << *mem << '\n');

    /* Go through all potential memory definitions for this and try to find
     * the previous writer */
    for( auto it = mem->defs_begin(); it != mem->defs_end(); ++it ) {
      auto* def = dyn_cast<MemoryDef>(*it);
      if( def == nullptr ) {
        errs() << "XXX: Unhandled memory def " << **it << " for " << *ld
               << '\n';
        return false;
      }
      input_mdefs->push_back(def);
      LLVM_DEBUG(dbgs() << "    Def: " << **it << '\n');
    }

  } else if( isa<StoreInst>(val) ){
    /* Stores have no memory INPUT dependency -> fall through! */
  } else {
    errs() << "XXX: unknown memory inputs for " << *val << '\n';
    return false;
  }
  return true;
}

#ifdef XXX_CONVERT_THIS_INTO_TRAVERSAL
      auto* def_inst = def->getMemoryInst();
      LLVM_DEBUG(dbgs() << "    Store Inst: " << *def_inst << '\n');
      auto* st = dyn_cast<StoreInst>(def_inst);
      if( st == nullptr ) {
        errs() << "XXX: Unhandled clobber instruction: "
               << *def->getMemoryInst() << '\n';
        return false;
      }

      auto mloc_st = MemoryLocation::get(st);
      dbgs() << "  Mloc (st): " << mloc_st << '\n';
      for( auto call_ptr_in : call_ptr_ins ) {
        auto mloc_arg = MemoryLocation::getForArgument(call, call_ptr_in, tli);

        auto aa_res = aa.alias(mloc_st, mloc_arg);
        LLVM_DEBUG(
          dbgs() << "  Arg " << call_ptr_in << " "
                 << *call->getArgOperand(call_ptr_in) << '\n';
          dbgs() << "  Mloc: " << mloc_arg << '\n';
          dbgs() << "    AA: " << aa_res << '\n';
        );
        if( aa_res != NoAlias ) {
          LLVM_DEBUG(dbgs() << "    Alias: " << *st << '\n');
          out.push_back(st);
        } else {
          /* NOTE: I am very unsure how AA and MemorySSA interact.  Would I
           * have to walk the def chain upwards, i.e., look at the previous
           * def now in case that aliases with this pointer? */
          // XXX: Probably the right thing to do here is to let hte caller
          // walk the chain backwards, but only to the first merge
          // candidate!  I.e., return the MemoryDef as a thing to walk?
          LLVM_DEBUG(dbgs() << "    No-Alias: " << *st << '\n');
          ;
        }
      }
    }
#endif

/**
 * Get the outputs of an instruction; this is the instruction value itself,
 * but also any side effects (memory writes) it may have.
 */
std::vector<Value*> Converge::get_outputs(Instruction* inst) {
  std::vector<Value*> out;
  out.push_back(inst);
  if( isa<CallInst>(inst) ){
    auto* call = cast<CallInst>(inst);
    // XXX: Query all arguments, check output memory locations and add them
    // XXX: Make sure the tracing back in get_inputs eventually matches
    // what we output here
    errs() << "XXX: Converge::get_outputs() - Implement me: " << *call
           << '\n';
    errs() << "This is part of NANO-52\n";
  } else if( isa<StoreInst>(inst) ){
    auto* st = cast<StoreInst>(inst);
    // XXX: Make sure that this matches up what get_inputs is producing
    errs() << "XXX: Converge::get_outputs() - Implement me: " << *st
           << '\n';
    errs() << "This is part of NANO-52\n";
  }
  return out;
}

/**
 * A very simple dependence analyser for two instructions inside a single
 * basic block.
 */
Converge::dependence_type Converge::intra_bb_dep(Instruction* prod,
                                                 Instruction* cons,
                                                 AliasAnalysis& aa,
                                                 MemorySSA& mssa,
                                                 TargetLibraryInfo& tli ) {
  assert( prod->getParent() == cons->getParent() );
  auto prod_out = get_outputs(prod);
  std::unordered_set<Value*> outs(prod_out.begin(), prod_out.end());

  /* backwards tracking of operands */
  std::vector<User*>         worklist;
  std::unordered_set<Value*> visited;
  worklist.push_back(cons);
  while( !worklist.empty() ) {
    auto* user = worklist.back();
    worklist.pop_back();
    visited.insert(user);

    std::vector<Value*>         input_vals;
    std::vector<MemoryLocation> input_mlocs;
    std::vector<MemoryDef*>     input_mdefs;

    bool success = get_inputs(user, &input_vals);
    if( !success )
      return OTHER;

    auto* inst = dyn_cast<Instruction>(user);
    if( (inst != nullptr) && inst->mayReadOrWriteMemory() ) {
      success = get_memory_inputs(user, &input_mlocs, &input_mdefs, aa, mssa, tli);
      if( !success )
        return OTHER;

      /* Walk the memory information and collect dependencies */
      auto* walker = mssa.getWalker();
      auto* memacc = walker->getClobberingMemoryAccess(inst);
      dbgs() << "Walking the walker...\n";
      while( true ) {
        if( mssa.isLiveOnEntryDef(memacc) )
          break;

        dbgs() << "  Memory access " << *memacc << '\n';
        auto* memdef = dyn_cast<MemoryDef>(memacc);
        if( memdef != nullptr ) {
          auto* meminst = memdef->getMemoryInst();
          /* If it is from another BB it cannot be influenced! */
          if( meminst->getParent() != prod->getParent() )
            break;
          dbgs() << "  Inst: " << *meminst << '\n';
          /* This is a cheap-ass version for a check whether meminst is
           * before prod */
          if( meminst == prod )
            break;
        }
        memacc = walker->getClobberingMemoryAccess(memacc);
      }

#ifdef WALKING_DEFS
      auto* memdef = input_mdefs.front();
      dbgs() << "Walking the defs...\n";
      while( true ) {
        auto* meminst = memdef->getMemoryInst();
        if( meminst->getParent() != prod->getParent() )
          break;
        dbgs() << "  Memory access " << *memdef << '\n';
        dbgs() << "  Inst: " << *memdef->getMemoryInst() << '\n';
        /* This is a cheap-ass version for a check whether meminst is
         * before prod */
        if( meminst == prod )
          break;
        memdef = dyn_cast<MemoryDef>(memdef->getDefiningAccess());
        if( (memdef == nullptr) || mssa.isLiveOnEntryDef(memdef) )
          break;
      }
#endif
    }

    /* Update our worklist with new information */
    for( auto* val : input_vals ) {
      /* Ignore values we have already analysed */
      if( visited.count(val) != 0 )
        continue;
      /* If we find an output, then we have a dependency! */
      if( outs.count(val) != 0 )
        return RAW;

      auto* u = dyn_cast<User>(val);
      if( u == nullptr ) {
        dbgs() << "  Ignoring non-user value " << *val << '\n';
        continue;
      }

      /* Check if instructions are from another basic block -> if so they
       * cannot be part of an intra-BB dependency! */
      auto* inst = dyn_cast<Instruction>(u);
      if( inst != nullptr ) {
        if( inst->getParent() != prod->getParent() ) {
          dbgs() << "  Ignoring input " << *inst << " from another BB\n";
          continue;
        }
      }
      dbgs() << "  Adding " << *u << " to worklist\n";
      worklist.push_back(u);
    }

  }
  return NONE;
}

/**
 * Go through Nanotube map / packet accesses in the current block and try
 * to merge them.
 */
void Converge::merge_intra_bb_accesses(BasicBlock* bb, AliasAnalysis& aa,
                                       MemorySSA& mssa,
                                       TargetLibraryInfo& tli) {
  auto first  = next_map_packet_access(*bb);
  auto second = next_map_packet_access(*bb, std::next(first));
  while( second != bb->end() ) {
    auto* c1 = cast<CallInst>(&*first);
    auto* c2 = cast<CallInst>(&*second);
    if( can_merge_accesses(c1, c2) ) {
      LLVM_DEBUG(dbgs() << "Looking at merging " << *first
                        << " and " << *second << '\n');
      auto dep = intra_bb_dep(&*first, &*second, aa, mssa, tli);
      LLVM_DEBUG(dbgs() << "  Dep: " << (unsigned)dep << '\n');
    }
    first  = second;
    second = next_map_packet_access(*bb, std::next(first));
  }
}

bool Converge::can_converge(const CallInst* lhs, const CallInst* rhs) {
  bool map_l, map_r;
  map_l = isMapAccess(lhs);
  map_r = isMapAccess(rhs);
  Intrinsics::ID id_l, id_r;
  id_l  = get_intrinsic(lhs);
  id_r  = get_intrinsic(rhs);

  /* A map access cannot merge with a packet access! */
  if( map_l != map_r )
    return false;

  if( map_l && map_r ) {
    assert(id_l == Intrinsics::map_op);
    assert(id_r == Intrinsics::map_op);
    /* Two map accesses can merge if they are:
     *   - to the same map (operand 1)
     *   - of the same type / map_op (operand 2)
     * XXX: Adjust offsets for the right level of Nanotube interface!
     * XXX: Pull the get_operand out here
     */
    return (id_l == id_r) &&
           (lhs->getOperand(1) == rhs->getOperand(1)) &&
           (lhs->getOperand(2) == rhs->getOperand(2));
  } else {
    /* Two packet access can merge if they are
     *   - of the same type
     *   - of the same size
     */
    bool is_read  = (id_l == Intrinsics::packet_read);
    bool is_write = (id_l == Intrinsics::packet_write_masked);
    return (id_l == id_r) &&
           (!is_read  || (lhs->getOperand(3) == rhs->getOperand(3))) &&
           (!is_write || (lhs->getOperand(4) == rhs->getOperand(4)));
  }
}
void Converge::pick_merge_set(Converge::access_set_t* result,
                              const Converge::access_set_t& candidates,
                              const LLGraph<BasicBlock>& reduced_cfg,
                              bb_to_unsigned_t* max_accesses_remaining) {
  /* Trivial case: merge only a single entry :) */
  if( candidates.size() == 1 ) {
    result->insert(*candidates.begin());
    return;
  }

  LLVM_DEBUG(dbgs() << "Picking merge candidates from " << candidates
                    << '\n');
  /* Find the access / bb with the longest chain */
  CallInst* critical_access = nullptr;
  unsigned  criticality     = 0;
  for( auto* acc : candidates ) {
    BasicBlock* bb = acc->getParent();
    unsigned n     = (*max_accesses_remaining)[bb];
    if( n > criticality ) {
      criticality     = n;
      critical_access = acc;
    }
  }
  assert(critical_access != nullptr);
  LLVM_DEBUG(dbgs() << "Most critical access: " << *critical_access
                    << " length: " << criticality << '\n');
  result->insert(critical_access);

  /* Add all potential merge candidates */
  for( auto* acc : candidates ) {
    if( acc == critical_access )
      continue;
    if( can_converge(acc, critical_access) )
      result->insert(acc);
  }

  LLVM_DEBUG(dbgs() << "Picked accesses: \n";
    for( auto* acc : *result)
      dbgs() << "  " << *acc << '\n';
  );
}

void Converge::construct_plan(std::vector<access_set_t>* converge_plan,
                              const LLGraph<BasicBlock>& reduced_cfg,
                              bb_to_unsigned_t* max_accesses_remaining) {
  typedef std::unordered_map<BasicBlock*, BasicBlock::iterator>
            bb_to_access_t;

  bb_dac_t       dac;
  bb_to_access_t bb_access;

  /* Setup: insert the nodes for in-order traversal */
  for( auto bb_node : reduced_cfg.T_pss ) {
    BasicBlock* bb   = bb_node.first;
    auto&       node = bb_node.second;
    dac.insert(bb, node.preds.size());
    /* Record first access per BasicBlock */
    bb_access[bb] = next_map_packet_access(*bb);
  }

  /*
   * Execution: go through and process the frontier of packet / map
   * accesses.  Figure out which accesses from the frontier should be
   * merged and advance the frontier there.
   */
  dac.execute(
    [&](bb_dac_t* dac, const bb_dac_t::items_t& candidates,
        bb_dac_t::items_t* processed) {
      LLVM_DEBUG(
        dbgs() << "In order traversal input: \n";
        for( auto* bb : candidates )
          dbgs() << "  " << bb << " starting: " << *bb->begin() << '\n';
      );

      /* Pull out the actual map / packet accesses into access_cand */
      access_set_t access_cand;
      for( auto* bb : candidates )
        access_cand.insert(&cast<CallInst>(*bb_access[bb]));

      LLVM_DEBUG(
        dbgs() << "Available accesses:\n";
        for( auto* acc : access_cand )
          dbgs() << "  " << *acc << '\n';
      );

      /* Pick the accesses that will be converged */
      access_set_t picked;
      pick_merge_set(&picked, access_cand, reduced_cfg, max_accesses_remaining);
      converge_plan->push_back(picked);

      LLVM_DEBUG(
        dbgs() << "Picked accesses:\n";
        for( auto* acc : picked )
          dbgs() << "  " << *acc << '\n';
      );

      /* Push the frontier past the picked accesses */
      for( auto* acc : picked ) {
        auto* bb = acc->getParent();
        assert( bb_access.find(bb)  != bb_access.end() );
        assert( candidates.find(bb) != candidates.end() );

        /* The critical length for this BasicBlock reduced */
        (*max_accesses_remaining)[bb]--;
        /* Get the next access for this BasicBlock */
        auto it = next_map_packet_access(*bb, ++bb_access[bb]);
        bb_access[bb] = it;
        if( it == bb->end() ) {
          /* If we reached the end of the BasicBlock, mark its successors
             ready. They will then pop up in candidates, next */
          processed->insert(bb);
          for( auto* succ : reduced_cfg.succs(bb))
            dac->mark_dep_ready(succ);
        }
      }
    }
  );
  LLVM_DEBUG(
    dbgs() << "Full converge plan\n";
    for( auto& s : *converge_plan ) {
      dbgs() << "  Node:\n";
      for( auto* acc : s ) {
        dbgs() << "  | " << *acc << '\n';
      }
      dbgs() << "  L\n";
    }
  );
}

Instruction*
Converge::find_dummy_insertion_place_before(CallInst* acc) {
  BasicBlock* bb      = acc->getParent();
  bool first_in_block = (&*next_map_packet_access(*bb) == acc);

  if( first_in_block ) {
    /* If we are the first, split as early in the BB as possible */
    /* NOTE: I tried edge splitting, but it was too tedious, so just
     * splitting off the head here */
    return bb->getFirstNonPHI();
  } else {
    /* This block has another access before this one, maybe we can inch the
     * split point back a wee bit.  Ideally we'd find the point of lowest
     * live state between the two NT accesses here, but that is a little
     * tedious (maybe?) to program. */
    Instruction* split = acc;
    auto*        prev  = split->getPrevNode();
    while( !isMapAccess(prev) && !isPktAccess(prev) ) {
      /* Simple heuristic for now: if we find something that is not used by
       * the next NT operation, split there. */
      if( !acc->hasArgument(split) )
        break;
      split = prev;
      prev  = split->getPrevNode();
    }
    return split;
  }
  return acc;
}

BasicBlock*
Converge::converge_into(converged_access_block* cab, CallInst* acc,
                         bool dummy, DominatorTree& dt,
                         PostDominatorTree& pdt) {
  BasicBlock* next_bb;
  BasicBlock* bb = acc->getParent();


  if( !dummy ) {
    /* Split the BasicBlock right before the access */
    /* XXX: Careful here, this updates only the DT, not the PDT.  Sadly
     * there is no code in this LLVM version to also update the PDT :( */
    next_bb = SplitBlock(bb, acc, &dt);
    next_bb->setName(bb->getName().take_front(10));
    /* Converge this access away */
    cab->add_access(bb, acc, next_bb);

    /* Unlink the now useless access, delete at the end */
    acc->replaceAllUsesWith(cab->get_call());
    acc->removeFromParent();
  } else {
    /* The dummy access has more freedom to be placed somewhere.  Variable
     * acc is another access waiting to be converged.  Instead of splitting
     * its basic block bb at acc, place it somewhere smarter.
     *
     * NOTE: I tried splitting existing edges, but it turned out to be too
     *       tedious.  So split at the head if necessary. */
    auto* split = find_dummy_insertion_place_before(acc);
    /* XXX: Does not update PDT, see above! */
    next_bb = SplitBlock(bb, split, &dt);
    next_bb->setName(bb->getName().take_front(10));
    cab->add_dummy_access(bb, next_bb);
  }

  /* Link the first part of the BB to the converged access */
  //assert(bb->getUniqueSuccessor() != nullptr);
  replace_targets(bb->getTerminator(), next_bb, cab->get_bb());

  LLVM_DEBUG(dbgs() << "BB first half:" << *bb << '\n'
                    << "BB converged access:"
                    << *cab->get_bb() << '\n'
                    << "BB remainder:" << *next_bb << '\n');

  auto* cabbb = cab->get_bb();
  typedef cfg::UpdateKind U;

  /* For PDT, do both the SplitBlock induced changes and the converge
   * through the CAB change in one update.  The PDT update code checks the
   * actual CFG, and so when we only do the SplitBlock update here and
   * combine DT & PDT below, that will not work, as the CFG has already
   * advanced past the state right after SplitBlock here (we have merged
   * the flow through the CAB already!) */
  std::vector<DominatorTree::UpdateType> pdt_sb;
  /* Threading through CAB */
  pdt_sb.emplace_back(U::Insert, bb, cabbb);
  pdt_sb.emplace_back(U::Insert, cabbb, next_bb);
  /* SplitBlock induced effects: bb -> bb + next_bb */
  std::unordered_set<BasicBlock*> done;
  for( auto* succ : successors(next_bb) ) {
    if( done.count(succ) != 0 )
      continue;
    pdt_sb.emplace_back(U::Delete, bb, succ);
    pdt_sb.emplace_back(U::Insert, next_bb, succ);
    done.insert(succ);
  }
  LLVM_DEBUG(
    dbgs() << "Updating PDT with \n";
    for( auto& up : pdt_sb ) {
      dbgs() << "  " << ((up.getKind() == U::Insert) ? "Insert" : "Delete")
             << " From: " << up.getFrom()->getName()
             << " To: "   << up.getTo()->getName() << '\n';
    }
  );
  pdt.applyUpdates(pdt_sb);

  /* Update DT with new edges: just threading through CAB instead of direct
   * linkage */
  DominatorTree::UpdateType dt_up[] = {
    {U::Delete, bb, next_bb},
    {U::Insert, bb, cabbb},
    {U::Insert, cabbb, next_bb}};
  dt.applyUpdates(dt_up);

  /* For debug, do heavy DT / PDT checking, as that stuff is not
   * particularly intuitive... */
  LLVM_DEBUG(
    DominatorTree local_dt(*bb->getParent());
    PostDominatorTree local_pdt(*bb->getParent());

    if( local_dt.compare(dt) ) {
      dbgs() << "WARNING: Different incremental DT vs new DT for "
	     << bb->getParent()->getName() << '\n';
      dbgs() << "Incremental DT\n";
      dt.print(dbgs());
      dbgs() << "Full DT\n";
      local_dt.print(dbgs());
    }

    if( local_pdt.compare(pdt) ) {
      dbgs() << "WARNING: Different incremental PDT vs new PDT for "
	     << bb->getParent()->getName() << '\n';
      dbgs() << "Incremental PDT\n";
      pdt.print(dbgs());
      dbgs() << "Full PDT\n";
      local_pdt.print(dbgs());
    }
  );

  return next_bb;
}

void Converge::execute_converge_plan(const std::vector<access_set_t>& converge_plan,
                              const LLGraph<BasicBlock>& reduced_cfg,
                              std::vector<converged_access_block*>* merge_blocks,
                              DominatorTree& dt, PostDominatorTree& pdt, Function& f) {
  typedef std::unordered_map<BasicBlock*, BasicBlock::iterator>
            bb_to_access_t;

  bb_dac_t       dac;
  bb_to_access_t bb_access;

  /* Setup: insert the nodes for in-order traversal */
  for( auto bb_node : reduced_cfg.T_pss ) {
    BasicBlock* bb   = bb_node.first;
    auto&       node = bb_node.second;
    dac.insert(bb, node.preds.size());
    /* Record first access per BasicBlock */
    bb_access[bb] = next_map_packet_access(*bb);
  }

  /*
   * Execution: go through and process the frontier of packet / map
   * accesses.  Figure out which accesses from the frontier should be
   * merged and advance the frontier there.
   */
  //XXX: Why is this so complicated?  Couldn't one just go through the plan
  //     sequence, and then simply converge the entries of the current
  //     converge set and get the BBs that need changing through the
  //     inst->getParent etc?!
  //
  //     Oh! Because the converge plan only has the entries that will need
  //     converging, but would leave edges (even on the reduced CFG!) that
  //     bypass the convergence point to another access that will be
  //     converged later.  Therefore, we need to track the frontier and
  //     ensure that we converge control flow across the entirety of the
  //     frontier and insert dummy accesses before map / packet accesses that
  //     are not part of the current converge plan step.
  //
  //     A potentially better way might be to introduce the dummy accesses
  //     in a separate step, add them to the converge plan, and then do a
  //     very simple converge as suggested above.  Or, the other way
  //     around: converge only the things that are in the plan and let the
  //     weave code handle the rest (not sure if it is capable of
  //     untangling the resuling mess, however!)  Maybe the cleanest verion
  //     would be to add dummy accesses and not just in the cases where the
  //     next packet / map access isn't the next in the converge plan, but
  //     also in those cases where there are no more map / packet accesses.
  //     That way, the weave would become a part of the dummy access
  //     creation and not need any special casing!
  auto plan_it = converge_plan.begin();
  std::unordered_map<BasicBlock*, BasicBlock*> reduced_cfg_map;

  dac.execute(
    [&](bb_dac_t* dac, const bb_dac_t::items_t& candidates,
        bb_dac_t::items_t* processed) {

      /* Pull out the actual map / packet accesses into access_cand.  This
       * is the frontier of available map / packet accesses that is
       * available in the code */
      access_set_t access_cand;
      for( auto* bb : candidates )
        access_cand.insert(&cast<CallInst>(*bb_access[bb]));

      assert(plan_it != converge_plan.end());
      LLVM_DEBUG(
        dbgs() << "  Available accesses:\n";
        for( auto* acc : access_cand )
          dbgs() << "  " << *acc << '\n';
        dbgs() << "  Plan:\n";
        for( auto* p : *plan_it )
          dbgs() << "  " << *p << '\n';
      );

      /* Make sure that all entries from the current plan are in the
       * frontier */
      assert(subset(access_cand, *plan_it));
      assert(access_cand.size() > 0);

      /* Create a converged access if there is more than one candidate */
      bool do_converge = (access_cand.size() > 1);
      if( access_cand.size() == 1 ) {
        /* There is only a single access on the frontier / plan.  We still
         * might have to "converge" it in case there are control-flow edges
         * bypassing it. */
        auto* access = *access_cand.begin();
        auto* to_bb  = access->getParent();

        /* Because we have not yet weaved all bypassing edges through the
         * previous merge node, we have to check whether the entire
         * function is properly converged by chance already.  So check from
         * entry to current BB. */
        BasicBlock* from_bb = &f.getEntryBlock();

        LLVM_DEBUG(
          dbgs() << "Checking for bypasing control-flow edges\n"
                 << "  From-BB: " << from_bb->getName() << '\n'
                 << "  To-BB: " << to_bb->getName() << '\n'
                 << "  dom(from, to): " << dt.dominates(from_bb, to_bb)
                 << "\n  pdt(to, from): " << pdt.dominates(to_bb, from_bb)
                 << '\n');
        do_converge = !dt.dominates(from_bb, to_bb) ||
                      !pdt.dominates(to_bb, from_bb);
      }
      converged_access_block* converged_access = nullptr;

      if( do_converge ) {
        converged_access = new converged_access_block(*plan_it->begin(),
                                 access_cand.size());
        merge_blocks->push_back(converged_access);
      }

      /* Review all accesses on the frontier */
      for( auto* acc : access_cand ) {
        bool in_plan = (plan_it->count(acc) != 0);
        auto* bb     = acc->getParent();

        /* Ensure the tracking of accesses per BB works */
        assert(&cast<CallInst>(*bb_access[bb]) == acc);

        /* Find the BB in the reduced CFG the current BB maps to */
        auto  it = reduced_cfg_map.find(bb);
        auto* reduced_cfg_bb = ( it == reduced_cfg_map.end() )
                               ? bb : it->second;

        if( !do_converge ) {
          /* We're not converging -> just skip the access */
          bb_access[bb] = next_map_packet_access(*bb,
                                                 std::next(bb_access[bb]));

          /* Mark this BB done when we reach its end */
          if( bb_access[bb] == bb->end() )
            processed->insert(bb);
        } else {
          /* Converge the current access into the converged_access */
          bool use_dummy = !in_plan;
          auto* new_bb = converge_into(converged_access, acc, use_dummy,
                                       dt, pdt);
          assert(new_bb != bb);

          /* The new BB logically is from the same BB in the reduced CFG */
          reduced_cfg_map.emplace(new_bb, reduced_cfg_bb);
          /* Continue processing in the split off BB */
          bb_access[new_bb] = next_map_packet_access(*new_bb);

          /* We have split things off, so this BB is done */
          processed->insert(bb);
          /* Insert remainder to work queue if it has more accesses */
          if( bb_access[new_bb] != new_bb->end() )
            dac->insert_ready(new_bb);
          bb = new_bb;
        }

        /* End of the BB -> ready the successors */
        assert((bb == reduced_cfg_bb) ||
               (reduced_cfg_map[bb] == reduced_cfg_bb));
        if( bb_access[bb] == bb->end() ) {
          /* No more accesses left in this BB, do mark successors */
          for( auto* succ : reduced_cfg.succs(reduced_cfg_bb))
            dac->mark_dep_ready(succ);
        }
      } /* End: loop through accesses of the frontier */

#ifdef VISUALISE_CONVERGE_STEPS
      write_map_packet_cfg(f, true);
#endif

      /* Delete merged accesses */
      if( do_converge ) {
        for( auto* inst : *plan_it ) {
          if( inst->getParent() != nullptr ) {
            errs() << "Instruction " << *inst << " still linked in BB: "
                   << *inst->getParent() << '\n';
            assert(false);
          }
          delete inst;
        }
      }

      /* Let's have a look at the next element in the plan */
      ++plan_it;
    }
  );
}


converged_access_block::converged_access_block(CallInst* example,
                                               unsigned fan_io)
                                               : fan_io(fan_io), cur_path(0)
{
  nt_api_call ex(example);
  auto& example_dl = example->getDebugLoc();
  StringRef name = ex.is_map() ? "map" : "packet";
  bb = BasicBlock::Create(example->getContext(),
                          "converged_" + name + "_access",
                          example->getFunction());
  IRBuilder<> ir(bb);
  /* Remember which path we were on */
  path_phi = ir.CreatePHI(ir.getInt32Ty(), fan_io, "path");
  path_phi->setDebugLoc(example_dl);

  /* PHI nodes for earch argument to the call */
  unsigned arg_count = 0;
  for( auto& arg : example->args() ) {
    auto* phi = ir.CreatePHI(arg->getType(), fan_io,
                             "arg_phi" + std::to_string(arg_count));
    phi->setDebugLoc(example_dl);
    arg_phis.push_back(phi);
    arg_count++;
  }

  /* The call itself */
  CallInst* c = ir.CreateCall(example->getCalledFunction(), arg_phis,
                              name + "_call");
  c->setDebugLoc(example_dl);
  call.initialise_from(c);

  /* And the divergence switch */
  unreachable_bb = BasicBlock::Create(example->getContext(), "unreachable",
                                      example->getFunction());
  new UnreachableInst(example->getContext(), unreachable_bb);
  switch_inst = ir.CreateSwitch(path_phi, unreachable_bb, fan_io - 1);
  switch_inst->setDebugLoc(example_dl);

  LLVM_DEBUG(dbgs() << "New converge BB: " << *bb <<'\n');
}

void converged_access_block::add_access(BasicBlock* src, CallInst* access,
                                        BasicBlock* dst) {
  assert(access->getCalledFunction() ==
         call.get_call()->getCalledFunction());

  unsigned our_id = cur_path++;
  auto* our_const = cast<ConstantInt>(ConstantInt::get(path_phi->getType(),
                                                       our_id));

  LLVM_DEBUG(dbgs() << "Adding access\n"
                    << "  " << *access << '\n'
                    << "  ID: " << our_id << '\n');
  path_phi->addIncoming(our_const, src);

  assert(access->arg_size() == arg_phis.size());
  for( unsigned a = 0; a < access->arg_size(); ++a) {
    auto* phi = cast<PHINode>(arg_phis[a]);
    phi->addIncoming(access->getOperand(a), src);
  }

  if( our_id > 0) {
    switch_inst->addCase(our_const, dst);
  } else {
    switch_inst->setDefaultDest(dst);
    unreachable_bb->eraseFromParent();
    unreachable_bb = nullptr;
  }

  LLVM_DEBUG(dbgs() << "Result: " << *bb << '\n');
}

void converged_access_block::add_dummy_access(BasicBlock* src,
                                              BasicBlock* dst) {
  unsigned our_id = cur_path++;
  auto* our_const = cast<ConstantInt>(ConstantInt::get(path_phi->getType(),
                                                       our_id));

  LLVM_DEBUG(dbgs() << "Adding dummy access ID: " << our_id << '\n');
  path_phi->addIncoming(our_const, src);

  assert(call.arg_size() == arg_phis.size());
  /* XXX: Could also go to a more data driven mode */
  for( unsigned a = 0; a < call.arg_size(); ++a) {
    auto* phi = cast<PHINode>(arg_phis[a]);
    phi->addIncoming(call.get_dummy_arg(a), src);
  }

  if( our_id > 0) {
    switch_inst->addCase(our_const, dst);
  } else {
    switch_inst->setDefaultDest(dst);
    unreachable_bb->eraseFromParent();
    unreachable_bb = nullptr;
  }

  LLVM_DEBUG(dbgs() << "Result: " << *bb << '\n');
}

void converged_access_block::simplify(DominatorTree& dt) {
  LLVM_DEBUG(dbgs() << "Converged access BB before simplify(): " << *bb
                    << '\n');
  /* Remove constant PHI nodes */
  for( auto* phiv : arg_phis ) {
    auto* phi = cast<PHINode>(phiv);
    bool const_undef = phi->hasConstantOrUndefValue();

    if( const_undef ) {
      /* Try to replace the entire PHINode with a single legal value */
      Value* v = nullptr;
      /* Get the first non-undef value */
      for( auto& iv : phi->incoming_values() ) {
        if( !isa<UndefValue>(*iv) ) {
          v = iv;
          break;
        }
      }
      /* Check the selected value can actually be used instead of the PHI */
      if( isa<Constant>(v) || isa<Argument>(v) ||
          dt.dominates(cast<Instruction>(v), phi) ) {
        phi->replaceAllUsesWith(v);
        phi->eraseFromParent();
        continue;
      }
    }
  }
  LLVM_DEBUG(dbgs() << "Converged access BB after simplify(): " << *bb
                    << '\n');

}

bool converged_access_block::convert_phi_of_addr(MemoryLocation& loc, bool is_input) {
  LLVM_DEBUG(
    dbgs() << "convert_phi_of_addr() on call: " << *call.get_call() << '\n'
           << "  Value: " << loc << '\n';
  );

  if( !loc.Size.hasValue() ) {
    errs() << "ERROR: Value has no known size!\n"
           << "Converged access: " << *call.get_call() << '\n'
           << "  Value: " << loc << '\n';
  }
  assert(loc.Size.hasValue());
  unsigned loc_size = loc.Size.getValue();

  if( !isa<PHINode>(loc.Ptr)) {
    errs() << "ERROR: Value is not a phi node!\n"
           << "Converged access: " << *call.get_call() << '\n'
           << "  Value: " << loc << '\n';
  }
  assert(isa<PHINode>(loc.Ptr));

  auto* ci   = call.get_call();
  auto* phi  = cast<PHINode>(const_cast<Value*>(loc.Ptr));
  auto* func = ci->getFunction();

  /* Ignore phis that are only here for weaving purposes */
#if 0
  if( phi->hasConstantOrUndefValue() )
    return false;
#endif

  bool is_long_range_phi = (phi->getParent() != bb);
  /* Ignore simple long-range phis for now */
  if( is_long_range_phi && phi->hasConstantOrUndefValue() ) {
    errs() << "ERROR: Used long-range simple PHI " << *phi
           << " defined in " << phi->getParent()->getName()
           << "\nThese must be cleaned up before converging!\n";
    abort();
  }

  /* NOTE: We ignore other, potentially long-range phi nodes here.  They
   * may not be supported by the Pipeliner, but only if they are so far
   * away from the usage that they end up in a different pipeline stage.
   * That is impossible to check here; checking is_long_range is not good
   * enough, because it triggers on cases where the phi node ends up in the
   * same pipeline stage as the use, but in just a slightly different BB.
   */

  /* Find the corresponding argument index for the parameter */
  assert(ci->hasArgument(phi));
  auto it = llvm::find(ci->args(), phi);
  unsigned arg_idx = it - ci->arg_begin();

  /* Get the associated request size */
  bool     req_size_bits = false;
  auto*    req_size_arg  = call.get_size_arg(arg_idx, &req_size_bits);
  PHINode* req_size_phi  = dyn_cast<PHINode>(req_size_arg);

  /* Construct the alloca that will hold the input / output data */
  IRBuilder<> ir(func->getEntryBlock().getFirstNonPHI());
  auto* alloca = ir.CreateAlloca(ir.getInt8Ty(),
                                 ir.getInt32(loc_size),
                                 ci->getName() + "_buffer");
  LLVM_DEBUG(dbgs() << "  Alloca: " << *alloca << '\n');

  /* Convert each part of the phi into a memcpy in the correct preceeding /
   * subsequent block */
  for( unsigned i = 0; i < phi->getNumIncomingValues(); i++ ) {
    auto* val = phi->getIncomingValue(i);
    auto* bb  = phi->getIncomingBlock(i);
    if( isa<UndefValue>(val) )
      continue;

    /* Extract the actual request size from the call which may be different
     * on different path, but must always fit into the maximum loc_size
     *
     * XXX: This could also be the result of a more complex computation and
     * not necessarily a constant int! We could just pull out the Value*
     * and feed that to the memcpy! */
    unsigned req_size = loc_size;
    if( req_size_phi != nullptr ) {
      assert(req_size_phi->getIncomingBlock(i) == bb);
      auto* rzv   = req_size_phi->getIncomingValue(i);
      unsigned rz = cast<ConstantInt>(rzv)->getZExtValue();
      req_size = !req_size_bits ? rz : (rz + 7) / 8;
    }
    assert(req_size <= loc_size);

    /* Insert the right memcpy to / from the alloca at the end of the
     * predecessor / beginning of the successor */
    if( is_input ) {
      /* Insert the copy-in at the end of the predecessor block. */
      ir.SetInsertPoint(bb->getTerminator());
      auto* memcpy = ir.CreateMemCpy(alloca, alloca->getAlignment(), val, 0, req_size);
      LLVM_DEBUG(dbgs() << "  Memcpy: " << *memcpy << '\n');
    } else {
      /* Insert copy-out out at the beginning of the right successor block */
      auto* out_bb = get_successor_bb_for(bb);
      ir.SetInsertPoint(out_bb, out_bb->getFirstInsertionPt());
      auto* memcpy = ir.CreateMemCpy(val, 0, alloca, alloca->getAlignment(), req_size);
      LLVM_DEBUG(dbgs() << "  Memcpy: " << *memcpy << '\n');
    }
  }

  /* Replace the phi-of-addr with the static alloca */
  call.get_call()->replaceUsesOfWith(phi, alloca);
  if( phi->use_empty() )
    phi->eraseFromParent();

  LLVM_DEBUG(dbgs() << "Converged access BB after convert_phi_of_addr(): "
                    << *bb << '\n');
  return true;
}

BasicBlock* converged_access_block::get_successor_bb_for(BasicBlock* pred) {
  auto idx  = path_phi->getBasicBlockIndex(pred);
  assert(idx >= 0);
  auto* val = cast<ConstantInt>(path_phi->getIncomingValue(idx));
  auto it   = switch_inst->findCaseValue(val);
  return it->getCaseSuccessor();
}

bool converged_access_block::remove_pointer_phis() {
  /**
   * The idea here is to (1) pull out the input / output pointer locations
   * and their sizes.  Then, (2a) for all inputs, create a separate alloaca
   * of the required size, and (2b) copy the values for these locations
   * into the newly allocated buffer in the source basic blocks! Finally,
   * do a similar thing for (3) all the outputs: allocate a buffer for the
   * maximum size output, and copy out the value to the application buffers
   * in the subsequent basic blocks.  That way, we will not have any
   * phi-of-pointer nodes.
   */
  std::vector<MemoryLocation> inputs  = call.get_input_mem();
  std::vector<MemoryLocation> outputs = call.get_output_mem();

  bool found = false;
  for( auto& l : inputs ) {
    const bool is_input = true;
    if( isa<PHINode>(l.Ptr) )
      found |= convert_phi_of_addr(l, is_input);
  }
  for( auto& l : outputs ) {
    const bool is_input = false;
    if( isa<PHINode>(l.Ptr) )
      found |= convert_phi_of_addr(l, is_input);
  }

  /* Special case check: check that the packet passed to the API call was
   * not doctored with and instead is the function call argument */
  if( call.is_packet() && !isa<Argument>(call.get_packet()) ) {
    errs() << "ERROR: Packet argument\n  " << *call.get_packet()
           << "\nto call\n  " << *call.get_call()
           << "\nis not the normal packet argument. "
           << "Please repair the application code.\n";
    abort();
  }

  /* Ensure that none of the arguments are phi-of-ptrs anymore */
  for( auto& arg : call.get_call()->args() ) {
    if( !arg->getType()->isPointerTy() )
      continue;
    if( !isa<PHINode>(arg) )
      continue;
    if( !cast<PHINode>(arg)->hasConstantOrUndefValue() ) {
      errs() << "ERROR: Unexpected PHI node pointer argument " << *arg
             << " left in call " << *call.get_call() <<'\n';
      abort();
    }
  }
  return found;
}

void sort_basic_blocks(Function& f) {
  typedef dep_aware_converter<BasicBlock> bb_dac_t;
  bb_dac_t dac;

  for( auto& bb : f )
    dac.insert(&bb, pred_size(&bb));

  BasicBlock* prev = &f.getEntryBlock();
  dac.execute([&](bb_dac_t* dac, BasicBlock* bb) {
    if( prev != bb )
      bb->moveAfter(prev);
    prev = bb;
    for(auto succ : successors(bb) )
      dac->mark_dep_ready(succ);
  });

}
bool Converge::converge(Function& f) {
  LLVM_DEBUG(dbgs().write_escaped(f.getName());
             dbgs() << '\n');

  Graph<MapID> map_ac_order;

  DominatorTree& dt = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PostDominatorTree& pdt =
    getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  //WriteGraph((const Function*)&f, f.getName() + ".initial_function");

  // Step 0: Check if accesses can be merged and do so -> XXX: This is
  // not yet fully implemented!
#ifdef IMPLEMENTED_ACCES_MERGE
  MemorySSA& mssa = getAnalysis<MemorySSAWrapperPass>().getMSSA();
  AliasAnalysis& aa = getAnalysis<AAResultsWrapperPass>().getAAResults();
  TargetLibraryInfo& tli = getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

  for( auto& bb : f )
    merge_intra_bb_accesses(&bb, aa, mssa, tli);
#endif

  // Step 1: Record map and packet accesses per basic block
  bb_to_access_vec_t  bb_mas, bb_pktas;
  map_to_access_vec_t map_mas;
  bool found_some = record_map_packet_accesses(f, &bb_mas, &bb_pktas, &map_mas);
  if( !found_some )
    return false;

  // Step 2: simplify CFG to only BBs with map & packet accesses
  LLGraph<BasicBlock> reduced_cfg;
  construct_reduced_cfg(f, bb_mas, bb_pktas, &reduced_cfg);

  /* Compute worst case number of map / packet accesses hanging off
   * each basic block */
  std::unordered_map<BasicBlock*, unsigned> max_accesses_remaining;
  compute_max_access_tail(reduced_cfg, &max_accesses_remaining);

  if( converge_stats )
    print_chain_stats(reduced_cfg, max_accesses_remaining);

  /* Construct the converge plan (sequence of merge sets) */
  std::vector<access_set_t> converge_plan;
  construct_plan(&converge_plan, reduced_cfg, &max_accesses_remaining);

  if( converge_stats )
    dbgs() << "Converge plan entries: " << converge_plan.size() << '\n';

  /* Perform the converge operation and recod the resulting converged
   * accesses */
  std::vector<converged_access_block*> merge_blocks;
  execute_converge_plan(converge_plan, reduced_cfg, &merge_blocks, dt,
                        pdt, f);

  //errs() << "FINAL FUNCTION:\n" << f << "END\n";
  //WriteGraph((const Function*)&f, f.getName() + ".after_converge");

  /* Due to operating on the reduced CFG, there may be control flow edges
   * going past the converge points.  "Weave" those strands back into the
   * strict converge shape. */
  weave_bypassing_flows(f, merge_blocks, dt, pdt);
  //WriteGraph((const Function*)&f, f.getName() + ".after_weave");

  /* Due to the converge pattern, there are new static control flow edges
   * that were not there before.  Imagine two packet accesses pA and pB
   * in two different, concurrent basic blocks A and B.  Converging pA
   * and pB into a single converged packet access p in BB P will split A
   * and B into A_head / A_tail and B_head / B_tail.  Due to converging,
   * there is now a control flow edge A_head -> P -> A_tail, and likewise
   * for B.  That does, however, introduce a connection A_head -> P ->
   * B_tail which is never followed (the incoming phi and outgoing switch
   * in P ensure that), but is violating the SSA properties.  Especially,
   * B_tail could reference values defined only in B_head, but not
   * A_head.  The solution is to trace these, and add PHI nodes into P
   * that collect the original value from B_head and an undef from
   * A_head.
   */
  repair_def_dominance(f, dt, pdt);

  /* Simplify converge blocks and clean them up */
  bool changed = false;
  for( auto* converged_access : merge_blocks ) {
    if( converged_access != nullptr ) {
      converged_access->simplify(dt);
      changed |= converged_access->remove_pointer_phis();
      delete converged_access;
    }
  }
  /* The removal of phi nodes may have introduced more breakage of the
   * def dominance -> repair it here */
  if( changed )
    repair_def_dominance(f, dt, pdt);

  /* Converge the function exit blocks
   *
   * - collect all basic blocks that can leave the function
   * - link them all to a single basic block with a ret inside
   * - profit
   */
  std::vector<BasicBlock*> exit_bbs;
  std::vector<Value*>      exit_vals;
  std::vector<BasicBlock*> unreach_bbs;
  for( auto& bb : f ) {
    auto* term = bb.getTerminator();
    bool is_intra = isa<BranchInst>(term) ||
                    isa<SwitchInst>(term);
    bool is_unrch = isa<UnreachableInst>(term);
    bool is_ret   = isa<ReturnInst>(term);
    /* No fancy exceptions, please */
    if( !(is_intra || is_ret || is_unrch) ) {
      errs() << "Unhandled block terminator " << *term << '\n';
    }
    assert(is_intra || is_ret || is_unrch);

    if( is_ret ) {
      auto* ret = cast<ReturnInst>(term);
      auto* rv = ret->getReturnValue();
      if( rv == nullptr) {
        errs() << "Void packet kernel " << f.getName()
               << " with terminator " << *ret << '\n';
      }
      assert(rv != nullptr);
      exit_bbs.emplace_back(&bb);
      exit_vals.emplace_back(rv);
    } else if( is_unrch ) {
      unreach_bbs.emplace_back(&bb);
    }
  }
  LLVM_DEBUG(
    dbgs() << "Function " << f.getName() << " exit blocks: ";
    for( auto* bb : exit_bbs )
      dbgs() << bb->getName() << " ";
    dbgs() << '\n';
    dbgs() << "Unreachable blocks: ";
    for( auto* bb : unreach_bbs )
      dbgs() << bb->getName() << " ";
    dbgs() << '\n';
  );

  assert(exit_bbs.size() >= 1);
  assert(exit_bbs.size() == exit_vals.size());

  /* Create single exit basic block, link the original exits to it, and use
   * a phi node to select the right return value. */
  unsigned nexits = exit_bbs.size() + unreach_bbs.size();
  if( nexits > 1) {
    auto& ctx = f.getParent()->getContext();
    auto* conv_exit = BasicBlock::Create(ctx, "converged_exit", &f);
    auto* type = exit_vals[0]->getType();
    auto* phi = PHINode::Create(type, nexits, "ret_val", conv_exit);
    ReturnInst::Create(ctx, phi, conv_exit);

    for( unsigned i = 0; i < exit_bbs.size(); i++ ) {
      auto* bb = exit_bbs[i];
      auto* rv = exit_vals[i];
      LLVM_DEBUG(dbgs() << "Changing BB: " << bb->getName() << '\n');
      auto* t = cast<ReturnInst>(bb->getTerminator());
      t->eraseFromParent();
      BranchInst::Create(conv_exit, bb);
      LLVM_DEBUG(dbgs() << *bb << '\n');
      phi->addIncoming(rv, bb);
    }
    for( auto* bb : unreach_bbs ) {
      LLVM_DEBUG(dbgs() << "Changing BB: " << bb->getName() << '\n');
      auto* t = cast<UnreachableInst>(bb->getTerminator());
      t->eraseFromParent();
      BranchInst::Create(conv_exit, bb);
      LLVM_DEBUG(dbgs() << *bb << '\n');
      phi->addIncoming(UndefValue::get(type), bb);
    }
    LLVM_DEBUG(dbgs() << *conv_exit << '\n');
  }

  /* Sort basic blocks so it is easier to follow them */
  sort_basic_blocks(f);

  /* Check PHI node matchup of edges with predecessor nodes */
  for( auto& inst : instructions(f) ) {
    if( !isa<PHINode>(inst) )
      continue;
    auto& phi = cast<PHINode>(inst);
    auto* bb  = phi.getParent();
    if( bb->hasNPredecessors(phi.getNumIncomingValues()) )
      continue;

    errs() << "PHI Node " << phi << " does not match predecessors of "
           << bb->getName() << '\n';
    SmallVector<BasicBlock*, 10> phi_bbs(phi.blocks());
    SmallVector<BasicBlock*, 10> preds(predecessors(bb));
    llvm::sort(phi_bbs);
    llvm::sort(preds);
    errs() << "PHI BB Values:\n";
    for( auto* bb  : phi_bbs )
      errs() << "  " << bb->getName() << '\n';
    errs() << "BB Predecessors:\n";
    for( auto* bb  : preds )
      errs() << "  " << bb->getName() << '\n';

    auto phi_it = phi_bbs.begin();
    auto pred_it = preds.begin();
    while( (phi_it != phi_bbs.end()) && (pred_it != preds.end()) ) {
      if( *phi_it == *pred_it ) {
        phi_it++; pred_it++;
        continue;
      }
      errs() << "Mismatching entries:\n"
             << "PHI: " << **phi_it
             << "Pred: " << **pred_it
             <<'\n';
      if( phi_bbs.size() > preds.size() )
        phi_it++;
      else
        pred_it++;
    }
  }
  return true;
}

bool Converge::check_nanotube_api_level(Function& f) {
  for( inst_iterator it = inst_begin(f); it != inst_end(f); ++it ) {
    if( !isa<CallInst>(*it) )
      continue;
    auto* call = &cast<CallInst>(*it);
    nt_api_call nt_call(call);
    if( nt_call.is_map() && nt_call.is_access() &&
        nt_call.get_intrinsic() != Intrinsics::map_op ) {
      errs() << "High-level Nnaotube map API used: " << *it << '\n';
      return false;
    }
  }
  return true;
}

/**
 * The converge pass merges control flow structurally, while using phi ins
 * and switch out to ensure that the code flows exactly as before.  Just
 * looking at the CFG however, there may now be cases where uses of values
 * might not be dominated by the definitions.
 *
 * This pass scans all definitions and checks whether that is a problem.
 * The repair is simple: replace the use with a Phi node that "sucks in"
 * the actual value when coming from the right path, and an undef for when
 * the merged BB is entered from the other side and the use never actuall
 * happens.
 *
 * This is a very simple brute-force approach to patching this up, rather
 * than doing something much more clever at construction time and tracking
 * live / dead values and whether they were defined in a dominator.
 */
void Converge::repair_def_dominance(Function&  f, DominatorTree& dt,
                                    PostDominatorTree& pdt) {
  for( auto& def_bb: f ) {
    for( auto& def : def_bb) {
      Instruction* ssaified_def = nullptr;
      for( auto use_it = def.use_begin(); use_it != def.use_end(); ) {
        /**
         * We step the iterator early here, because we will do naughty
         * things (change the current use!) later on, so step to the next
         * person straight away!
         */
        auto& use = *(use_it++);

        /* Definition dominates use -> nothing to do */
        if( dt.dominates(&def, use) )
          continue;

        /* If we have a good PHI node already, and it also dominates this
         * use, we reuse it. */
        if( ssaified_def && dt.dominates(ssaified_def, use) ) {
          LLVM_DEBUG(dbgs() << "Reusing PHI node " << *ssaified_def
                            << " for user " << *use.getUser() << '\n');
          use.set(ssaified_def);
          LLVM_DEBUG(dbgs() << "New user: " << *use.getUser() << '\n');
          continue;
        }

        LLVM_DEBUG(dbgs() << "Value\n" << def << "\ndoes not dominate "
                             "use\n" << *use.getUser() << '\n');
        /**
         * We have a use use and a def_bb, we now walk the post-dominators
         * of def_bb (i.e. towards the end of the program!) until we find
         * one that dominates the use (happening even later in the
         * program).  Because of the hub structure of the program, we will
         * not have to scan far, usually just to the next hub node that
         * lives between the def and the use.  Note that there always has
         * to be a hub between def and use, as otherwise, the def and use
         * relationship should not have been disturbed by the conversion
         * passes!
         */
        auto* use_bb = cast<Instruction>(use.getUser())->getParent();
        LLVM_DEBUG(dbgs() << "Def BB " << def_bb.getName() << " use BB "
                          << use_bb->getName() << '\n');
        auto* cur_post_node = pdt[&def_bb];
        auto* cur_bb = cur_post_node->getBlock();
        while( !dt.dominates(cur_bb, use_bb) ) {
          LLVM_DEBUG(dbgs() << "Checking BB " << cur_bb->getName() << '\n');
          auto* next_post_node = cur_post_node->getIDom();

          /* Knowing the structure of the program, we always have to find a
           * node!
           * XXX: We could also check against the roots of the PDT \_o_/
           */
          if( (next_post_node == nullptr) ||
              (next_post_node->getBlock() == nullptr) ) {
            errs() << "Unexpected NULL post-dominator tree node / block\n"
                   << "Previous block: " << *cur_bb << '\n';
            assert((next_post_node != nullptr) &&
                   (next_post_node->getBlock() != nullptr));
          }
          cur_post_node = next_post_node;
          cur_bb = cur_post_node->getBlock();
        } // while( !dt.dominates(cur_bb, use_bb) )
        assert((cur_bb != nullptr) && dt.dominates(cur_bb, use_bb));
        LLVM_DEBUG(dbgs() << "Found merge BB: " << cur_bb->getName() << '\n');

        /* Add a PHI node that captures the value (and undefs for the other
         * blocks!) to the merge BB */
        IRBuilder<> ir(cur_bb->getFirstNonPHI());
        auto* phi = ir.CreatePHI(def.getType(), pred_size(cur_bb),
                                 def.getName() + ".ssa");
        for( auto* pred : predecessors(cur_bb) ) {
          if( dt.dominates(&def_bb, pred) )
            phi->addIncoming(&def, pred);
          else
            phi->addIncoming(UndefValue::get(def.getType()), pred);
        }
        assert(dt.dominates(phi, use));
        /* NOTE:Unsafe, but we carefully control the iterator above */
        use.set(phi);
        ssaified_def = phi;
        LLVM_DEBUG(dbgs() << "New PHI node: " << *phi << '\n'
                          << "New merge BB: " << *cur_bb << '\n'
                          << "New user: " << *use.getUser() << '\n');
      } // for( auto& use : def.uses() )
    } // for( auto& def : def_bb)
  } // for( auto& def_bb: f )
}

/**
 * Construct the (backwards) web CFG between the two hub nodes from-BB
 * to to-BB.  Which is all the BBs that are reachable backwards from
 * to-BB until reaching from-BB.  Obviously this code does not like
 * having stray incoming control flow.
 */
std::unordered_set<BasicBlock*>
Converge::backwards_reachable_between(BasicBlock* from_bb,
                                      BasicBlock* to_bb)
{
  std::vector<BasicBlock*>        todo;
  std::unordered_set<BasicBlock*> web;
  for( auto* bb : predecessors(to_bb) ) {
    assert(succ_size(bb) == 1);
    todo.push_back(bb);
    web.insert(bb);
  }

  while( !todo.empty() ) {
    BasicBlock* bb = todo.back();
    todo.pop_back();

    /* Add unvisted predecessors to work list */
    for( auto* pred : predecessors(bb) ) {
      if( (pred == from_bb) || (web.count(pred) != 0) )
        continue;
      web.insert(pred);
      todo.push_back(pred);
    }
  }
  /* Don't forget to check the from_bb for any naughty outgoing control
   * flow edges! */
  web.insert(from_bb);
  return web;
}

/**
 * Rewrite all matching targets of a terminator (branch / switch etc.)
 * instruction to point elsewhere.
 */
void Converge::replace_targets(Instruction* term, BasicBlock* from,
                               BasicBlock* to)
{
  if( isa<BranchInst>(term) ) {
    auto* br = cast<BranchInst>(term);
    for( unsigned idx = 0; idx < br->getNumSuccessors(); idx++ ) {
      if( br->getSuccessor(idx) == from )
        br->setSuccessor(idx, to);
    }
  } else if( isa<SwitchInst>(term) ) {
    auto* sw = cast<SwitchInst>(term);
    for( unsigned idx = 0; idx < sw->getNumSuccessors(); idx++ ) {
      if( sw->getSuccessor(idx) == from )
        sw->setSuccessor(idx, to);
    }
  } else {
    errs() << "Unhandled instruction " << *term << '\n';
    assert(false);
  }
}

/**
 * Route a single outgoing stray edge by merging it through the out_hub.
 */
void Converge::weave_single_edge(Function& f, BasicBlock* src, BasicBlock* dst,
                                 converged_access_block* out_hub,
                                 DominatorTree* dt, PostDominatorTree* pdt,
                                 std::unordered_set<BasicBlock*>* pads)
{
  BasicBlock* out_hub_bb = out_hub->get_bb();

  /* A incoming pad node is needed to disambiguate incoming PHI nodes into
   * the hub.  An outgoing pad node is needed to disambiguate multiple
   * incoming edges into succ
   *
   * Insert the pads and then redirect control flow as follows, from:
   *   src -> dst
   * to:
   *   src -> dst.pad (if needed) -> out_hub -> src.pad (if needed) -> dst
   */

  /* Add a jump pad between src and out_hub, if needed */
  bool pad_before = succ_size(src) > 1;
  BasicBlock* padin = nullptr;
  if( pad_before ) {
    padin = BasicBlock::Create(f.getParent()->getContext(),
              dst->getName() + ".pre", &f, out_hub_bb);
    pads->insert(padin);
    BranchInst::Create(out_hub_bb, padin);
  }

  /* Pad from out_hub to the destination needed */
  bool pad_after = pred_size(dst) > 1;
  BasicBlock* padout = nullptr;
  if( pad_after ) {
    padout = BasicBlock::Create(f.getParent()->getContext(),
               src->getName() + ".post", &f, dst);
    BranchInst::Create(dst, padout);
  }

  /* Redirect out edge src -> dst => src -> padin / out_hub */
  BasicBlock* src_target = pad_before ? padin : out_hub_bb;
  assert(src_target != nullptr);
  replace_targets(src->getTerminator(), dst, src_target);
  LLVM_DEBUG(dbgs() << "Updated culprit BB:" << *src);

  /* Add the new route to the merge node to_bb / to_cab */
  BasicBlock* before_hub = pad_before ? padin : src;
  BasicBlock* after_hub  = pad_after ? padout : dst;
  assert((before_hub != nullptr) && (after_hub != nullptr));
  out_hub->add_dummy_access(before_hub, after_hub);
  LLVM_DEBUG(dbgs() << "Updated hub To-BB:" << *out_hub_bb);

  /* Change incoming PHI nodes in dst as we are now flowing
   * through padout / out_hub rather than coming from src */
  BasicBlock* dst_in = pad_after ? padout : out_hub_bb;
  assert(dst_in != nullptr);
  //dst->replacePhiUsesWith(src, dst_in);  // Not in old LLVM
  for( auto& inst : *dst ) {
    auto* phi = dyn_cast<PHINode>(&inst);
    if( phi  == nullptr )
      break;
    int idx;
    /* Instead of simply replacing src -> dst_in in the PHI, we also need
     * to check whether there were any doubled up edges between src -> dst
     * that will have to be tracked in the PHI node.  Those edges are only
     * doubled up between src -> dst.pad now, so we must remove duplicate
     * values, here */
    Value* v = nullptr;
    while( (idx = phi->getBasicBlockIndex(src)) > -1 ) {
      if( v == nullptr ) {
        /* Rewrite only the first */
        v = phi->getIncomingValue(idx);
        phi->setIncomingBlock(idx, dst_in);
      } else {
        /* Delete duplicates and check the value matched */
        assert(v == phi->getIncomingValue(idx));
        phi->removeIncomingValue(idx);
      }
    }
  }
  LLVM_DEBUG(dbgs() << "Updated successor BB:" << *dst);

  // Update pdt and dt
  std::vector<DominatorTree::UpdateType> dt_up;
  /* NOTE: I had DominatorTree::Delete etc, but the linker
   * *sometimes* complains about those.  What can I say??!?!? */
  dt_up.emplace_back(cfg::UpdateKind::Delete, src, dst);
  dt_up.emplace_back(cfg::UpdateKind::Insert, src, src_target);
  if( pad_before )
    dt_up.emplace_back(cfg::UpdateKind::Insert, padin, out_hub_bb);
  dt_up.emplace_back(cfg::UpdateKind::Insert, out_hub_bb, after_hub);
  if( pad_after )
    dt_up.emplace_back(cfg::UpdateKind::Insert, padout, dst);
  dt->applyUpdates(dt_up);
  pdt->applyUpdates(dt_up);
}

/**
 * The new converge pass does not add dummy accesses for paths which do not
 * have any remaining packet / map accesses.  That means that not all
 * control-flow is converged through the created converge nodes!
 *
 * This function finds these loose strands of control flow, and weaves them
 * back into the existing merge nodes.
 */
void Converge::weave_bypassing_flows(Function& f,
                                     const std::vector<converged_access_block*>& merge_blocks,
                                      DominatorTree& dt, PostDominatorTree& pdt) {

  /* Iterate through the existing merge / converge / hub points and weave
   * back any stragglers between merge point from_bb and to_bb whose
   * control flow does not go through the to_bb block */
  BasicBlock* from_bb = &f.getEntryBlock();
  for( auto* to_cab : merge_blocks ) {
    BasicBlock* to_bb = to_cab->get_bb();
    LLVM_DEBUG(
      dbgs() << "From BB:" << *from_bb << "\nTo BB:" << *to_bb << '\n');

    LLVM_DEBUG(
      dbgs() << "Dominator Tree for " << f.getName() << ":\n";
      dt.print(dbgs());
      dbgs() << "Post-Dominator Tree for " << f.getName() << ":\n";
      pdt.print(dbgs());
    );

    /* If there are no stray threads, check the next interval */
    bool incoming_stray = !dt.dominates(from_bb, to_bb);
    bool outgoing_stray = !pdt.dominates(to_bb, from_bb);
    if( !incoming_stray && !outgoing_stray ) {
      from_bb = to_bb;
      continue;
    }

    LLVM_DEBUG(
      dbgs() << "From-BB " << from_bb->getName() << " does not dominate "
             << "To-BB " << to_bb->getName()
             << " => finding stray threads.\n");

    if( incoming_stray ) {
      errs() << "Incoming stray control flow should have originated from "
             << " an earlier interval and been repaired there!\n";
      auto ret = WriteGraph((const Function*)&f, "Weird Case", false, "First weird edge "
          " interval " + from_bb->getName() + " to " + to_bb->getName()
          + " in Function " + f.getName());
      errs() << "Wrote a CFG in file " << ret << '\n';
      assert(!incoming_stray);
    }

    assert(!incoming_stray);
    auto web = backwards_reachable_between(from_bb, to_bb);
    LLVM_DEBUG(
      dbgs() << "Basic blocks in interval " << from_bb->getName()
             << " -> " << to_bb->getName() << '\n';
      for( auto* bb : web )
        dbgs() << "  " << bb->getName() << '\n';
    );

    /**
     * Check the nodes in the nodes in the web for having any edges poking
     * out.  Do this in the brute-force way, as it has the same time
     * complexity as all the other smart ways I could think of:
     *
     * O(N * log N * num_successors)
     *
     * which I don't think is too terrible.  Actually, the log N should
     * become 1, as the lookups are in a hash table, so hopefully O(1).
     */
    std::unordered_set<BasicBlock*> pads;
    for( auto* bb : web ) {
      LLVM_DEBUG(dbgs() << "Inspecting BB:" << *bb);

      /* Find the successor(s) that poking out (if any!) */
      for( auto* succ : successors(bb) ) {
        /* A node in the web, pad, or the end hub node are fine! */
        if( (succ == to_bb) || (web.count(succ) != 0) || (pads.count(succ) != 0))
          continue;

        /* This node is reachable from the web, but not a part of is -> so
         * it must be "bad"! */
        LLVM_DEBUG(
          dbgs() << "Culprit BB: " << bb->getName()
                 << " with stray edge to BB: " << succ->getName() << '\n');

        weave_single_edge(f, bb, succ, to_cab, &dt, &pdt, &pads);
      } // for( auto* succ : successors(bb) )
    } // for( auto* bb: web)

#ifdef VISUALISE_CONVERGE_STEPS
      write_map_packet_cfg(f, true);
#endif

    /* All flows to the end of the interval must come in via from_bb */
    assert(dt.dominates(from_bb, to_bb));
    /* .. and leave via to_bb */
    assert(pdt.dominates(to_bb, from_bb));

    from_bb = to_bb;
  } // for( auto* to_cab : merge_blocks )
}

char Converge::ID = 0;


static RegisterPass<Converge>
    X("converge_mapa", "Converge map and packet accesses",
      true,
      false
      );
#if(0) //does not work with LLVM 8, but instead needs 9
// Run at the end of most passes so the names do not disappear :)
static RegisterStandardPasses Y(
    PassManagerBuilder::EP_OptimizerEnd,
    [](const llvm::PassManagerBuilder &Builder,
        llvm::legacy::PassManagerBase &PM) { PM.add(new Hello()); });
#endif

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
