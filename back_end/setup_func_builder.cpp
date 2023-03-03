/**************************************************************************\
*//*! \file setup_func_builder.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A class for building Nanotube setup function objects.
**   \date  2020-08-18
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "setup_func_builder.hpp"
#include "setup_func.hpp"

#include <bitset>

#include "utils.h"

#include "llvm/IR/GetElementPtrTypeIterator.h"

using namespace nanotube;

#define DEBUG_TYPE "nanotube-setup"

///////////////////////////////////////////////////////////////////////////

setup_func_builder::setup_func_builder(setup_func &func,
                                       setup_tracer *tracer,
                                       bool strict):
  m_setup_func(func),
  m_func(func.get_function()),
  m_tracer(tracer),
  m_data_layout(m_func.getParent()->getDataLayout()),
  m_strict(strict)
{
  m_max_ptr_bits = m_setup_func.m_max_ptr_bits;
}

setup_value setup_func_builder::eval_operand(Value *op)
{
  auto *insn = dyn_cast<Instruction>(op);
  if (insn != nullptr) {
    auto it = m_insn_values.find(insn);
    if (it == m_insn_values.end()) {
      LLVM_DEBUG(dbgs() << formatv("Eval unknown insn {0}\n",
                                   as_operand(*insn)));
      return setup_value::unknown();
    }
    LLVM_DEBUG(dbgs() << formatv("Eval insn {0} = {1}\n",
                                 as_operand(*insn), it->second));
    return it->second;
  }

  auto *const_int = dyn_cast<ConstantInt>(op);
  if (const_int != nullptr) {
    LLVM_DEBUG(dbgs() << formatv("Eval const: {0}\n", *const_int));
    return setup_value::of_int(const_int->getValue());
  }

  Type *ty = op->getType();
  unsigned int ty_size = m_data_layout.getTypeSizeInBits(ty);
  auto offset = APInt(ty_size, 0);
  while (true) {
    auto *bit_cast = dyn_cast<BitCastOperator>(op);
    if (bit_cast != nullptr) {
      LLVM_DEBUG(dbgs() << formatv("Bitcast: {0}\n", *op));
      op = bit_cast->getOperand(0);
      continue;
    }

    auto *gep = dyn_cast<GEPOperator>(op);
    if (gep != nullptr) {
      bool succ = gep->accumulateConstantOffset(m_data_layout, offset);
      if (!succ) {
        LLVM_DEBUG(dbgs() << formatv("Failed GEP: {0}\n", *op));
        return setup_value();
      }
      op = gep->getPointerOperand();
      continue;
    }

    break;
  }

  auto *var = dyn_cast<GlobalVariable>(op);
  if (var != nullptr) {
    LLVM_DEBUG(dbgs() << formatv("Eval var: {0}\n", *var));
    auto alloc_id = m_setup_func.find_alloc_of_var(var);
    auto ptr = m_setup_func.get_alloc_base(alloc_id);
    return setup_value::of_ptr(ptr+offset.getZExtValue());
  }

  LLVM_DEBUG(dbgs() << formatv("Eval unknown: {0}\n", *op));
  return setup_value();
}

void setup_func_builder::build()
{
  m_prev_bb = nullptr;
  auto &entry_bb = m_func.getEntryBlock();
  Instruction *insn = &(entry_bb.front());
  while (true) {
    if (insn->getOpcode() == Instruction::Ret)
      break;

    LLVM_DEBUG(dbgv("Processing instruction {0}\n", *insn););

    switch(insn->getOpcode()) {
    case Instruction::Alloca:
      process_alloca(cast<AllocaInst>(*insn));
      break;

    case Instruction::Add:
    case Instruction::And:
    case Instruction::AShr:
    case Instruction::LShr:
    case Instruction::Or:
    case Instruction::Shl:
    case Instruction::Sub:
      process_binop(cast<BinaryOperator>(*insn));
      break;

    case Instruction::BitCast:
      process_bitcast(cast<BitCastInst>(*insn));
      break;

    case Instruction::Br:
      insn = process_branch(cast<BranchInst>(*insn));
      continue;

    case Instruction::SExt:
    case Instruction::Trunc:
    case Instruction::ZExt:
      process_cast(cast<CastInst>(*insn));
      break;

    case Instruction::ICmp:
      process_icmp(cast<ICmpInst>(*insn));
      break;

    case Instruction::GetElementPtr:
      process_gep(cast<GetElementPtrInst>(*insn));
      break;

    case Instruction::Load:
      process_load(cast<LoadInst>(*insn));
      break;

    case Instruction::PHI:
      process_phi(cast<PHINode>(*insn));
      break;

    case Instruction::Select:
      process_select(cast<SelectInst>(*insn));
      break;

    case Instruction::Store:
      process_store(cast<StoreInst>(*insn));
      break;

    case Instruction::Call:
      process_call(cast<CallBase>(*insn));
      break;

    default:
      report_fatal_errorv("Cannot handle opcode '{0}' ({1}) in"
                          " Nanotube setup function.",
                          insn->getOpcodeName(), insn->getOpcode());
    }

    insn = insn->getNextNode();
  }
}

void setup_func_builder::set_insn_val(Instruction *insn, setup_value val)
{
  m_insn_values[insn] = val;
  LLVM_DEBUG(dbgs() << formatv("Value of {0} is {1}\n",
                               as_operand(*insn), val));
}

void setup_func_builder::process_alloca(AllocaInst &insn)
{
  // Determine the number of bytes to allocate.
  Type *elem_ty = insn.getAllocatedType();
  auto type_size = m_data_layout.getTypeStoreSize(elem_ty);
  setup_value num_elem = eval_operand(insn.getArraySize());

  if (!num_elem.is_int()) {
    report_fatal_errorv("Allocation has non-constant size: {0}",
                        insn.getArraySize());
  }

  APInt size = APInt(m_max_ptr_bits, type_size);
  size *= num_elem.get_int().zextOrTrunc(m_max_ptr_bits);

  // Allocate the memory.
  auto alloc_id = m_setup_func.do_alloc(size, &insn);
  setup_ptr_value_t ptr = m_setup_func.get_alloc_base(alloc_id);

  LLVM_DEBUG(
    dbgs() << formatv("alloca: size={0}, addr={1}\n", size, ptr);
  );

  // Store the result for later.
  set_insn_val(&insn, setup_value::of_ptr(ptr));

  if (m_tracer != nullptr) {
    m_tracer->process_alloca(this, &insn, num_elem, alloc_id);
  }
}

void setup_func_builder::process_binop(BinaryOperator &insn)
{
  setup_value arg0 = eval_operand(insn.getOperand(0));
  setup_value arg1 = eval_operand(insn.getOperand(1));

  Type *ty = insn.getType();

  if (!ty->isIntegerTy() || !arg0.is_int() || !arg1.is_int()) {
    set_insn_val(&insn, setup_value::unknown());
    return;
  }

  setup_int_value_t arg0_int = arg0.get_int();
  setup_int_value_t arg1_int = arg1.get_int();

  setup_int_value_t result;
  switch (insn.getOpcode()) {
  case Instruction::Add:
    result = arg0_int + arg1_int;
    break;

  case Instruction::And:
    result = arg0_int & arg1_int;
    break;

  case Instruction::AShr:
    result = arg0_int.ashr(arg1_int);
    break;

  case Instruction::LShr:
    result = arg0_int.lshr(arg1_int);
    break;

  case Instruction::Or:
    result = arg0_int | arg1_int;
    break;

  case Instruction::Shl:
    result = arg0_int << arg1_int;
    break;

  case Instruction::Sub:
    result = arg0_int - arg1_int;
    break;

  default:
    assert(false);
  }

  // Store the result for later.
  set_insn_val(&insn, setup_value::of_int(result));
}

void setup_func_builder::process_bitcast(BitCastInst &insn)
{
  // Determine the value of the operand.
  Value *op = insn.getOperand(0);
  setup_value val = eval_operand(op);

  // Store the result for later.
  set_insn_val(&insn, val);
}

Instruction *setup_func_builder::process_branch(BranchInst &insn)
{
  m_prev_bb = insn.getParent();
  if (insn.isUnconditional()) {
    auto *target = insn.getSuccessor(0);
    return &(target->front());
  }

  setup_value cond = eval_operand(insn.getCondition());
  if (!cond.is_int()) {
    report_fatal_errorv("Could not evaluate branch condition {0} of {1}.",
                        cond, insn);
  }

  int succ_num = (cond.get_int().getLimitedValue() ? 0 : 1);
  auto *target = insn.getSuccessor(succ_num);
  return &(target->front());
}

void setup_func_builder::process_cast(CastInst &insn)
{
  setup_value arg0 = eval_operand(insn.getOperand(0));
  Type *ty = insn.getType();

  if (!ty->isIntegerTy() || !arg0.is_int()) {
    set_insn_val(&insn, setup_value::unknown());
    return;
  }

  setup_int_value_t arg0_int = arg0.get_int();
  unsigned width = cast<IntegerType>(ty)->getBitWidth();

  setup_int_value_t result;
  switch (insn.getOpcode()) {
  case Instruction::SExt:
    result = arg0_int.sext(width);
    break;

  case Instruction::Trunc:
    result = arg0_int.trunc(width);
    break;

  case Instruction::ZExt:
    result = arg0_int.zext(width);
    break;

  default:
    assert(false);
  }

  set_insn_val(&insn, setup_value::of_int(result));
}

void setup_func_builder::process_icmp(ICmpInst &insn)
{
  setup_value arg0 = eval_operand(insn.getOperand(0));
  setup_value arg1 = eval_operand(insn.getOperand(1));

  Type *ty = insn.getType();

  if (!ty->isIntegerTy() || !arg0.is_int() || !arg1.is_int()) {
    set_insn_val(&insn, setup_value::unknown());
    return;
  }

  setup_int_value_t arg0_int = arg0.get_int();
  setup_int_value_t arg1_int = arg1.get_int();

  bool result;
  switch (insn.getPredicate()) {
  case ICmpInst::ICMP_EQ:
    result = arg0_int == arg1_int;
    break;

  case ICmpInst::ICMP_NE:
    result = arg0_int != arg1_int;
    break;

  case ICmpInst::ICMP_SGE:
    result = arg0_int.sge(arg1_int);
    break;

  case ICmpInst::ICMP_SGT:
    result = arg0_int.sgt(arg1_int);
    break;

  case ICmpInst::ICMP_SLE:
    result = arg0_int.sle(arg1_int);
    break;

  case ICmpInst::ICMP_SLT:
    result = arg0_int.slt(arg1_int);
    break;

  case ICmpInst::ICMP_UGE:
    result = arg0_int.uge(arg1_int);
    break;

  case ICmpInst::ICMP_UGT:
    result = arg0_int.ugt(arg1_int);
    break;

  case ICmpInst::ICMP_ULE:
    result = arg0_int.ule(arg1_int);
    break;

  case ICmpInst::ICMP_ULT:
    result = arg0_int.ult(arg1_int);
    break;

  default:
    report_fatal_errorv("Unknown integer comparison in setup function: {0}.",
                        insn);
  }

  setup_int_value_t result_int = APInt(1, result);

  // Store the result for later.
  set_insn_val(&insn, setup_value::of_int(result_int));
}

void setup_func_builder::process_gep(GetElementPtrInst &insn)
{
  // Determine the base value.
  Value *base = insn.getPointerOperand();
  setup_value val = eval_operand(base);

  if (!val.is_ptr()) {
    report_fatal_errorv("Cannot perform GEP on a non-pointer: {0}", insn);
  }

  auto ptr = val.get_ptr();
  auto GTE = llvm::gep_type_end(insn);
  for (auto GTI=llvm::gep_type_begin(insn); GTI!=GTE; GTI++) {
    // The index value.
    setup_value op_val = eval_operand(GTI.getOperand());
    if (!op_val.is_int()) {
      report_fatal_errorv("GEP operand '{0}' is not an integer in {1}.",
                          op_val, insn);
    }
    setup_int_value_t op_int_val = op_val.get_int();

    StructType *sty = GTI.getStructTypeOrNull();
    if (sty != nullptr) {
      auto *layout = m_data_layout.getStructLayout(sty);
      ptr += layout->getElementOffset(op_int_val.getZExtValue());

    } else {
      Type *ty = GTI.getIndexedType();
      ptr += (m_data_layout.getTypeStoreSize(ty) *
              op_int_val.getZExtValue());
    }
  }

  // Store the result for later.
  set_insn_val(&insn, setup_value::of_ptr(ptr));
}

void setup_func_builder::process_load(LoadInst &insn)
{
  setup_value pointer = eval_operand(insn.getPointerOperand());

  LLVM_DEBUG(
    dbgs() << formatv("load addr:  {0} is {1}\n",
                      *insn.getPointerOperand(), pointer);
  );

  if (!pointer.is_ptr()) {
    report_fatal_errorv("Cannot handle load through non-pointer '{0}'",
                        *insn.getPointerOperand());
  }

  Type *ty = insn.getType();
  setup_ptr_value_t ptr = pointer.get_ptr();
  setup_value result = m_setup_func.load_value(ty, ptr);

  // Store the result for later.
  set_insn_val(&insn, result);
}

void setup_func_builder::process_phi(PHINode &phi)
{
  Value *val = phi.getIncomingValueForBlock(m_prev_bb);
  assert(val != nullptr);
  setup_value result = eval_operand(val);

  // Store the result for later.
  set_insn_val(&phi, result);
}

void setup_func_builder::process_select(SelectInst &insn)
{
  setup_value cond = eval_operand(insn.getCondition());
  if (!cond.is_int()) {
    report_fatal_errorv("Cannot handle select on non-integer '{0}'",
                        as_operand(insn.getCondition()));
  }

  setup_int_value_t cond_int = cond.get_int();
  Value *selected = ( cond_int.isNullValue()
                      ? insn.getFalseValue()
                      : insn.getTrueValue() );
  setup_value result = eval_operand(selected);

  LLVM_DEBUG(
    dbgv("select {0}, operand {1} has value {2}.\n",
         cond, *selected, result);
  );

  // Store the result for later.
  set_insn_val(&insn, result);
}

void setup_func_builder::process_store(StoreInst &insn)
{
  setup_value pointer = eval_operand(insn.getPointerOperand());
  setup_value value = eval_operand(insn.getValueOperand());

  LLVM_DEBUG(
    dbgs() << formatv("store addr:  {0} is {1}\n      value: {2} is {3}\n",
                      *insn.getPointerOperand(), pointer,
                      *insn.getValueOperand(), value);
  );

  if (!pointer.is_ptr()) {
    report_fatal_errorv("Cannot handle store through non-pointer '{0}'",
                        *insn.getPointerOperand());
  }

  Type *ty = insn.getValueOperand()->getType();
  auto size = m_data_layout.getTypeStoreSize(ty);
  m_setup_func.do_store(pointer.get_ptr(), size, value);

  if (m_tracer != nullptr) {
    m_tracer->process_store(this, &insn, pointer, value);
  }
}

void setup_func_builder::process_call(CallBase &insn)
{
  Intrinsics::ID iid = get_intrinsic(&insn);
  switch (iid) {
  case Intrinsics::llvm_memcpy:
    process_memcpy(insn);
    break;

  case Intrinsics::llvm_memset:
    process_memset(insn);
    break;

  case Intrinsics::malloc:
    process_malloc(insn);
    break;

  case Intrinsics::context_create:
    process_context_create(insn);
    break;

  case Intrinsics::context_add_channel:
    process_context_add_channel(insn);
    break;

  case Intrinsics::channel_create:
    // Make sure the channel structure has been created.
    process_channel_create(insn);
    break;

  case Intrinsics::channel_set_attr:
    process_channel_set_attr(insn);
    break;

  case Intrinsics::channel_export:
    process_channel_export(insn);
    break;

  case Intrinsics::thread_create:
    process_thread_create(insn);
    break;

  case Intrinsics::add_plain_packet_kernel:
    process_add_plain_kernel(insn);
    break;

  case Intrinsics::map_create:
    process_map_create(insn);
    break;

  case Intrinsics::context_add_map:
    process_context_add_map(insn);
    break;

  case Intrinsics::tap_packet_resize_ingress_state_init:
  case Intrinsics::tap_packet_resize_egress_state_init:
  case Intrinsics::tap_map_create:
  case Intrinsics::tap_map_add_client:
  case Intrinsics::tap_map_build:
    if( m_strict ) {
      report_fatal_errorv("Intrinsic {0} \"{1}\" is invalid in strict setup "\
                          "function parser.", insn, intrinsic_to_string(iid));
    }
    process_non_strict_call(insn, iid);
    break;

  case Intrinsics::none:
    report_fatal_errorv("Invalid call to {0} in setup function.",
                        insn);
    break;

  default:
    if (!intrinsic_is_nop(iid))
      report_fatal_errorv("Invalid intrinsic {0} \"{1}\" in setup function.",
                          insn, intrinsic_to_string(iid));
    break;
  }
}

void
setup_func_builder::process_memcpy(CallBase &insn)
{
  auto dest = eval_operand(insn.getArgOperand(0));
  auto src = eval_operand(insn.getArgOperand(1));
  auto size = eval_operand(insn.getArgOperand(2));

  if (!dest.is_ptr()) {
    report_fatal_errorv("Invalid memcpy destination: {0}",
                        insn.getArgOperand(0));
  }

  if (!src.is_ptr()) {
    report_fatal_errorv("Invalid memcpy sourrce: {0}",
                        insn.getArgOperand(0));
  }

  if (!size.is_int()) {
    report_fatal_errorv("Invalid memcpy size: {0}",
                        insn.getArgOperand(2));
  }

  LLVM_DEBUG(dbgv("memcpy: dest={0}, src={1}, size={2}\n",
                  dest, src, size););

  auto size_int = size.get_int().getZExtValue();
  m_setup_func.do_memcpy(dest.get_ptr(), src.get_ptr(), size_int);

  if (m_tracer != nullptr) {
    m_tracer->process_memcpy(this, &insn, dest, src, size);
  }
}

void
setup_func_builder::process_memset(CallBase &insn)
{
  auto dest = eval_operand(insn.getArgOperand(0));
  auto val = eval_operand(insn.getArgOperand(1));
  auto size = eval_operand(insn.getArgOperand(2));

  if (!dest.is_ptr()) {
    report_fatal_errorv("Invalid memset destination: {0}",
                        insn.getArgOperand(0));
  }

  if (!val.is_int()) {
    report_fatal_errorv("Invalid memset value: {0}",
                        insn.getArgOperand(1));
  }

  if (!size.is_int()) {
    report_fatal_errorv("Invalid memset size: {0}",
                        insn.getArgOperand(2));
  }

  uint8_t val_u8 = val.get_int().extractBits(8, 0).getLimitedValue();

  LLVM_DEBUG(dbgv("memset: dest={0}, size={1}, val={2}\n",
                  dest, size, val_u8););

  auto size_int = size.get_int().getZExtValue();
  m_setup_func.do_memset(dest.get_ptr(), size_int, val_u8);

  if (m_tracer != nullptr) {
    m_tracer->process_memset(this, &insn, dest, val, size);
  }
}

void
setup_func_builder::process_malloc(CallBase &insn)
{
  // Allocate the memory and get the base address.
  malloc_args args(&insn);

  auto size_val = eval_operand(args.size);
  if (!size_val.is_int())
    report_fatal_errorv("Size operand {0} is not a constant int in {1}",
                        size_val, insn);
  APInt size = size_val.get_int();

  auto alloc_id = m_setup_func.do_alloc(size, &insn);
  setup_ptr_value_t ptr = m_setup_func.get_alloc_base(alloc_id);

  LLVM_DEBUG(
    dbgs() << formatv("malloc: size={0}, addr={1}\n", size, ptr);
  );

  if (m_tracer != nullptr)
    m_tracer->process_malloc(this, &insn, size_val, alloc_id);

  // Store the result for later.
  set_insn_val(&insn, setup_value::of_ptr(ptr));
}

void
setup_func_builder::process_context_create(CallBase &call)
{
  context_index_t context_index = m_setup_func.contexts().size();

  LLVM_DEBUG(
    dbgs() << "Creating context " << &call
           << " with ID " << context_index << "\n";
  );

  m_setup_func.m_context_infos.emplace_back(context_index);

  if (m_tracer != nullptr) {
    auto &info = m_setup_func.m_context_infos.back();
    m_tracer->process_context_create(this, &call, &info,
                                     context_index);
  }

  // Store the result for later.
  set_insn_val(&call, setup_value::of_context(context_index));
}

void
setup_func_builder::process_context_add_channel(CallBase &call)
{
  context_add_channel_args args{&call};

  auto channel_id_val = eval_operand(args.channel_id);
  if (!channel_id_val.is_int())
    report_fatal_errorv("Invalid channel_id argument in {0}", call);
  auto channel_id = channel_id_val.get_int().getLimitedValue();

  auto channel_index_val = eval_operand(args.channel);
  auto context_index_val = eval_operand(args.context);

  LLVM_DEBUG(
    dbgs() << "Adding channel " << channel_index_val
           << " with ID " << channel_id
           << " and flags " << args.flags
           << " to context " << args.context << "\n";
  );

  if (!context_index_val.is_context())
    report_fatal_errorv("Could not find context for "
                        "nanotube_context_add_channel {0}.", call);
  context_index_t context_index = context_index_val.get_context();

  if (!channel_index_val.is_channel())
    report_fatal_errorv("Could not find channel for "
                        "nanotube_context_add_channel {0}.", call);
  auto channel_index = channel_index_val.get_channel();

  context_info &context = m_setup_func.get_context_info(context_index);
  context.add_channel(m_setup_func, call, channel_id, channel_index,
                      args.flags);

  if (m_tracer != nullptr) {
    m_tracer->process_context_add_channel(this, &call,
                                          context_index_val,
                                          channel_id_val,
                                          channel_index_val,
                                          args.flags);
  }
}

void
setup_func_builder::process_channel_create(CallBase &call)
{
  channel_index_t channel_index = m_setup_func.m_channel_infos.size();

  LLVM_DEBUG(
    dbgs() << "Creating channel " << &call
           << " with index " << channel_index << "\n";
  );

  m_setup_func.m_channel_infos.emplace_back(channel_index, &call, this);

  if (m_tracer != nullptr) {
    auto &info = m_setup_func.m_channel_infos.back();
    m_tracer->process_channel_create(this, &call, &info, channel_index);
  }

  // Store the result for later.
  set_insn_val(&call, setup_value::of_channel(channel_index));
}

void
setup_func_builder::process_channel_set_attr(CallBase &call)
{
  channel_set_attr_args args(&call);
  setup_value channel_index_val = eval_operand(args.channel);

  LLVM_DEBUG(
    dbgs() << "Setting attribute on channel " << channel_index_val
           << " with id " << args.attr_id
           << " and val " << args.attr_val << "\n";
  );

  if (!channel_index_val.is_channel())
    report_fatal_errorv("Could not find channel for "
                        "nanotube_channel_set_attr {0}.", call);
  channel_index_t channel_index = channel_index_val.get_channel();

  channel_info &info = m_setup_func.get_channel_info(channel_index);
  if (args.attr_id == NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES)
    info.set_sideband_size(args.attr_val);
  if (args.attr_id == NANOTUBE_CHANNEL_ATTR_SIDEBAND_SIGNALS)
    info.set_sideband_signals_size(args.attr_val);

  if (m_tracer != nullptr) {
    m_tracer->process_channel_set_attr(this, &call, channel_index_val,
                                       args.attr_id, args.attr_val);
  }
}

void
setup_func_builder::process_channel_export(CallBase &call)
{
  channel_export_args args(&call);
  setup_value channel_index_val = eval_operand(args.channel);

  LLVM_DEBUG(
    dbgs() << "Exporting channel " << channel_index_val
           << " with type " << args.type
           << " and flags " << args.flags << "\n";
  );

  if (!channel_index_val.is_channel())
    report_fatal_errorv("Could not find channel for "
                        "nanotube_channel_export {0}.", call);
  channel_index_t channel_index = channel_index_val.get_channel();

  channel_info &info = m_setup_func.get_channel_info(channel_index);
  info.set_export_type(args.type, args.flags);

  if (m_tracer != nullptr) {
    m_tracer->process_channel_export(this, &call, channel_index_val,
                                     args.type, args.flags);
  }
}

void
setup_func_builder::process_thread_create(CallBase &call)
{
  thread_create_args args(&call);

  LLVM_DEBUG(
    dbgv("Function:     {0}\n", args.func->getName());
    dbgv("\n");
  );

  auto context_index_val = eval_operand(args.context);
  if (!context_index_val.is_context())
    report_fatal_errorv("Failed to find context for thread {0}.",
                        args.name);
  context_index_t context_index = context_index_val.get_context();

  thread_id_t thread_id = m_setup_func.threads().size();
  m_setup_func.m_thread_infos.emplace_back(&call, args, context_index);

  // Set the link from the context to the thread.
  context_info &cinfo = m_setup_func.get_context_info(context_index);
  thread_id_t prev_thread_id = cinfo.get_thread_id();
  if (prev_thread_id != thread_id_none) {
    auto &prev_thread = m_setup_func.get_thread_info(prev_thread_id);
    report_fatal_errorv("The context bound to thread '{0}' was also"
                        " bound to thread '{1}'.",
                        prev_thread.args().name, args.name);
  }
  cinfo.set_thread_id(thread_id);

  if (m_tracer != nullptr) {
    auto &info = m_setup_func.m_thread_infos.back();
    m_tracer->process_thread_create(this, &call, context_index_val,
                                    &info);
  }
}

void
setup_func_builder::process_add_plain_kernel(CallBase &call)
{
  add_plain_kernel_args args(&call);

  LLVM_DEBUG(
      dbgs() << "Adding " << (args.is_capsule ? "capsule" : "packet") << " kernel"
             << " with function " << args.kernel->getName() << " name: "
             << args.name << '\n';
  );

  kernel_index_t kernel_idx = m_setup_func.kernels().size();
  m_setup_func.m_kernel_infos.emplace_back(kernel_idx, &call, this);

  if (m_tracer != nullptr) {
    auto &info = m_setup_func.m_kernel_infos.back();
    m_tracer->process_add_plain_kernel(this, &call, kernel_idx,
                                       &info);
  }
}

void
setup_func_builder::process_map_create(CallBase &call)
{
  map_index_t idx = m_setup_func.maps().size();

  LLVM_DEBUG(dbgs() << "Map create " << call << " idx: " << idx << '\n');
  m_setup_func.m_map_infos.emplace_back(idx, &call, this);

  if (m_tracer != nullptr) {
    auto &info = m_setup_func.m_map_infos.back();
    m_tracer->process_map_create(this, &call, idx, &info);
  }

  // Store the result for later.
  set_insn_val(&call, setup_value::of_map(idx));
}

void
setup_func_builder::process_context_add_map(CallBase &call)
{
  context_add_map_args args(&call);

  auto ctx_idx_val = eval_operand(args.context);
  auto map_idx_val = eval_operand(args.map);

  LLVM_DEBUG(
    dbgs() << "Adding map " << map_idx_val << " (" << *args.map
           << ") to context " << ctx_idx_val << " (" << *args.context
           << ")\n";
  );

  if (!ctx_idx_val.is_context())
    report_fatal_errorv("Could not find context for "
                        "nanotube_context_add_map {0}.", call);
  context_index_t ctx_idx = ctx_idx_val.get_context();

  if (!map_idx_val.is_map())
    report_fatal_errorv("Could not find channel for "
                        "nanotube_context_add_channel {0}.", call);
  map_index_t map_idx = map_idx_val.get_map();

  LLVM_DEBUG(dbgs() << ctx_idx << " " << map_idx << '\n');

  context_info &ctx = m_setup_func.get_context_info(ctx_idx);
  ctx.add_map(m_setup_func, call, map_idx);

  if (m_tracer != nullptr) {
    m_tracer->process_context_add_map(this, &call,
                                      ctx_idx,
                                      map_idx);
  }
}

void
setup_func_builder::process_non_strict_call(CallBase &call, Intrinsics::ID iid)
{
  assert(m_strict == false);
  switch (iid) {
    default: {
      static std::bitset<Intrinsics::ID::end> warned;
      if( warned[iid] )
        break;
      warned[iid] = true;

      errs() << "WARNING: Ignoring call " << call << " to "
             << intrinsic_to_string(iid) << '\n'
             << "If needed, implement in " << __FILE__ << ":" << __LINE__
             << '\n';
    }
  }
}
///////////////////////////////////////////////////////////////////////////
