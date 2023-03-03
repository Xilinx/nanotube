#ifndef __LIVENESS_HPP__
#define __LIVENESS_HPP__
/*******************************************************/
/*! \file Liveness.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Liveness analysis of values and stack allocations.
**   \date 2021-01-11
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <unordered_map>
#include <unordered_set>

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Pass.h"

#include "../include/nanotube_api.h"
#include "Dep_Aware_Converter.h"
#include "Intrinsics.h"
#include "printing_helpers.h"
#include "Nanotube_Alias.hpp"

using namespace llvm;
using namespace nanotube;

namespace nanotube {
class liveness_analysis;

class liveness_info_t {
public:
  void get_live_in(Instruction* inst,
                   std::vector<Value*>* values,
                   std::vector<MemoryLocation>* stack);
  void get_live_out(Instruction* inst,
                    std::vector<Value*>* values,
                    std::vector<MemoryLocation>* stack);
  void recompute(Function& f);
private:
  friend class liveness_analysis;

  /* Used analysis passes */
  MemorySSA*         mssa;
  AliasAnalysis*     aa;
  TargetLibraryInfo* tli;

  typedef std::unordered_map<Instruction*, std::vector<MemoryLocation>>
    inst_to_memloc_vec_t;
  typedef std::unordered_map<BasicBlock*, std::vector<MemoryLocation>>
    bb_to_memloc_vec_t;
  typedef std::unordered_map<Instruction*, std::vector<Value*>>
    inst_to_val_vec_t;
  typedef std::unordered_map<BasicBlock*, std::vector<Value*>>
    bb_to_val_vec_t;

  inst_to_memloc_vec_t live_mem_in_inst, live_mem_out_inst;
  bb_to_memloc_vec_t   live_mem_in_bb,   live_mem_out_bb;
  inst_to_val_vec_t    live_in_inst,     live_out_inst;
  bb_to_val_vec_t      live_in_bb,       live_out_bb;

  void reset_state();

  template<class M, class K, class V>
  void mark(M& store, K& key, V& val) {
    if( find(store[key], val) == store[key].end() )
      store[key].emplace_back(val);
  }

  void print_liveness_results(raw_ostream& os, Function& f);

  /* Tracing the liveness of LLVM Values */
  std::unordered_set<BasicBlock*> done;
  void liveness_analysis(Function& f);
  void liveness_analysis(Value* v);

  void live_out_at_block(BasicBlock* bb, Value* v);
  void live_out_at_statement(Instruction* inst, Value* v);
  void live_in_at_statement(Instruction* inst, Value* v);

  /* Tracing memory values and their liveness */
  bool get_memory_inputs(Value* val,
                         std::vector<MemoryLocation>* input_mlocs,
                         std::vector<MemoryDef*>* input_mdefs);


  bool ignore_function(CallBase* call);

  /* Consumer -> producer backwards tracing */
  void collect_memory_producers(MemoryAccess* acc, MemoryLocation mloc,
                                std::vector<Value*>* prods);
  void collect_memory_producers(MemoryDef* acc, MemoryLocation mloc,
                                std::vector<Value*>* prods);
  void collect_memory_producers(MemoryPhi* acc, MemoryLocation mloc,
                                std::vector<Value*>* prods);
  void memory_liveness_analysis_backward(Function& f);


  /* Alloca -> potential writes / reads forward tracing */
  void memory_liveness_analysis_forward(Function& f);

  /* Capture memory uses */
  struct memory_use_t {
    Value*         producer;
    Value*         consumer;
    MemoryLocation loc;

    memory_use_t(Value* p, Value* c, MemoryLocation l) : producer(p),
                                                         consumer(c),
                                                         loc(l) {}

    static bool cmp(const memory_use_t& a, const memory_use_t& b) {
      if( a.producer != b.producer )
        return a.producer < b.producer;
      if( a.loc.Ptr != b.loc.Ptr )
        return a.loc.Ptr < b.loc.Ptr;
      return a.consumer < b.consumer;
    }
  };

  void find_producers_in_def(Instruction* inst, MemoryAccess* acc,
         MemoryLocation mloc, std::vector<memory_use_t>* memory_uses);
};

struct liveness_analysis : public FunctionPass {
  liveness_info_t info;
  liveness_info_t& get_liveness_info() { return info; }
  const liveness_info_t& get_liveness_info() const { return info; }

  /* LLVM-specific */
  static char ID;
  liveness_analysis() : FunctionPass(ID) {
  }

  void getAnalysisUsage(AnalysisUsage &info) const override {
    info.addRequired<NanotubeAAPass>();
    info.addRequired<MemorySSAWrapperPass>();
    info.addRequired<AAResultsWrapperPass>();
    info.addRequired<TargetLibraryInfoWrapperPass>();
    info.setPreservesAll();
    // TODO: Add things we preserve
  }

  void get_all_analysis_results() {
    info.mssa = &getAnalysis<MemorySSAWrapperPass>().getMSSA();
    info.aa   = &getAnalysis<AAResultsWrapperPass>().getAAResults();
    info.tli  = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  }

  bool runOnFunction(Function& f) override {
    get_all_analysis_results();
    info.recompute(f);
    return false;
  }
};
} // anonymous namespace
#endif //__LIVENESS_HPP__
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
