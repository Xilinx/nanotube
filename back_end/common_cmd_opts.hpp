/*******************************************************/
/*! \file  common_cmd_opts.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Common command line options for all passes
**   \date 2022-08-23
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "llvm/Support/CommandLine.h"

namespace nanotube {
  extern llvm::cl::opt<bool> opt_print_analysis_info;
};
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
