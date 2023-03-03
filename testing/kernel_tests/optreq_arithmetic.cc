/**************************************************************************\
*//*! \file optreq_arithmetic.cc
** \author  Stephan Diestelhorst <stephand@amd.com>
**  \brief  Simple packet manipulation logic for testing.
**   \date  2021-09-21
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdio.h>
#include "nanotube_api.h"

nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet) {
  uint16_t offs;
  nanotube_packet_read(packet, (uint8_t*)&offs, 0, sizeof(offs));

  uint8_t sel0, sel1;
  nanotube_packet_read(packet, &sel0, 2, sizeof(sel0));
  nanotube_packet_read(packet, &sel1, 3, sizeof(sel1));

  uint32_t bit_offset = 0;
  if( (sel0 & 1) != 0 )
    bit_offset = 32;
  else
    bit_offset = 40;

  // reads relative to bit_offset / zero
  uint8_t d0, d1, d2, d3;


  if( (sel0 & 2) != 0 ) {
    nanotube_packet_read(packet, &d0, bit_offset / 8, sizeof(d0));
    bit_offset += 8;
    nanotube_packet_read(packet, &d1, bit_offset / 8, sizeof(d0));
    bit_offset += 12; // just messing with the pass here!
    nanotube_packet_read(packet, &d2, bit_offset / 8, sizeof(d0));
    bit_offset += 4;  // and once again!
    nanotube_packet_read(packet, &d3, bit_offset / 8, sizeof(d0));
    bit_offset += 4;  // and one more time!
    bit_offset += 4;
  } else {
    nanotube_packet_read(packet, &d3, bit_offset / 8, sizeof(d0));
    bit_offset += 8;
    nanotube_packet_read(packet, &d2, bit_offset / 8, sizeof(d0));
    bit_offset += 12; // just messing with the pass here!
    nanotube_packet_read(packet, &d1, bit_offset / 8, sizeof(d0));
    bit_offset += 4;  // and once again!
    nanotube_packet_read(packet, &d0, bit_offset / 8, sizeof(d0));
    bit_offset += 4;  // and one more time!
    bit_offset += 12;
  }
  //printf("%02x %02x %02x %02x\n", d0, d1, d2, d3);

  // double PHI increment (but that might get turned into a select :(
  uint8_t d4, d5;
  nanotube_packet_read(packet, &d4, bit_offset / 8, sizeof(d0));
  bit_offset += 8;
  nanotube_packet_read(packet, &d5, bit_offset / 8, sizeof(d0));
  bit_offset += 8;

  /* now move to separate location */
  bit_offset += 8 * offs;
  if( (sel1 & 1) != 0 )
    bit_offset += 4;
  else
    bit_offset += 12;
  // We're at 4 bits
  uint8_t d6, d7;
  nanotube_packet_read(packet, &d6, bit_offset / 8, sizeof(d0));
  bit_offset += 4;
  nanotube_packet_read(packet, &d7, bit_offset / 8, sizeof(d0));
  bit_offset += 8;
  // back to byte aligned
  //printf("%02x %02x %02x %02x\n", d4, d5, d6, d7);

  uint8_t all_one = 0xff;
  nanotube_packet_write_masked(packet, &d0, &all_one, 0, 1);
  nanotube_packet_write_masked(packet, &d1, &all_one, 1, 1);
  nanotube_packet_write_masked(packet, &d2, &all_one, 2, 1);
  nanotube_packet_write_masked(packet, &d3, &all_one, 3, 1);
  nanotube_packet_write_masked(packet, &d4, &all_one, 4, 1);
  nanotube_packet_write_masked(packet, &d5, &all_one, 5, 1);
  nanotube_packet_write_masked(packet, &d6, &all_one, 6, 1);
  nanotube_packet_write_masked(packet, &d7, &all_one, 7, 1);

  nanotube_packet_write_masked(packet, (uint8_t*)&bit_offset, &all_one, 8,
                               4);
  
  return NANOTUBE_PACKET_PASS;
}

void nanotube_setup() {
  nanotube_add_plain_packet_kernel("packets_in", process_packet, -1, 0 /*false*/);
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
