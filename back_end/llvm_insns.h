/**************************************************************************\
*//*! \file llvm_insns.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  LLVM headers for instructions
**   \date  2019-06-27
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_LLVM_INSNS_H
#define NANOTUBE_LLVM_INSNS_H

#include "llvm/IR/Instructions.h"

namespace nanotube
{
  using llvm::AllocaInst;
  using llvm::BinaryOperator;
  using llvm::BitCastInst;
  using llvm::BranchInst;
  using llvm::CallBase;
  using llvm::CallInst;
  using llvm::CastInst;
  using llvm::CmpInst;
  using llvm::GetElementPtrInst;
  using llvm::ICmpInst;
  using llvm::LoadInst;
  using llvm::PHINode;
  using llvm::ReturnInst;
  using llvm::SelectInst;
  using llvm::StoreInst;
  using llvm::SwitchInst;
}

#endif // NANOTUBE_LLVM_INSNS_H
