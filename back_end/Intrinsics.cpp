/**************************************************************************\
*//*! \file Intrinsics.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube intrinsics
**   \date  2019-03-14
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "Intrinsics.h"
#include "llvm_common.h"
#include "llvm_insns.h"
#include "utils.h"

#include "bus_type.hpp"

#include <limits>
#include <map>

using namespace nanotube;
using namespace llvm;

Intrinsics::ID nanotube::get_intrinsic(const Instruction* insn)
{
  assert(insn != NULL);

  const CallBase *call = dyn_cast<CallBase>(insn);
  if (call == NULL)
    return Intrinsics::none;

  Function *callee = call->getCalledFunction();
  if( callee == nullptr )
    return Intrinsics::none;

  return get_intrinsic(callee);
}

Intrinsics::ID nanotube::get_intrinsic(const Function* func)
{
  assert(func != NULL);

  std::string name = func->getName();
  auto it = intrinsic_name_to_id.find(name);
  if (it != intrinsic_name_to_id.end())
    return it->second;

  auto iid = intrinsic_id_from_llvm(func->getIntrinsicID());
  if (iid != Intrinsics::none)
    return iid;

#if 0
  llvm::LibFunc lib_func;
  if (TLI->getLibFunc(func, lib_func)) {
    switch(lib_func) {
    case llvm::LibFunc_memcmp:
      return Intrinsics::llvm_memcmp;

    default:
      break;
    }
  }
#else
  // A workaround since we do not have a TargetLibraryInfo.
  if (name == "memcmp")
    return Intrinsics::llvm_memcmp;
#endif

  return Intrinsics::none;
}

std::string nanotube::intrinsic_to_string(Intrinsics::ID id) {
  auto it = intrinsic_id_to_name.find(id);
  if( it != intrinsic_id_to_name.end() )
    return it->second;
  else
    return "<unknown>";
}

/********** Helpers to examine LLVM IR for intrinsics **********/

static void
check_intrinsic_call(const CallBase &call, unsigned int arg_count)
{
  if (call.arg_size() != arg_count) {
    report_fatal_errorv("Call to {0} has {1} arguments, expected {2}.",
                        call.getCalledFunction()->getName(),
                        call.arg_size(), arg_count);
  }
}

static uint64_t
get_uint_arg(const CallBase &call, unsigned int arg,
             unsigned int num_bits)
{
  assert(num_bits <= 64);

  const Value *val = call.getArgOperand(arg);
  const ConstantInt *c = dyn_cast<ConstantInt>(val);
  if (c == nullptr)
    report_fatal_errorv("Argument {0} to {1} is not a constant integer.",
                        arg, call.getCalledFunction()->getName());

  if (!c->getValue().isIntN(num_bits))
    report_fatal_errorv("Argument {0} to call {1} is more than {2}-bits"
                        " width.", arg, call, num_bits);
  return c->getLimitedValue();
}

template<typename T>
static T
get_uint_arg(const CallBase &call, unsigned int arg)
{
  return get_uint_arg(call, arg, std::numeric_limits<T>::digits);
}

static StringRef
get_string_arg(const CallBase &call, unsigned int arg)
{
  const Value *val = call.getArgOperand(arg);
  const Module *mod = call.getModule();
  const DataLayout &data_layout = mod->getDataLayout();
  Type *pointer_type = val->getType();
  unsigned pointer_bits = data_layout.getIndexTypeSizeInBits(pointer_type);

  APInt offset_ap = APInt(pointer_bits, 0, false);
  const Value *name_base =
    val->stripAndAccumulateInBoundsConstantOffsets(
      data_layout, offset_ap);
  std::size_t offset = offset_ap.getLimitedValue();

  const auto *name_var = dyn_cast<GlobalVariable>(name_base);
  if ( name_var == nullptr )
    report_fatal_errorv("Argument {0} to {1} is not a GlobalVariable.",
                        arg, call);

  if ( (!name_var->hasDefinitiveInitializer()) ||
       (!name_var->isConstant()) )
    report_fatal_errorv("Argument {0} to {1} is not a constant.",
                        arg, call);

  const Constant *name_init = name_var->getInitializer();
  const auto *name_array = dyn_cast<llvm::ConstantDataArray>(name_init);
  if ( name_array == nullptr )
    report_fatal_errorv("Argument {0} to {1} is not an array.",
                        arg, call);

  if ( !name_array->isString() )
    report_fatal_errorv("Argument {0} to {1} is not a string.",
                        arg, call);

  auto s = name_array->getAsString();
  if ( offset >= s.size() )
    report_fatal_errorv("Argument {0} to {1} is out of bounds.",
                        arg, call);

  std::size_t end = s.find(0, offset);
  if ( end == StringRef::npos )
    report_fatal_errorv("Argument {0} to {1} is not NUL"
                        " terminated.", arg, call);

  return s.substr(offset, end-offset);
}

static Function *
get_func_arg(const CallBase &call, unsigned int arg)
{
  Function *result = dyn_cast<Function>(call.getArgOperand(arg));
  if (result == nullptr)
    report_fatal_errorv("Argument {0} to call {1} is not a constant"
                        " function pointer.", arg, call);
  return result;
}

malloc_args::malloc_args(const Instruction *insn)
{
  const CallBase &call = cast<CallBase>(*insn);
  check_intrinsic_call(call, 1);

  size = call.getArgOperand(0);
}

channel_create_args::channel_create_args(const Instruction *insn)
{
  const CallBase &call = cast<CallBase>(*insn);
  check_intrinsic_call(call, 3);

  name = get_string_arg(call, 0);
  elem_size = call.getArgOperand(1);
  num_elem = call.getArgOperand(2);
}

channel_set_attr_args::channel_set_attr_args(const Instruction *insn)
{
  const CallBase &call = cast<CallBase>(*insn);
  check_intrinsic_call(call, 3);
  channel = call.getArgOperand(0);
  attr_id = (nanotube_channel_attr_id_t)get_uint_arg(call, 1, 32);
  attr_val = get_uint_arg(call, 2, 32);
}

channel_export_args::channel_export_args(const Instruction *insn)
{
  const CallBase &call = cast<CallBase>(*insn);
  check_intrinsic_call(call, 3);
  channel = call.getArgOperand(0);
  type = (nanotube_channel_type_t)get_uint_arg(call, 1, 32);
  flags = get_uint_arg(call, 2, 32);
}

channel_read_write_args::channel_read_write_args(const Instruction *insn)
{
  const CallBase &call = cast<CallBase>(*insn);
  check_intrinsic_call(call, 4);

  context = call.getArgOperand(0);
  channel_id = get_uint_arg<nanotube_channel_id_t>(call, 1);
  data = call.getArgOperand(2);
  data_size = get_uint_arg(call, 3, 32);
}

thread_create_args::thread_create_args(const Instruction *insn)
{
  const CallBase &call = cast<CallBase>(*insn);
  check_intrinsic_call(call, 5);

  context = call.getArgOperand(0);
  name = get_string_arg(call, 1);
  func = get_func_arg(call, 2);
  info_area = call.getArgOperand(3);
  info_area_size = get_uint_arg(call, 4, 32);

  if (func->arg_size() != 2)
    report_fatal_errorv("Thread function {0} does not have exactly"
                        " two arguments.", func->getName());
}

add_plain_packet_kernel_args::add_plain_packet_kernel_args(const Instruction *insn)
{
  const CallBase &call = cast<CallBase>(*insn);
  check_intrinsic_call(call, 4);

  name = get_string_arg(call, 0);
  func = get_func_arg(call, 1);
  bus_type = get_uint_arg(call, 2, 32);
  is_capsule = get_uint_arg(call, 3, 32);

  if (func->arg_size() != 2)
    report_fatal_errorv("Kernel function {0} does not have exactly"
                        " two arguments.", func->getName());
}

context_add_channel_args::context_add_channel_args(const Instruction *insn)
{
  const CallBase &call = cast<CallBase>(*insn);
  check_intrinsic_call(call, 4);

  context = call.getArgOperand(0);
  channel_id = call.getArgOperand(1);
  channel = call.getArgOperand(2);
  flags = get_uint_arg<nanotube_channel_flags_t>(call, 3);

  if ( (flags & (NANOTUBE_CHANNEL_READ |
                 NANOTUBE_CHANNEL_WRITE)) != flags )
    report_fatal_errorv("Unsupported flags in call to "
                        "nanotube_context_add_channel: {0}.",
                        call);
}

debug_trace_args::debug_trace_args(const Instruction *insn)
{
  const CallBase &call = cast<CallBase>(*insn);
  check_intrinsic_call(call, 2);
  id = call.getArgOperand(0);
  value = call.getArgOperand(1);
}

packet_read_args::packet_read_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);
  auto  nt_call = nt_api_call(call);

  /* Make sure it is all solid :) */
  check_intrinsic_call(*call, 4);
  assert(nt_call.get_intrinsic() == Intrinsics::packet_read);

  packet   = call->getArgOperand(0);
  data_out = call->getArgOperand(1);
  offset   = call->getArgOperand(2);
  length   = call->getArgOperand(3);
}

packet_write_args::packet_write_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);
  auto  nt_call = nt_api_call(call);

  /* Make sure it is all solid :) */
  auto id = nt_call.get_intrinsic();
  assert((id == Intrinsics::packet_write) ||
         (id == Intrinsics::packet_write_masked));

  if( id == Intrinsics::packet_write ) {
    check_intrinsic_call(*call, 4);
    packet  = call->getArgOperand(0);
    data_in = call->getArgOperand(1);
    mask    = nullptr;
    offset  = call->getArgOperand(2);
    length  = call->getArgOperand(3);
  } else {
    check_intrinsic_call(*call, 5);
    packet  = call->getArgOperand(0);
    data_in = call->getArgOperand(1);
    mask    = call->getArgOperand(2);
    offset  = call->getArgOperand(3);
    length  = call->getArgOperand(4);
  }
}

packet_bounded_length_args::packet_bounded_length_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);
  auto  nt_call = nt_api_call(call);

  /* Make sure it is all solid :) */
  check_intrinsic_call(*call, 2);
  assert(nt_call.get_intrinsic() == Intrinsics::packet_bounded_length);

  packet     = call->getArgOperand(0);
  max_length = call->getArgOperand(1);
}

packet_resize_args::packet_resize_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);

  check_intrinsic_call(*call, 3);
  assert(get_intrinsic(call) == Intrinsics::packet_resize);

  packet = call->getArgOperand(0);
  offset = call->getArgOperand(1);
  adjust = call->getArgOperand(2);
}

packet_resize_ingress_args::packet_resize_ingress_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);

  check_intrinsic_call(*call, 5);
  assert(get_intrinsic(call) == Intrinsics::packet_resize_ingress);

  out_cword = call->getArgOperand(0);
  packet_length_out = call->getArgOperand(1);
  packet    = call->getArgOperand(2);
  offset    = call->getArgOperand(3);
  adjust    = call->getArgOperand(4);
}

packet_resize_egress_args::packet_resize_egress_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);

  check_intrinsic_call(*call, 3);
  assert(get_intrinsic(call) == Intrinsics::packet_resize_egress);

  packet = call->getArgOperand(0);
  cword  = call->getArgOperand(1);
  new_length = call->getArgOperand(2);
}

packet_drop_args::packet_drop_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);

  check_intrinsic_call(*call, 2);
  assert(get_intrinsic(call) == Intrinsics::packet_drop);

  packet = call->getArgOperand(0);
  drop   = call->getArgOperand(1);
}

add_plain_kernel_args::add_plain_kernel_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);
  check_intrinsic_call(*call, 4);
  assert(get_intrinsic(call) == Intrinsics::add_plain_packet_kernel);

  name   = get_string_arg(*call, 0);
  kernel = get_func_arg(*call, 1);
  bus_type = get_uint_arg(*call, 2, 32);
  is_capsule = get_uint_arg(*call, 3, 32);
}

map_create_args::map_create_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);
  check_intrinsic_call(*call, 4);
  assert(get_intrinsic(call) == Intrinsics::map_create);

  id       = get_uint_arg<nanotube_map_id_t>(*call, 0);
  type     = (map_type_t)get_uint_arg(*call, 1, 32);
  key_sz   = call->getArgOperand(2);
  value_sz = call->getArgOperand(3);
}

context_add_map_args::context_add_map_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);
  check_intrinsic_call(*call, 2);
  assert(get_intrinsic(call) == Intrinsics::context_add_map);

  context = call->getArgOperand(0);
  map     = call->getArgOperand(1);
}

map_op_args::map_op_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);
  check_intrinsic_call(*call, 10);
  assert(get_intrinsic(call) == Intrinsics::map_op);

  ctx         = call->getArgOperand(0);
  map         = get_uint_arg<nanotube_map_id_t>(*call, 1);
  type        = call->getArgOperand(2);
  key         = call->getArgOperand(3);
  key_length  = call->getArgOperand(4);
  data_in     = call->getArgOperand(5);
  data_out    = call->getArgOperand(6);
  mask        = call->getArgOperand(7);
  offset      = call->getArgOperand(8);
  data_length = call->getArgOperand(9);
}

map_op_send_args::map_op_send_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);
  check_intrinsic_call(*call, 9);
  assert(get_intrinsic(call) == Intrinsics::map_op_send);

  ctx         = call->getArgOperand(0);
  map         = get_uint_arg<nanotube_map_id_t>(*call, 1);
  type        = call->getArgOperand(2);
  key         = call->getArgOperand(3);
  key_length  = call->getArgOperand(4);
  data_in     = call->getArgOperand(5);
  mask        = call->getArgOperand(6);
  offset      = call->getArgOperand(7);
  data_length = call->getArgOperand(8);
}

map_op_receive_args::map_op_receive_args(Instruction* inst) {
  auto* call = cast<CallInst>(inst);
  check_intrinsic_call(*call, 4);
  assert(get_intrinsic(call) == Intrinsics::map_op_receive);

  ctx         = call->getArgOperand(0);
  map         = get_uint_arg<nanotube_map_id_t>(*call, 1);
  data_out    = call->getArgOperand(2);
  data_length = call->getArgOperand(3);
}

int nanotube::get_size_arg(const CallBase* call, unsigned idx, bool* bits) {
  auto intr     = get_intrinsic(call);
  int  size_arg = -1;
  *bits         = false;

  switch( intr ) {
    case Intrinsics::llvm_memset:
      if( idx == 0 )
        size_arg = 2;
      break;

    case Intrinsics::llvm_memcpy:
    case Intrinsics::llvm_memcmp:
      if( idx == 0 || idx == 1 )
        size_arg = 2;
      break;

    case Intrinsics::channel_try_read:
    case Intrinsics::channel_write:
      if( idx == 2 )
        size_arg = 3;
      break;

    case Intrinsics::packet_read:
    case Intrinsics::packet_write:
      if( idx == 1 )                    /* data_in / data_out */
        size_arg = 3;                   /* length */
      break;

    case Intrinsics::packet_write_masked:
      if( (idx == 1) || (idx == 2) )    /* data_in / mask */
        size_arg = 4;                   /* length */
      *bits = (idx == 2);               /* mask */
      break;

    case Intrinsics::map_op:
      if( (idx == 3) )                  /* key */
        size_arg = 4;                   /* key_length */
      if( (idx == 5) || (idx == 6) ||   /* data_in / data_out */
          (idx == 7) )                  /* mask */
        size_arg = 9;
      *bits = (idx == 7);               /* mask */
      break;

    case Intrinsics::map_lookup:
    case Intrinsics::map_remove:
      if( (idx == 2) )                  /* key */
        size_arg = 3;                   /* key_length */
      break;

    case Intrinsics::map_read:
    case Intrinsics::map_write:
      if( (idx == 2) )                  /* key */
        size_arg = 3;                   /* key_length */
      if( (idx == 4) )                  /* data_in / data_out */
        size_arg = 6;                   /* data_length */
      break;

    case Intrinsics::map_insert:
    case Intrinsics::map_update:
      if( (idx == 2) )                  /* key */
        size_arg = 3;                   /* key_length */
      if( (idx == 4) || (idx == 5) )    /* data_in / enables */
        size_arg = 7;
      *bits = (idx == 5);               /* enables */
      break;

    default:
      size_arg = -1;
  };
  return size_arg;
}

MemoryLocation
nanotube::get_memory_location(const CallBase* call, unsigned idx,
                              const TargetLibraryInfo& tli) {
  auto loc  = MemoryLocation::getForArgument(call, idx, tli);
  if( loc.Size.hasValue() && loc.Size.isPrecise() )
    return loc;
  int  size_arg = -1;
  bool bits = false;

  size_arg = get_size_arg(call, idx, &bits);

  if( size_arg == -1 )
    return loc;

  Value* v = call->getOperand(size_arg);
  unsigned max = 0;
  bool precise = false;
  if( get_max_value(v, &max, &precise) ) {
    max = bits ? ((max + 7) / 8) : max;
    if( precise )
      loc.Size = LocationSize::precise(max);
    else
      loc.Size = LocationSize::upperBound(max);
  }
  return loc;
}

bool
nanotube::get_max_value(Value* v, unsigned* max_out, bool* precise) {
  *precise = true;
  ConstantInt* ci = dyn_cast<ConstantInt>(v);
  if( ci != nullptr ) {
    *max_out = ci->getZExtValue();
    return true;
  }

  PHINode* phi = dyn_cast<PHINode>(v);
  if( phi != nullptr ) {
    unsigned max = 0;
    bool empty = true;

    for( auto& use : phi->incoming_values() ) {
      if( isa<UndefValue>(use.get()) )
        continue;
      ci = dyn_cast<ConstantInt>(use.get());
      if( ci == nullptr )
        return false;
      if( empty ) {
        max   = ci->getZExtValue();
        empty = false;
      } else if( ci->getZExtValue() > max ) {
        max = ci->getZExtValue();
        *precise = false;
      }
    }
    *max_out = max;
    return true;
  }

  return false;
}
/********** Helpers for reporting enum values **********/

const std::string &
nanotube::get_enum_name(nanotube_channel_type_t t)
{
  static const std::string
    none_name = "NANOTUBE_CHANNEL_TYPE_NONE",
    sp_name = "NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET",
    sh_name = "NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET",
    x3rx_name = "NANOTUBE_CHANNEL_TYPE_X3RX_PACKET",
    null_name = "";

  switch(t) {
  case NANOTUBE_CHANNEL_TYPE_NONE:           return none_name;
  case NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET:  return sp_name;
  case NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET: return sh_name;
  case NANOTUBE_CHANNEL_TYPE_X3RX_PACKET:    return x3rx_name;
  default: return null_name;
  }
}

/********** Helpers to create and parse LLVM IR for intrinsics **********/

namespace nanotube {
static void set_nt_attrs(Constant* co) {
  auto* f = cast<Function>(co);
  auto id = get_intrinsic(f);
  if( id == Intrinsics::none )
    return;
  if( (id < Intrinsics::packet_read) ||
      (id > Intrinsics::get_time_ns) )
    return;

  auto fmrb = get_nt_fmrb(id);

  switch( fmrb ) {
    case FMRB_OnlyReadsArgumentPointees:
    case FMRB_OnlyAccessesArgumentPointees:
      f->addFnAttr(Attribute::ArgMemOnly);
      break;

    case FMRB_OnlyAccessesInaccessibleOrArgMem:
      f->addFnAttr(Attribute::InaccessibleMemOrArgMemOnly);
      break;

    case FMRB_OnlyAccessesInaccessibleMem:
      f->addFnAttr(Attribute::InaccessibleMemOnly);
      break;

    default:
      /* Do nothing */
      ;
  }
}

template <typename...Ts>
static Constant*
get_or_insert_function(Module& m, StringRef name, Type* ret, Ts...args) {
  bool fresh = m.getNamedValue(name) != nullptr;
  auto* f = m.getOrInsertFunction(name, ret, args...);
  if( fresh )
    set_nt_attrs(f);
  return f;
}
static Constant*
get_or_insert_function(Module& m, StringRef name, FunctionType* ft) {
  bool fresh = m.getNamedValue(name) != nullptr;
  auto* f = m.getOrInsertFunction(name, ft);
  if( fresh )
    set_nt_attrs(f);
  return f;
}
};

/********** Map functions **********/
namespace nanotube {
Constant* create_nt_map_create(Module& m) {
  LLVMContext& c = m.getContext();
  auto* ret_ty   = get_nt_map_type(m)->getPointerTo();
  Type *argtys[] = { get_nt_map_id_type(m), Type::getInt32Ty(c),
                     Type::getInt64Ty(c), Type::getInt64Ty(c)};
  auto* ty = FunctionType::get(ret_ty, argtys, false);
  return m.getOrInsertFunction("nanotube_map_create", ty);
}
Constant* create_nt_map_lookup(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** uint8_t* nanotube_map_lookup(nanotube_context_t* ctx,
  **                              nanotube_map_id_t map, const uint8_t* key,
  **                              size_t key_length, size_t data_length);
  **/
  return get_or_insert_function(m, "nanotube_map_lookup",
                                Type::getInt8PtrTy(c),
                                get_nt_context_type(m)->getPointerTo(),
                                get_nt_map_id_type(m),
                                Type::getInt8PtrTy(c),
                                Type::getInt64Ty(c),
                                Type::getInt64Ty(c));
}
Constant* create_nt_map_op(Module& m) {
  LLVMContext& c = m.getContext();
  /*
   * size_t nanotube_map_op(nanotube_context_t* ctx, nanotube_map_id_t map,
   *                        enum map_access_t type, const uint8_t* key,
   *                        size_t key_length, const uint8_t* data_in,
   *                        uint8_t* data_out, const uint8_t* mask,
   *                        size_t offset, size_t data_length);
   */
  return get_or_insert_function(m, "nanotube_map_op",
                                Type::getInt64Ty(c),
                                get_nt_context_type(m)->getPointerTo(),
                                get_nt_map_id_type(m),
                                Type::getInt32Ty(c),
                                Type::getInt8PtrTy(c),
                                Type::getInt64Ty(c),
                                Type::getInt8PtrTy(c),
                                Type::getInt8PtrTy(c),
                                Type::getInt8PtrTy(c),
                                Type::getInt64Ty(c),
                                Type::getInt64Ty(c));
}
FunctionType* get_nt_map_op_send_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /* NOTE: This is not an externally visible Nanotube API function, but the
   * protoype looks like this:
   * void nanotube_map_op_send(nanotube_context_t* ctx,
   *                           nanotube_map_id_t * map,
   *                           enum map_access_t type, const uint8_t* key,
   *                           size_t key_length, const uint8_t* data_in,
   *                           const uint8_t* mask, size_t offset,
   *                           size_t data_length);
   */
  std::array<Type*, 9> args = {
    get_nt_context_type(m)->getPointerTo(),
    get_nt_map_id_type(m),
    Type::getInt32Ty(c),
    Type::getInt8PtrTy(c),
    Type::getInt64Ty(c),
    Type::getInt8PtrTy(c),
    Type::getInt8PtrTy(c),
    Type::getInt64Ty(c),
    Type::getInt64Ty(c),
  };
  return FunctionType::get(Type::getVoidTy(c), args, false);
}
Constant* create_nt_map_op_send(Module& m) {
  return get_or_insert_function(m, "nanotube_map_op_send",
                                get_nt_map_op_send_ty(m));
}

FunctionType* get_nt_map_op_receive_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /* NOTE: This is not an externally visible Nanotube API function, but the
   * protoype looks like this:
   * size_t nanotube_map_op_receive(nanotube_context_t* ctx,
   *                              nanotube_map_id_t map,
   *                              uint8_t* data_out,
   *                              size_t data_length);
   */
  std::array<Type*, 4> args = {
    get_nt_context_type(m)->getPointerTo(),
    get_nt_map_id_type(m),
    Type::getInt8PtrTy(c),
    Type::getInt64Ty(c),
  };
  return FunctionType::get(Type::getInt64Ty(c), args, false);
}
Constant* create_nt_map_op_receive(Module& m) {
  return get_or_insert_function(m, "nanotube_map_op_receive",
                                get_nt_map_op_receive_ty(m));
}

FunctionType* get_nt_map_process_capsule_ty(Module& m)
{
  LLVMContext& c = m.getContext();
  /* void nanotube_map_process_capsule(nanotube_context_t *ctxt,
   *                                   nanotube_map_id_t map_id,
   *                                   uint8_t *capsule,
   *                                   size_t key_len,
   *                                   size_t value_len);
   */
  Type *args[] = {
    get_nt_context_type(m)->getPointerTo(),
    get_nt_map_id_type(m),
    Type::getInt8PtrTy(c),
    Type::getInt64Ty(c),
    Type::getInt64Ty(c),
  };
  return FunctionType::get(Type::getVoidTy(c), args, false);
}

Constant* create_nt_map_process_capsule(Module& m)
{
  return get_or_insert_function(m, "nanotube_map_process_capsule",
                                get_nt_map_process_capsule_ty(m));
}

Constant* create_nt_map_read(Module& m) {
  LLVMContext& c = m.getContext();
  /**
   ** size_t nanotube_map_read(nanotube_context_t* ctx, nanotube_map_id_t map,
   **                          const uint8_t* key, size_t key_length,
   **                          uint8_t* data_out, size_t offset,
   **                          size_t data_length);
   **/
  return get_or_insert_function(m, "nanotube_map_read",
                               Type::getInt64Ty(c),
                               get_nt_context_type(m)->getPointerTo(),
                               get_nt_map_id_type(m),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt64Ty(c));
}
Constant* create_nt_map_write(Module& m) {
  LLVMContext& c = m.getContext();
  /**
   ** size_t nanotube_map_write(nanotube_context_t* ctx, nanotube_map_id_t map,
   **                          const uint8_t* key, size_t key_length,
   **                          const uint8_t* data_in, size_t offset,
   **                          size_t data_length);
   **/
  return get_or_insert_function(m, "nanotube_map_write",
                               Type::getInt64Ty(c),
                               get_nt_context_type(m)->getPointerTo(),
                               get_nt_map_id_type(m),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt64Ty(c));
}
Constant* create_map_op_adapter(Module& m) {
  LLVMContext& c = m.getContext();
  /*
   * int32_t map_op_adapter(nanotube_context_t* ctx, nanotube_map_id_t map_id,
   *                        enum map_access_t type, const uint8_t* key,
   *                        size_t key_length, const uint8_t* data_in,
   *                        uint8_t* data_out, const uint8_t* mask,
   *                        size_t offset, size_t data_length);
   */
  return m.getOrInsertFunction("map_op_adapter",
                               Type::getInt32Ty(c),
                               get_nt_context_type(m)->getPointerTo(),
                               get_nt_map_id_type(m),
                               Type::getInt32Ty(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt64Ty(c));
}
};

/********** Packet accesses **********/
namespace nanotube {
Constant* create_nt_packet_data(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** uint8_t* nanotube_packet_data(nanotube_packet_t* p);
  **/
  return get_or_insert_function(m, "nanotube_packet_data",
                                Type::getInt8PtrTy(c),
                                get_nt_packet_type(m)->getPointerTo());
}
Constant* create_nt_packet_end(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** uint8_t* nanotube_packet_end(nanotube_packet_t* p);
  **/
  return get_or_insert_function(m, "nanotube_packet_end",
                                Type::getInt8PtrTy(c),
                                get_nt_packet_type(m)->getPointerTo());
}
Constant* create_nt_packet_meta(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** uint8_t* nanotube_packet_meta(nanotube_packet_t* p);
  **/
  return get_or_insert_function(m, "nanotube_packet_meta",
                                Type::getInt8PtrTy(c),
                                get_nt_packet_type(m)->getPointerTo());
}
Constant* create_nt_packet_bounded_length(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** size_t nanotube_packet_bounded_length(nanotube_packet_t* packet,
  **                                       size_t max_length);
  **/
  return get_or_insert_function(m, "nanotube_packet_bounded_length",
                                Type::getInt64Ty(c),
                                get_nt_packet_type(m)->getPointerTo(),
                                Type::getInt64Ty(c));
}
Constant* create_nt_packet_resize(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** int32
  ** nanotube_packet_resize(nanotube_packet_t* p, size_t offset, int32 adjust);
  **/
  return get_or_insert_function(m, "nanotube_packet_resize",
                                Type::getInt32Ty(c),
                                get_nt_packet_type(m)->getPointerTo(),
                                Type::getInt64Ty(c),
                                Type::getInt32Ty(c));
}
FunctionType* get_nt_packet_resize_ingress_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_packet_resize_ingress(
  **   nanotube_tap_packet_resize_cword_t* out_cword,
  **   nanotube_tap_offset_t *packet_length_out,
  **   nanotube_packet_t* p,
  **   size_t offset, int32 adjust);
  **/
  std::array<Type*, 5> args = {
    get_nt_tap_packet_resize_cword_ty(m)->getPointerTo(),
    get_nt_tap_offset_ty(m)->getPointerTo(),
    get_nt_packet_type(m)->getPointerTo(),
    Type::getInt64Ty(c),
    Type::getInt32Ty(c)};
  return FunctionType::get(Type::getVoidTy(c), args, false);
}
Constant* create_nt_packet_resize_ingress(Module& m) {
  return get_or_insert_function(m, "nanotube_packet_resize_ingress",
                                get_nt_packet_resize_ingress_ty(m));
}
FunctionType* get_nt_packet_resize_egress_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** int32 nanotube_packet_resize_egress(nanotube_packet_t* p,
  **         nanotube_tap_packet_resize_cword_t* cword,
  **         nanotube_tap_offset_t new_length);
  **/
  std::array<Type*, 3> args = {
    get_nt_packet_type(m)->getPointerTo(),
    get_nt_tap_packet_resize_cword_ty(m)->getPointerTo(),
    get_nt_tap_offset_ty(m)};
  return FunctionType::get(Type::getInt32Ty(c), args, false);
}
Constant* create_nt_packet_resize_egress(Module& m) {
  return get_or_insert_function(m, "nanotube_packet_resize_egress",
                                get_nt_packet_resize_egress_ty(m));
}
FunctionType* get_nt_packet_drop_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_packet_drop(nanotube_packet_t* p,
  **         int32 drop);
  **/
  std::array<Type*, 2> args = {
    get_nt_packet_type(m)->getPointerTo(),
    Type::getInt32Ty(c)};
  return FunctionType::get(Type::getVoidTy(c), args, false);
}
Constant* create_nt_packet_drop(Module& m) {
  return get_or_insert_function(m, "nanotube_packet_drop",
                                get_nt_packet_drop_ty(m));
}
Constant* create_nt_packet_read(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** size_t nanotube_packet_read(nanotube_packet_t* packet, uint8_t* data_out,
  **                             size_t offset, size_t length);
  **/
  return get_or_insert_function(m, "nanotube_packet_read",
                               Type::getInt64Ty(c),
                               get_nt_packet_type(m)->getPointerTo(),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt64Ty(c));
}
Constant* create_nt_packet_write(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** size_t nanotube_packet_write(nanotube_packet_t* packet, uint8_t* data_in,
  **                              size_t offset, size_t length);
  **/
  return get_or_insert_function(m, "nanotube_packet_write",
                               Type::getInt64Ty(c),
                               get_nt_packet_type(m)->getPointerTo(),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt64Ty(c));
}
Constant* create_nt_packet_write_masked(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** size_t nanotube_packet_write_masked(nanotube_packet_t* packet,
  **                                     const uint8_t* data_in, uint8_t* mask,
  **                                     size_t offset, size_t length);
  **/
  return get_or_insert_function(m, "nanotube_packet_write_masked",
                               Type::getInt64Ty(c),
                               get_nt_packet_type(m)->getPointerTo(),
                               Type::getInt8PtrTy(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt64Ty(c));
}
Constant* create_nt_merge_data_mask(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void
  ** nanotube_merge_data_mask(uint8_t* inout_data, uint8_t* inout_mask,
  **                          const uint8_t* in_data, const uint8_t* in_mask,
  **                          size_t offset, size_t data_length);
  **/
  return m.getOrInsertFunction("nanotube_merge_data_mask",
                               Type::getVoidTy(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c),
                               Type::getInt64Ty(c));
}

Constant* create_bus_packet_set_port(Module& m, nanotube_bus_id_t bus_type)
{
  LLVMContext& c = m.getContext();
  /**
  ** void
  ** nanotube_packet_set_port_BUS(nanotube_packet_t* packet,
  **                              nanotube_packet_port_t port);
  **/
  auto fname = ( std::string("nanotube_packet_set_port") +
                 get_bus_suffix(bus_type) );
  return m.getOrInsertFunction(fname,
                               Type::getVoidTy(c),
                               get_nt_packet_type(m)->getPointerTo(),
                               Type::getInt8Ty(c));
}

Constant* create_packet_adjust_head_adapter(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** int32 packet_adjust_head_adapter(nanotube_packet_t* p, int32 offset);
  **/
  return m.getOrInsertFunction("packet_adjust_head_adapter",
                               Type::getInt32Ty(c),
                               get_nt_packet_type(m)->getPointerTo(),
                               Type::getInt32Ty(c));
}

Constant* create_packet_adjust_meta_adapter(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** int32 packet_adjust_meta_adapter(nanotube_packet_t* p, int32 offset);
  **/
  return m.getOrInsertFunction("packet_adjust_meta_adapter",
                               Type::getInt32Ty(c),
                               get_nt_packet_type(m)->getPointerTo(),
                               Type::getInt32Ty(c));
}

FunctionType* get_capsule_classify_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
   ** nanotube_capsule_class nanotube_capsule_classify(
   **   nanotube_packet_t* p);
   **/
  auto* res_ty   = Type::getInt32Ty(c);
  Type* arg_ty[] = {
    get_nt_packet_type(m)->getPointerTo(),
  };
  return FunctionType::get(res_ty, arg_ty, false);
}

Constant* create_capsule_classify(Module& m, nanotube_bus_id_t bus_type) {
  auto fname = ( std::string("nanotube_capsule_classify") +
                 get_bus_suffix(bus_type) );
  return get_or_insert_function(m, fname, get_capsule_classify_ty(m));
}

};

/********** Channel accesses **********/
namespace nanotube {
Type* get_nt_channel_id_ty(Module& m) {
  return Type::getInt32Ty(m.getContext());
}
unsigned get_nt_channel_type_bits()
{
  return 32;
}
Type* get_nt_channel_type_ty(Module& m) {
  return Type::getInt32Ty(m.getContext());
}
unsigned get_nt_channel_flags_bits()
{
  return 32;
}
Type* get_nt_channel_flags_ty(Module& m) {
  return Type::getInt32Ty(m.getContext());
}
unsigned get_nt_channel_attr_id_bits()
{
  return 32;
}
Type* get_nt_channel_attr_id_ty(Module& m) {
  return Type::getInt32Ty(m.getContext());
}
unsigned get_nt_channel_attr_val_bits()
{
  return 32;
}
Type* get_nt_channel_attr_val_ty(Module& m) {
  return Type::getInt32Ty(m.getContext());
}
StructType* get_nt_channel_ty(Module& m) {
  /* struct nanotube_channel is opaque! */
  const char name[] = "struct.nanotube_channel";
  auto* ty = m.getTypeByName(name);
  if( ty != nullptr )
    return ty;

  ty = StructType::create(m.getContext(), name);
  return ty;
}
Constant* create_nt_channel_read(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_channel_read(nanotube_context_t* context,
  **                            nanotube_channel_id_t channel_id,
  **                            void* data,
  **                            size_t data_size);
  **/
  return m.getOrInsertFunction("nanotube_channel_read",
                               Type::getVoidTy(c),
                               get_nt_context_type(m)->getPointerTo(),
                               get_nt_channel_id_ty(m),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c));
}
Constant* create_nt_channel_try_read(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** int32 nanotube_channel_try_read(nanotube_context_t* context,
  **                                 nanotube_channel_id_t channel_id,
  **                                 void* data,
  **                                 size_t data_size);
  **/
  return m.getOrInsertFunction("nanotube_channel_try_read",
                               Type::getInt32Ty(c),
                               get_nt_context_type(m)->getPointerTo(),
                               get_nt_channel_id_ty(m),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c));
}
Constant* create_nt_channel_write(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_channel_write(nanotube_context_t* context,
  **                             nanotube_channel_id_t channel_id,
  **                             void* data,
  **                             size_t data_size);
  **/
  return m.getOrInsertFunction("nanotube_channel_write",
                               Type::getVoidTy(c),
                               get_nt_context_type(m)->getPointerTo(),
                               get_nt_channel_id_ty(m),
                               Type::getInt8PtrTy(c),
                               Type::getInt64Ty(c));
}
FunctionType* get_nt_channel_create_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** nanotube_channel_t* nanotube_channel_create(const char* name,
  **                                             size_t elem_size,
  **                                             size_t num_elem);
  */
  std::array<Type*, 3> args = {
    Type::getInt8PtrTy(c),
    Type::getInt64Ty(c),
    Type::getInt64Ty(c),
  };
  return
    FunctionType::get(get_nt_channel_ty(m)->getPointerTo(), args, false);
}
Constant* create_nt_channel_create(Module& m) {
  return m.getOrInsertFunction("nanotube_channel_create",
                               get_nt_channel_create_ty(m));
}
FunctionType* get_nt_channel_set_attr_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** int
  ** nanotube_channel_set_attr(nanotube_channel_t *channel,
  **                           nanotube_channel_attr_id_t attr_id,
  **                           int32_t attr_val)
  */
  std::array<Type*, 3> args = {
    get_nt_channel_ty(m)->getPointerTo(),
    get_nt_channel_attr_id_ty(m),
    get_nt_channel_attr_val_ty(m),
  };
  return
    FunctionType::get(Type::getInt32Ty(c), args, false);
}
Constant* create_nt_channel_set_attr(Module& m) {
  return m.getOrInsertFunction("nanotube_channel_set_attr",
                               get_nt_channel_set_attr_ty(m));
}
FunctionType* get_nt_channel_export_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void
  ** nanotube_channel_export(nanotube_channel_t *channel,
  **                         nanotube_channel_type_t type,
  **                         nanotube_channel_flags_t flags);
  */
  std::array<Type*, 3> args = {
    get_nt_channel_ty(m)->getPointerTo(),
    get_nt_channel_type_ty(m),
    get_nt_channel_flags_ty(m),
  };
  return
    FunctionType::get(Type::getVoidTy(c), args, false);
}
Constant* create_nt_channel_export(Module& m) {
  return m.getOrInsertFunction("nanotube_channel_export",
                               get_nt_channel_export_ty(m));
}
};

/********** Thread functions **********/
namespace nanotube {
Constant* create_nt_thread_wait(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_thread_wait();
  **/
  return m.getOrInsertFunction("nanotube_thread_wait",
                               Type::getVoidTy(c));
}
FunctionType* get_nt_thread_func_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /*
  ** typedef void nanotube_thread_func_t(nanotube_context_t* context,
  **                                     void* data);
  */
  std::array<Type*, 2> args = {
    get_nt_context_type(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
  };
  return
    FunctionType::get(Type::getVoidTy(c), args, false);
}
FunctionType* get_nt_thread_create_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_thread_create(nanotube_context_t* context,
  **                             const char* name,
  **                             nanotube_thread_func_t* func,
  **                             const void* data,
  **                             size_t data_size);
  */
  std::array<Type*, 5> args = {
    get_nt_context_type(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
    get_nt_thread_func_ty(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
    Type::getInt64Ty(c),
  };
  return
    FunctionType::get(Type::getVoidTy(c), args, false);
}
Constant* create_nt_thread_create(Module& m) {
  return m.getOrInsertFunction("nanotube_thread_create",
                               get_nt_thread_create_ty(m));
}
};

/********** Misc accesses **********/
namespace nanotube {
Constant* create_nt_get_time_ns(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** uint8_t* nanotube_packet_end(nanotube_packet_t* p);
  **/
  return m.getOrInsertFunction("nanotube_get_time_ns",
                               Type::getInt64Ty(c));
}
Constant* create_packet_handle_xdp_result(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** int packet_handle_xdp_result(nanotube_packet_t* packet, int xdp_ret);
  **/
  return m.getOrInsertFunction("packet_handle_xdp_result",
                               Type::getInt32Ty(c),
                               get_nt_packet_type(m)->getPointerTo(),
                               Type::getInt32Ty(c));
}
};


/********** Nanotube Taps **********/
namespace nanotube {
StructType* get_nt_tap_packet_read_req_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_read_req";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*,3> elements = {
    IntegerType::getInt8Ty(c),    /* valid */
    IntegerType::getInt16Ty(c),   /* read_offset */
    IntegerType::getInt16Ty(c),   /* read_length */
  };
  ty = StructType::create(c, elements, name);
  return ty;
}

StructType* get_nt_tap_packet_read_resp_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_read_resp";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*,2> elements = {
    IntegerType::getInt8Ty(c),    /* valid */
    IntegerType::getInt16Ty(c),   /* result_length */
  };
  return StructType::create(c, elements, name);
}

StructType* get_nt_tap_packet_read_state_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_read_state";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*, 6>  elements = {
    IntegerType::getInt16Ty(c),   /* packet_length */
    IntegerType::getInt16Ty(c),   /* packet_offset */
    IntegerType::getInt16Ty(c),   /* rotate_amount */
    IntegerType::getInt16Ty(c),   /* result_offset */
    IntegerType::getInt8Ty(c),    /* done */
    IntegerType::getInt8Ty(c),    /* data_eop_seen */
  };
  return StructType::create(c, elements, name);
}

FunctionType* get_tap_packet_read_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_tap_packet_read(
  **   uint16_t result_buffer_length,
  **   uint8_t result_buffer_index_bits,
  **   struct nanotube_tap_packet_read_resp *resp_out,
  **   uint8_t *result_buffer_inout,
  **   struct nanotube_tap_packet_read_state *state_inout,
  **   const uint8_t *packet_word_in,
  **   const struct nanotube_tap_packet_read_req *req_in);
  **/
  auto* res_ty   = Type::getVoidTy(c);
  Type* arg_ty[] = {
    Type::getInt16Ty(c),
    Type::getInt8Ty(c),
    get_nt_tap_packet_read_resp_ty(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
    get_nt_tap_packet_read_state_ty(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
    get_nt_tap_packet_read_req_ty(m)->getPointerTo()
  };
  return FunctionType::get(res_ty, arg_ty, false);
}

Constant* create_tap_packet_read(Module& m, nanotube_bus_id_t bus_type) {
  auto fname = std::string("nanotube_tap_packet_read") +
                 get_bus_suffix(bus_type);
  return get_or_insert_function(m, fname, get_tap_packet_read_ty(m));
}

StructType* get_nt_tap_packet_write_req_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_write_req";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*,3> elements = {
    IntegerType::getInt8Ty(c),    /* valid */
    IntegerType::getInt16Ty(c),   /* write_offset */
    IntegerType::getInt16Ty(c),   /* write_length */
  };
  ty = StructType::create(c, elements, name);
  return ty;
}

StructType* get_nt_tap_packet_write_state_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_write_state";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*, 6>  elements = {
    IntegerType::getInt16Ty(c),   /* packet_length */
    IntegerType::getInt16Ty(c),   /* packet_offset */
    IntegerType::getInt16Ty(c),   /* rotate_amount */
    IntegerType::getInt16Ty(c),   /* result_offset */
    IntegerType::getInt8Ty(c),    /* done */
    IntegerType::getInt8Ty(c),    /* data_eop_seen */
  };
  return StructType::create(c, elements, name);
}

FunctionType* get_tap_packet_write_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_tap_packet_write(
  **   uint16_t request_buffer_length,
  **   uint8_t request_buffer_index_bits,
  **   uint8_t *packet_word_out,
  **   struct nanotube_tap_packet_write_state *state_inout,
  **   const uint8_t *packet_word_in,
  **   const struct nanotube_tap_packet_write_req *req_in,
  **   const uint8_t *request_bytes_in,
  **   const uint8_t *request_mask_in);
  **/
  auto* res_ty   = Type::getVoidTy(c);
  Type* arg_ty[] = {
    Type::getInt16Ty(c),
    Type::getInt8Ty(c),
    Type::getInt8PtrTy(c),
    get_nt_tap_packet_write_state_ty(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
    get_nt_tap_packet_write_req_ty(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
    Type::getInt8PtrTy(c),
  };
  return FunctionType::get(res_ty, arg_ty, false);
}

Constant* create_tap_packet_write(Module& m, nanotube_bus_id_t bus_type) {
  auto fname = std::string("nanotube_tap_packet_write") +
                 get_bus_suffix(bus_type);
  return get_or_insert_function(m, fname, get_tap_packet_write_ty(m));
}

StructType* get_nt_tap_packet_length_req_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_length_req";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*,2> elements = {
    IntegerType::getInt8Ty(c),    /* valid */
    IntegerType::getInt16Ty(c),   /* max_length */
  };
  ty = StructType::create(c, elements, name);
  return ty;
}

StructType* get_nt_tap_packet_length_resp_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_length_resp";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*,2> elements = {
    IntegerType::getInt8Ty(c),    /* valid */
    IntegerType::getInt16Ty(c),   /* result_length */
  };
  return StructType::create(c, elements, name);
}

StructType* get_nt_tap_packet_length_state_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_length_state";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*, 4>  elements = {
    IntegerType::getInt16Ty(c),   /* packet_offset */
    IntegerType::getInt16Ty(c),   /* packet_length */
    IntegerType::getInt8Ty(c),    /* done */
    IntegerType::getInt8Ty(c),    /* data_eop_seen */
  };
  return StructType::create(c, elements, name);
}

FunctionType* get_tap_packet_length_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_tap_packet_length(
  **   struct nanotube_tap_packet_length_resp  *resp_out,
  **   struct nanotube_tap_packet_length_state *state_inout,
  **   const uint8_t *packet_word_in,
  **   const struct nanotube_tap_packet_length_req *req_in);
  **/
  auto* res_ty   = Type::getVoidTy(c);
  Type* arg_ty[] = {
    get_nt_tap_packet_length_resp_ty(m)->getPointerTo(),
    get_nt_tap_packet_length_state_ty(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
    get_nt_tap_packet_length_req_ty(m)->getPointerTo()
  };
  return FunctionType::get(res_ty, arg_ty, false);
}

Constant* create_tap_packet_length(Module& m, nanotube_bus_id_t bus_type) {
  auto fname = std::string("nanotube_tap_packet_length") +
                 get_bus_suffix(bus_type);
  return get_or_insert_function(m, fname, get_tap_packet_length_ty(m));
}

/* Resize Tap */
StructType* get_nt_tap_packet_resize_cword_ty(Module& m) {
  LLVMContext& c = m.getContext();
  std::string  name = "struct.nanotube_tap_packet_resize_cword_t";

  auto* ty = m.getTypeByName(name);
  if( ty != nullptr )
    return ty;

  std::array<Type*, 10> elements = {
    get_nt_tap_offset_ty(m),      /* packet_rot */
    get_nt_tap_offset_ty(m),      /* output_insert_start */
    get_nt_tap_offset_ty(m),      /* output_insert_end */
    get_nt_tap_offset_ty(m),      /* carried_insert_start */
    get_nt_tap_offset_ty(m),      /* carried_insert_end */
    IntegerType::getInt8Ty(c),    /* select_carried */
    IntegerType::getInt8Ty(c),    /* push_1 */
    IntegerType::getInt8Ty(c),    /* push_2 */
    IntegerType::getInt8Ty(c),    /* eop */
    get_nt_tap_offset_ty(m),      /* word_length */
  };
  return StructType::create(c, elements, name);
}

StructType* get_nt_tap_packet_resize_req_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_resize_req_t";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*,3> elements = {
    get_nt_tap_offset_ty(m),      /* write_offset */
    get_nt_tap_offset_ty(m),      /* delete_length */
    get_nt_tap_offset_ty(m),      /* insert_length */
  };
  ty = StructType::create(c, elements, name);
  return ty;
}

StructType* get_nt_tap_packet_resize_ingress_state_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_resize_ingress_state_t";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*, 12>  elements = {
    IntegerType::getInt8Ty(c),    /* new_req */
    IntegerType::getInt16Ty(c),   /* packet_length */
    IntegerType::getInt16Ty(c),   /* packet_offset */
    get_nt_tap_offset_ty(m),      /* packet_rot_amount */
    get_nt_tap_offset_ty(m),      /* unshifted_length */
    IntegerType::getInt8Ty(c),    /* edit_started */
    get_nt_tap_offset_ty(m),      /* edit_delete_length */
    get_nt_tap_offset_ty(m),      /* edit_insert_length */
    get_nt_tap_offset_ty(m),      /* edit_carried_len */
    IntegerType::getInt8Ty(c),    /* shifted_started */
    get_nt_tap_offset_ty(m),      /* shifted_carried_len */
    IntegerType::getInt8Ty(c),    /* data_eop_seen */
  };
  return StructType::create(c, elements, name);
}
FunctionType* get_tap_packet_resize_ingress_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_tap_packet_resize_ingress(
  **   bool *packet_done_out,
  **   nanotube_tap_packet_resize_cword_t *cword_out,
  **   nanotube_tap_offset_t *packet_length_out,
  **   nanotube_tap_packet_resize_ingress_state_t *state,
  **   nanotube_tap_packet_resize_req_t *resize_req_in
  **   uint8_t *packet_word_in);
  **/
  auto* res_ty   = Type::getVoidTy(c);
  Type* arg_ty[] = {
    IntegerType::getInt8Ty(c)->getPointerTo(),
    get_nt_tap_packet_resize_cword_ty(m)->getPointerTo(),
    get_nt_tap_offset_ty(m)->getPointerTo(),
    get_nt_tap_packet_resize_ingress_state_ty(m)->getPointerTo(),
    get_nt_tap_packet_resize_req_ty(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
  };
  return FunctionType::get(res_ty, arg_ty, false);
}
Constant* create_tap_packet_resize_ingress(Module& m, nanotube_bus_id_t bus_type) {
  auto fname = std::string("nanotube_tap_packet_resize_ingress") +
                 get_bus_suffix(bus_type);
  return get_or_insert_function(m, fname, get_tap_packet_resize_ingress_ty(m));
}

StructType* get_nt_tap_packet_resize_egress_state_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_resize_egress_state_t";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*, 4>  elements = {
    IntegerType::getInt8Ty(c),    /* new_req */
    IntegerType::getInt8Ty(c),    /* new_pkt */
    IntegerType::getInt16Ty(c),   /* packet_length */
    IntegerType::getInt16Ty(c),   /* packet_offset */
 };
  return StructType::create(c, elements, name);
}
FunctionType* get_tap_packet_resize_egress_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_tap_packet_resize_egress(
  **   bool* input_done_out,
  **   bool* packet_done_out,
  **   bool* packet_valid_out,
  **   uint8_t* packet_word_out,
  **   nanotube_tap_packet_resize_egress_state_t* state,
  **   void* state_packet_word,
  **   nanotube_tap_packet_resize_cword_t* cword,
  **   uint8_t* packet_word_in,
  **   nanotube_tap_offset_t new_packet_len);
  **/
  auto* res_ty   = Type::getVoidTy(c);
  Type* arg_ty[] = {
    Type::getInt8PtrTy(c),
    Type::getInt8PtrTy(c),
    Type::getInt8PtrTy(c),
    Type::getInt8PtrTy(c),
    get_nt_tap_packet_resize_egress_state_ty(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
    get_nt_tap_packet_resize_cword_ty(m)->getPointerTo(),
    Type::getInt8PtrTy(c),
    get_nt_tap_offset_ty(m),
  };
  return FunctionType::get(res_ty, arg_ty, false);
}
Constant* create_tap_packet_resize_egress(Module& m, nanotube_bus_id_t bus_type) {
  auto fname = std::string("nanotube_tap_packet_resize_egress") +
                 get_bus_suffix(bus_type);
  return get_or_insert_function(m, fname,
           get_tap_packet_resize_egress_ty(m));
}

FunctionType* get_tap_packet_resize_egress_state_init_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_tap_packet_resize_egress_state_init(
  **   nanotube_tap_packet_resize_egress_state_t *state);
  **/
  auto* res_ty   = Type::getVoidTy(c);
  Type* arg_ty[] = {
    get_nt_tap_packet_resize_egress_state_ty(m)->getPointerTo(),
  };
  return FunctionType::get(res_ty, arg_ty, false);
}
Constant* create_tap_packet_resize_egress_state_init(Module& m) {
  return m.getOrInsertFunction("nanotube_tap_packet_resize_egress_state_init",
      get_tap_packet_resize_egress_state_init_ty(m));
}
FunctionType* get_tap_packet_resize_ingress_state_init_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** void nanotube_tap_packet_resize_ingress_state_init(
  **   nanotube_tap_packet_resize_ingress_state_t *state);
  **/
  auto* res_ty   = Type::getVoidTy(c);
  Type* arg_ty[] = {
    get_nt_tap_packet_resize_ingress_state_ty(m)->getPointerTo(),
  };
  return FunctionType::get(res_ty, arg_ty, false);
}
Constant* create_tap_packet_resize_ingress_state_init(Module& m) {
  return m.getOrInsertFunction("nanotube_tap_packet_resize_ingress_state_init",
      get_tap_packet_resize_ingress_state_init_ty(m));
}

StructType* get_nt_tap_packet_eop_state_ty(Module& m) {
  static const std::string name = "struct.nanotube_tap_packet_eop_state";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  LLVMContext& c = m.getContext();
  std::array<Type*, 2>  elements = {
    IntegerType::getInt16Ty(c),   /* packet_offset */
    IntegerType::getInt16Ty(c),   /* packet_length */
  };
  return StructType::create(c, elements, name);
}

FunctionType* get_tap_packet_is_eop_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /**
  ** bool nanotube_tap_packet_is_eop(
  **   const uint8_t        *packet_word_in,
  **   struct nanotube_tap_packet_eop_state *state_inout);
  **/
  auto* res_ty   = Type::getInt1Ty(c);
  Type* arg_ty[] = {
    Type::getInt8PtrTy(c),
    get_nt_tap_packet_eop_state_ty(m)->getPointerTo(),
  };
  return FunctionType::get(res_ty, arg_ty, false);
}
Constant* create_tap_packet_is_eop(Module& m, nanotube_bus_id_t bus_type) {
  auto fname = std::string("nanotube_tap_packet_is_eop") +
                 get_bus_suffix(bus_type);
  return get_or_insert_function(m, fname, get_tap_packet_is_eop_ty(m));
}
};

/********** Context **********/
namespace nanotube {
StructType* get_nt_context_type(Module& m) {
  /* nanotube_context is now opaque! */
  const char nt_ctx_name[] = "struct.nanotube_context";
  Type* nt_ctx_ty = m.getTypeByName(nt_ctx_name);
  if( nt_ctx_ty != nullptr )
    return cast<StructType>(nt_ctx_ty);

  nt_ctx_ty = StructType::create(m.getContext(), nt_ctx_name);
  return cast<StructType>(nt_ctx_ty);
}
FunctionType* get_nt_context_create_ty(Module& m) {
  /*
  ** nanotube_context_t *nanotube_context_create();
  */
  return FunctionType::get(get_nt_context_type(m)->getPointerTo(),
                           false);
}
Constant* create_nt_context_create(Module& m) {
  return m.getOrInsertFunction("nanotube_context_create",
      get_nt_context_create_ty(m));
}
FunctionType* get_nt_context_add_channel_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /*
  ** void
  ** nanotube_context_add_channel(nanotube_context_t* context,
  **                              nanotube_channel_id_t channel_id,
  **                              nanotube_channel_t* channel,
  **                              nanotube_channel_flags_t flags);
  */
  std::array<Type*, 4> args = {
    get_nt_context_type(m)->getPointerTo(),
    get_nt_channel_id_ty(m),
    get_nt_channel_ty(m)->getPointerTo(),
    get_nt_channel_flags_ty(m),
  };
  return FunctionType::get(Type::getVoidTy(c), args, false);
}
Constant* create_nt_context_add_channel(Module& m) {
  return m.getOrInsertFunction("nanotube_context_add_channel",
      get_nt_context_add_channel_ty(m));
}
FunctionType* get_nt_context_add_map_ty(Module& m) {
  LLVMContext& c = m.getContext();
  /*
  ** void
  ** nanotube_context_add_map(nanotube_context_t* context,
  **                          nanotube_map_t*     map);
  */
  std::array<Type*, 2> args = {
    get_nt_context_type(m)->getPointerTo(),
    get_nt_map_type(m)->getPointerTo(),
  };
  return FunctionType::get(Type::getVoidTy(c), args, false);
}
Constant* create_nt_context_add_map(Module& m) {
  return m.getOrInsertFunction("nanotube_context_add_map",
      get_nt_context_add_map_ty(m));
}

};

/********** Kernels **********/
namespace nanotube {
  FunctionType* get_nt_kernel_func_ty(Module& m) {
    LLVMContext& c = m.getContext();
    /*
    ** typedef int nanotube_kernel_t(nanotube_context_t *context,
    **                                nanotube_packet_t *packet);
    */
    std::array<Type*, 2> args = {
      get_nt_context_type(m)->getPointerTo(),
      get_nt_packet_type(m)->getPointerTo(),
    };
    return
      FunctionType::get(Type::getInt32Ty(c), args, false);
  }

  FunctionType* add_nt_add_plain_packet_kernel_ty(Module& m)
  {
    LLVMContext& c = m.getContext();
    /*
    ** void
    ** nanotube_add_plain_packet_kernel(const char* name,
    **                                  nanotube_kernel_t* kernel,
    **                                  int bus_type,
    **                                  int capsules);
    */
    std::array<Type*, 4> args = {
      Type::getInt8PtrTy(c),
      get_nt_kernel_func_ty(m)->getPointerTo(),
      Type::getInt32Ty(c),
      Type::getInt32Ty(c),
    };
    return FunctionType::get(Type::getVoidTy(c), args, false);
  }

  Constant* create_nt_add_plain_packet_kernel(Module& m)
  {
    return m.getOrInsertFunction("nanotube_add_plain_packet_kernel",
                                 add_nt_add_plain_packet_kernel_ty(m));
  }

};

/********** Maps **********/
namespace nanotube {
  FunctionType* get_nt_tap_map_recv_resp_ty(Module& m) {
    LLVMContext& c = m.getContext();
    /*
     * bool
     * nanotube_tap_map_recv_resp(
     *   nanotube_context_t *context,
     *   nanotube_tap_map_t *map,
     *   unsigned int client_id,
     *   uint8_t *data_out,
     *   nanotube_map_result_t *result_out);
     */
    std::array<Type*, 5> args = {
      get_nt_context_type(m)->getPointerTo(),
      get_nt_tap_map_ty(m)->getPointerTo(),
      Type::getInt32Ty(c),
      Type::getInt8PtrTy(c),
      get_nt_map_result_ty(m)->getPointerTo(),
    };
    return FunctionType::get(Type::getInt1Ty(c), args, false);
  }
  Constant* create_nt_tap_map_recv_resp(Module& m) {
    return m.getOrInsertFunction("nanotube_tap_map_recv_resp",
                                 get_nt_tap_map_recv_resp_ty(m));

  }

  FunctionType* get_nt_tap_map_send_req_ty(Module& m) {
    LLVMContext& c = m.getContext();
    /*
     * void
     * nanotube_tap_map_send_req(
     *   nanotube_context_t *context,
     *   nanotube_tap_map_t *map,
     *   unsigned int client_id,
     *   enum map_access_t access,
     *   uint8_t *key_in,
     *   uint8_t *data_in);
     */
    std::array<Type*, 6> args = {
      get_nt_context_type(m)->getPointerTo(),
      get_nt_tap_map_ty(m)->getPointerTo(),
      Type::getInt32Ty(c),
      Type::getInt32Ty(c),
      Type::getInt8PtrTy(c),
      Type::getInt8PtrTy(c),
    };
    return FunctionType::get(Type::getVoidTy(c), args, false);
  }
  Constant* create_nt_tap_map_send_req(Module& m) {
    return m.getOrInsertFunction("nanotube_tap_map_send_req",
                                 get_nt_tap_map_send_req_ty(m));

  }

  FunctionType* get_nt_tap_map_create_ty(Module& m) {
    LLVMContext& c = m.getContext();
    /*
     * nanotube_tap_map_t*
     * nanotube_tap_map_create(
     *   enum map_type_t        map_type,
     *   nanotube_map_width_t   key_length,
     *   nanotube_map_width_t   data_length,
     *   nanotube_map_depth_t   capacity,
     *   unsigned int           num_clients);
     */
    std::array<Type*, 5> args = {
      Type::getInt32Ty(c),
      get_nt_map_width_ty(m),
      get_nt_map_width_ty(m),
      get_nt_map_depth_ty(m),
      Type::getInt32Ty(c),
    };
    return FunctionType::get(get_nt_tap_map_ty(m)->getPointerTo(),
                             args, false);
  }
  Constant* create_nt_tap_map_create(Module& m) {
    return m.getOrInsertFunction("nanotube_tap_map_create",
                                 get_nt_tap_map_create_ty(m));

  }
  Type* get_nt_map_width_ty(Module& m) {
    return Type::getInt16Ty(m.getContext());
  }
  Type* get_nt_map_depth_ty(Module& m) {
    return Type::getInt64Ty(m.getContext());
  }
  FunctionType* get_nt_tap_map_add_client_ty(Module& m) {
    auto& c = m.getContext();
    /*
     * void
     * nanotube_tap_map_add_client(
     *   nanotube_tap_map_t    *map,
     *   nanotube_map_width_t   key_in_length,
     *   nanotube_map_width_t   data_in_length,
     *   bool                   need_result_out,
     *   nanotube_map_width_t   data_out_length,
     *   nanotube_context_t    *req_context,
     *   nanotube_channel_id_t  req_channel_id,
     *   nanotube_context_t    *resp_context,
     *   nanotube_channel_id_t  resp_channel_id);
     */
    std::array<Type*, 9> args = {
      get_nt_tap_map_ty(m)->getPointerTo(),
      get_nt_map_width_ty(m),
      get_nt_map_width_ty(m),
      Type::getInt1Ty(c),
      get_nt_map_width_ty(m),
      get_nt_context_type(m)->getPointerTo(),
      get_nt_channel_id_ty(m),
      get_nt_context_type(m)->getPointerTo(),
      get_nt_channel_id_ty(m),
    };
    return FunctionType::get(Type::getVoidTy(c), args, false);
  }
  Constant* create_nt_tap_map_add_client(Module& m) {
    return get_or_insert_function(m, "nanotube_tap_map_add_client",
                                  get_nt_tap_map_add_client_ty(m));
  }
  FunctionType* get_nt_tap_map_build_ty(Module& m) {
    auto& c = m.getContext();
    /*
     * void
     * nanotube_tap_map_build(
     *   nanotube_tap_map_t *map);
     */
    return FunctionType::get(Type::getVoidTy(c),
             get_nt_tap_map_ty(m)->getPointerTo(), false);
  }
  Constant* create_nt_tap_map_build(Module& m) {
    return get_or_insert_function(m, "nanotube_tap_map_build",
                                  get_nt_tap_map_build_ty(m));
  }
};


/********** Nanotube Data Types **********/
namespace nanotube {

StructType* get_nt_map_type(Module& m) {
  LLVMContext& c = m.getContext();

  const char ty_name[] ="struct.nanotube_map";

  StructType* ty = m.getTypeByName(ty_name);
  if( ty == nullptr ) {
    ty = StructType::create(ty_name, Type::getInt64Ty(c),
                                     Type::getInt64Ty(c),
                                     Type::getInt32Ty(c),
                                     Type::getInt32Ty(c),
                                     Type::getInt32Ty(c));
  }
  assert(ty != nullptr);
  return ty;
}

IntegerType* get_nt_map_id_type(Module& m) {
  // typedef uint16_t nanotube_map_id_t;
  return Type::getInt16Ty(m.getContext());
}

StructType* get_nt_packet_type(Module& m) {
  LLVMContext& c = m.getContext();
  //XXX: Check this as there is a typedef to nanotube_packet_t
  const char ty_name[] ="struct.nanotube_packet";

  StructType* ty = m.getTypeByName(ty_name);
  if( ty == nullptr ) {
    ty = StructType::create(c, ty_name);
  }
  assert(ty != nullptr);
  return ty;
}

Type* get_simple_bus_word_ty(Module& m) {
  static const std::string name = "struct.simple_bus::word";
  auto* ty = m.getTypeByName(name);
  if (ty != nullptr)
    return ty;

  auto& c = m.getContext();
  auto *ar_ty = ArrayType::get(Type::getInt8Ty(c), 65);
  return StructType::create(name, ar_ty);
}

Type* get_nt_tap_offset_ty(Module& m) {
  auto& c = m.getContext();
  return IntegerType::getInt16Ty(c);
}

Type* get_nt_map_result_ty(Module& m) {
  LLVMContext& c = m.getContext();
  return Type::getInt32Ty(c);
}

Type* get_nt_tap_map_ty(Module& m) {
  LLVMContext& c = m.getContext();
  const char ty_name[] ="struct.nanotube_tap_map";

  StructType* ty = m.getTypeByName(ty_name);
  if( ty == nullptr ) {
    ty = StructType::create(c, ty_name);
  }
  assert(ty != nullptr);
  return ty;
}

/* Constants */
uint64_t get_simple_bus_word_control_offs() { return   64; }
uint8_t  get_simple_bus_word_control_eop()  { return 0x80; }
}; //namespace nanotube

namespace nanotube {
void nt_api_call::initialise_from(CallInst* call) {
  this->call = call;
  intrin = ::get_intrinsic(call);
}

bool nt_api_call::is_packet() {
  return intrin == Intrinsics::packet_read ||
         intrin == Intrinsics::packet_write ||
         intrin == Intrinsics::packet_write_masked ||
         intrin == Intrinsics::packet_edit ||
         intrin == Intrinsics::packet_bounded_length ||
         intrin == Intrinsics::packet_data ||
         intrin == Intrinsics::packet_end ||
         intrin == Intrinsics::packet_resize;
}

bool nt_api_call::is_map() {
  return intrin == Intrinsics::map_op ||
         intrin == Intrinsics::map_create ||
         intrin == Intrinsics::map_op ||
         intrin == Intrinsics::map_get_id ||
         intrin == Intrinsics::map_lookup ||
         intrin == Intrinsics::map_read ||
         intrin == Intrinsics::map_write ||
         intrin == Intrinsics::map_insert ||
         intrin == Intrinsics::map_update ||
         intrin == Intrinsics::map_remove;
}

bool nt_api_call::is_read() {
  return intrin == Intrinsics::packet_read ||
         intrin == Intrinsics::map_read ||
         intrin == Intrinsics::map_op;
}
bool nt_api_call::is_write() {
  return intrin == Intrinsics::packet_write ||
         intrin == Intrinsics::packet_write_masked ||
         intrin == Intrinsics::map_write ||
         intrin == Intrinsics::map_insert ||
         intrin == Intrinsics::map_update ||
         intrin == Intrinsics::map_op;
}
bool nt_api_call::is_access() {
  return is_read() || is_write() ||
         intrin == Intrinsics::map_lookup ||
         intrin == Intrinsics::map_remove;
}

Value* nt_api_call::get_context() {
  return nullptr;
}
Value* nt_api_call::get_packet() {
  if( !is_packet() )
    return nullptr;
  return call->getOperand(0);
}
Value* nt_api_call::get_data_ptr() {
  return nullptr;
}
Value* nt_api_call::get_mask_ptr() {
  return nullptr;
}

int nt_api_call::get_map_op_arg_idx() {
  if( intrin != Intrinsics::map_op )
    return -1;
  return 2;
}

Value* nt_api_call::get_map_op_arg() {
  auto idx = get_map_op_arg_idx();
  if( idx < 0 )
    return nullptr;
  return call->getOperand(idx);
}

int nt_api_call::get_map_op() {
  auto* type = get_map_op_arg();
  if( !isa<ConstantInt>(type) ) {
    errs() << "get_map_op: " << *call << '\n'
           << "Type: " << *call->getOperand(2) << '\n'
           << "Expecting a constant map-op type.\n";
    assert(false);
  }
  return cast<ConstantInt>(call->getOperand(2))->getSExtValue();
}

Value* nt_api_call::get_dummy_arg(unsigned a) {
  Value* arg  = call->getOperand(a);
  Type*  ty   = arg->getType();
  Value* v    = nullptr;
  switch( intrin ) {
    case Intrinsics::packet_read:
    case Intrinsics::packet_write:
      switch( a ) {
        case 0: /*fall-through */
        case 1: /*fall-through */
        case 2: v = UndefValue::get(ty); break;

        case 3: v = ConstantInt::get(ty, 0); break;
      };
      break;
    case Intrinsics::packet_write_masked:
      switch( a ) {
        case 0: /*fall-through */
        case 1: /*fall-through */
        case 2: /*fall-through */
        case 3: v = UndefValue::get(ty); break;

        case 4: v = ConstantInt::get(ty, 0); break;
      };
      break;
    case Intrinsics::packet_resize:
      switch( a ) {
        case 0: /*fall-through*/
        case 1: v = UndefValue::get(ty); break;

        case 2: v = ConstantInt::get(ty, 0); break;
      };
      break;
    case Intrinsics::packet_bounded_length:
      v = UndefValue::get(ty);
      break;
    case Intrinsics::map_op:
      switch( a ) {
        /* type = NANOTUBE_MAP_NOP */
        case 2: v = ConstantInt::get(ty, 5); break;

        case 0: /*fall-through */
        case 1: /*fall-through */
        case 3: /*fall-through */
        case 4: /*fall-through */
        case 5: /*fall-through */
        case 6: /*fall-through */
        case 7: /*fall-through */
        case 8: /*fall-through */
        case 9: v = UndefValue::get(ty); break;
      };
      break;
    default:
      errs() << "Unhandled intrinsic " << (unsigned)intrin
             << " " << *call << '\n';
      assert(false);
  };
  return v;
}
MemoryLocation nt_api_call::get_memory_location(unsigned arg_idx) {
  return ::get_memory_location(call, arg_idx, *tli);
}

Value* nt_api_call::get_size_arg(unsigned arg_idx, bool* bits) {
  int size_arg = ::get_size_arg(call, arg_idx, bits);
  if( size_arg < 0 ) {
    errs() << "ERROR: Unknown size argument for arg " << arg_idx << " in "
           << "call: " << *call << '\n';
  }
  assert(size_arg >= 0);
  return call->getOperand(size_arg);
}

int nt_api_call::get_data_size_arg_idx() {
  switch( intrin ) {
    case Intrinsics::packet_read:
    case Intrinsics::packet_write:
      return 3; /* length */
    case Intrinsics::packet_write_masked:
      return 4; /* length */
    default:
      errs() << "XXX: Unimplemented get_data_size_arg for " << *call << '\n';
      assert(false);
  };
  return -1;
}
Value* nt_api_call::get_data_size_arg() {
  int idx = get_data_size_arg_idx();
  if( idx > 0 )
    return call->getOperand(idx);
  else
    return nullptr;
}

int nt_api_call::get_max_data_size() {
  int arg = get_data_size_arg_idx();
  if( arg < 0 )
    return -1;
  unsigned max = 0;
  bool precise = false;
  if( get_max_value(call->getOperand(arg), &max, &precise) )
    return (int)max;
  return -1;
}

int nt_api_call::get_key_size_arg_idx() {
  errs() << "XXX: Implement me get_max_key_size()\n";
  abort();
}
Value* nt_api_call::get_key_size_arg() {
  int idx = get_key_size_arg_idx();
  if( idx > 0 )
    return call->getOperand(idx);
  else
    return nullptr;
}

int nt_api_call::get_max_key_size() {
  errs() << "XXX: Implement me get_max_key_size()\n";
  abort();
}

std::vector<MemoryLocation> nt_api_call::get_input_mem() {
  std::vector<MemoryLocation> mem;
  switch( intrin ) {
    case Intrinsics::packet_read:
    case Intrinsics::packet_edit:
    case Intrinsics::packet_bounded_length:
    case Intrinsics::packet_get_port:
    case Intrinsics::packet_set_port:
    case Intrinsics::packet_data:
    case Intrinsics::packet_end:
    case Intrinsics::packet_resize:
    case Intrinsics::map_create:
    case Intrinsics::map_get_id:
      break;

    case Intrinsics::packet_write:
      mem.emplace_back(get_memory_location(1)); /* data_in */
      break;
    case Intrinsics::packet_write_masked:
      mem.emplace_back(get_memory_location(1)); /* data_in */
      mem.emplace_back(get_memory_location(2)); /* mask */
      break;
    case Intrinsics::map_op:
      mem.emplace_back(get_memory_location(3)); /* key */
      mem.emplace_back(get_memory_location(5)); /* data_in */
      mem.emplace_back(get_memory_location(7)); /* mask */
#if 0
      // NOTE: This does not work, because the converge pass turns dummy
      // accesses into NANOTUBE_MAP_NOP accesses, and therefore we end up
      // with non-constant map_op types. => just keep it static therefore
      switch( get_map_op() ) {
        case NANOTUBE_MAP_READ:
        case NANOTUBE_MAP_REMOVE:
        case NANOTUBE_MAP_NOP:
          break;
        case NANOTUBE_MAP_INSERT:
        case NANOTUBE_MAP_UPDATE:
        case NANOTUBE_MAP_WRITE:
          mem.emplace_back(get_memory_location(5)); /* data_in */
          mem.emplace_back(get_memory_location(7)); /* mask */
          break;
      };
#endif
      break;
    case Intrinsics::map_lookup:
    case Intrinsics::map_read:
    case Intrinsics::map_remove:
      mem.emplace_back(get_memory_location(2)); /* key */
      break;
      break;
    case Intrinsics::map_write:
      mem.emplace_back(get_memory_location(2)); /* key */
      mem.emplace_back(get_memory_location(4)); /* data_in */
      break;
    /* XXX: This does not exist in INtrinsics.def! */
#if 0
    case Intrinsics::map_write_masked:
    case Intrinsics::map_insert_masked:
    case Intrinsics::map_update_masked:
      mem.emplace_back(get_memory_location(2)); /* key */
      mem.emplace_back(get_memory_location(4)); /* data_in */
      mem.emplace_back(get_memory_location(5)); /* enables */
      break;
#endif
    default:
      errs() << "Cannot identify memory inputs for " << *call << '\n';
      assert(false);
  };
  return mem;
}
std::vector<MemoryLocation> nt_api_call::get_output_mem() {
  std::vector<MemoryLocation> mem;
  switch( intrin ) {
    case Intrinsics::packet_write:
    case Intrinsics::packet_write_masked:
    case Intrinsics::packet_edit:
    case Intrinsics::packet_bounded_length:
    case Intrinsics::packet_get_port:
    case Intrinsics::packet_set_port:
    case Intrinsics::packet_data:
    case Intrinsics::packet_end:
    case Intrinsics::packet_resize:
    case Intrinsics::map_create:
    case Intrinsics::map_get_id:
    case Intrinsics::map_lookup:
    case Intrinsics::map_remove:
    case Intrinsics::map_write:
      break;
    /* XXX: This does not exist in INtrinsics.def! */
#if 0
    case Intrinsics::map_write_masked:
    case Intrinsics::map_insert_masked:
    case Intrinsics::map_update_masked:
      break;
#endif

    case Intrinsics::packet_read:
      mem.emplace_back(get_memory_location(1)); /* data_out */
      break;
    case Intrinsics::map_op:
      mem.emplace_back(get_memory_location(6)); /* data_out */
#if 0
      switch( get_map_op() ) {
        /* XXX: map_op should always read all input memory, and write all
         *      memory outputs?
         *      * Phi on map_op? -> allow different ops on same map
         *      * phi_on_addr of memory allocations here? or single
         *        allocation?
         *      * nullptr functions
         *
         *
         * We need to think about this some more.
         *    * converge all paths, none need memory -> nullptr
         *    * converge some paths, some need memory -> single pointer
         *      with memcpy conversion
         * ??
         *
         * At the moment, map_op is not a phi, as the converge pass does
         * not merge different map_op types, even to the same map.
         *
         * Update: the converge pass actually merges in a NANOTUBE_MAP_NOP
         * here for dummy accesses. So we end up with a phi over the type,
         * so lets just turn this static.
         */
        case NANOTUBE_MAP_INSERT:
        case NANOTUBE_MAP_UPDATE:
        case NANOTUBE_MAP_WRITE:
        case NANOTUBE_MAP_REMOVE:
        case NANOTUBE_MAP_NOP:
          break;
        case NANOTUBE_MAP_READ:
          mem.emplace_back(get_memory_location(6)); /* data_out */
          break;
      };
#endif
      break;
    case Intrinsics::map_read:
      mem.emplace_back(get_memory_location(4)); /* data_out */
      break;
    default:
      errs() << "Cannot identify memory output for " << *call << '\n';
      assert(false);
  };
  return mem;
}

const TargetLibraryInfo* nt_api_call::tli = nullptr;
};

/* Information about NT API calls for Alias Analysis */
namespace nanotube {
ModRefInfo get_nt_arg_info(Intrinsics::ID intr, unsigned arg_idx) {
  if (intr == Intrinsics::none ||
      intr == Intrinsics::llvm_unknown)
    return ModRefInfo::ModRef;

  assert(intr < Intrinsics::end);
  assert(intrinsic_arg_info.size() == Intrinsics::end);
  assert(arg_idx < intrinsic_arg_info[intr].size());

  return intrinsic_arg_info[intr][arg_idx];
}

FunctionModRefBehavior get_nt_fmrb(Intrinsics::ID intr) {
  /* Make sure the table covers all enums */
  assert(intr < Intrinsics::end);
  assert(intrinsic_fmrb.size() == Intrinsics::end);
  return intrinsic_fmrb[intr];
}
};

/*************************************************************************/

#include <iostream>

class IntrinsicDebugPass: public llvm::ImmutablePass {
public:
  static char ID;
  IntrinsicDebugPass();
};

char IntrinsicDebugPass::ID;

IntrinsicDebugPass::IntrinsicDebugPass(): ImmutablePass(ID) {
  raw_os_ostream output(std::cout);

  for (auto &p: intrinsic_id_to_name) {
    output << p.first << " -> " << p.second << "\n";
  }
  for (auto &p: intrinsic_name_to_id) {
    output << p.first << " -> " << p.second << "\n";
  }

  for (unsigned i=0; i<intrinsic_arg_info.size(); i++) {
    output << intrinsic_to_string((Intrinsics::ID)i)
           << ":";
    for (unsigned j=0; j<intrinsic_arg_info[i].size(); j++) {
      const char *s = "? ";
      switch (intrinsic_arg_info[i][j]) {
      case ModRefInfo::MustRef:    s = "R "; break;
      case ModRefInfo::MustMod:    s = "W "; break;
      case ModRefInfo::MustModRef: s = "RW"; break;
      case ModRefInfo::NoModRef:   s = "N "; break;
      case ModRefInfo::ModRef:     s = "_ "; break;
      default:                               break;
      }
      output << ' ' << s;
    }
    output << '\n';
  }

  for (int i=Intrinsics::none; i<Intrinsics::end; i++) {
    auto iid = (Intrinsics::ID)i;
    output << intrinsic_to_string(iid) << ": ";
    const char *s = "(unknown)";
    switch (get_nt_fmrb(iid)) {
    case FMRB_OnlyReadsArgumentPointees:        s = "RO";  break;
    case FMRB_OnlyAccessesArgumentPointees:     s = "RW";  break;
    case FMRB_OnlyAccessesInaccessibleOrArgMem: s = "RWI"; break;
    case FMRB_OnlyAccessesInaccessibleMem:      s = "I";   break;
    case FMRB_DoesNotAccessMemory:              s = "DNA"; break;
    case FMRB_UnknownModRefBehavior:            s = "UK";  break;
    default: break;
    }
    output << s << '\n';
  }
}

static RegisterPass<IntrinsicDebugPass>
register_pass("intrinsic-debug", "Debug the intrinsics",
               false,
               false
  );

/*************************************************************************/

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
