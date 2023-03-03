/**************************************************************************\
*//*! \file setup_func_builder.hpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A class for building Nanotube setup function objects.
**   \date  2020-08-18
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_SETUP_FUNC_BUILDER_HPP
#define NANOTUBE_SETUP_FUNC_BUILDER_HPP

#include "llvm_common.h"
#include "llvm_insns.h"
#include "llvm_pass.h"
#include "Intrinsics.h"

///////////////////////////////////////////////////////////////////////////

namespace nanotube
{
class setup_func;
class setup_tracer;
class setup_value;

// Used for building a setup function.
class setup_func_builder
{
private:
  // Construct a setup_func_builder.
  //
  // \param func   The setup function.
  // \param tracer The tracer
  // \param strict Whether the builder should complain about Nanotube API calls
  //               that should not be present after linking the taps and
  //               inlining them.
  setup_func_builder(setup_func &func, setup_tracer *tracer,
                     bool strict = true);

  friend class setup_func;

public:
  setup_func *get_setup_func() { return &m_setup_func; }

  // Find the current value of an operand.
  setup_value eval_operand(Value *op);

private:
  // Build the setup_func.
  void build();

  // Set the value of an instruction.
  void set_insn_val(Instruction *insn, setup_value val);

  // Process an alloca instruction.
  void process_alloca(AllocaInst &insn);

  // Process a binary operator.
  void process_binop(BinaryOperator &insn);

  // Process a bitcast instruction.
  void process_bitcast(BitCastInst &insn);

  // Process a branch instruction.
  Instruction *process_branch(BranchInst &insn);

  // Process a cast instruction.
  void process_cast(CastInst &insn);

  // Process a integer comparison instruction.
  void process_icmp(ICmpInst &insn);

  // Process a GEP instruction.
  void process_gep(GetElementPtrInst &insn);

  // Update the memory contents for a load instruction.
  void process_load(LoadInst &insn);

  // Update the value of a PHI node.
  void process_phi(PHINode &insn);

  // Process a select instruction.
  void process_select(SelectInst &insn);

  // Update the memory contents for a store instruction.
  void process_store(StoreInst &insn);

  // Process a call instruction.
  void process_call(CallBase &insn);

  // Process a call to memcpy.
  void process_memcpy(CallBase &call);

  // Process a call to memset.
  void process_memset(CallBase &call);

  // Process a call to nanotube_malloc.
  void process_malloc(CallBase &call);

  // Process a call to nanotube_context_create.
  void process_context_create(CallBase &call);

  // Process a call to nanotube_context_add_channel.
  void process_context_add_channel(CallBase &call);

  // Process a call to nanotube_channel_create.
  void process_channel_create(CallBase &call);

  // Process a call to nanotube_channel_set_attr.
  void process_channel_set_attr(CallBase &call);

  // Process a call to nanotube_channel_export.
  void process_channel_export(CallBase &call);

  // Process a call to nanotube_thread_create;
  void process_thread_create(CallBase &call);

  void process_add_plain_kernel(CallBase &call);
  void process_map_create(CallBase &call);
  void process_context_add_map(CallBase &call);

  void process_non_strict_call(CallBase &call, Intrinsics::ID iid);

  // The setup_func object.
  setup_func &m_setup_func;

  // The LLVM function.
  Function &m_func;

  // The setup function tracer.
  setup_tracer *m_tracer;

  // The LLVM data layout.
  const DataLayout &m_data_layout;

  // The maximum number of bits required for a pointer.
  unsigned m_max_ptr_bits;

  // The current values of instructions in the setup function.
  std::map<const Instruction *, setup_value> m_insn_values;

  // The basic block which was executing before the current one.
  BasicBlock *m_prev_bb;

  // Complain about calls that should not be present after linking and inlining
  // the taps.
  bool m_strict;
};
} // namespace nanotube

///////////////////////////////////////////////////////////////////////////

#endif // NANOTUBE_SETUP_FUNC_BUILDER_HPP
