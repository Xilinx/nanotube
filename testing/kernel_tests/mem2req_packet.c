/*******************************************************/
/*! \file  mem2req_packet.c
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

nanotube_kernel_rc_t memset_test(nanotube_context_t *nt_ctx,
                                 nanotube_packet_t *packet)
{
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 42 ) {
    return NANOTUBE_PACKET_DROP;
  }

  memset(data, 0, 21);
  memset(data + 21, 0x0f, 21);

  return NANOTUBE_PACKET_PASS;
}


nanotube_kernel_rc_t select_test(nanotube_context_t *nt_ctx,
                                 nanotube_packet_t *packet)
{
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 42 ) {
    return NANOTUBE_PACKET_DROP;
  }

  uint8_t choice = data[0];
  uint8_t* sel   = (choice < 128) ? &data[21] : &data[20] ;

  memset(sel, 0xaa, 4);
  memcpy(sel + 4, data, 4);
  sel[8] = data[1];

  return NANOTUBE_PACKET_PASS;
}

void nanotube_setup(void) {
  nanotube_add_plain_packet_kernel("memset_test", memset_test, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("select_test", select_test, -1, 0 /*false*/);
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
