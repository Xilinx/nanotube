/**************************************************************************\
*//*! \file add_trace.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Add trace calls to the program.
**   \date  2021-01-26
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "llvm_common.h"
#include "llvm_insns.h"
#include "llvm_pass.h"

#include "llvm/IR/IRBuilder.h"

#define DEBUG_TYPE "add_trace"

// The add-trace pass
// ==================
//
// This pass adds a nanotube_trace call for each integer value
// computed by the program.  Each trace call is assigned a unique ID.
//
// Input conditions
// ----------------
//
// None.
//
// Output conditions
// -----------------
//
// Calls to nanotube_trace have been inserted.
//
// Theory of operation
// -------------------
//
// The pass runs on each function.  It iterates over each basic block
// and then the instructions in that basic block.  It inserts a trace
// call after each instruction which produces an integer value.
//
// PHI nodes and terminator instructions need to be handled specially
// because the LLVM-IR validity rules do not always allow a call to be
// added immediately afterwards.
//
// PHI nodes are handled as a group, so the trace calls are added
// after the last PHI node.  Terminator instructions which produce a
// value are ignored as these instructions are not expected.

using namespace nanotube;
using namespace llvm;

namespace
{
  class add_trace: public llvm::FunctionPass
  {
  public:
    static char ID;
    add_trace();
    bool runOnFunction(Function& f) override;
  private:
    /* The next ID to assign to a trace call. */
    uint64_t m_next_id;
  };
} // anonymous namespace

char add_trace::ID;

add_trace::add_trace():
  FunctionPass(ID),
  m_next_id(0)
{
}

bool add_trace::runOnFunction(Function& func)
{
  /* Create the trace intrinsic function. */
  Module *module = func.getParent();
  LLVMContext &context = module->getContext();
  Type *result_type = Type::getVoidTy(context);
  Type *uint64_type = Type::getInt64Ty(context);
  Type *arg_types[] = { uint64_type, uint64_type };
  FunctionType *func_type = FunctionType::get(result_type, arg_types, false);
  Constant *trace_func = module->getOrInsertFunction("nanotube_debug_trace", func_type);

  /* Indicates whether any changes have been made. */
  bool any_changes = false;

  /* Iterate over the basic blocks. */
  for (BasicBlock &bb: func) {
    /* Find out where to insert the first PHI. */
    auto insert_point = bb.getFirstInsertionPt();

    IRBuilder<> builder(&bb, insert_point);

    /* Process the PHI nodes. */
    for (Instruction &insn: bb.phis()) {
      Type *insn_type = insn.getType();
      if (insn_type->isIntegerTy()) {
        ConstantInt *id_val = builder.getInt64(m_next_id);
        Value *value_arg = builder.CreateIntCast(&insn, uint64_type, false);
        Value *trace_args[] = { id_val, value_arg };
        builder.CreateCall(trace_func, trace_args);
        any_changes = true;
        m_next_id += 1;
      }
    }

    /* Process the non-PHI nodes. */
    while (builder.GetInsertPoint() != bb.end()) {
      Instruction *insn = &*(builder.GetInsertPoint());

      /* Do not trace the terminator. */
      if (insn->isTerminator())
        break;

      /* Move the iterator on and get ready to insert the trace
       * call. */
      builder.SetInsertPoint(insn->getNextNode());

      /* Insert a trace call if necessary. */
      Type *insn_type = insn->getType();
      if (insn_type->isIntegerTy()) {
        ConstantInt *id_val = builder.getInt64(m_next_id);
        Value *value_arg = builder.CreateIntCast(insn, uint64_type, false);
        Value *trace_args[] = { id_val, value_arg };
        builder.CreateCall(trace_func, trace_args);
        any_changes = true;
        m_next_id += 1;
      }
    }
  }

  return any_changes;
}

static RegisterPass<add_trace>
    X("add-trace", "Add trace statements for integer values produced.",
      true, // IsCFGOnlyPass
      false // IsAnalysis
      );
