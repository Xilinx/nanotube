/*******************************************************/
/*! \file printing_helpers.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Helpers for easy printing of types.
**   \date 2021-01-20
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "printing_helpers.h"

#include "llvm/Analysis/AliasAnalysis.h"

llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const llvm::ModRefInfo& i) {
  uint8_t j = (uint8_t)i;
  return os << ((j & 1) ? "MustRef " : "")
            << ((j & 2) ? "MustMod " : "")
            << ((j & 4) ? "NoMR"     : "");
}

llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const llvm::FunctionModRefBehavior& i) {
  uint8_t j = (uint8_t)i;
  return os << ((j & 1) ? "MustRef " : "")
            << ((j & 2) ? "MostMod " : "")
            << ((j & 4) ? "NoMR "    : "")
            << ((j & 8) ? "ArgPointee " : "")
            << ((j & 16) ? "InaccMem " : "")
            << ((j & 32) ? "Anywhere " : "")
            << ((j & 56) ? "" : "Nowhere");
}

llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const llvm::AAMDNodes& aa) {
  os << "(";
  if( aa.TBAA != nullptr )
    os << "tbaa: " << *aa.TBAA << " ";
  if( aa.Scope != nullptr )
    os << "scope: " << *aa.Scope << " ";
  if( aa.NoAlias != nullptr )
    os << "noalias: " << *aa.NoAlias << " ";
  os << ")";

  return os;
}

llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const llvm::MemoryLocation& loc) {
  return os << *loc.Ptr << " Size: " << loc.Size << " AA: " << loc.AATags;
}
