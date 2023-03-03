/*******************************************************/
/*! \file flatten_cfg.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Flatten the control flow in LLVM IR.
**   \date 2022-11-28
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Pass.h"

namespace {
struct flatten_cfg : public llvm::ModulePass {
  flatten_cfg() : llvm::ModulePass(ID) {
  }
  void getAnalysisUsage(llvm::AnalysisUsage &info) const override;
  void get_all_analysis_results(llvm::Function& f);

  llvm::DominatorTree*     dt;
  llvm::PostDominatorTree* pdt;

  /* LLVM-specific */
  static char ID;
  bool runOnModule(llvm::Module& f) override;
};
}; // anonymous namespace

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
