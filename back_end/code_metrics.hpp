#ifndef __CODE_METRICS_HPP__
#define __CODE_METRICS_HPP__
/*******************************************************/
/*! \file code_metrics.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Compute code metrics over packet kernel code.
**   \date 2022-03-02
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/


#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

#include "Nanotube_Alias.hpp"

namespace {
struct code_metrics : public llvm::ModulePass {
  code_metrics() : ModulePass(ID) {
  }
  static char ID;
  llvm::PostDominatorTree* pdt;
  llvm::DominatorTree* dt;
  llvm::AliasAnalysis* aa;
  void get_all_analysis_results(llvm::Function* f);
  void getAnalysisUsage(llvm::AnalysisUsage &info) const override;
  bool runOnModule(llvm::Module& m) override;
};
} // anonymous namespace
#endif //ifdef __CODE_METRICS_HPP__
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
