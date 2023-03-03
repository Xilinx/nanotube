/*******************************************************/
/*! \file Byteify.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert wide loads and stores into bytes.
**   \date 2020-01-10
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/*!
** LLVM optimisation pass that converts wide loads and stores into byte
** accesses with appropriate merging / splitting.
**/

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#define DEBUG_TYPE "byteify"
using namespace llvm;

namespace {
struct Byteify : public FunctionPass {
  static char ID;
  // Note: using Big-Endian will need the constants to be swizzled in the program, too
  static const bool little_endian = true;
  Byteify() : FunctionPass(ID) {}

  void static print_all_sizes(raw_ostream &o, PointerType *ptr,
                              const DataLayout &DL) {
    Type *tgt = ptr->getElementType();
    o << "TypeStoreSize: "   << DL.getTypeStoreSize(ptr)
      << " TypeStoreSizeInBits: " << DL.getTypeStoreSizeInBits(ptr)
      << " TypeSizeInBits: " << DL.getTypeSizeInBits(ptr)
      << " PointerTypeSize: " << DL.getPointerTypeSize(ptr)
      << " PointerTypeSizeInBits: " << DL.getPointerTypeSizeInBits(ptr)
      << " IndexTypeSizeInBits: " << DL.getIndexTypeSizeInBits(ptr);

    o << " | TypeStoreSize: "   << DL.getTypeStoreSize(tgt)
      << " TypeStoreSizeInBits: " << DL.getTypeStoreSizeInBits(tgt)
      << " TypeSizeInBits: " << DL.getTypeSizeInBits(tgt);
    o << '\n';
  }

  void static print_inst_info(raw_ostream &o, const Instruction &inst) {
    o << "Inst: " << inst << '\n';

    // Go through the operands to discover the sizes
    for (auto op = inst.op_begin(); op != inst.op_end(); ++op) {
      Value *v = op->get();
      o << "Op: " << *v << '\n';
      Type *t  = v->getType();
      o << "Type: " << *t << '\n';
    }
  }

  /**
   * Convert a memory access (load to variable X) of a simple type of
   * length N bytes into a byte access sequence.
   *
   * More detail::
   *   * convert original pointer to a pointer to an array with N
   *   bytes
   *   * use GEP to get pointers to the separate bytes
   *   * issue loads from these bytes
   *   * and fiddle the resulting bytes into the right places in the original
   *   type and variable X
   *
   * C-like:
   *   u8 *input = (u8 *)addr;
   *   res32  = (u32)input[0];
   *   res32 |= (u32)input[1]  << 8;
   *   res32 |= (u32)input[2] << 16;
   *   res32 |= (u32)input[3] << 24;
   */
  bool convert_load(BasicBlock::iterator &bi) {
    LoadInst *ld = dyn_cast<LoadInst>(bi);
    assert(ld != nullptr);

    auto *F  = ld->getFunction();
    auto &DL = F->getParent()->getDataLayout();
    auto &C  = F->getContext();

    auto *u8_ty  = IntegerType::getInt8Ty(C);
    auto *u8p_ty = u8_ty->getPointerTo();

    LLVM_DEBUG(print_inst_info(dbgs(), *ld));

    // Get pointer
    auto *addr_op  = ld->getPointerOperand();
    auto *ptr      = cast<PointerType>(addr_op->getType());

    // Pull out the target that tells us the size of the load
    Type *tgt_ty   = ptr->getElementType();
    auto bytes     = DL.getTypeStoreSize(tgt_ty);

    LLVM_DEBUG(dbgs() << "Size: " << bytes << '\n');
    LLVM_DEBUG(print_all_sizes(dbgs(), ptr, DL));

    // Access is already a single byte -> skip
    if (bytes == 1)
      return false;

    // Step 1: pointer to intN -> pointer to i8
    // NOTE: Previously this used an array which is "mathematically"
    // nicer, but creates ugly code in the C back-end
    // errs() << "BitCast: " << *addr_op << " => " << *u8p_ty << '\n';
    auto *bc    = new BitCastInst(addr_op, u8p_ty, "base", ld);
    LLVM_DEBUG(dbgs() << *bc << '\n');

    // Force data to be read to always be an integer
    Type *mix_ty = IntegerType::get(C, bytes * 8);

    // Step 2: Generate per-byte access sequences
    Value *last_val = nullptr;
    for (unsigned b = 0; b < bytes; b++) {
      // Endianness: simply mirror the indexing into memory
      int32_t mem_byte = little_endian ? b : (bytes - b - 1);

      // Get a pointer to ptr[b]
      // GEP i8, i8* base, 0, b
      auto *idx  = ConstantInt::get(Type::getInt32Ty(C), mem_byte);
      Value *a[] = {idx};
      auto *gep  = GetElementPtrInst::Create(u8_ty, bc, a, "elemp", ld);
      LLVM_DEBUG(dbgs() << *gep << '\n');

      // Load from the pointer
      // Load i8, i8* elemp
      auto *ld8  = new LoadInst(u8_ty, gep, "val8", ld);
      LLVM_DEBUG(dbgs() << *ld8 << '\n');

      // Zero-extend the byte to the original width
      // zext i8 val8 to i(i*8)
      auto *zext = new ZExtInst(ld8, mix_ty, "wide", ld);
      LLVM_DEBUG(dbgs() << *zext << '\n');

      if (b == 0) {
        // Lowest byte does not need shifting / ORing
        last_val = zext;
        continue;
      }

      // Shift the byte in the right place in the wider word
      // shl i(N*8) wide, b * 8
      // NB: the constant must be of the same type as the value
      auto *n   = ConstantInt::get(mix_ty, b * 8);
      auto *shl = BinaryOperator::Create(Instruction::BinaryOps::Shl,
            zext, n, "shifted", ld);
      LLVM_DEBUG(dbgs() << *shl << '\n');

      // Combine this byte with values loaded so far
      // Or i(N*8) shifted, ORed
      assert(last_val != nullptr);
      auto *ored = BinaryOperator::Create(Instruction::BinaryOps::Or,
             shl, last_val, "ORed", ld);
      LLVM_DEBUG(dbgs() << *ored << '\n');
      last_val = ored;
    }

    // The value now lives in last_val so..
    // Step 3: in case the load read a pointer, cast it back
    if (tgt_ty->isPointerTy()) {
      auto *i2p = new IntToPtrInst(last_val, tgt_ty, "casted", ld);
      LLVM_DEBUG(dbgs() << *i2p << '\n');
      last_val = i2p;
    }
    // Step 4: Remove the load, and replace all uses with last_val
    assert(last_val != nullptr);
    ReplaceInstWithValue(ld->getParent()->getInstList(), bi, last_val);
    // Unwind the iterator to before the deleted load
    --bi;
    return true; // we made changes
  }

  /**
   * Convert store operation of width N bytes into N separate stores to the
   * respecitve bytes of the memory location.
   *
   * More detail
   *   * convert original pointer to a pointer to an array with N
   *   bytes
   *   * use GEP to get pointers to the separate bytes
   *   * extract the data bytes one-by-one by shifting & truncating
   *   * store each byte to its address
   *
   * C-like:
   *   u8 *dest = (u8 *)addr;
   *   dest[0] = (u8)data;
   *   dest[1] = (u8)(data >>  8);
   *   dest[2] = (u8)(data >> 16);
   *   dest[3] = (u8)(data >> 24);
   */
  bool convert_store(BasicBlock::iterator &bi) {
    StoreInst *st = dyn_cast<StoreInst>(bi);
    assert(st != nullptr);

    auto *F  = st->getFunction();
    auto &DL = F->getParent()->getDataLayout();
    auto &C  = F->getContext();

    auto *u8_ty  = IntegerType::getInt8Ty(C);
    auto *u8p_ty = u8_ty->getPointerTo();

    LLVM_DEBUG(print_inst_info(dbgs(), *st));

    // Get pointer
    auto *addr_op  = st->getPointerOperand();
    auto *type     = addr_op->getType();
    assert(type->isPointerTy());
    PointerType *ptr = dyn_cast<PointerType>(type);

    // Pull out the target that tells us the size of the store
    Type *tgt_ty   = ptr->getElementType();
    auto bytes     = DL.getTypeStoreSize(tgt_ty);
    LLVM_DEBUG(dbgs() << "Size: " << bytes << '\n');

    // print_all_sizes(type);

    // Access is already a single byte -> skip
    if (bytes == 1)
      return false;

    // Step 1: pointer to intN -> pointer to i8
    // NOTE: Previously this used an array which is "mathematically"
    // nicer, but creates ugly code in the C back-end
    // errs() << "BitCast: " << *addr_op << " => " << *arrp_ty << '\n';
    auto *bc    = new BitCastInst(addr_op, u8p_ty, "base", st);
    LLVM_DEBUG(dbgs() << *bc << '\n');

    // Step 2: Generate per-byte access sequences
    Value *data = st->getValueOperand();
    LLVM_DEBUG(dbgs() << *data << '\n');
    if (data->getType()->isPointerTy()) {
      // Data to be stored is a pointer -> convert to integer
      tgt_ty = IntegerType::get(C, bytes * 8);
      data = new PtrToIntInst(data, tgt_ty, "casted", st);
      LLVM_DEBUG(dbgs() << *data << '\n');
    }

    for (unsigned b = 0; b < bytes; b++) {
      // Shift the wanted data to the lowest byte
      Value *data_shifted = data;
      if (b) {
        auto *n   = ConstantInt::get(tgt_ty, b * 8);
        auto *shr = BinaryOperator::Create(Instruction::BinaryOps::LShr,
              data, n, "shifted", st);
        data_shifted = shr;
      }
      LLVM_DEBUG(dbgs() << *data_shifted << '\n');

      // Truncate data to i8
      auto *trunc = new TruncInst(data_shifted, u8_ty, "trunc", st);
      LLVM_DEBUG(dbgs() << *trunc << '\n');

      // Endianness: simply mirror the indexing into memory
      int32_t mem_byte = little_endian ? b : (bytes - b - 1);

      // Get a pointer to ptr[b]
      // GEP [N x i8], [N x i8]* base, 0, b
      auto *idx  = ConstantInt::get(Type::getInt32Ty(C), mem_byte);
      Value *a[] = {idx};
      auto *gep  = GetElementPtrInst::Create(u8_ty, bc, a, "elemp", st);
      LLVM_DEBUG(dbgs() << *gep << '\n');

      // Store the byte to the right location
      auto *st8 = new StoreInst(trunc, gep, st);
      LLVM_DEBUG(dbgs() << *st8 << '\n');
    }

    // Step 3: delete the old, wide store
    bi = st->eraseFromParent();
    // Unwind the iterator to before the deleted load
    --bi;

    return true;
  }

  void analyse_structs(BasicBlock::iterator &bi) {
    auto *F  = bi->getFunction();
    auto &DL = F->getParent()->getDataLayout();

    Type *ty = bi->getType();
    while (ty->isPointerTy()) {
      ty = ty->getPointerElementType();
    }
    if (!ty->isStructTy())
      return;
    LLVM_DEBUG(
      dbgs() << *ty << " Size: "
             << (ty->isSized() ? DL.getTypeStoreSize(ty) : -1) << '\n'
    );
  }
  /**
   * XXX: Add a description here
   *
   * XXX: How about bitfields?
   */
  bool runOnFunction(Function &F) override {
    LLVM_DEBUG(
      dbgs().write_escaped(F.getName());
      dbgs() << '\n'
    );

    bool changes_made = false;
    for (auto &bbl : F) {
      for (auto ii = bbl.begin(); ii != bbl.end(); ++ii) {
        auto op = ii->getOpcode();

        analyse_structs(ii);

        // We only care about loads and stores
        if ((op != Instruction::Load) && (op != Instruction::Store))
          continue;

        switch (op) {
          case Instruction::Load:
            changes_made |= convert_load(ii);
            break;
          case Instruction::Store:
            changes_made |= convert_store(ii);
            break;
          default:
            assert(false);
        }
      }
    }
    return changes_made;
  }
};
} // anonymous namespace

char Byteify::ID = 0;
static RegisterPass<Byteify>
  X("byteify", "Convert wide memory accesses to multiple byte accesses",
    false,
    false
    );

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
