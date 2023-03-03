/**************************************************************************\
*//*! \file replace_malloc.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Replace each call to nanotube_malloc with a static buffer.
**   \date  2021-07-14
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

// The replace-malloc pass
// =======================
//
// This pass finds calls to nanotube_malloc in the setup function and
// replaces each call with a newly created static buffer of the same
// size.
//
// Input conditions
// ----------------
//
// A setup function exists and consists of a single basic block.
//
// Output conditions
// -----------------
//
// All calls to nanotube_malloc have been replaced with newly created
// static buffers.
//
// Theory of operation
// -------------------
//
// The pass iterates over the instructions in the entry block of the
// setup function.  When it encounters a call, it determines whether
// it is a call to nanotube_malloc.  If it is, it creates a new static
// buffer of the same size and replaces all uses of the call with
// references to the static buffer.  It then deletes the call and
// continues processing.

#include "llvm_common.h"
#include "llvm_insns.h"
#include "llvm_pass.h"
#include "setup_func.hpp"
#include "utils.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

#define DEBUG_TYPE "replace-malloc"

///////////////////////////////////////////////////////////////////////////

using namespace nanotube;

namespace
{
/*! The replace-malloc pass. */
class replace_malloc_pass: public llvm::ModulePass
{
public:
  /*! A variable at a unique address. */
  static char ID;

  /*! Construct the pass. */
  replace_malloc_pass();

  /*! Create a new static buffer. */
  Value *make_buf(Module *module, uint32_t size);

  /*! Modify a call to nanotube_thread_create to use a static
   *  buffer. */
  void update_thread_func(CallBase *call,
                          thread_create_args *args,
                          Value *buf);

  /*! Run the pass on a module. */
  bool runOnModule(Module &m);
};
} // namespace

///////////////////////////////////////////////////////////////////////////

replace_malloc_pass::replace_malloc_pass():
  ModulePass(ID)
{
}

Value *
replace_malloc_pass::make_buf(Module *module, uint32_t size)
{
  llvm::LLVMContext *ctxt = &(module->getContext());
  static const std::string name("nanotube_malloc_buf");
  LLVM_DEBUG(
    dbgs() << "Name is " << name << "\n";
  );
  IntegerType *int8_ty = Type::getInt8Ty(*ctxt);
  Type *buf_ty = ArrayType::get(int8_ty, size);
  ConstantAggregateZero *init = ConstantAggregateZero::get(buf_ty);
  GlobalVariable *buf = new GlobalVariable(*module, buf_ty, false,
                                           GlobalValue::ExternalLinkage,
                                           init, name);
  Constant *zero = ConstantInt::get(int8_ty, 0, false);
  Constant *gep_indexes[] = { zero, zero };
  auto *res = ConstantExpr::getGetElementPtr(buf_ty, buf, gep_indexes,
                                             true);
  LLVM_DEBUG(
    dbgs() << "Replacement is " << *res << "\n";
  );
  return res;
}

void replace_malloc_pass::update_thread_func(CallBase *call,
                                             thread_create_args *args,
                                             Value *buf)
{
  auto func_arg_num = thread_create_args::func_arg_num;
  auto info_area_arg_num = thread_create_args::info_area_arg_num;
  auto info_area_size_arg_num = thread_create_args::info_area_size_arg_num;

  // Copy the operands into the buffer.
  Value *data_arg = call->getArgOperand(info_area_arg_num);
  Value *size_arg = call->getArgOperand(info_area_size_arg_num);
  auto builder = llvm::IRBuilder<>(call);
  builder.CreateMemCpy(buf, 1, data_arg, 1, args->info_area_size);

  // Find the old function.
  Value *func_val = call->getArgOperand(func_arg_num);
  Function *old_func = dyn_cast<Function>(func_val);

  if (old_func == nullptr)
    report_fatal_errorv("Function argument is not constant: {0}", *call);

  // Create a copy of the function.
  llvm::ValueToValueMapTy values;
  Function *new_func = CloneFunction(old_func, values);
  new_func->setLinkage(Function::PrivateLinkage);

  // Update the function to use the buffer directly.
  Argument *arg = &*(new_func->arg_begin()+THREAD_FUNC_INFO_ARG);
  LLVM_DEBUG(dbgv("Replace {0} with {1}\n", *arg, *buf););
  arg->replaceAllUsesWith(buf);

  // Update the call to use the new function.
  call->setArgOperand(func_arg_num, new_func);

  // Clear the data argument.
  PointerType *ptr_ty = cast<PointerType>(data_arg->getType());
  data_arg = ConstantPointerNull::get(ptr_ty);
  call->setArgOperand(info_area_arg_num, data_arg);

  // Clear the size argument.
  IntegerType *int_ty = cast<IntegerType>(size_arg->getType());
  size_arg = ConstantInt::get(int_ty, 0);
  call->setArgOperand(info_area_size_arg_num, size_arg);

  // Delete the old function if possible.
  if (old_func->user_begin() == old_func->user_end()) {
    LLVM_DEBUG(dbgv("Removing unused function {0}.\n", as_operand(old_func)););
    old_func->eraseFromParent();
  }
}

bool replace_malloc_pass::runOnModule(Module &module)
{
  // Keep track of whether there have been any changes.
  bool any_changes = false;

  // Find the entry basic block of the setup function.
  auto setup = setup_func(module);
  Function &f = setup.get_function();
  BasicBlock &bb = f.getEntryBlock();

  Instruction *terminator = bb.getTerminator();
  assert(terminator != nullptr);
  if (terminator->getOpcode() != Instruction::Ret) {
    report_fatal_errorv("Cannot handle instruction in Nanotube setup"
                        " function: {0}", *terminator);
  }

  // Iterate over the instructions in the basic block.
  Instruction *curr;
  Instruction *next;
  for (curr = bb.getFirstNonPHI(); curr != nullptr; curr = next) {
    next = curr->getNextNode();
    LLVM_DEBUG(
      dbgs() << "Found instruction " << *curr << "\n";
    );
    auto iid = get_intrinsic(curr);
    if (iid != Intrinsics::malloc)
      continue;

    malloc_args args(curr);
    auto const_size = dyn_cast<ConstantInt>(args.size);
    if (const_size == nullptr)
      report_fatal_errorv("Non-constant malloc size in {0}", *curr);
    auto size = const_size->getValue();

    LLVM_DEBUG(
      dbgs() << "  Buffer size " << size << "\n";
    );

    // Create a static buffer and replace the malloc.
    llvm::Value *buf = make_buf(&module, size.getLimitedValue());
    curr->replaceAllUsesWith(buf);
    curr->eraseFromParent();
    any_changes = true;
    // NB. curr has been deleted.
  }

  // Iterate over the threads looking for threads which have user
  // data.
  for (auto &info: setup.threads()) {
    CallBase *call = info.call();
    auto args = thread_create_args(call);
    if (args.info_area_size == 0)
      continue;

    Value *buf = make_buf(&module, args.info_area_size);
    update_thread_func(call, &args, buf);

    any_changes = true;
  }

  return any_changes;
}

char replace_malloc_pass::ID = 0;
static RegisterPass<replace_malloc_pass>
  reg("replace-malloc", "Replace each call to malloc with a static buffer",
    false, // CFGOnly
    false  // is_analysis
    );

///////////////////////////////////////////////////////////////////////////
