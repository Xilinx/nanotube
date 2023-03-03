/*******************************************************/
/*! \file test_harness_main.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief The test harness main function.
**   \date 2020-11-26
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "test_harness_run.h"

// The function "main" needs to be compiled using Vivado HLS.
// Compiling the test harness with Vivado HLS results in link errors
// due to changes in the C++ ABI.  This is a thin wrapper around the
// test harness so that this code is compiled using Vivado HLS while
// the rest of the code is compiled using the normal compiler.

int main(int argc, char *argv[])
{
  return test_harness_run(argc, argv);
}
