/**************************************************************************\
*//*! \file pipeline_maps.cc
** \author  Stephan Diestelhorst <stephand@amd.com>
**  \brief  Simple map / packet test code for the pipeline pass.
**   \date  2021-12-21
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

struct map_A_def {
  uint32_t key;
  uint32_t value;
};
static const nanotube_map_id_t map_A = 0;

/**
 * A very simple test code /after/ the converge and platform pass have been
 * run.  Just a few data dependent packet reads and write.  The converge part
 * restricts the control flow, and the platform pass will adjust from packet ->
 * capsule, primarily updating offsets and turning special operations into
 * capsule header accesses.
 */
nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet) {
#ifdef DEBUG
  uint8_t d0, d1, d2, d3;
  nanotube_packet_read(packet, &d0, 0, 1);
  nanotube_packet_read(packet, &d1, 1, 1);
  nanotube_packet_read(packet, &d2, 2, 1);
  nanotube_packet_read(packet, &d3, 3, 1);
  printf("%02x %02x %02x %02x\n", d0, d1, d2, d3);
#endif
  /* Create a data-dependent offset from packet data */
  uint8_t sel1 = 42;
  nanotube_packet_read(packet, &sel1, 0 + platform_offset, 1);
  uint16_t next_read_offs;
  if( sel1 > 127 ) {
    next_read_offs = 2;
  } else {
    next_read_offs = 1;
  }

  /* Read the map operation from the packet at the variable offset */
  struct {
    uint32_t key;
    uint32_t data;
    uint8_t  op;
  } map_req;

  nanotube_packet_read(packet, (uint8_t*)&map_req, next_read_offs +
                       platform_offset, sizeof(map_req));

  /* Perform the map opertation */
#ifdef DEBUG
  printf("sel1: %i key: %0x val: %0x op: %i\n", (unsigned)sel1, map_req.key,
      map_req.data, map_req.op);
#endif
  const uint8_t one_mask = 0xff;
  uint32_t dat = 0x00223344;
  nanotube_map_op(nt_ctx, map_A, (enum map_access_t)map_req.op,
                  (uint8_t*)&map_req.key, sizeof(map_req.key),
                  (uint8_t*)&map_req.data, (uint8_t*)&dat, &one_mask, 0,
                  sizeof(dat));

  /* Computation on the data to test the correct taps and code synthesis */
  uint8_t chk = 0;
  chk ^=  dat        & 255;
  chk ^= (dat >>  8) & 255;
  chk ^= (dat >> 16) & 255;
  chk ^= (dat >> 24) & 255;
  struct {
    uint32_t dat;
    uint8_t chk;
    uint8_t sel;
  } write_data;
  write_data.dat = dat;
  write_data.chk = chk;
  write_data.sel = ~sel1;
  /* Write the data back to the packet */
  nanotube_packet_write_masked(packet, (uint8_t*)&write_data, &one_mask,
                               3 + platform_offset, sizeof(write_data));

  return NANOTUBE_PACKET_PASS;
}


void nanotube_setup() {
  // Note: name adjusted to match manual test output :)
  nanotube_context_t* ctx = nanotube_context_create();
  nanotube_map_t* map_A_  = nanotube_map_create(map_A,
                              NANOTUBE_MAP_TYPE_HASH,
                              sizeof(map_A_def::key),
                              sizeof(map_A_def::value));
  nanotube_context_add_map(ctx, map_A_);
  nanotube_add_plain_packet_kernel("packets_in", process_packet, NANOTUBE_BUS_ID_SB, 1 /*true*/);
}
