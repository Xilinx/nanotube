/**************************************************************************\
*//*! \file pipeline_map_offset.mem.cc
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

struct map_val {
  uint64_t count;
  uint64_t tot_size;
};
static const nanotube_map_id_t map_A = 0;
static const nanotube_map_id_t map_B = 1;

/**
 * Keep track of the total packet count and aggregate packet size in a
 * simple map and write the running totals to each outgoing packet.
 */
nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet) {
  uint64_t len = nanotube_packet_bounded_length(packet, 32768);

  const uint32_t zero_idx = 0;
  const uint32_t one_idx  = 0;

  /* Perform the update on two different values so that they cannot be
   * merged by optreq */
  auto *val_a = (map_val*)nanotube_map_lookup(nt_ctx, map_A,
                  (const uint8_t*)&zero_idx, sizeof(zero_idx),
                  sizeof(map_val));
  auto *val_b = (map_val*)nanotube_map_lookup(nt_ctx, map_B,
                  (const uint8_t*)&one_idx, sizeof(one_idx),
                  sizeof(map_val));

  val_a->count++;
  val_b->tot_size += len;

  auto *packet_val = (map_val*)nanotube_packet_data(packet);
  packet_val->count    = val_a->count;
  packet_val->tot_size = val_b->tot_size;

  return NANOTUBE_PACKET_PASS;
}


void nanotube_setup() {
  // Note: name adjusted to match manual test output :)
  nanotube_context_t* ctx = nanotube_context_create();
  nanotube_map_t* map_A_  = nanotube_map_create(map_A,
                              NANOTUBE_MAP_TYPE_ARRAY_LE,
                              4,
                              sizeof(map_val));
  nanotube_context_add_map(ctx, map_A_);
  nanotube_map_t* map_B_  = nanotube_map_create(map_B,
                              NANOTUBE_MAP_TYPE_ARRAY_LE,
                              4,
                              sizeof(map_val));
  nanotube_context_add_map(ctx, map_B_);
  const bool is_capsule_kernel = false;
  nanotube_add_plain_packet_kernel("packets_in", process_packet,
                                   NANOTUBE_BUS_ID_SB, is_capsule_kernel);
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
