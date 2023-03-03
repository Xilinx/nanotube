/*******************************************************/
/*! \file  Mem2req.h
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert Nanotube memory interface to requests
**   \date 2020-07-14
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"

#include "Dep_Aware_Converter.h"

namespace {
  using llvm::CallInst;
  using llvm::Function;
  using llvm::IRBuilder;
  using llvm::Instruction;
  using llvm::IntToPtrInst;
  using llvm::LLVMContext;
  using llvm::Module;
  using llvm::Twine;
  using llvm::Type;
  using llvm::Value;

  using nanotube::dep_aware_converter;

  struct mem_to_req : public llvm::FunctionPass {

    mem_to_req() : FunctionPass(ID) {}
    bool runOnFunction(Function& f) override;

    static char ID;

    static bool convert_to_req(Function& f);
  }; // struct mem_to_req

  struct nt_meta {
    Value* base;      /* base package / map */
    Value* key;       /* key for a map lookup; nullptr for packets */
    Value* key_sz;    /* size of the key as a value */
    Value* dummy_read_ret;  /* return value of the dummy map read */

    bool   is_map;    /* true -> map; false -> packet */
    bool   is_start;
  };
  typedef std::unordered_map<Value*, nt_meta*> val_meta_map;
  typedef std::vector<nt_meta*>                meta_vec;

  struct flow_conversion {
    meta_vec      meta_nodes;
    val_meta_map  val_to_meta;
    std::vector<Instruction*> deletion_candidates;
    std::unordered_map<Instruction*, unsigned> input_deps;
    dep_aware_converter<Value> dac;

    Module&       m;
    LLVMContext&  c;
    flow_conversion(Function& f);
    void one_dep_ready(Value *val);
    void flow(Value* v);
    void flow(Instruction* inst);

    void convert_nanotube_root(CallInst* inst);
    void convert_call(CallInst* call);
    void convert_special_case_call(CallInst* call);

    void convert_mem(Instruction* inst);
    void convert_memcpy(Instruction* inst);
    void replace_and_cleanup(Instruction* inst, Value* repl,
                             IntToPtrInst* i2p);

    Instruction* typed_alloca(Type* ty, const Twine& name, Value* size,
                              IRBuilder<>* ir);
  };
}; //anonymous namespace
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
