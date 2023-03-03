/**************************************************************************\
*//*! \file llvm_pass.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  LLVM headers for passes
**   \date  2019-06-27
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_LLVM_IR_H
#define NANOTUBE_LLVM_IR_H

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Pass.h"

namespace nanotube
{
  using llvm::AnalysisUsage;
  using llvm::Argument;
  using llvm::APInt;
  using llvm::BasicBlock;
  using llvm::BitCastOperator;
  using llvm::ConstantAggregate;
  using llvm::ConstantAggregateZero;
  using llvm::ConstantDataSequential;
  using llvm::ConstantExpr;
  using llvm::ConstantInt;
  using llvm::ConstantPointerNull;
  using llvm::DataLayout;
  using llvm::DenseMap;
  using llvm::Function;
  using llvm::GEPOperator;
  using llvm::GlobalValue;
  using llvm::GlobalVariable;
  using llvm::Instruction;
  using llvm::LLVMContext;
  using llvm::Module;
  using llvm::Pass;
  using llvm::PointerType;
  using llvm::RegisterPass;
  using llvm::SmallVector;
  using llvm::Type;
  using llvm::User;
  using llvm::Value;
  using llvm::ValueSymbolTable;
}

#endif // NANOTUBE_LLVM_IR_H
