/**************************************************************************\
*//*! \file Intrinsics.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube intrinsics
**   \date  2019-03-14
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_INTRINSICS_H
#define NANOTUBE_INTRINSICS_H

#include "llvm_pass.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "nanotube_api.h"
#include "bus_type.hpp"

#include <cstdint>
#include <string>

/* NB: Putting a using namespace llvm line breaks comilation elsewhere, as
 * there is no way of un-using that namespace, so every file including this
 * will be then using namespace llvm. :-| */
using llvm::ArrayRef;
using llvm::ArrayType;
using llvm::CallBase;
using llvm::CallInst;
using llvm::Constant;
using llvm::FunctionType;
using llvm::IntegerType;
using llvm::LLVMContext;
using llvm::MemoryLocation;
using llvm::StructType;
using llvm::TargetLibraryInfo;
using llvm::Value;

namespace nanotube
{
  namespace Intrinsics
  {
    typedef enum {
      none,

      // LLVM intrinsics.
      llvm_bswap,
      llvm_dbg_declare,
      llvm_dbg_value,
      llvm_lifetime_start,
      llvm_lifetime_end,
      llvm_memset,
      llvm_memcpy,
      llvm_memcmp,
      llvm_stackrestore,
      llvm_stacksave,
      llvm_unknown,

      // Nanotube intrinsics.
#include "Intrinsics.def"

      // Not an intrinsic.  Indicates the maximum value.
      end,
    } ID;
  }

  // This can be called to determine whether an instruction is a call
  // to an intrinsic and which intrinsic if it is.  For a call to a
  // normal function or a non-call instruction it will return
  // Intrinsics::none.  It handles both Nanotube intrinsics and LLVM
  // intrinsics.  Some LLVM intrinstics have their own enum value.
  // For any other LLVM intrinsic it will return
  // Intrinsics::llvm_unknown.
  Intrinsics::ID get_intrinsic(const Instruction* insn);
  Intrinsics::ID get_intrinsic(const Function* func);

  // Gets the name of a specific intrinsic; this can be useful for debug
  // printouts.
  std::string intrinsic_to_string(Intrinsics::ID id);

  // Returns true if the specified intrinsic is known to have no
  // runtime effect.  This kind of intrinsic is used to provide
  // compile-time information about the program.
  static inline
  bool intrinsic_is_nop(Intrinsics::ID id) {
    return ( id == Intrinsics::llvm_lifetime_start ||
             id == Intrinsics::llvm_lifetime_end ||
             id == Intrinsics::llvm_dbg_declare ||
             id == Intrinsics::llvm_dbg_value );
  }

  /********** Helpers to examine LLVM IR for intrinsics **********/

  static inline
  bool is_nt_map_lookup(const CallInst* call) {
    if( call == nullptr )
      return false;
    return get_intrinsic(call) == Intrinsics::map_lookup;
  }

  static inline
  bool is_nt_packet_data(const CallInst* call) {
    if( call == nullptr )
      return false;

    bool pkt     = get_intrinsic(call) == Intrinsics::packet_data;
    bool pkt_end = get_intrinsic(call) == Intrinsics::packet_end;
    return pkt | pkt_end;
  }

  static inline
  bool is_nt_get_time_ns(const CallInst* call) {
    if( call == nullptr )
      return false;
    return get_intrinsic(call) == Intrinsics::get_time_ns;
  }

  // A structure used to unpack the arguments of calls to
  // nanotube_malloc.
  struct malloc_args
  {
    malloc_args(const Instruction *insn);
    Value *size;
  };

  // A structure used to unpack the arguments of calls to
  // nanotube_channel_create.
  struct channel_create_args
  {
    channel_create_args(const Instruction *insn);
    std::string name;
    Value *elem_size;
    Value *num_elem;
  };

  // A structure used to unpack the arguments of calls to
  // nanotube_channel_set_attr
   struct channel_set_attr_args
  {
    channel_set_attr_args(const Instruction *insn);
    Value *channel;
    nanotube_channel_attr_id_t attr_id;
    int32_t attr_val;
  }; 

  // A structure used to unpack the arguments of calls to
  // nanotube_channel_export.
  struct channel_export_args
  {
    channel_export_args(const Instruction *insn);
    Value *channel;
    nanotube_channel_type_t type;
    nanotube_channel_flags_t flags;
  };

  // A structure used to unpack the arguments of calls to
  // nanotube_channel_read and nanotube_channel_write.
  struct channel_read_write_args
  {
    channel_read_write_args(const Instruction *insn);
    Value *context;
    nanotube_channel_id_t channel_id;
    Value *data;
    uint32_t data_size;
  };

  // A structure used to unpack the arguments of calls to
  // nanotube_thread_create.
  struct thread_create_args
  {
    static const unsigned context_arg_num = 0;
    static const unsigned name_arg_num = 1;
    static const unsigned func_arg_num = 2;
    static const unsigned info_area_arg_num = 3;
    static const unsigned info_area_size_arg_num = 4;

    thread_create_args(const Instruction *insn);
    Value *context;
    std::string name;
    Function *func;
    Value *info_area;
    uint32_t info_area_size;
  };
  static const unsigned THREAD_FUNC_INFO_ARG = 1;

  struct add_plain_packet_kernel_args
  {
    add_plain_packet_kernel_args(const Instruction *insn);
    std::string name;
    Function *func;
    int32_t bus_type;
    int32_t is_capsule;
  };
  static const unsigned ADD_PLAIN_PACKET_KERNEL_FUNC_ARG = 1;
  static const unsigned ADD_PLAIN_PACKET_KERNEL_BUS_TYPE_ARG = 2;
  static const unsigned ADD_PLAIN_PACKET_KERNEL_CAPSULES_ARG = 3;

  // A structure used to unpack the arguments of calls to
  // nanotube_context_add_channel.
  struct context_add_channel_args
  {
    context_add_channel_args(const Instruction *insn);
    Value *context;
    Value *channel_id;
    Value *channel;
    nanotube_channel_flags_t flags;
  };

  // A structure used to unpack the arguments of calls to
  // nanotube_debug_trace.
  struct debug_trace_args
  {
    debug_trace_args(const Instruction *insn);
    Value *id;
    Value *value;
  };

  // Information about nanotube_packet_bounded_length.
  static const unsigned PACKET_BOUNDED_LENGTH_MAX_LEN_ARG = 1;

  // Information about nanotube_packet_get_port.
  static const unsigned PACKET_GET_PORT_PACKET_ARG = 0;

  // Information about nanotube_packet_set_port.
  static const unsigned PACKET_SET_PORT_PACKET_ARG = 0;
  static const unsigned PACKET_SET_PORT_PORT_ARG = 1;

  /* Unpack arguments of nanotube_packet_read */
  struct packet_read_args {
    packet_read_args(Instruction* inst);
    Value* packet;
    Value* data_out;
    Value* offset;
    Value* length;
  };
  static const unsigned PACKET_READ_OFFSET_ARG = 2;

  /* Unpack arguments of nanotube_packet_write */
  struct packet_write_args {
    packet_write_args(Instruction* inst);
    Value* packet;
    Value* data_in;
    Value* mask;
    Value* offset;
    Value* length;
  };
  static const unsigned PACKET_WRITE_OFFSET_ARG = 2;
  static const unsigned PACKET_WRITE_OFFSET_BIT_WIDTH = 64;
  static const unsigned PACKET_WRITE_LENGTH_ARG = 3;
  static const unsigned PACKET_WRITE_LENGTH_BIT_WIDTH = 64;

  // Information about nanotube_packet_write_masked.
  static const unsigned PACKET_WRITE_MASKED_OFFSET_ARG = 3;

  /* Unpack arguments of nanotube_packet_bounded_length */
  struct packet_bounded_length_args {
    packet_bounded_length_args(Instruction* inst);
    Value* packet;
    Value* max_length;
  };
  static const unsigned PACKET_BOUNDED_LENGTH_ARG = 1;
  static const unsigned PACKET_BOUNDED_LENGTH_BIT_WIDTH = 64;

  /* Unpack arguments of nanotube_packet_drop */
  struct packet_drop_args {
    packet_drop_args(Instruction* inst);
    Value* packet;
    Value* drop;
  };

  /* Unpack arguments of nanotube_packet_resize */
  struct packet_resize_args {
    packet_resize_args(Instruction* inst);
    Value* packet;
    Value* offset;
    Value* adjust;
  };
  // Information about nanotube_packet_resize.
  static const unsigned PACKET_RESIZE_OFFSET_ARG = 1;
  static const unsigned PACKET_RESIZE_ADJUST_ARG = 2;

  /* Unpack arguments of nanotube_packet_resize_ingress */
  struct packet_resize_ingress_args {
    packet_resize_ingress_args(Instruction* inst);
    Value* out_cword;
    Value* packet_length_out;
    Value* packet;
    Value* offset;
    Value* adjust;
  };

  /* Unpack arguments of nanotube_packet_resize_egress */
  struct packet_resize_egress_args {
    packet_resize_egress_args(Instruction* inst);
    Value* packet;
    Value* cword;
    Value* new_length;
  };

  struct add_plain_kernel_args {
    add_plain_kernel_args(Instruction* inst);
    std::string name;
    Function*   kernel;
    int32_t     bus_type;
    bool        is_capsule;
  };

  struct map_create_args {
    map_create_args(Instruction* inst);
    nanotube_map_id_t id;
    enum map_type_t   type;
    Value*            key_sz;
    Value*            value_sz;
  };

  struct context_add_map_args {
    context_add_map_args(Instruction* inst);
    Value* context;
    Value* map;
  };

  struct map_op_args {
    map_op_args(Instruction* inst);
    Value*            ctx;
    nanotube_map_id_t map;
    Value*            type;
    Value*            key;
    Value*            key_length;
    Value*            data_in;
    Value*            data_out;
    Value*            mask;
    Value*            offset;
    Value*            data_length;
  };

  /* Splits of map_op, internal to the Pipeline pass. */
  struct map_op_send_args {
    map_op_send_args(Instruction* inst);
    Value*            ctx;
    nanotube_map_id_t map;
    Value*            type;
    Value*            key;
    Value*            key_length;
    Value*            data_in;
    Value*            mask;
    Value*            offset;
    Value*            data_length;
  };
  struct map_op_receive_args {
    map_op_receive_args(Instruction* inst);
    Value*            ctx;
    nanotube_map_id_t map;
    Value*            data_out;
    Value*            data_length;
  };
  /* Splits of map_op END */

  /**
   * Examine the call and return a more precise MemoryLocation for Nanotube
   * intrinsics and daisy-chain to the existing implementation for others.
   */
  MemoryLocation get_memory_location(const CallBase* call, unsigned arg_idx,
                                     const TargetLibraryInfo& tli);
  int get_size_arg(const CallBase* call, unsigned idx, bool* bits);
  bool get_max_value(Value* v, unsigned* max, bool* precise);
  /********** Helpers for reporting enum values **********/

  const std::string &get_enum_name(nanotube_channel_type_t t);

  /********** Helpers to create LLVM IR for intrinsics **********/
  Constant* create_nt_map_create(Module& m);
  Constant* create_nt_map_lookup(Module& m);
  Constant* create_nt_map_op(Module& m);
  Constant* create_nt_packet_data(Module& m);
  Constant* create_nt_packet_end(Module& m);
  Constant* create_nt_packet_meta(Module& m);
  Constant* create_nt_packet_bounded_length(Module& m);
  Constant* create_nt_packet_drop(Module& m);
  Constant* create_nt_packet_resize(Module& m);
  Constant* create_nt_packet_resize_ingress(Module& m);
  Constant* create_nt_packet_resize_egress(Module& m);
  Constant* create_nt_packet_drop(Module& m);
  Constant* create_nt_get_time_ns(Module& m);
  FunctionType* get_nt_packet_resize_ingress_ty(Module& m);
  FunctionType* get_nt_packet_resize_egress_ty(Module& m);
  FunctionType* get_nt_packet_drop_ty(Module& m);

  Constant* create_nt_packet_read(Module& m);
  Constant* create_nt_packet_write(Module& m);
  Constant* create_nt_packet_write_masked(Module& m);
  Constant* create_nt_map_read(Module& m);
  Constant* create_nt_map_write(Module& m);
  Constant* create_map_op_adapter(Module& m);
  Constant* create_packet_adjust_head_adapter(Module& m);
  Constant* create_packet_adjust_meta_adapter(Module& m);
  Constant* create_packet_handle_xdp_result(Module& m);
  Constant* create_nt_merge_data_mask(Module& m);
  Constant* create_bus_packet_set_port(Module& m, nanotube_bus_id_t bus_type);
  FunctionType* get_capsule_classify_ty(Module& m);
  Constant* create_capsule_classify(Module& m, nanotube_bus_id_t bus_type);

  StructType* get_nt_tap_packet_read_req_ty(Module& m);
  StructType* get_nt_tap_packet_read_resp_ty(Module& m);
  StructType* get_nt_tap_packet_read_state_ty(Module& m);
  FunctionType* get_tap_packet_read_ty(Module& m);
  Constant* create_tap_packet_read(Module& m, nanotube_bus_id_t bus_type);

  StructType* get_nt_tap_packet_write_req_ty(Module& m);
  StructType* get_nt_tap_packet_write_state_ty(Module& m);
  FunctionType* get_tap_packet_write_ty(Module& m);
  Constant* create_tap_packet_write(Module& m, nanotube_bus_id_t bus_type);

  StructType* get_nt_tap_packet_length_req_ty(Module& m);
  StructType* get_nt_tap_packet_length_resp_ty(Module& m);
  StructType* get_nt_tap_packet_length_state_ty(Module& m);
  FunctionType* get_tap_packet_length_ty(Module& m);
  Constant* create_tap_packet_length(Module& m, nanotube_bus_id_t bus_type);

  StructType* get_nt_tap_packet_resize_cword_ty(Module& m);
  StructType* get_nt_tap_packet_resize_req_ty(Module& m);

  StructType*   get_nt_tap_packet_resize_ingress_state_ty(Module& m);
  FunctionType* get_tap_packet_resize_ingress_ty(Module& m);
  Constant*     create_tap_packet_resize_ingress(Module& m, nanotube_bus_id_t bus_type);
  FunctionType* get_tap_packet_resize_ingress_state_init_ty(Module& m);
  Constant*     create_tap_packet_resize_ingress_state_init(Module& m);

  StructType*   get_nt_tap_packet_resize_egress_state_ty(Module& m);
  FunctionType* get_tap_packet_resize_egress_ty(Module& m);
  Constant*     create_tap_packet_resize_egress(Module& m, nanotube_bus_id_t bus_type);
  FunctionType* get_tap_packet_resize_egress_state_init_ty(Module& m);
  Constant*     create_tap_packet_resize_egress_state_init(Module& m);

  StructType*   get_nt_tap_packet_eop_state_ty(Module& m);
  FunctionType* get_tap_packet_is_eop_ty(Module& m);
  Constant*     create_tap_packet_is_eop(Module& m, nanotube_bus_id_t bus_type );

  /* Channels */
  Type*         get_nt_channel_id_ty(Module& m);
  unsigned      get_nt_channel_type_bits();
  Type*         get_nt_channel_type_ty(Module& m);
  unsigned      get_nt_channel_flags_bits();
  Type*         get_nt_channel_flags_ty(Module& m);
  unsigned      get_nt_channel_attr_id_bits();
  Type*         get_nt_channel_attr_id_ty(Module& m);
  unsigned      get_nt_channel_attr_val_bits();
  Type*         get_nt_channel_attr_val_ty(Module& m);
  StructType*   get_nt_channel_ty(Module& m);
  FunctionType* get_nt_channel_create_ty(Module& m);
  FunctionType* get_nt_channel_set_attr_ty(Module& m);
  FunctionType* get_nt_channel_export_ty(Module& m);

  Constant* create_nt_channel_read(Module& m);
  Constant* create_nt_channel_try_read(Module& m);
  Constant* create_nt_channel_write(Module& m);
  Constant* create_nt_channel_create(Module& m);
  Constant* create_nt_channel_set_attr(Module& m);
  Constant* create_nt_channel_export(Module& m);

  /* Threads */
  FunctionType* get_nt_thread_create_ty(Module& m);
  FunctionType* get_nt_thread_func_ty(Module& m);

  Constant* create_nt_thread_wait(Module& m);
  Constant* create_nt_thread_create(Module& m);

  /* Context */
  StructType*  get_nt_context_type(Module& m);
  FunctionType* get_nt_context_create_ty(Module& m);
  FunctionType* get_nt_context_add_channel_ty(Module& m);
  FunctionType* get_nt_context_add_map_ty(Module& m);

  Constant* create_nt_context_create(Module& m);
  Constant* create_nt_context_add_channel(Module& m);
  Constant* create_nt_context_add_map(Module& m);

  /* Kernels */
  static const unsigned KERNEL_CONTEXT_ARG = 0;
  static const unsigned KERNEL_PACKET_ARG = 1;
  static const unsigned KERNEL_NUM_ARGS = 2;
  Constant* create_nt_add_plain_packet_kernel(Module& m);

  /* Maps */
  FunctionType* get_nt_map_op_send_ty(Module& m);
  Constant* create_nt_map_op_send(Module& m);
  FunctionType* get_nt_map_op_receive_ty(Module& m);
  Constant* create_nt_map_op_receive(Module& m);
  FunctionType* get_nt_map_process_capsule_ty(Module& m);
  Constant* create_nt_map_process_capsule(Module& m);

  FunctionType* get_nt_tap_map_recv_resp_ty(Module& m);
  Constant* create_nt_tap_map_recv_resp(Module& m);
  FunctionType* get_nt_tap_map_send_req_ty(Module& m);
  Constant* create_nt_tap_map_send_req(Module& m);
  FunctionType* get_nt_tap_map_create_ty(Module& m);
  Constant* create_nt_tap_map_create(Module& m);
  FunctionType* get_nt_tap_map_add_client_ty(Module& m);
  Constant* create_nt_tap_map_add_client(Module& m);
  FunctionType* get_nt_tap_map_build_ty(Module& m);
  Constant* create_nt_tap_map_build(Module& m);

  /* Data Types */
  StructType*  get_nt_map_type(Module& m);
  IntegerType* get_nt_map_id_type(Module& m);
  StructType*  get_nt_packet_type(Module& m);
  Type*        get_simple_bus_word_ty(Module& m);
  Type*        get_nt_tap_offset_ty(Module& m);
  Type*        get_nt_map_result_ty(Module& m);
  Type*        get_nt_tap_map_ty(Module& m);
  Type*        get_nt_map_width_ty(Module& m);
  Type*        get_nt_map_depth_ty(Module& m);

  /* Constants */
  uint64_t     get_simple_bus_word_control_offs();
  uint8_t      get_simple_bus_word_control_eop();
  static const int SIMPLE_BUS_DATA_BYTES = 64;

  static inline
  bool is_nt_packet_kernel(Function& f) {
    auto* fty = f.getFunctionType();
    auto* nt_ctx_p = get_nt_context_type(*f.getParent())->getPointerTo();
    auto* nt_pkt_p = get_nt_packet_type(*f.getParent())->getPointerTo();
    return (fty->getNumParams() == KERNEL_NUM_ARGS) &&
           (fty->getParamType(KERNEL_CONTEXT_ARG) == nt_ctx_p) &&
           (fty->getParamType(KERNEL_PACKET_ARG) == nt_pkt_p) &&
           (fty->getReturnType()->isIntegerTy());
  }

  /********** Helpers to query Nanotube API calls **********/
  class nt_api_call {
  public:
    nt_api_call(CallInst* call) { initialise_from(call); }
    nt_api_call() {}

    static void set_tli(const TargetLibraryInfo& tli_) { tli = &tli_; }
    void initialise_from(CallInst* call);

    bool is_packet();
    bool is_map();
    bool is_read();
    bool is_write();
    bool is_access();

    llvm::CallInst* get_call() const { return call; }
    Intrinsics::ID  get_intrinsic() const { return intrin; }

    unsigned arg_size() { return call->arg_size(); }
    Value* get_context();
    Value* get_packet();

    Value* get_data_ptr();
    Value* get_mask_ptr();

    int get_map_op();
    Value* get_map_op_arg();
    int get_map_op_arg_idx();

    Value* get_dummy_arg(unsigned arg_idx);
    MemoryLocation get_memory_location(unsigned arg_idx);
    Value* get_size_arg(unsigned arg_idx, bool* bits);
    std::vector<MemoryLocation> get_input_mem();
    std::vector<MemoryLocation> get_output_mem();

    int    get_data_size_arg_idx();
    Value* get_data_size_arg();
    int    get_max_data_size();

    int    get_key_size_arg_idx();
    Value* get_key_size_arg();
    int    get_max_key_size();
  private:
    static const TargetLibraryInfo* tli;

    llvm::CallInst*                 call;
    Intrinsics::ID                  intrin;
  };

  /* Information about NT API calls for Alias Analysis */
  llvm::ModRefInfo get_nt_arg_info(Intrinsics::ID intr, unsigned arg_idx);
  llvm::FunctionModRefBehavior get_nt_fmrb(Intrinsics::ID intr);
}

#endif // NANOTUBE_INTRINSICS_H
