/**************************************************************************\
*//*! \file pipeline_phi_maps.cc
** \author  Stephan Diestelhorst <stephand@amd.com>
**  \brief  Testing phi node handling in the Pipeline pass
**   \date  2021-02-02
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
/* Cause code to be wrong in order to test the pipeline pass failing correctly */
//#define TEST_BAD_CODE_DETECTION

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

struct map_B_def {
  uint32_t key;
  uint32_t value;
};
static const nanotube_map_id_t map_B = 1;

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

  uint32_t map_A_buf;
  uint32_t map_B_buf;
  uint32_t default_buf = 0x08040201;
  uint32_t* map_buf = nullptr;
  uint8_t  len = 4;

  /* Create a data-dependent offset from packet data */
  uint8_t sel1 = 42;
  nanotube_packet_read(packet, &sel1, 0 + platform_offset, 1);

  /* Read the map operation next */
  struct {
    uint32_t key;
    uint32_t data;
    uint8_t  op;
  } map_req;

  nanotube_packet_read(packet, (uint8_t*)&map_req, 1 + platform_offset,
                       sizeof(map_req));

  /* Perform the map operation */
#ifdef DEBUG
  printf("sel1: %i key: %0x val: %0x op: %i\n", (unsigned)sel1, map_req.key,
      map_req.data, map_req.op);
#endif
  const uint8_t one_mask = 0xff;

  uint8_t map_A_op, map_B_op;
  if( sel1 == 0 ) {
    map_A_op = map_req.op;
    map_B_op = NANOTUBE_MAP_NOP;
  } else if( sel1 == 1 ) {
    map_A_op = NANOTUBE_MAP_NOP;
    map_B_op = map_req.op;
  } else {
    map_A_op = NANOTUBE_MAP_NOP;
    map_B_op = NANOTUBE_MAP_NOP;
  }

  nanotube_map_op(nt_ctx, map_A, (enum map_access_t)map_A_op,
                  (uint8_t*)&map_req.key, sizeof(map_req.key),
                  (uint8_t*)&map_req.data, (uint8_t*)&map_A_buf, &one_mask, 0,
                  sizeof(map_A_def::value));


  nanotube_map_op(nt_ctx, map_B, (enum map_access_t)map_B_op,
                  (uint8_t*)&map_req.key, sizeof(map_req.key),
                  (uint8_t*)&map_req.data, (uint8_t*)&map_B_buf, &one_mask, 0,
                  sizeof(map_B_def::value));

  /* Assign the buffer only after the map accesses so that we can do the
   * phi-to-memcpy conversion */
  if( sel1 == 0 ) {
    map_buf = &map_A_buf;
  } else if( sel1 == 1 ) {
    map_buf = &map_B_buf;
  } else {
    map_buf = &default_buf;
  }

  /* Have another packet access here just to break the pipeline */
  uint8_t ignore[8];
  nanotube_packet_read(packet, ignore, 0 + platform_offset, sizeof(ignore));

#ifdef TEST_BAD_CODE_DETECTION
  /* Deliberately introduce a programming error that will cause the actual
   * pointers becoming part of the live state.  The pipeline pass should detect
   * this scenario and complain! */
  nanotube_packet_write_masked(packet, (uint8_t*)&map_buf, &one_mask,
                               5 + platform_offset, len);
#else
  /* Write the data back to the packet */
  nanotube_packet_write_masked(packet, (uint8_t*)map_buf, &one_mask,
                               5 + platform_offset, len);
#endif

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

  nanotube_map_t* map_B_  = nanotube_map_create(map_B,
                              NANOTUBE_MAP_TYPE_HASH,
                              sizeof(map_B_def::key),
                              sizeof(map_B_def::value));
  nanotube_context_add_map(ctx, map_B_);
  nanotube_add_plain_packet_kernel("packets_in", process_packet, NANOTUBE_BUS_ID_SB, 1 /*true*/);
}
