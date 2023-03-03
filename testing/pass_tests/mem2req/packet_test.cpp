/*******************************************************/
/*! \file  packet_test.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Test mem2req conversion for packet accesses.
**   \date 2020-08-04
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>
#include <string.h>
#include "nanotube_api.h"

int memset_test(nanotube_context_t *nt_ctx,
                nanotube_packet_t *packet)
{
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 42 )
    return -1;

  memset(data, 0, 21);
  memset(data + 21, 0x0f, 21);
  return 0;
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
