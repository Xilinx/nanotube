/**************************************************************************\
*//*! \file packet_resize.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  Test changes to the packet length.
**   \date  2022-09-22
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_api.h"
#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_bus.h"
#include "simple_bus.hpp"
#include <cstdio>

nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet)
{
  struct {
    uint8_t pad[2];
    uint8_t offset[2];
    uint8_t adjust[2];
  } req;
  nanotube_packet_read(packet, (uint8_t*)&req, 0, sizeof(req));
  size_t offset = ( ( size_t(req.offset[0]) << 0 ) |
                    ( size_t(req.offset[1]) << 8 ) );
  int32_t adjust = ( ( int32_t(req.adjust[0]) << 0 ) |
                     ( int32_t(req.adjust[1]^0x80) << 8 ) ) - 0x8000;
  nanotube_packet_resize(packet, offset, adjust);

  return NANOTUBE_PACKET_PASS;
}


void nanotube_setup()
{
  nanotube_add_plain_packet_kernel("kernel", process_packet,
                                   NANOTUBE_BUS_ID_SB, false);
}
