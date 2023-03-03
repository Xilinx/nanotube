/**************************************************************************\
*//*! \file llvm_common.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  Common LLVM headers
**   \date  2019-06-27
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_LLVM_COMMON_H
#define NANOTUBE_LLVM_COMMON_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#pragma GCC diagnostic pop

namespace nanotube
{
  using llvm::cast;
  using llvm::dbgs;
  using llvm::dyn_cast;
  using llvm::dyn_cast_or_null;
  using llvm::errs;
  using llvm::formatv;
  using llvm::isa;
  using llvm::Optional;
  using llvm::raw_ostream;
  using llvm::raw_os_ostream;
  using llvm::raw_pwrite_stream;
  using llvm::report_fatal_error;
  using llvm::StringRef;
  using llvm::Triple;
};

#endif // NANOTUBE_LLVM_COMMON_H
