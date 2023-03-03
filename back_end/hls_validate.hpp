/**************************************************************************\
*//*! \file hls_validate.hpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Validate a thread function for HLS output.
**   \date  2020-08-03
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_HLS_VALIDATE_HPP
#define NANOTUBE_HLS_VALIDATE_HPP

#include "llvm/IR/Function.h"

namespace nanotube
{
  using llvm::Function;
  // Validate a function before writing HLS output.
  //
  // This function makes sure the thread function
  //
  // \param thread_func The function to examine.
  void validate_hls_thread_function(const Function &thread_func);
};

#endif // NANOTUBE_HLS_VALIDATE_HPP
