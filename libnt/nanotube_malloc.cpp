/**************************************************************************\
*//*! \file nanotube_malloc.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube memory allocation.
**   \date  2021-06-09
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_api.h"
#include "processing_system.hpp"

#include <cstdlib>
#include <iostream>

void *nanotube_malloc(size_t size)
{
  void *result = malloc(size);
  if (result == nullptr) {
    std::cerr << "ERROR: Memory allocation failed!\n";
    std::exit(1);
  }
  memset(result, 0, size);
  auto &ps = processing_system::get_current();
  ps.add_malloc(result);
  return result;
}
