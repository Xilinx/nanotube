/**************************************************************************\
*//*! \file control_capsule.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube pass to add control capsule processing code.
**   \date  2022-07-29
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

// The control capsule pass
// =========================
//
// This pass adds code to the packet kernel which will process control
// capsules.  Currently, control capsules can be used to access maps.
//
// Input conditions
// ----------------
//
// There is a setup function which registers a capsule processing
// kernel.
//
// Output conditions
// -----------------
//
// The capsule kernel will have been modified to include code which
// processes control capsules.
//
// Theory of operation
// --------------------
//
// The pass creates a setup_func object to parse the setup function.
// It then locates the capsule kernel.  Next it enumerates the maps
// and adds code to the capsule kernel to process those maps.
//
// To add code to capsule kernel, it creates a new entry basic block
// which determines the type of capsule being processed and then
// dispatches to the appropriate code.  If the capsule is a network
// packet, it dispatches to the original entry block.

#define DEBUG_TYPE "control-capsule"

#include "Intrinsics.h"
#include "llvm_common.h"
#include "llvm_pass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "nanotube_capsule.h"
#include "setup_func.hpp"
#include "utils.h"

using namespace nanotube;
using llvm::IRBuilder;
using llvm::Twine;

///////////////////////////////////////////////////////////////////////////

namespace {
  typedef std::vector<const map_info *> map_vec_t;

  class cc_code_builder {
  public:
    cc_code_builder(const kernel_info *a_kernel, const map_vec_t *a_maps);
    cc_code_builder() = delete;
    cc_code_builder(const cc_code_builder &) = delete;
    cc_code_builder &operator =(const cc_code_builder &) = delete;

    size_t get_buffer_length(const map_info *map);
    size_t get_buffer_length();

    void create_pass_through_bb();
    void read_capsule();
    void write_capsule();
    void create_capsule_write_bb();
    void write_response_code(uint16_t code);
    void create_capsule_bad_resource_bb();
    void create_capsule_read_bb();
    void create_map_bb(uint64_t index, const map_info *info);
    void create_map_bbs();
    void create_entry_bb();
    void finish_entry_bb();

    void build();

  private:
    // Provided resources.
    const kernel_info *m_kernel = nullptr;
    const map_vec_t *m_maps = nullptr;
    nanotube_bus_id_t m_bus_type;
    Function *m_func = nullptr;
    Module *m_module = nullptr;
    LLVMContext *m_context = nullptr;
    BasicBlock *m_old_entry_bb = nullptr;
    IRBuilder<> m_builder;

    // Created resources.
    Argument *m_context_arg = nullptr;
    Argument *m_packet_arg = nullptr;
    BasicBlock *m_new_entry_bb = nullptr;
    BasicBlock *m_pass_through_bb = nullptr;
    BasicBlock *m_capsule_write_bb = nullptr;
    BasicBlock *m_capsule_bad_resource_bb = nullptr;
    BasicBlock *m_capsule_read_bb = nullptr;
    SwitchInst *m_resource_switch = nullptr;
    Value *m_capsule_buffer = nullptr;
    Value *m_length_val = nullptr;
    SwitchInst *m_map_switch = nullptr;
  };

  class control_capsule_pass: public llvm::ModulePass {
  public:
    static char ID;

    control_capsule_pass();
    StringRef getPassName() const override {
      return "Insert control capsule handling code";
    }
    void getAnalysisUsage(AnalysisUsage &info) const override;

    bool runOnModule(Module &m) override;
  };
}

///////////////////////////////////////////////////////////////////////////

cc_code_builder::cc_code_builder(const kernel_info *kernel,
                                 const map_vec_t *maps):
  m_kernel(kernel),
  m_maps(maps),
  m_bus_type(nanotube_bus_id_t(kernel->args().bus_type)),
  m_func(kernel->args().kernel),
  m_module(m_func->getParent()),
  m_context(&(m_module->getContext())),
  m_builder(*m_context)
{
  // Get the packet pointer.
  if (m_func->arg_size() != KERNEL_NUM_ARGS) {
    auto *ty = m_func->getType();
    report_fatal_errorv("Invalid prototype for capsule kernel: {0}", *ty);
  }

  // Clone the function to avoid changing the behaviour of other users
  // of it.
  llvm::ValueToValueMapTy values;
  Function *new_func = CloneFunction(m_func, values);
  new_func->setLinkage(Function::PrivateLinkage);

  CallBase *call = kernel->creator();
  assert(call->getArgOperand(ADD_PLAIN_PACKET_KERNEL_FUNC_ARG) == m_func);
  call->setArgOperand(ADD_PLAIN_PACKET_KERNEL_FUNC_ARG, new_func);

  std::string func_name = m_func->getName();
  if (m_func->user_empty()) {
    LLVM_DEBUG(dbgv("Removing unused function {0}.\n", as_operand(m_func)););
    m_func->eraseFromParent();
  }
  m_func = new_func;
  m_func->setName(func_name);
  m_old_entry_bb = &(m_func->getEntryBlock());

  // Extract the function arguments.
  m_context_arg = (m_func->arg_begin() + KERNEL_CONTEXT_ARG);
  m_packet_arg = (m_func->arg_begin() + KERNEL_PACKET_ARG);
}

size_t cc_code_builder::get_buffer_length(const map_info *map)
{
  auto *key_size_val = dyn_cast_or_null<ConstantInt>(map->args().key_sz);
  auto *value_size_val = dyn_cast_or_null<ConstantInt>(map->args().value_sz);
  if (key_size_val == nullptr) {
    report_fatal_errorv("Map create call has non-constant key size: {0}",
                        *(map->args().key_sz));
  }
  if (value_size_val == nullptr) {
    report_fatal_errorv("Map create call has non-constant value size: {0}",
                        *(map->args().value_sz));
  }

  uint64_t key_size = key_size_val->getLimitedValue();
  uint64_t value_size = value_size_val->getLimitedValue();

  return NANOTUBE_CAPSULE_MAP_CAPSULE_SIZE(key_size, value_size);
}

size_t cc_code_builder::get_buffer_length()
{
  size_t result = NANOTUBE_CAPSULE_HEADER_SIZE;
  for (auto map: *m_maps) {
    size_t size = get_buffer_length(map);
    if (result < size)
      result = size;
  }
  return result;
}

void cc_code_builder::create_pass_through_bb()
{
  auto *bb = BasicBlock::Create(*m_context, "pass_through_capsule",
                                m_func, m_old_entry_bb);

  m_builder.SetInsertPoint(bb);
  m_builder.CreateRetVoid();
  m_pass_through_bb = bb;
}

void cc_code_builder::read_capsule()
{
  auto *size_ty = m_length_val->getType();
  auto offset = ConstantInt::get(size_ty, 0);

  auto *packet_read = create_nt_packet_read(*m_module);
  Value *args[] = {
    m_packet_arg,
    m_capsule_buffer,
    offset,
    m_length_val,
  };
  m_builder.CreateCall(packet_read, args, "");
}

void cc_code_builder::write_capsule()
{
  auto *size_ty = m_length_val->getType();
  auto offset = ConstantInt::get(size_ty, 0);

  auto *packet_write = create_nt_packet_write(*m_module);
  Value *args[] = {
    m_packet_arg,
    m_capsule_buffer,
    offset,
    m_length_val,
  };
  m_builder.CreateCall(packet_write, args, "");
}

void cc_code_builder::create_capsule_write_bb()
{
  BasicBlock *bb = BasicBlock::Create(*m_context, "control_capsule_write",
                                      m_func, m_old_entry_bb);
  m_builder.SetInsertPoint(bb);
  write_capsule();
  m_builder.CreateRetVoid();
  m_capsule_write_bb = bb;
}

void cc_code_builder::write_response_code(uint16_t resp_code)
{
  Type *i8_ty = m_builder.getInt8Ty();
  Value *gep;
  Value *value;
  uint64_t index;

  static_assert(NANOTUBE_CAPSULE_RESPONSE_CODE_SIZE == 2);

  index = NANOTUBE_CAPSULE_RESPONSE_CODE_OFFSET + 0;
  gep = m_builder.CreateConstInBoundsGEP1_64(i8_ty, m_capsule_buffer, index,
                                             "cc.rc0.gep");
  value = m_builder.getInt8(resp_code>>0);
  m_builder.CreateStore(value, gep);

  index = NANOTUBE_CAPSULE_RESPONSE_CODE_OFFSET + 1;
  gep = m_builder.CreateConstInBoundsGEP1_64(i8_ty, m_capsule_buffer, index,
                                             "cc.rc1.gep");
  value = m_builder.getInt8(resp_code>>8);
  m_builder.CreateStore(value, gep);
}

void cc_code_builder::create_capsule_bad_resource_bb()
{
  BasicBlock *bb = BasicBlock::Create(*m_context, "control_capsule_bad_resource",
                                      m_func, m_capsule_write_bb);

  m_builder.SetInsertPoint(bb);
  write_response_code(NANOTUBE_CAPSULE_RESPONSE_CODE_UNKNOWN_RESOURCE);
  m_builder.CreateBr(m_capsule_write_bb);
  m_capsule_bad_resource_bb = bb;
}

void cc_code_builder::create_capsule_read_bb()
{
  BasicBlock *bb = BasicBlock::Create(*m_context, "control_capsule_read",
                                      m_func, m_capsule_bad_resource_bb);
  m_builder.SetInsertPoint(bb);
  read_capsule();

  static_assert(NANOTUBE_CAPSULE_RESOURCE_ID_SIZE == 2);

  Type *i8_ty = m_builder.getInt8Ty();
  Type *i16_ty = m_builder.getInt16Ty();
  Value *gep;
  uint64_t index;

  index = NANOTUBE_CAPSULE_RESOURCE_ID_OFFSET + 0;
  gep = m_builder.CreateConstInBoundsGEP1_64(i8_ty, m_capsule_buffer, index,
                                             "cc.res_id0.get");
  Value *res_id0 = m_builder.CreateLoad(i8_ty, gep, false,
                                             "cc.res_id0.load");
  res_id0 = m_builder.CreateZExt(res_id0, i16_ty, "cc.res_id0.zext");

  index = NANOTUBE_CAPSULE_RESOURCE_ID_OFFSET + 1;
  gep = m_builder.CreateConstInBoundsGEP1_64(i8_ty, m_capsule_buffer, index,
                                             "cc.res_id1.gep");
  Value *res_id1 = m_builder.CreateLoad(i8_ty, gep, false,
                                             "cc.res_id1.load");
  res_id1 = m_builder.CreateZExt(res_id1, i16_ty, "cc.res_id1.zext");
  res_id1 = m_builder.CreateShl(res_id1, 8, "cc.res_id1.shl");

  Value *res_id = m_builder.CreateOr(res_id0, res_id1, "cc.res_id");

  m_resource_switch =
    m_builder.CreateSwitch(res_id, m_capsule_bad_resource_bb, 0);
  m_capsule_read_bb = bb;
}

void cc_code_builder::create_map_bb(uint64_t index, const map_info *info)
{
  BasicBlock *bb = BasicBlock::Create(
    *m_context, formatv("control_capsule_map{0}_bb", index),
    m_func, m_capsule_bad_resource_bb);
  m_builder.SetInsertPoint(bb);

  auto map_id = info->args().id;
  IntegerType* id_ty = get_nt_map_id_type(*m_module);
  auto *map_id_value = ConstantInt::get(id_ty, uint64_t(map_id));
  auto *proc_func = create_nt_map_process_capsule(*m_module);
  Value *args[] = {
    m_context_arg,
    map_id_value,
    m_capsule_buffer,
    info->args().key_sz,
    info->args().value_sz,
  };
  m_builder.CreateCall(proc_func, args);

  m_builder.CreateBr(m_capsule_write_bb);

  static_assert(NANOTUBE_CAPSULE_RESOURCE_ID_SIZE == 2);

  if (index > 0xffff) {
    errs() << "Too many maps for control capsule processing.\n";
    exit(1);
  }

  ConstantInt *val = m_builder.getInt16(index);
  m_resource_switch->addCase(val, bb);
}

void cc_code_builder::create_map_bbs()
{
  for (size_t index=0; index<m_maps->size(); index++) {
    create_map_bb(uint64_t(index), m_maps->at(index));
  }
}

void cc_code_builder::create_entry_bb()
{
  // Create the new basic blocks.
  BasicBlock *succ_bb = &(m_func->getEntryBlock());
  m_new_entry_bb = BasicBlock::Create(*m_context, "entry", m_func,
                                      succ_bb);
  m_builder.SetInsertPoint(m_new_entry_bb);

  const DataLayout *dl = &(m_module->getDataLayout());

  auto alloca_as = dl->getAllocaAddrSpace();
  auto ptr_bits = dl->getPointerSizeInBits(alloca_as);
  auto size_ty = Type::getIntNTy(*m_context, ptr_bits);
  auto length = get_buffer_length();
  m_length_val = ConstantInt::get(size_ty, length);
  auto int8_ty = Type::getInt8Ty(*m_context);
  m_capsule_buffer = m_builder.CreateAlloca(int8_ty, m_length_val,
                                            "control_capsule_buffer");
}

void cc_code_builder::finish_entry_bb()
{
  // Build the entry basic block.
  m_builder.SetInsertPoint(m_new_entry_bb);
  auto *classify_func = create_capsule_classify(*m_module, m_bus_type);
  Value *args[] = { m_packet_arg, };
  auto *capsule_class = m_builder.CreateCall(classify_func,
                                             args, "capsule_class");
  auto *cc_ty = cast<IntegerType>(capsule_class->getType());
  auto *sw = m_builder.CreateSwitch(capsule_class, m_pass_through_bb, 0);
  sw->addCase(ConstantInt::getSigned(cc_ty, NANOTUBE_CAPSULE_CLASS_NETWORK),
              m_old_entry_bb);
  sw->addCase(ConstantInt::getSigned(cc_ty, NANOTUBE_CAPSULE_CLASS_CONTROL),
              m_capsule_read_bb);
}

void cc_code_builder::build()
{
  // Renamed the old entry basic block.
  m_old_entry_bb->setName("process_net_packet");

  // Create the code.
  create_entry_bb();
  create_pass_through_bb();
  create_capsule_write_bb();
  create_capsule_bad_resource_bb();
  create_capsule_read_bb();
  create_map_bbs();
  finish_entry_bb();
}

///////////////////////////////////////////////////////////////////////////

char control_capsule_pass::ID;

control_capsule_pass::control_capsule_pass():
  ModulePass(ID)
{
}

void control_capsule_pass::getAnalysisUsage(AnalysisUsage &info) const
{
}

bool control_capsule_pass::runOnModule(Module &m)
{
  setup_func setup(m);
  map_vec_t maps;
  for (auto &map: setup.maps()) {
    maps.push_back(&map);
  }
  for (auto &kernel: setup.kernels()) {
    if (!kernel.args().is_capsule ||
        kernel.args().kernel->empty())
      continue;
    cc_code_builder b(&kernel, &maps);
    b.build();
    return true;
  }
  return false;
}

static RegisterPass<control_capsule_pass>
register_pass("control-capsule", "Insert control-capsule handling code",
               false,
               false
  );

///////////////////////////////////////////////////////////////////////////
