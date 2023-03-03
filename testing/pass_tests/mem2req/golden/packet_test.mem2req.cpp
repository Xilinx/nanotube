/*******************************************************/
/*! \file  packet_test.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Expected output for mem2req conversion
**   \date 2020-08-04
**    \cop (c) 2020 Xilinx
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
  size_t len = nanotube_packet_bounded_length(packet, 65535);

  if( len < 42 )
    return -1;

  uint8_t tmp1[21];
  memset(tmp1, 0, 21);
  nanotube_packet_write(packet, tmp1, 0, 21);

  uint8_t tmp2[21];
  memset(tmp2, 0x0f, 21);
  nanotube_packet_write(packet, tmp2, 21, 21);
  return 0;
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
