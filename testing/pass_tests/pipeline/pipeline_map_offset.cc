/**************************************************************************\
*//*! \file pipeline_map_offset.cc
** \author  Stephan Diestelhorst <stephand@amd.com>
**  \brief  Test the handling of map offsets & masks in the Pipeline pass.
**   \date  2023-07-25
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

struct map_A_val {
  uint64_t count;
  uint64_t tot_size;
};
static const nanotube_map_id_t map_A = 0;

/**
 * Keep track of the total packet count and aggregate packet size in a
 * simple map and write the running totals to each outgoing packet.
 */
nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet) {
  uint64_t len = nanotube_packet_bounded_length(packet, 32768, NANOTUBE_SECTION_WHOLE) -
                   platform_offset;

  uint64_t count, tot_size;

  const uint8_t  one_mask = 0xff;
  const uint32_t zero_idx = 0;
  const uint32_t one_idx  = 0;

  /* Read the count from map entry 0 */
  nanotube_map_op(nt_ctx, map_A, NANOTUBE_MAP_READ,
                  (uint8_t*)&zero_idx, sizeof(zero_idx),
                  nullptr, (uint8_t*)&count, &one_mask,
                  offsetof(map_A_val, count), sizeof(count));

  /* Read the tot_size from map entry 1 */
  nanotube_map_op(nt_ctx, map_A, NANOTUBE_MAP_READ,
                  (uint8_t*)&one_idx, 4,
                  nullptr, (uint8_t*)&tot_size, &one_mask,
                  offsetof(map_A_val, tot_size), sizeof(tot_size));

  /* Increment accordingly */
  count++;
  tot_size += len;

  /* Write the updated count to the map */
  nanotube_map_op(nt_ctx, map_A, NANOTUBE_MAP_WRITE,
                  (uint8_t*)&zero_idx, 4,
                  (uint8_t*)&count, nullptr, &one_mask,
                  offsetof(map_A_val, count), sizeof(count));
  /* Write the updated tot_size to the map */
  nanotube_map_op(nt_ctx, map_A, NANOTUBE_MAP_WRITE,
                  (uint8_t*)&one_idx, 4,
                  (uint8_t*)&tot_size, nullptr, &one_mask,
                  offsetof(map_A_val, tot_size), sizeof(tot_size));

  /* Write both values to the packet, too */
  nanotube_packet_write_masked(packet, (uint8_t*)&count, &one_mask,
                               0 + platform_offset, sizeof(count));
  nanotube_packet_write_masked(packet, (uint8_t*)&tot_size, &one_mask,
                               sizeof(count) + platform_offset,
                               sizeof(tot_size));

  return NANOTUBE_PACKET_PASS;
}


void nanotube_setup() {
  // Note: name adjusted to match manual test output :)
  nanotube_context_t* ctx = nanotube_context_create();
  nanotube_map_t* map_A_  = nanotube_map_create(map_A,
                              NANOTUBE_MAP_TYPE_ARRAY_LE,
                              4,
                              sizeof(map_A_val));
  nanotube_context_add_map(ctx, map_A_);
  const bool is_capsule_kernel = true;
  nanotube_add_plain_packet_kernel("packets_in", process_packet,
                                   NANOTUBE_BUS_ID_SB, is_capsule_kernel);
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
