/**************************************************************************\
*//*! \file nanotube_debug.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube debug functions.
**   \date  2021-01-28
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_api.h"
#include "nanotube_thread.hpp"

#include <iomanip>
#include <iostream>

void nanotube_debug_trace(uint64_t id, uint64_t value)
{
  nanotube_stderr_guard guard;
  std::cerr << "Trace " << id << " Value " << value << '\n';
}

void nanotube_trace_buffer(uint64_t id, uint8_t *buffer,
                           uint64_t size)
{
  nanotube_stderr_guard guard;
  std::cerr << "Trace buffer " << id;
  std::cerr << std::hex << std::setfill('0');
  for(uint64_t i=0; i<size; i++) {
    if ((i % 16) == 0) {
      std::cerr << "\n  ";
    } else {
      std::cerr << " ";
    }
    std::cerr << std::setw(2) << unsigned(buffer[i]);
  }
  std::cerr << std::dec << std::setfill(' ');
  std::cerr << "\n";
}


