#ifndef __UNIFY_FUNCTION_RETURNS_HPP__
#define __UNIFY_FUNCTION_RETURNS_HPP__
/*******************************************************/
/*! \file single_function_exit.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert functions to have a single exit.
**   \date 2023-02-07
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Dominators.h"

namespace nanotube {
  bool unify_function_returns(llvm::Function& f,
                              llvm::DominatorTree* dt = nullptr,
                              llvm::PostDominatorTree* pdt = nullptr);
}; // namespace nanotube
#endif //__UNIFY_FUNCTION_RETURNS_HPP__
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
