/*******************************************************/
/*! \file  mem2req_packet.manual.c
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Expected output for mem2req conversion
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
  size_t len = nanotube_packet_bounded_length(packet, 65535);

  if( len < 42 ) {
    return NANOTUBE_PACKET_DROP;
  }

  uint8_t tmp1[21];
  memset(tmp1, 0, 21);
  nanotube_packet_write(packet, tmp1, 0, 21);

  uint8_t tmp2[21];
  memset(tmp2, 0x0f, 21);
  nanotube_packet_write(packet, tmp2, 21, 21);

  return NANOTUBE_PACKET_PASS;
}

nanotube_kernel_rc_t select_test(nanotube_context_t *nt_ctx,
                                 nanotube_packet_t *packet)
{
  size_t len = nanotube_packet_bounded_length(packet, 65535);

  if( len < 42 ) {
    return NANOTUBE_PACKET_DROP;
  }

  uint8_t choice;
  nanotube_packet_read(packet, &choice, 0, 1);

  size_t sel   = (choice < 128) ? 21 : 20 ;

  uint8_t tmp[4];
  memset(tmp, 0xaa, 4);
  nanotube_packet_write(packet, tmp, sel, 4);

  uint8_t tmp2[4];
  nanotube_packet_read(packet, tmp2, 0, 4);
  nanotube_packet_write(packet, tmp2, sel + 4, 4);

  uint8_t tmp3;
  nanotube_packet_read(packet, &tmp3, 1, 1);
  nanotube_packet_write(packet, &tmp3, sel + 8, 1);

  return NANOTUBE_PACKET_PASS;
}

void nanotube_setup(void) {
  nanotube_add_plain_packet_kernel("memset_test", memset_test, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("select_test", select_test, -1, 0 /*false*/);
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
