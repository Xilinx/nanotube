/*******************************************************/
/*! \file Nanotube_Alias.hpp
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

#ifndef __NANOTUBE_ALIAS_HPP__
#define __NANOTUBE_ALIAS_HPP__

#include <map>

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Pass.h"

#include "Intrinsics.h"
#include "printing_helpers.h"
#include "Provenance.h"
#include "set_ops.hpp"
#include "../include/nanotube_api.h"

#define DEBUG_TYPE "ntaa"

//#define LEGACY_REGISTER

#ifndef LEGACY_REGISTER
namespace llvm {
  void initializeNanotubeAAPassPass(PassRegistry&);
};
#endif

//namespace {
struct NanotubeAAResult : public AAResultBase<NanotubeAAResult> {
  friend AAResultBase<NanotubeAAResult>;
  using Base = AAResultBase<NanotubeAAResult>;

  Function* func;

  LoopInfo*                loop_info;
  const DataLayout&        data_layout;
  const TargetLibraryInfo& target_lib_info;
  BasicAAResult*           basic_aa;

  NanotubeAAResult(Function* f, LoopInfo* li, const DataLayout& dl,
                   const TargetLibraryInfo& tli, BasicAAResult* baa)
                  : AAResultBase(), func(f), loop_info(li),
                    data_layout(dl), target_lib_info(tli), basic_aa(baa), in_alias(false) {
    LLVM_DEBUG(dbgs() << "NanotubeAAResult constructor, called with " <<
      f->getName() << '\n');
  }
  NanotubeAAResult() : AAResultBase(), func(nullptr), loop_info(nullptr), data_layout(*(DataLayout*)nullptr), target_lib_info(*(TargetLibraryInfo*)nullptr), basic_aa(nullptr), in_alias(false) {
    LLVM_DEBUG(dbgs() << "NanotubeAAResult constructor, nofunc\n");
  }
  ~NanotubeAAResult() {
    LLVM_DEBUG(dbgs() << "NanotubeAAResult destructor\n");
  }

  bool in_alias;
  /* Just a noddy RAII wrapper to make sure the in_alias flag is properly
   * set / reset */
  struct manage_in_alias {
    NanotubeAAResult& ntaares;
    manage_in_alias(NanotubeAAResult& n) : ntaares(n) { ntaares.in_alias = true; }
    ~manage_in_alias() { ntaares.in_alias = false; }
  };

  AliasResult alias(const MemoryLocation& loc_a, const MemoryLocation& loc_b);
  ModRefInfo getArgModRefInfo(const CallBase* call, unsigned arg_idx);
  ModRefInfo getModRefInfo(const CallBase* call, const MemoryLocation& loc);
  ModRefInfo getModRefInfo(const CallBase* call1, const CallBase* call2);
  FunctionModRefBehavior getModRefBehavior(const CallBase* call);
  FunctionModRefBehavior getModRefBehavior(const Function* func);
  bool PointsToConstantMemory(const MemoryLocation& loc, bool or_local) __attribute__((unused));

  /******** Helper functions *********/
  Value* get_nt_map_arg(const CallInst* call);
};

struct NanotubeAAPass : public FunctionPass {
  static char ID;
  NanotubeAAResult* result;

  NanotubeAAPass() : FunctionPass(ID) {
    LLVM_DEBUG(dbgs() << "Hello from NanotubeAAPass :)\n");
  }
  ~NanotubeAAPass() {
    LLVM_DEBUG(dbgs() << "NanotubeAAPass Destructor. Bye!\n");
  }

  NanotubeAAResult& getResult() {
    LLVM_DEBUG(dbgs() << "NTAAPass::getResult non-const\n");
    return *result;
  }
  const NanotubeAAResult& getResult() const {
    LLVM_DEBUG(dbgs() << "NTAAPass::getResult const\n");
    return *result;
  }

  bool runOnFunction(Function& f) override;
  void getAnalysisUsage(AnalysisUsage& AU) const override {
    AU.setPreservesAll();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    AU.addRequired<BasicAAWrapperPass>();
    AU.addRequired<AAResultsWrapperPass>();
  }
};

namespace nanotube {
class aa_helper {
public:
  aa_helper(AAResults* aa);
  /* Regularise ModRefInfo queries for various instructions */
  ModRefInfo get_mri(Instruction* inst, MemoryLocation mloc);
  ModRefInfo get_mri(LoadInst* ld,      MemoryLocation mloc);
  ModRefInfo get_mri(StoreInst* st,     MemoryLocation mloc);
  ModRefInfo get_mri(CallBase* call,    MemoryLocation mloc);
private:
  AAResults* aa;
};
};

//} // anonymous namespace
#undef DEBUG_TYPE
#endif //__NANOTUBE_ALIAS_HPP__

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
