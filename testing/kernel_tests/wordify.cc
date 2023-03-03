/**************************************************************************\
*//*! \file wordify.cc
** \author  Stephan Diestelhorst <stephand@amd.com>
**  \brief  Base-line code for the manual wordify example for the pipeline /
**          wordify pass.
**   \date  2021-02-15
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

static const uint16_t platform_offset = sizeof(simple_bus::header);
//static const uint16_t platform_header = 0;

/**
 * A very simple test code /after/ the converge and platform pass have been
 * run.  Just a few data dependent packet reads and write.  The converge part
 * restricts the control flow, and the platform pass will adjust from packet ->
 * capsule, primarily updating offsets and turning special operations into
 * capsule header accesses.
 */
nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet) {
  uint8_t sel1;
  nanotube_packet_read(packet, &sel1, 0 + platform_offset, 1);

  uint16_t next_read_offs;
  if( sel1 > 127 ) {
    next_read_offs = 1;
  } else {
    next_read_offs = 2;
  }
  uint8_t dat;
  nanotube_packet_read(packet, &dat, next_read_offs + platform_offset, 1);
  dat = ~dat;

  uint8_t one_mask = 0xff;
  nanotube_packet_write_masked(packet, &dat, &one_mask, 3 + platform_offset, 1);

  return NANOTUBE_PACKET_PASS;
}


void nanotube_setup() {
  // Note: name adjusted to match manual test output :)
  nanotube_add_plain_packet_kernel("packets_in", process_packet, NANOTUBE_BUS_ID_SB, 1/*true*/);
}
