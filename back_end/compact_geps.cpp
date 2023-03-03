/*******************************************************/
/*! \file compact_geps.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Move GEPs and casts closer to pointer sources.
**   \date 2021-04-05
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
 * This pass moves (constant) GEPs and casts next to the pointers they are
 * referencing, away from LLVM's default placement next to the usage of those
 * casted pointers.  Futher, it then removes duplicate casts and simplifies phi
 * nodes that select between identical casted / GEPped pointers.
 *
 * LLVM likes to place those address calculations closer to the usage site,
 * which makes sense for a CPU which can do some address computation in the
 * memory access itself.  For Nanotube, however, this is not beneficial and we
 * would benefit more form removing phi-of-identical-pointer constructs.
 */

#include <unordered_map>
#include <unordered_set>

#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Pass.h"

#define DEBUG_TYPE "compact-geps"
using namespace llvm;
//using namespace nanotube;

namespace {
struct compact_geps : public FunctionPass {
  /* LLVM-specific */
  static char ID;
  compact_geps() : FunctionPass(ID) {
  }

  /* Used analysis passes */
  DominatorTree*     dt;
  PostDominatorTree* pdt;
  TargetLibraryInfo* tli;

  void getAnalysisUsage(AnalysisUsage &info) const override {
    info.addRequired<DominatorTreeWrapperPass>();
    info.addRequired<PostDominatorTreeWrapperPass>();
    info.addRequired<TargetLibraryInfoWrapperPass>();
  }

  void get_all_analysis_results() {
    dt   = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    pdt  = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    tli  = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  }

  bool move_gep_and_casts(Function& f);
  Instruction* move_inst(Instruction* inst, Instruction* dst);
  bool simplify_phis(Function& f);

  bool runOnFunction(Function& f) override {
    bool mod     = false;
    bool new_mod = false;
    get_all_analysis_results();

    const unsigned max_iters = 10; // Totally arbitrary iteration cut-off
    unsigned n = 0;
    do {
      LLVM_DEBUG(dbgs() << "Simplifying GEPs in " << f.getName() << '\n');
      new_mod = move_gep_and_casts(f);
      new_mod |= simplify_phis(f);

      mod |= new_mod;
      LLVM_DEBUG(
        if( new_mod )
          dbgs() << "Modifications found, optimising again.\n";
      );
      if( n++ == max_iters ) {
        errs() << "WARNING: Function " << f.getName() << " did not reach a "
               << "fixpoint in compact GEPs after " << max_iters
               << " iterations.  Leaving it potentially not fully "
               << "optimised.\n";
        break;
      }
    } while( new_mod );
    return mod;
  }

};
} //anonymous namespace

/**
 * Pull out the constants from a constant-index GEP
 */
static void
collect_gep_idx(GetElementPtrInst* gep, SmallVectorImpl<unsigned>* idx) {
  assert(gep->hasAllConstantIndices());
  for( auto& val : gep->indices() )
    idx->push_back(cast<ConstantInt>(val)->getZExtValue());
}

/**
 * Hash for a variable depth array of integers (indexes into a
 * constant-index GEP).
 */
static uint64_t
hash_gep_idx(ArrayRef<unsigned> idx) {
  uint64_t hash = 0;
  for( auto i : idx )
    hash = ((hash << 9) | (hash >> (64 - 9))) ^ i;
  return hash;
}

struct pair_ptr_hash {
  template <typename T, typename U>
  std::size_t operator()(const std::pair<T*, U*>& p) const {
    return std::size_t(p.first) ^ std::size_t(p.second);
  }
};

struct pair_ptr_equal {
  template <typename T, typename U>
  bool operator()(const std::pair<T*, U*>& lhs,
                  const std::pair<T*, U*>& rhs) const {
    return (lhs.first == rhs.first) && (lhs.second == rhs.second);
  }
};

struct pair_ptr_vec_hash {
  template <typename T, typename U>
  std::size_t operator()(const std::pair<T*, SmallVector<U, 4>>& p) const {
    return std::size_t(p.first) ^ hash_gep_idx(p.second);
  }
};

struct pair_ptr_vec_equal {
  template <typename T, typename U>
  bool operator()(const std::pair<T*, SmallVector<U, 4>>& lhs,
                  const std::pair<T*, SmallVector<U, 4>>& rhs) const {
    return (lhs.first == rhs.first) && (lhs.second == rhs.second);
  }
};

Instruction* compact_geps::move_inst(Instruction* inst, Instruction* dst) {
  /* No-op moves */
  if( inst->getPrevNode() == dst )
    return nullptr;

  static std::map<BasicBlock*, Instruction*> bb_post_phi_ip;

  /* Don't move into the phi node block, instead keep a list of post-phi
   * target instructions */
  if( isa<PHINode>(dst) ) {
    auto* bb = dst->getParent();
    auto  it = bb_post_phi_ip.find(bb);

    /* If an entry is found, move inst right at the end of that list; if
     * not, it is simply right after the PHI nodes. */
    if( it != bb_post_phi_ip.end() )
      dst = it->second;
    else
      dst = bb->getFirstNonPHI()->getPrevNode();

    /* The next isntruction that wants to move "right after the phis" will
     * be behind us. */
    bb_post_phi_ip[bb] = inst;
  }
  if( inst->getPrevNode() == dst )
    return nullptr;

  /* Skip debug instructions */
  dst = dst->getNextNonDebugInstruction()->getPrevNode();
  if( dst->getPrevNode() == inst )
    return nullptr;

  LLVM_DEBUG(dbgs() << "Moving " << *inst << " right after "
                    << *dst << '\n');
  inst->moveAfter(dst);
  //LLVM_DEBUG(dbgs() << "Destination BB: " << *dst->getParent());
  return dst;
}

/**
 * Move constant GEPs and bitcasts closer to the defining pointer.  Also,
 * remove redundant GEPs and bitcasts (those with the same base, and same
 * indexes / target type).
 */
bool compact_geps::move_gep_and_casts(Function& f) {
  bool changes = false;

  std::unordered_map<std::pair<Instruction*, Type*>, BitCastInst*,
                     pair_ptr_hash, pair_ptr_equal> bitcasts;
  std::unordered_map<std::pair<Instruction*, SmallVector<unsigned, 4>>,
                     GetElementPtrInst*,
                     pair_ptr_vec_hash, pair_ptr_vec_equal> geps;
  std::unordered_map<Instruction*, Instruction*> insert_place;

  std::vector<Instruction*> insts;
  for( auto iit = inst_begin(f); iit != inst_end(f); ++iit )
    insts.push_back(&*iit);

  for( auto* inst : insts ) {
    /* Skip non-pointer instructions */
    if( !inst->getType()->isPointerTy() )
      continue;

    Instruction* dst  = nullptr;
    Instruction* repl = nullptr;

    /* Handle GEPs with constant offsets */
    auto* gep = dyn_cast<GetElementPtrInst>(inst);
    if( gep != nullptr ) {
      auto* ptr = dyn_cast<Instruction>(gep->getPointerOperand());
      LLVM_DEBUG(dbgs() << "Optimising GEP: " << *gep << '\n');

      if( gep->hasAllConstantIndices() && (ptr != nullptr) ) {
        LLVM_DEBUG(dbgs() << "  All constant indexes.\n");
        /* We can move / unify constant GEPs only */
        SmallVector<unsigned, 4> idx;
        collect_gep_idx(gep, &idx);

        /* Make sure that every GEP is unique (base + idxs) */
        auto key = std::make_pair(ptr, idx);
        auto it  = geps.find(key);
        if( it == geps.end() ) {
          LLVM_DEBUG(dbgs() << "  Registering new for base " << *ptr
                            << " and indices.\n");
          /* Move the GEP just behind the pointer definition */
          dst       = ptr;
          geps[key] = gep;
        } else {
          /* Replace the GEP with the found, equivalent one */
          repl = it->second;
          LLVM_DEBUG(dbgs() << "  Found identical GEP: " << *repl
                            << " for " << *gep << '\n');
        }
      }
    }

    /* Handle all bitcasts */
    auto* bc = dyn_cast<BitCastInst>(inst);
    if( bc != nullptr ) {
      LLVM_DEBUG(dbgs() << "Optimising BC: " << *bc << '\n');
      auto* ptr = dyn_cast<Instruction>(bc->getOperand(0));

      if( ptr != nullptr ) {
        /* Make all bitcasts unique (base + target type) */
        auto key = std::make_pair(ptr, bc->getType());
        auto it  = bitcasts.find(key);
        if( it == bitcasts.end() ) {
          LLVM_DEBUG(dbgs() << "  Registering new for type "
                            << *bc->getType() << '\n');
          /* We are the only one -> move right after the pointer */
          dst           = ptr;
          bitcasts[key] = bc;
        } else {
          repl = it->second;
          /* We already have an identical BC -> replace */
          LLVM_DEBUG(dbgs() << "  Found equivalent BC: " << *repl
                            << " for " << *bc << '\n');
        }
      }
    }

    /* Check if we need to move / replace the current instruction */
    if( dst != nullptr ) {
      LLVM_DEBUG(dbgs() << "Moving to just after " << *dst << '\n');
      /* Keep moved instructions in-order at the insertion site, by keeping
       * track of the last moved instruction */
      auto* dst_tail = dst;

      auto it = insert_place.find(dst);
      if( it != insert_place.end() ) {
        dst_tail = it->second;
        LLVM_DEBUG(dbgs() << "Insertion place for " << *dst << " is "
                          << *dst_tail << '\n');
      }
      auto* dst_new = move_inst(inst, dst_tail);

      changes |= (dst_new != nullptr);
      if( dst != dst_tail )
        insert_place[dst] = inst;
      insert_place[dst_tail] = inst;
    } else if( repl != nullptr ) {
      LLVM_DEBUG(dbgs() << "Replacing with " << *repl << '\n');
      inst->replaceAllUsesWith(repl);
      inst->eraseFromParent();
      changes = true;
    }
  }
  return changes;
}

/**
 * Simplify phi nodes if they are now constant or constant_or_undef.
 */
bool compact_geps::simplify_phis(Function& f) {
  bool changes     = false;

  /* Loop around multiple times, in case there are cascading
   * simplifications */
  for( auto iit = inst_begin(f); iit != inst_end(f); ) {
    auto* inst = &*iit;
    ++iit;

    /* We only want pointer phis */
    if( !inst->getType()->isPointerTy() || !isa<PHINode>(inst))
      continue;

    auto* phi = cast<PHINode>(inst);
    auto* val = phi->hasConstantValue();
    if( val != nullptr ) {
      /* Easy case: all values are the same -> just drop the phi */
      phi->replaceAllUsesWith(val);
      phi->eraseFromParent();
      changes = true;
    } else if( phi->hasConstantOrUndefValue() ) {
      /* Harder case: same value + some undefs */

      /* Find the first non-undef value */
      for( auto& in_val : phi->incoming_values() ) {
        if( !isa<UndefValue>(in_val) ) {
          val = in_val;
          break;
        }
      }
      assert(val != nullptr);

      /* If the found value dominates the phi, replace the phi */
      auto* repl = cast<Instruction>(val);
      if( (repl == nullptr) || dt->dominates(repl, phi) ) {
        phi->replaceAllUsesWith(val);
        phi->eraseFromParent();
        changes = true;
      }
    }
  }
  return changes;
}

char compact_geps::ID = 0;
static RegisterPass<compact_geps>
  X("compact-geps", "Move and compact pointer casts, constant GEPs, and simplify PHIs.",
    true,
    false
  );
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
