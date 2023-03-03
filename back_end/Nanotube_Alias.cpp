/*******************************************************/
/*! \file Nanotube_Alias.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Alias pass that knows about Nanotube specifics.
**   \date 2020-04-09
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
** A custom alias analysis that knows about EBPF specifics, such as map
** lookups (they never write to the key, their result only potentially
** aliases with lookups from the same map, ...)
**/

#include "Nanotube_Alias.hpp"

#define DEBUG_TYPE "ntaa"

using namespace llvm;
using namespace nanotube;

//namespace {

AliasResult NanotubeAAResult::alias(const MemoryLocation& loc_a,
                  const MemoryLocation& loc_b) {
  /* Get a reasonable first result from some other AA, which is sometimes
   * in NTAA to make better decisions. */
  // auto res = (basic_aa != nullptr) ? basic_aa->alias(loc_a, loc_b)
  //                                 : Base::alias(loc_a, loc_b);

  /* Instead of calling into a specific analysis directly, simply use the
   * getBestAAResults and protect against it calling us again. */
  if( in_alias )
    return AliasResult::MayAlias;
  manage_in_alias mia(*this);
  auto res = getBestAAResults().alias(loc_a, loc_b);

  LLVM_DEBUG(
    dbgs() << "alias: \n"
           << "  A: " << loc_a << '\n'
           << "  B: " << loc_b << '\n'
           << " Res: " << res << '\n');

  /* Bail very early if the other stuff is already good :) */
  if( res != MayAlias )
    return res;

  SmallVector<Value*, 4> vals_a;
  SmallVector<Value*, 4> vals_b;

  // XXX: More caching here!
  GetUnderlyingObjects(const_cast<Value*>(loc_a.Ptr), vals_a,
                       data_layout, loop_info, 0);
  GetUnderlyingObjects(const_cast<Value*>(loc_b.Ptr), vals_b,
                       data_layout, loop_info, 0);

  LLVM_DEBUG(
    dbgs() << "  A underlying objects " << vals_a.size() << '\n';
    for( auto* v : vals_a )
      dbgs() << "    " << *v << " " << (unsigned)get_provenance(v) << '\n';
    dbgs() << "  B underlying objects " << vals_b.size() << '\n';
    for( auto* v : vals_b )
      dbgs() << "    " << *v << " " << (unsigned)get_provenance(v) << '\n';
  );

  unsigned prov_a, prov_b;
  prov_a = get_provenance(vals_a);
  prov_b = get_provenance(vals_b);

  /* Bail early if we know nothing :) */
  if( (prov_a == NONE) && (prov_b == NONE))
    return res;

  /* Obvious case: completely disjoint types of data */
  if( (prov_a & prov_b) == 0 ) {
    LLVM_DEBUG(dbgs() << "  Disjoint sources " << prov_a << " vs "
                      << prov_b << " => NoAlias\n");
    return NoAlias;
  }

  /* If both sets are map accesses, check if the maps where they come
   * from are different */
  if( prov_a == MAP && prov_b == MAP ) {
    SmallPtrSet<Value*, 4> maps_a;
    SmallPtrSet<Value*, 4> maps_b;
    for( auto* v : vals_a )
      maps_a.insert(get_nt_map_arg(cast<CallInst>(v)));
    for( auto* v : vals_b )
      maps_b.insert(get_nt_map_arg(cast<CallInst>(v)));

    LLVM_DEBUG(
      dbgs() << "  A source maps:\n";
      for( auto* v : maps_a )
        dbgs() << "    " << *v << '\n';
      dbgs() << "  B source maps:\n";
      for( auto* v : maps_b )
        dbgs() << "    " << *v << '\n';
    );

    /* Check that the source maps in A and B are pairwise different */
    if( disjoint(maps_a, maps_b) ) {
      LLVM_DEBUG(dbgs() << "  No overlap => NoAlias\n");
      return NoAlias;
    }

    /* There is at least one map identical to both sets. */
    /* Special case: one lookup, check same map and args */
    //XXX: Implement me!
  }

  /* The context pointers and the context content never overlap */
  if( ((prov_a & prov_b) == CTX) && ((prov_a ^ prov_b) == INDIR) ) {
    return NoAlias;
  }

  /* Two entries loaded from the context are always separate */
  if( (prov_a == (CTX | INDIR)) && (prov_b == (CTX | INDIR)) ) {
    if( !overlap(vals_a, vals_b) )
      return NoAlias;
  }

  /* Two variables on the stack (from alloca) never overlap unless their
     base is the same */
  if( prov_a == STACK && prov_b == STACK ) {
    if( !overlap(vals_a, vals_b) ) {
      LLVM_DEBUG(
        dbgs() << "Different underlying stack values. NoAlias!\n" );
      return NoAlias;
    }
  }

#if 0
    /* NOTE SD: I will leave this unimplemented for now, as it is not yet
     * needed.  Once we know a location is a packet location, we don't care
     * too much about alias analysis disambiguating different bytes of the
     * packet.
     *
     * The challenge here is of course cases where (one of) the offsets are
     * not constant, but instead come from a (memory) PHI node.  Fully
     * enumerating all options back to the base package can lead to O(2^N)
     * runtime.  Fortunately, the case where that happens:
     *
     *             foo = nt_get_packet_data
     *              /                     \
     *        pkt1 = GEP foo, A        pkt2 = GEP foo, B
     *              \                     /
     *               pkt12 = PHI(pk1, pkt2)
     *              /                     \
     *        pkt3 = GEP pkt12, C      pkt4 = GEP pkt12, D
     *              \                     /
     *              pkt32 = PHI(pkt3,pkt4)
     *
     * we can avoid the blow-up, because for alias analysis, we only ever
     * have to trace back to the common pkt12 ancestor if the query is for
     * pkt3 and pkt4.  Of course, in the general case, we would have to
     * trace back to the dominator(pkt_X, pkt_Y), and suck up the
     * exponential blow-up to there; which is not great. Further, loops
     * need to be treated proeprly.
     *
     * In the example code looked at so far, the distances are rarely a
     * problem, but in the general case, this can be tough.  So I am not
     * going to implement this for now!
     */
    /* If both guys are packet locations, have a closer look. */
    if( prov_a == PKT && prov_b == PKT) {
      int64_t off_a, off_b;
      Value* base_a =
        GetPointerBaseWithConstantOffset(const_cast<Value*>(loc_a.Ptr),
                                         off_a, data_layout);
      Value* base_b =
        GetPointerBaseWithConstantOffset(const_cast<Value*>(loc_b.Ptr),
                                         off_b, data_layout);
      if( base_a != nullptr && base_b != nullptr) {
        dbgs() << "A: Base " << *base_a << " Offset: " << off_a << '\n';
        dbgs() << "B: Base " << *base_b << " Offset: " << off_b << '\n';
      }
    }
#endif

  /**
   * XXX: There are other cases where we could be more precise:
   *  * same map, different keys => no alias
   *  * same map, same key => check offsets
   *  * different packet base pointers (after an resize) => do the
   *    math, and then check offsets
   */

  LLVM_DEBUG(
    /* Murky case => print */
    dbgs() << "alias: \n"
           << "  A: " << loc_a << '\n'
           << "  B: " << loc_b << '\n'
           << " Res: " << res << '\n';
    dbgs() << "  A underlying objects " << vals_a.size() << '\n';
    for( auto* v : vals_a )
      dbgs() << "    " << *v << " " << (unsigned)get_provenance(v) << '\n';
    dbgs() << "  B underlying objects " << vals_b.size() << '\n';
    for( auto* v : vals_b )
      dbgs() << "    " << *v << " " << (unsigned)get_provenance(v) << '\n';
    dbgs() << "  ProvA: " << prov_a << " ProvB: " << prov_b << '\n';
  );

  return res;
}

ModRefInfo
NanotubeAAResult::getArgModRefInfo(const CallBase* call, unsigned arg_idx) {
  auto res = Base::getArgModRefInfo(call, arg_idx);

  LLVM_DEBUG(
    dbgs() << "getArgModRefInfo: Call: " << *call << " Arg: " << arg_idx
           << " Res: " << res << '\n');

  auto intr = get_intrinsic(call);
  if( (intr < Intrinsics::packet_read) ||
      (intr > Intrinsics::map_remove) )
    return res;

  Value* v = call->getArgOperand(arg_idx);
  if( !v->getType()->isPointerTy() )
    return ModRefInfo::NoModRef;

  res = get_nt_arg_info(intr, arg_idx);
  LLVM_DEBUG(dbgs() << "New res: " << res << '\n');
  return res;
}

ModRefInfo
NanotubeAAResult::getModRefInfo(const CallBase* call, const MemoryLocation& loc) {
  auto res = Base::getModRefInfo(call, loc);

  LLVM_DEBUG(
    dbgs() << "getModRefInfo: Call: " << *call << " Loc: " << loc
           << " Res: " << res << '\n');

  auto intr = get_intrinsic(call);
  if( (intr < Intrinsics::packet_read) ||
      (intr > Intrinsics::map_remove) )
    return res;

  res = ModRefInfo::NoModRef;

  /* Check whether the "behind the scenes" memory is queried */
  // XXX: Implement me.  But also, how should that memory end up in
  // loc???

  /* Check the remaining interaction options: arguments */
  bool all_precise  = true;
  for( unsigned arg = 0; arg < call->arg_size(); arg++ ) {
    Value* v = call->getArgOperand(arg);
    if( !v->getType()->isPointerTy() )
      continue;
    auto v_loc = get_memory_location(call, arg, target_lib_info);
    auto aares = alias(v_loc, loc);
    if( aares != NoAlias) {
      res = unionModRef(res, get_nt_arg_info(intr, arg));
      all_precise &= (aares != MayAlias);
    }
  }

  /* If all results were precise (MustAlias / NoAlias) we know that the
   * function MustMod / MustRef! */
  if( res != ModRefInfo::NoModRef )
    res = all_precise ? setMust(res) : clearMust(res);

  LLVM_DEBUG(dbgs() << "New res: " << res << '\n');
  return res;
}

ModRefInfo
NanotubeAAResult::getModRefInfo(const CallBase* call1, const CallBase* call2) {
  auto res = Base::getModRefInfo(call1, call2);
  LLVM_DEBUG(
    dbgs() << "getModRefInfo: Call1: " << *call1 << " Call2: "
           << *call2 << " Res: " << res << '\n');
  return res;
}

FunctionModRefBehavior
NanotubeAAResult::getModRefBehavior(const CallBase* call) {
  auto res = Base::getModRefBehavior(call);

  LLVM_DEBUG(
    dbgs() << "getModRefBehavior: " << *call << " Res: " << res << '\n');

  /* Simply get the information for the called function, ignoring any
   * specifics from the arguments for now, e.g., map_op specialisations
   * to the various operations */
  Function* callee = call->getCalledFunction();
  if( callee == nullptr )
    return res;
  else
    return getModRefBehavior(callee);
}

FunctionModRefBehavior
NanotubeAAResult::getModRefBehavior(const Function* func) {
  auto res = Base::getModRefBehavior(func);

  LLVM_DEBUG(
    dbgs() << "getModRefBehavior: " << func->getName() << " Res: " << res
           << '\n');

  auto intr = get_intrinsic(func);
  if( (intr < Intrinsics::packet_read) ||
      (intr > Intrinsics::get_time_ns) )
    return res;

  return get_nt_fmrb(intr);
}

bool
NanotubeAAResult::PointsToConstantMemory(const MemoryLocation& loc, bool or_local) {
  auto res = Base::pointsToConstantMemory(loc, or_local);

  LLVM_DEBUG(
    dbgs() << "PointsToConstantMemory: Loc: " << loc << " or_local: "
           << or_local << " Res: " << res << '\n');

  return res;
}

Value* NanotubeAAResult::get_nt_map_arg(const CallInst* call) {
  return call->getOperand(0);
}


struct NanotubeAA : public AnalysisInfoMixin<NanotubeAA> {
  friend AnalysisInfoMixin<NanotubeAA>;
  NanotubeAAResult run(Function& F, FunctionAnalysisManager& AM) {
    return NanotubeAAResult(&F,
             &AM.getResult<LoopAnalysis>(F),
             F.getParent()->getDataLayout(),
             AM.getResult<TargetLibraryAnalysis>(F),
             &AM.getResult<BasicAA>(F));
  }
};

bool NanotubeAAPass::runOnFunction(Function& f) {
  LLVM_DEBUG(dbgs() << "NTAAPass::runOnFunction " << f.getName()
                    << '\n');
  auto& li  = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto& tli = getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  auto& baa = getAnalysis<BasicAAWrapperPass>().getResult();

  /* Stash the AA result into the AA aggregator directly, instead of
   * relying on callbacks etc. */
  auto& aar = getAnalysis<AAResultsWrapperPass>().getAAResults();
  result = new NanotubeAAResult(&f, &li, f.getParent()->getDataLayout(),
                                tli, &baa);
  aar.addAAResult(*result);

  return false;
}

//} // anonymous namespace

char NanotubeAAPass::ID    = 0;

#ifndef LEGACY_REGISTER
struct InitMyPasses {
  InitMyPasses() {
    initializeNanotubeAAPassPass(*PassRegistry::getPassRegistry());
  }
};
static InitMyPasses X;

INITIALIZE_PASS_BEGIN(NanotubeAAPass, "nanotube-aa",
  "AliasAnalysis for Nanotube specifics FP", false, true);
// NOTE: Using below causes an infinite loop!
//INITIALIZE_PASS_DEPENDENCY(NanotubeAAWrapper)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(BasicAAWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_END(NanotubeAAPass, "nanotube-aa",
  "AliasAnalysis for Nanotube specifics FP", false, true);

#else
static RegisterPass<NanotubeAAPass>
    X("nanotube-aa", "AliasAnalysis for Nanotube specifics FP", false, true);
#endif

namespace nanotube {
ModRefInfo aa_helper::get_mri(Instruction* inst, MemoryLocation mloc) {
  StoreInst* st = dyn_cast<StoreInst>(inst);
  if( st != nullptr )
    return get_mri(st, mloc);

  LoadInst* ld = dyn_cast<LoadInst>(inst);
  if( ld != nullptr )
    return get_mri(ld, mloc);

  CallBase* call = dyn_cast<CallBase>(inst);
  if( call != nullptr )
    return get_mri(call, mloc);

  errs() <<"XX: Unknown and unhandled Instruction " << *inst << '\n';
  return ModRefInfo::ModRef;
}
ModRefInfo aa_helper::get_mri(StoreInst* st, MemoryLocation mloc) {
  auto st_mloc = MemoryLocation::get(st);
  auto aares = aa->alias(st_mloc, mloc);
  //dbgs() << "      " << mloc << " AA: " << aares << '\n';
  switch( aares ) {
    case NoAlias:      return ModRefInfo::NoModRef;
    case MayAlias:     return ModRefInfo::Mod;
    case PartialAlias: return ModRefInfo::MustMod;
    case MustAlias:    return ModRefInfo::MustMod;
    default: assert(false);
  }
  return ModRefInfo::ModRef;
}
ModRefInfo aa_helper::get_mri(LoadInst* ld, MemoryLocation mloc) {
  auto ld_mloc = MemoryLocation::get(ld);
  auto aares = aa->alias(ld_mloc, mloc);
  //dbgs() << "      " << mloc << " AA: " << aares << '\n';
  switch( aares ) {
    case NoAlias:      return ModRefInfo::NoModRef;
    case MayAlias:     return ModRefInfo::Ref;
    case PartialAlias: return ModRefInfo::MustRef;
    case MustAlias:    return ModRefInfo::MustRef;
    default: assert(false);
  }
  return ModRefInfo::ModRef;
}
ModRefInfo aa_helper::get_mri(CallBase* call, MemoryLocation mloc) {
  auto mri = aa->getModRefInfo(call, mloc);
  //dbgs() << "      " << mloc << " MRI: " << mri << '\n';
  return mri;
}
aa_helper::aa_helper(AAResults* aa) : aa(aa) {}
};

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
