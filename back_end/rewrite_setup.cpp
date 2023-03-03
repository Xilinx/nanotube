/**************************************************************************\
*//*! \file rewrite_setup.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A pass to rewrite the Nanotube setup function.
**   \date  2021-12-08
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

// The rewrite-setup pass
// ======================
//
// This pass is intended to simplify the Nanotube setup function by
// rewriting it.  It removes all branch instructions so that there are
// no loops.
//
// Input conditions
// ----------------
//
// The Nanotube setup function is valid according to the setup_func
// class.
//
// Output conditions
// -----------------
//
// The Nanotube setup function has been rewritten.  The new definition
// will be equivalent to the old one, but it will contain no branch
// instructions.
//
// Theory of operation
// -------------------
//
// The general idea is to simulate the setup function using the
// setup_func and setup_func_builder classes.  Every simulated
// instruction which has a side-effect is replicated into a new basic
// block which will become the body of the setup function.  This
// includes calls which create Nanotube resources and stores to
// memory.  It doesn't include branch instructions, so the new setup
// function will contain only a single basic block.
//
// The outline for the pass is:
//   1. Create a new basic block to be populated.
//   2. Create a tracer object to receive setup function callbacks.
//   3. Create the setup_func object, passing it the tracer.
//   3.1. Create instructions in the new basic block when
//        callbacks fron the setup_func_builder occur.
//   4. Destroy the setup_func object.
//   5. Delete the old basic blocks from the setup function.
//   6. Insert the new basic block into the setup function.
//
// The new basic block will be unreachable from the old basic blocks,
// so the setup_func object will ignore it.

#include "setup_func.hpp"
#include "setup_func_builder.hpp"
#include "utils.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <iostream>

#define DEBUG_TYPE "rewrite-setup"

using namespace nanotube;

///////////////////////////////////////////////////////////////////////////

namespace {
class rewrite_setup_tracer: public setup_tracer
{
public:
  rewrite_setup_tracer(Module *m, BasicBlock *bb);

  void process_alloca(
    setup_func_builder *builder,
    AllocaInst *insn,
    setup_value num_elem,
    setup_ptr_value_t ptr) override;

  void process_channel_create(
    setup_func_builder *builder,
    CallBase *insn,
    channel_info *info,
    channel_index_t result) override;

  void process_channel_set_attr(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value index,
    nanotube_channel_attr_id_t attr_id,
    int32_t attr_val) override;

  void process_channel_export(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value index,
    nanotube_channel_type_t type,
    nanotube_channel_flags_t flags) override;

  void process_context_create(
    setup_func_builder *builder,
    CallBase *insn,
    context_info *info,
    context_index_t result) override;

  void process_context_add_channel(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value context_index,
    setup_value channel_id,
    setup_value channel_index,
    nanotube_channel_flags_t flags) override;

  void process_malloc(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value size_val,
    setup_alloc_id_t alloc_id) override;

  void process_memcpy(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value dest,
    setup_value src,
    setup_value size);

  void process_memset(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value dest,
    setup_value val,
    setup_value size) override;

  void process_store(
    setup_func_builder *builder,
    StoreInst *insn,
    setup_value pointer,
    setup_value data) override;

  void process_thread_create(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value context_index,
    thread_info *info) override;

  void process_map_create(
    setup_func_builder *builder,
    CallBase *insn,
    map_index_t map_index,
    map_info *info) override;

  void process_context_add_map(
    setup_func_builder *builder,
    CallBase *insn,
    context_index_t ctx_index,
    map_index_t map_index) override;

  void process_add_plain_kernel(
    setup_func_builder *builder,
    CallBase *insn,
    kernel_index_t kernel_index,
    kernel_info* info) override;

private:
  // Clone a channel pointer into the new basic block.
  Value *clone_channel_value(Type *ty, channel_index_t val);

  // Clone a context pointer into the new basic block.
  Value *clone_context_value(Type *ty, context_index_t val);

  // Clone an int value into the new basic block.
  Value *clone_int_value(Type *ty, setup_int_value_t val);

  // Adjust a pointer by the specified offset and cast it to the
  // specified type.
  Value *adjust_pointer(Type *ty, Value *ptr,
                        setup_ptr_value_t offset);

  // Clone an ptr value into the new basic block.
  Value *clone_ptr_value(setup_func_builder *builder, Type *ty,
                         setup_ptr_value_t val);

  // Clone a value into the new basic block.
  Value *clone_value(setup_func_builder *builder, Value *l_val,
                     const setup_value *s_val);

  // Clone a call into the new basic block, converting all the
  // arguments based on the values passed in array args of size
  // num_args.
  CallBase *clone_call(setup_func_builder *builder, CallBase *orig,
                       const setup_value *args, std::size_t num_args);

  // Clone a call into the new basic block, converting all the
  // arguments based on the values passed in array args.
  template <int N>
  CallBase *clone_call(setup_func_builder *builder, CallBase *orig,
                       const setup_value (&args)[N]) {
    return clone_call(builder, orig, args, N);
  }

  // The Module to generate code into.
  Module *m_module;

  // The BasicBlock to generate code into.
  BasicBlock *m_bb;

  // A builder object for generating instructions.
  llvm::IRBuilder<> m_builder;

  // The channel creation instructions.
  DenseMap<channel_index_t, Instruction *> m_channels;

  // The context creation instructions.
  DenseMap<context_index_t, Instruction *> m_contexts;

  // The memory allocation instructions.
  DenseMap<setup_alloc_id_t, Instruction *> m_allocs;
};
}

///////////////////////////////////////////////////////////////////////////

namespace {
class rewrite_setup_pass: public llvm::ModulePass
{
public:
  /*! A variable at a unique address. */
  static char ID;

  /*! Construct the pass. */
  rewrite_setup_pass();

  /*! Run the pass on the module. */
  bool runOnModule(Module &M) override;
};
} // namespace

///////////////////////////////////////////////////////////////////////////

rewrite_setup_tracer::rewrite_setup_tracer(Module *m, BasicBlock *bb):
  m_module(m), m_bb(bb), m_builder(bb)
{
  auto *ret_insn = m_builder.CreateRetVoid();
  m_builder.SetInsertPoint(ret_insn);
}

void rewrite_setup_tracer::process_alloca(
  setup_func_builder *builder,
  AllocaInst *insn,
  setup_value num_elem,
  setup_alloc_id_t alloc_id)
{
  Type *elem_ty = insn->getAllocatedType();
  LLVM_DEBUG(dbgv("alloca({0}, {1}) = {2}\n",
                  *elem_ty, num_elem, alloc_id););
  auto *ptr_ty = cast<PointerType>(insn->getType());
  Value *num_elem_old = insn->getArraySize();
  Value *num_elem_new = clone_value(builder, num_elem_old, &num_elem);
  auto addr_space = ptr_ty->getAddressSpace();
  auto clone = m_builder.CreateAlloca(elem_ty, addr_space, num_elem_new);
  clone->setName(insn->getName());
  clone->setAlignment(insn->getAlignment());
  clone->setUsedWithInAlloca(insn->isUsedWithInAlloca());
  auto ins = m_allocs.try_emplace(alloc_id, clone);
  assert(ins.second);
}

void rewrite_setup_tracer::process_channel_create(
  setup_func_builder *builder,
  CallBase *insn,
  channel_info *info,
  channel_index_t result)
{
  LLVM_DEBUG(dbgv("nanotube_channel_create(\"{0}\", {1}, {2}) = {3}\n",
                  info->get_name(), info->get_elem_size(),
                  info->get_num_elem(), result););
  APInt elem_size(info->get_elem_size_bits(), info->get_elem_size());
  APInt num_elem(info->get_num_elem_bits(), info->get_num_elem());
  setup_value args[] = {
    setup_value::unknown(),
    setup_value::of_int(elem_size),
    setup_value::of_int(num_elem),
  };
  auto clone = clone_call(builder, insn, args);
  auto ins = m_channels.try_emplace(result, clone);
  assert(ins.second);
}


void rewrite_setup_tracer::process_channel_set_attr(
  setup_func_builder *builder,
  CallBase *insn,
  setup_value index,
  nanotube_channel_attr_id_t attr_id,
  int32_t attr_val)
{
  LLVM_DEBUG(dbgv("nanotube_channel_set_attr({0}, {1}, {2})\n",
                  index, attr_id, attr_val););
  auto attr_id_bits = get_nt_channel_attr_id_bits();
  auto attr_id_ap = APInt(attr_id_bits, attr_id);
  auto attr_val_bits = get_nt_channel_attr_val_bits();
  auto attr_val_ap = APInt(attr_val_bits, attr_val);
  setup_value args[] = {
    index,
    setup_value::of_int(attr_id_ap),
    setup_value::of_int(attr_val_ap),
  };
  clone_call(builder, insn, args);
}


void rewrite_setup_tracer::process_channel_export(
  setup_func_builder *builder,
  CallBase *insn,
  setup_value index,
  nanotube_channel_type_t type,
  nanotube_channel_flags_t flags)
{
  LLVM_DEBUG(dbgv("nanotube_channel_export({0}, {1}, {2})\n",
                  index, type, flags););
  auto type_bits = get_nt_channel_type_bits();
  auto type_ap = APInt(type_bits, type);
  auto flags_bits = get_nt_channel_flags_bits();
  auto flags_ap = APInt(flags_bits, flags);
  setup_value  args[] = {
    index,
    setup_value::of_int(type_ap),
    setup_value::of_int(flags_ap),
  };
  clone_call(builder, insn, args);
}

void rewrite_setup_tracer::process_context_create(
  setup_func_builder *builder,
  CallBase *insn,
  context_info *info,
  context_index_t result)
{
  LLVM_DEBUG(dbgv("nanotube_context_create() = {0}\n",
                  result););
  auto clone = clone_call(builder, insn, nullptr, 0);
  auto ins = m_contexts.try_emplace(result, clone);
  assert(ins.second);
}

void rewrite_setup_tracer::process_context_add_channel(
  setup_func_builder *builder,
  CallBase *insn,
  setup_value context_id,
  setup_value channel_id,
  setup_value channel_index,
  nanotube_channel_flags_t flags)
{
  LLVM_DEBUG(dbgv("nanotube_context_add_channel({0}, {1}, {2}, {3})\n",
                  context_id, channel_id, channel_index, flags););
  auto flags_bits = get_nt_channel_flags_bits();
  auto flags_ap = APInt(flags_bits, flags);
  setup_value args[] = {
    context_id,
    channel_id,
    channel_index,
    setup_value::of_int(flags_ap),
  };
  clone_call(builder, insn, args);
}

void rewrite_setup_tracer::process_malloc(
  setup_func_builder *builder,
  CallBase *insn,
  setup_value size_val,
  setup_alloc_id_t alloc_id)
{
  LLVM_DEBUG(dbgv("nanotube_malloc({0}) = {1}\n", size_val, alloc_id));
  setup_value args[] = { size_val };
  auto clone = clone_call(builder, insn, args);
  auto ins = m_allocs.try_emplace(alloc_id, clone);
  assert(ins.second);
}

void rewrite_setup_tracer::process_memcpy(
  setup_func_builder *builder,
  CallBase *insn,
  setup_value dest,
  setup_value src,
  setup_value size)
{
  LLVM_DEBUG(dbgv("memcpy({0}, {1}, {2})\n", dest, src, size););
  setup_value args[] = { dest, src, size, setup_value::unknown() };
  clone_call(builder, insn, args);
}

void rewrite_setup_tracer::process_memset(
  setup_func_builder *builder,
  CallBase *insn,
  setup_value dest,
  setup_value val,
  setup_value size)
{
  LLVM_DEBUG(dbgv("memset({0}, {1}, {2})\n", dest, val, size););
  setup_value args[] = { dest, val, size, setup_value::unknown() };
  clone_call(builder, insn, args);
}

void rewrite_setup_tracer::process_store(
  setup_func_builder *builder,
  StoreInst *insn,
  setup_value pointer,
  setup_value data)
{
  LLVM_DEBUG(dbgv("store({0}, {1})\n", data, pointer););
  Value *data_old = insn->getValueOperand();
  Value *data_new = clone_value(builder, data_old, &data);
  Value *pointer_old = insn->getPointerOperand();
  Value *pointer_new = clone_value(builder, pointer_old, &pointer);
  m_builder.CreateStore(data_new, pointer_new, insn->isVolatile());
}

void rewrite_setup_tracer::process_thread_create(
  setup_func_builder *builder,
  CallBase *insn,
  setup_value context_index,
  thread_info *info)
{
  setup_value info_area = builder->eval_operand(info->args().info_area);
  LLVM_DEBUG(dbgv("thread_create({0}, \"{1}\", {2}, {3}, {4})\n",
                  context_index, info->args().name,
                  as_operand(info->args().func),
                  info_area,
                  info->args().info_area_size););
  setup_value args[] = {
    context_index,
    setup_value::unknown(), // name
    setup_value::unknown(), // func
    info_area,
    setup_value::unknown(), // size
  };
  clone_call(builder, insn, args);
}

void rewrite_setup_tracer::process_map_create(
    setup_func_builder *builder,
    CallBase *insn,
    map_index_t map_index,
    map_info *info)
{
  errs() << "ERROR: Unexpected map_create " << *insn << " found in setup "
         << "function.  Implement me in " << __FILE__ << ":" << __LINE__
         << "\nAborting!\n";
  exit(1);
}
void rewrite_setup_tracer::process_context_add_map(
    setup_func_builder *builder,
    CallBase *insn,
    context_index_t ctx_index,
    map_index_t map_index)
{
  errs() << "ERROR: Unexpected context_add_map " << *insn << " found in setup "
         << "function.  Implement me in " << __FILE__ << ":" << __LINE__
         << "\nAborting!\n";
  exit(1);
}

void rewrite_setup_tracer::process_add_plain_kernel(
    setup_func_builder *builder,
    CallBase *insn,
    kernel_index_t kernel_index,
    kernel_info *info)
{
  errs() << "ERROR: Unexpected add_plain_*_kernel " << *insn << " found in setup "
         << "function.  Implement me in " << __FILE__ << ":" << __LINE__
         << "\nAborting!\n";
  exit(1);
}

Value *
rewrite_setup_tracer::clone_channel_value(Type *ty,
                                          channel_index_t val)
{
  auto it = m_channels.find(val);
  if (it == m_channels.end()) {
    report_fatal_errorv("rewrite-setup: Could not find channel {0}.",
                        val);
  }

  assert(ty == it->second->getType());
  return it->second;
}

Value *
rewrite_setup_tracer::clone_context_value(Type *ty,
                                          context_index_t val)
{
  auto it = m_contexts.find(val);
  if (it == m_contexts.end()) {
    report_fatal_errorv("rewrite-setup: Could not find context {0}.",
                        val);
  }

  assert(ty == it->second->getType());
  return it->second;
}

Value *
rewrite_setup_tracer::clone_int_value(Type *ty, setup_int_value_t val)
{
  auto int_ty = cast<IntegerType>(ty);
  auto width = int_ty->getBitWidth();
  return Constant::getIntegerValue(ty, val.zextOrTrunc(width));
}

Value *
rewrite_setup_tracer::adjust_pointer(Type *ty, Value *ptr,
                                     setup_ptr_value_t offset)
{
  // Find the relevant types.
  auto *ptr_ty = dyn_cast<PointerType>(ptr->getType());
  assert(ptr_ty != nullptr);

  // Create an expression which describes the pointer.  The general
  // form is:
  //
  //   bitcast(get_element_ptr(bitcast(ptr)))
  //
  // The get_element_ptr and inner bitcast are omitted if the offset
  // is zero.  The bitcasts are omitted if they have no effect.
  if (offset != 0) {
    unsigned address_space = ptr_ty->getAddressSpace();
    PointerType *i8ptr_ty = m_builder.getInt8PtrTy(address_space);
    if (ptr_ty != i8ptr_ty)
      ptr = m_builder.CreateBitCast(ptr, i8ptr_ty);

    Type *i8_ty = m_builder.getInt8Ty();

    auto &dl = m_module->getDataLayout();
    auto num_index_bits = dl.getIndexTypeSizeInBits(ptr_ty);
    Type *index_ty = m_builder.getIntNTy(num_index_bits);
    Constant *gep_offset = ConstantInt::get(index_ty, offset);

    ptr = m_builder.CreateGEP(i8_ty, ptr, gep_offset);
    ptr_ty = i8ptr_ty;
  }

  // Cast the result if necessary.
  if (ptr_ty != ty)
    ptr = m_builder.CreateBitCast(ptr, ty);

  return ptr;
}

Value *
rewrite_setup_tracer::clone_ptr_value(setup_func_builder *builder,
                                      Type *ty, setup_ptr_value_t val)
{
  setup_func *setup = builder->get_setup_func();
  auto alloc_id = setup->find_alloc_of_ptr(val);
  auto base = setup->get_alloc_base(alloc_id);
  auto offset = val - base;

  auto it = m_allocs.find(alloc_id);
  if (it == m_allocs.end()) {
    report_fatal_errorv("rewrite-setup: Could not find allocation"
                        " for pointer {0}.", val);
  }

  return adjust_pointer(ty, it->second, offset);
}

Value *
rewrite_setup_tracer::clone_value(setup_func_builder *builder,
                                  Value *l_val,
                                  const setup_value *s_val)
{
  // Constants can be used from anywhere.
  if (isa<Constant>(l_val)) {
    return l_val;
  }

  Type *ty = l_val->getType();

  if (s_val->is_channel())
    return clone_channel_value(ty, s_val->get_channel());

  if (s_val->is_context())
    return clone_context_value(ty, s_val->get_context());

  if (s_val->is_int())
    return clone_int_value(ty, s_val->get_int());

  if (s_val->is_ptr())
    return clone_ptr_value(builder, ty, s_val->get_ptr());

  report_fatal_errorv("rewrite-setup: Cannot convert value {0} to"
                      " type {1}.", *s_val, *ty);
}

CallBase *
rewrite_setup_tracer::clone_call(setup_func_builder *builder,
                                 CallBase *orig, const setup_value *args,
                                 std::size_t num_args)
{
  LLVM_DEBUG(dbgv("Clone call {0}.\n", *orig););

  // Prepare the arguments.
  assert(orig->data_operands_size() == num_args);
  SmallVector<Value *, 8> new_args;
  std::size_t idx = 0;
  for (auto it=orig->arg_begin(); it!=orig->arg_end(); ++it, ++idx) {
    const setup_value *arg = &(args[idx]);
    Value *val = clone_value(builder, *it, arg);
    LLVM_DEBUG(dbgv("  Argument {0} value {1} converted to {2}.\n",
                    as_operand(*it), *arg, as_operand(val)););
    new_args.emplace_back(val);
  }

  // Create the new call instruction.
  StringRef call_name = orig->getName();
  LLVM_DEBUG(dbgv("  Name '{0}'\n", call_name););

  FunctionType *fn_ty = orig->getFunctionType();
  Value *callee = orig->getCalledOperand();
  LLVM_DEBUG(dbgv("  Callee {0} with type {1}.\n",
                  as_operand(callee), *fn_ty););
  auto result = m_builder.CreateCall(fn_ty, callee, new_args, call_name);
  return result;
}

///////////////////////////////////////////////////////////////////////////

rewrite_setup_pass::rewrite_setup_pass():
  ModulePass(ID)
{
}

bool rewrite_setup_pass::runOnModule(Module &M)
{
  // Find the setup function.
  Function *func = &(setup_func::get_setup_func(M));
  LLVM_DEBUG(dbgv("Setup function: {0}\n", func->getName()););

  // Create and populate the new basic block.
  BasicBlock *bb = BasicBlock::Create(M.getContext(), "");
  {
    rewrite_setup_tracer tracer(&M, bb);
    setup_func setup(M, &tracer);
  }

  // Remove the terminator instructions so that none of the basic
  // blocks have predecessors.
  for (auto &old_bb: *func) {
    old_bb.getTerminator()->eraseFromParent();
  }

  // Remove the old basic blocks.  Using eraseFromParent here would
  // cause instructions to be deleted while still being referenced by
  // other basic blocks which will be deleted later.  Instead, use
  // DeleteDeadBlock which breaks the references cleanly.
  while (!func->empty()) {
    llvm::DeleteDeadBlock(&(func->front()));
  }

  // Add the new basic block to the function as the only one.
  bb->setName("entry");
  bb->insertInto(func);

  // This pass always makes changes.
  return true;
}

char rewrite_setup_pass::ID = 0;
static RegisterPass<rewrite_setup_pass>
  reg("rewrite-setup", "Rewrite the setup function as a single basic block.",
      false,
      false
  );

///////////////////////////////////////////////////////////////////////////
