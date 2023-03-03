/**************************************************************************\
*//*! \file platform.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube pass to add platform-specific code.
**   \date  2021-09-24
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

// The platform pass
// =================
//
// This pass modifies the program according to the requirements of the
// platform.  Currently, it simply modifies the offset arguments to
// all the packet access calls.  It then changes the kernel type from
// a packet kernel to a capsule kernel.
//
// Input conditions
// ----------------
//
// There is a setup function.
//
// Output conditions
// -----------------
//
// Any kernel functions are updated to take into account the capsule
// header.  The call to register the kernel is modified to register a
// capsule kernel rather than a packet kernel.
//
// Theory of operation
// --------------------
//
// The pass iterates over the setup function, finding calls which add
// packet kernels.  It updates the kernel and then updates the
// registration call to register a capsule kernel rather than a packet
// kernel.
//
// To update a kernel, it iterates over the instructions and locates
// intrinsics which access packets.  For each one, it updates the
// offset argument and/or result to take account of the capsule
// header.

#define DEBUG_TYPE "platform"

#include "bus_type.hpp"
#include "Intrinsics.h"
#include "llvm_common.h"
#include "llvm_pass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "setup_func.hpp"
#include "simple_bus.hpp"
#include "utils.h"

#include <cstddef>
#include <iostream>

using namespace nanotube;

///////////////////////////////////////////////////////////////////////////

namespace {
  class platform: public llvm::ModulePass {
  public:
    static char ID;

    platform();
    StringRef getPassName() const override {
      return "Insert Nanotube platform-specific code ";
    }
    void getAnalysisUsage(AnalysisUsage &info) const override;

    void adjust_offset_arg(Instruction *insn, int offset_arg);
    void adjust_return(Instruction *insn);
    void adjust_set_port(Instruction *insn);
    void process_packet_kernel(CallBase *call);
    bool runOnModule(Module &m) override;

  private:
    nanotube_packet_size_t m_md_size;
    enum nanotube_bus_id_t m_bus_type;
  };
};

///////////////////////////////////////////////////////////////////////////

char platform::ID;

platform::platform():
  ModulePass(ID),
  m_bus_type(get_bus_type())
{
}

void platform::getAnalysisUsage(AnalysisUsage &info) const
{
}

void platform::adjust_offset_arg(Instruction *insn, int offset_arg)
{
  auto *call = cast<CallBase>(insn);
  Value *val = call->getArgOperand(offset_arg);
  auto *int_ty = dyn_cast<IntegerType>(val->getType());
  if (int_ty == nullptr) {
    report_fatal_errorv("Invalid type for offset operand {0} of {1}",
                        offset_arg, *insn);
  }
  auto bit_width = int_ty->getBitWidth();
  auto offset = APInt(bit_width, m_md_size);
  auto const_offset = ConstantInt::get(int_ty, offset);
  auto ir = llvm::IRBuilder<>(insn);
  Value *new_val = ir.CreateAdd(val, const_offset);
  call->setArgOperand(offset_arg, new_val);
}

void platform::adjust_return(Instruction *insn)
{
  auto *int_ty = cast<IntegerType>(insn->getType());
  auto bit_width = int_ty->getBitWidth();

  // Subtract the length of the capsule header from the result since
  // the return from the call will be larger than expected.
  auto offset = APInt(bit_width, m_md_size);
  auto const_offset = ConstantInt::get(int_ty, -offset);
  auto *add = BinaryOperator::Create(Instruction::Add, insn,
                                     const_offset);
  add->insertAfter(insn);
  insn->replaceAllUsesWith(add);
  // Set the operand to add back to the result of the call since
  // replaceAllUsesWith will have changed it.
  add->setOperand(0, insn);
}

void platform::adjust_set_port(Instruction *insn)
{
  // Replace:
  //   call @nanotube_packet_set_port(%packet, i<8n> %port)
  // with:
  //   call @nanotube_packet_set_port_sb(%packet, i<8n> %port)

  Module *m = insn->getModule();

  // Replace the function. 
  auto *call = cast<CallBase>(insn);
  auto *set_port = create_bus_packet_set_port(*m, m_bus_type);
  call->setCalledFunction(set_port);
}

void platform::process_packet_kernel(CallBase *setup_call)
{
  add_plain_packet_kernel_args args(setup_call);

  // Clone the function in case it is used elsewhere.
  llvm::ValueToValueMapTy values;
  Function *new_func = CloneFunction(args.func, values);
  new_func->setLinkage(Function::PrivateLinkage);

  setup_call->setArgOperand(ADD_PLAIN_PACKET_KERNEL_FUNC_ARG, new_func);

  if (args.func->user_begin() == args.func->user_end()) {
    LLVM_DEBUG(dbgv("Removing unused function {0}.\n", as_operand(args.func)););
    args.func->eraseFromParent();
  }

  for (auto &bb: *new_func) {
    for (auto it = bb.begin(); it != bb.end(); ) {
      auto &insn = *it;
      ++it;
      auto iid = get_intrinsic(&insn);
      switch (iid) {
      case Intrinsics::packet_read:
        adjust_offset_arg(&insn, PACKET_READ_OFFSET_ARG);
        break;

      case Intrinsics::packet_write:
        adjust_offset_arg(&insn, PACKET_WRITE_OFFSET_ARG);
        break;

      case Intrinsics::packet_write_masked:
        adjust_offset_arg(&insn, PACKET_WRITE_MASKED_OFFSET_ARG);
        break;

      case Intrinsics::packet_edit:
        assert(false);

      case Intrinsics::packet_bounded_length:
        adjust_offset_arg(&insn, PACKET_BOUNDED_LENGTH_MAX_LEN_ARG);
        adjust_return(&insn);
        break;

      case Intrinsics::packet_set_port:
        adjust_set_port(&insn);
        break;

      case Intrinsics::packet_resize:
        adjust_offset_arg(&insn, PACKET_RESIZE_OFFSET_ARG);
        break;

      case Intrinsics::packet_get_port:
      case Intrinsics::packet_data:
      case Intrinsics::packet_end:
        report_fatal_errorv("Unsupported call in platform pass: {0}",
                            insn);

      default:
        continue;
      }
    }
  }
}

bool platform::runOnModule(Module &module)
{
  // Determine the metadata size.
  m_md_size = get_bus_md_size();

  // Find calls to nanotube_add_plain_packet_kernel.
  bool changes = false;
  Function &func = setup_func::get_setup_func(module);
  for (auto &bb: func) {
    for (auto &insn: bb) {
      // Only process calls to nanotube_add_plain_packet_kernel.
      auto iid = get_intrinsic(&insn);
      if (iid != Intrinsics::add_plain_packet_kernel)
        continue;

      // Update the kernel itself.
      auto *call = cast<CallBase>(&insn);
      process_packet_kernel(call);

      // Update the call to change arguments to use capsules.
      Value *capsules_val;
      capsules_val = call->getArgOperand(ADD_PLAIN_PACKET_KERNEL_CAPSULES_ARG);
      ConstantInt *old_capsules = dyn_cast<ConstantInt>(capsules_val);
      if(old_capsules == nullptr) {
        report_fatal_errorv("Invalid type for nanotube_add_plain_packet_kernel capsules operand {0}",
                            ADD_PLAIN_PACKET_KERNEL_CAPSULES_ARG);
      }
      if(old_capsules->isZero()) {
        auto *int_ty = cast<IntegerType>(capsules_val->getType());
        call->setArgOperand(ADD_PLAIN_PACKET_KERNEL_CAPSULES_ARG, ConstantInt::get(int_ty, 1));
        Value *bus_type_val = call->getArgOperand(ADD_PLAIN_PACKET_KERNEL_BUS_TYPE_ARG);
        int_ty = cast<IntegerType>(bus_type_val->getType());
        call->setArgOperand(ADD_PLAIN_PACKET_KERNEL_BUS_TYPE_ARG, ConstantInt::get(int_ty, get_bus_type()));
      }

      changes = true;
    }
  }
  return changes;
}

///////////////////////////////////////////////////////////////////////////

static RegisterPass<platform>
register_pass("platform", "Insert Nanotube platform-specific code",
               false,
               false
  );

///////////////////////////////////////////////////////////////////////////
