/*******************************************************/
/*! \file test_harness_run.h
** \author Neil Turton <neilt@amd.com>
**  \brief Interface to the test harness.
**   \date 2020-11-26
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef TEST_HARNESS_RUN_H
#define TEST_HARNESS_RUN_H

#ifdef __cplusplus
extern "C" {
#endif

int test_harness_run(int argc, char *argv[]);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // TEST_HARNESS_RUN_H
