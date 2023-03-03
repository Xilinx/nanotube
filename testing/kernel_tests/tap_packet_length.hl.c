/**************************************************************************\
*//*! \file tap_packet_length.hl.c
** \author  Neil Turton <neilt@amd.com>,
**          Stephan Diestelhorst <stephand@amd.com>
**  \brief  High-level testing of packet length function
**   \date  2021-09-28
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_api.h"

nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet) {
  uint16_t max_length;
  nanotube_packet_read(packet, (uint8_t*)&max_length, 0, 2);
  uint16_t length =
    (uint16_t)nanotube_packet_bounded_length(packet, (size_t)max_length);
  length ^= 0x8080;
  nanotube_packet_write(packet, (uint8_t*)&length, 0, 2);
  return NANOTUBE_PACKET_PASS;
}

void nanotube_setup()
{
  nanotube_add_plain_packet_kernel("packets_in", process_packet, -1, 0/*false*/);
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
