/*******************************************************/
/*! \file misc_inline.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Implementations of other inline Nanotube functions.
**   \date 2022-03-03
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_api.h"

#ifdef __clang__
__attribute__((always_inline))
#endif
uint64_t nanotube_get_time_ns(void) {
  /* This is (obviously) a placeholder implementation. More detail and
   * thoughts in NANO-91 */
  return 0;
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
