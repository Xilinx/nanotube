/**************************************************************************\
*//*! \file tap_map.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  A packet passing test for the map tap.
**   \date  2021-06-21
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_api.h"
#include "nanotube_map_taps.h"
#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_sb.h"
#include "simple_bus.hpp"

enum channel {
  PACKETS_IN,
  PACKETS_OUT,
  MAP_REQ,
  MAP_RESP,
};

// Each packet consists of two chunks.  Each chunk corresponds with a
// request or a response and consists of:
//
//   access (request only) / result (response only)
//   key (request only)
//   data
//
// Input packets contain requests.  Output packets contain responses.
//
// The test code is split into stages:
//   Stage 0:
//     Read chunk 0
//     Send request 0
//   Stage 1:
//     Receive response 0
//     Write chunk 0
//     Read chunk 1
//     Send request 1
//   Stage 2:
//     Receive response 1
//     Write chunk 1

typedef struct
{
  nanotube_tap_map_t *map;
} info_t;

static const int key_length = 8;
static const int data_length = 8;
static const int capacity = 4;

static const uint8_t chunk_bytes_index_bits = 6;
typedef struct
{
  union {
    uint8_t access; // For the request.
    uint8_t result; // For the response.
  } u;
  uint8_t key[key_length];
  uint8_t data[data_length];
} chunk_t;

extern "C"
void stage_0(nanotube_context_t* context, void *arg)
{
  info_t *info = (info_t*)arg;

  // Read a packet word.
  simple_bus::word word_in;
  if (!nanotube_channel_try_read(context, PACKETS_IN, &word_in, sizeof(word_in))) {
    nanotube_thread_wait();
    return;
  }

  // Read input chunk 0.
  static struct nanotube_tap_packet_read_state read_state = nanotube_tap_packet_read_state_init;
  static chunk_t chunk_in;
  struct nanotube_tap_packet_read_req read_req = {0,};
  struct nanotube_tap_packet_read_resp read_resp;

  read_req.valid = true;
  read_req.read_offset = sizeof(simple_bus::header) + 0*sizeof(chunk_in);
  read_req.read_length = sizeof(chunk_in);

  static_assert(sizeof(chunk_in) <= (1U<<chunk_bytes_index_bits),
                "Not enough index bits.");
  nanotube_tap_packet_read_sb(
    sizeof(chunk_in), chunk_bytes_index_bits,
    &read_resp, (uint8_t*)&chunk_in,
    &read_state,
    &word_in, &read_req);

  // Send request 0.
  if (read_resp.valid) {
    nanotube_tap_map_send_req(context, info->map, /*client_id*/0,
                              (enum map_access_t)chunk_in.u.access,
                              chunk_in.key, chunk_in.data);
  }

  // Write the packet word.
  nanotube_channel_write(context, PACKETS_OUT, &word_in, sizeof(word_in));
}

extern "C"
void stage_1(nanotube_context_t* context, void *arg)
{
  info_t *info = (info_t*)arg;

  // Receive response 0.
  static bool have_resp = false;
  static chunk_t chunk_out = {{0},};
  if (!have_resp) {
    nanotube_map_result_t result_out;
    if (!nanotube_tap_map_recv_resp(context, info->map, /*client_id*/0,
                                    chunk_out.data, &result_out)) {
      nanotube_thread_wait();
      return;
    }
    chunk_out.u.result = (uint8_t)result_out;
    have_resp = true;
  }

  // Read a packet word.
  simple_bus::word word_in;
  if (!nanotube_channel_try_read(context, PACKETS_IN, &word_in, sizeof(word_in))) {
    nanotube_thread_wait();
    return;
  }

  if (word_in.get_eop())
    have_resp = false;

  // Write output chunk 0.
  static nanotube_tap_packet_write_state write_state = nanotube_tap_packet_write_state_init;
  uint8_t chunk_mask[(sizeof(chunk_out)+7)/8];
  memset(chunk_mask, 0xff, sizeof(chunk_mask));

  simple_bus::word word_out;
  nanotube_tap_packet_write_req write_req = {0,};

  write_req.valid = true;
  write_req.write_offset = sizeof(simple_bus::header) + 0*sizeof(chunk_out);
  write_req.write_length = sizeof(chunk_out);

  nanotube_tap_packet_write_sb(
    sizeof(chunk_out), chunk_bytes_index_bits,
    &word_out, &write_state,
    &word_in, &write_req,
    (uint8_t*)&chunk_out, chunk_mask);

  // Read input chunk 1.
  static struct nanotube_tap_packet_read_state read_state = nanotube_tap_packet_read_state_init;
  static chunk_t chunk_in;
  struct nanotube_tap_packet_read_req read_req = {0,};
  struct nanotube_tap_packet_read_resp read_resp;

  read_req.valid = true;
  read_req.read_offset = sizeof(simple_bus::header) + 1*sizeof(chunk_in);
  read_req.read_length = sizeof(chunk_in);

  static_assert(sizeof(chunk_in) <= (1U<<chunk_bytes_index_bits),
                "Not enough index bits.");
  nanotube_tap_packet_read_sb(
    sizeof(chunk_in), chunk_bytes_index_bits,
    &read_resp, (uint8_t*)&chunk_in,
    &read_state,
    &word_in, &read_req);

  // Send request 1.
  if (read_resp.valid) {
    nanotube_tap_map_send_req(context, info->map, /*client_id*/1,
                              (enum map_access_t)chunk_in.u.access,
                              chunk_in.key, chunk_in.data);
  }

  // Write the packet word.
  nanotube_channel_write(context, PACKETS_OUT, &word_out, sizeof(word_out));
}

extern "C"
void stage_2(nanotube_context_t* context, void *arg)
{
  info_t *info = (info_t*)arg;

  // Receive response 1.
  static bool have_resp = false;
  static chunk_t chunk = {{0},};
  if (!have_resp) {
    nanotube_map_result_t result_out;
    if (!nanotube_tap_map_recv_resp(context, info->map, /*client_id*/1,
                                    chunk.data, &result_out)) {
      nanotube_thread_wait();
      return;
    }
    chunk.u.result = (uint8_t)result_out;
    have_resp = true;
  }

  // Read a packet word.
  simple_bus::word word_in;
  if (!nanotube_channel_try_read(context, PACKETS_IN, &word_in, sizeof(word_in))) {
    nanotube_thread_wait();
    return;
  }

  if (word_in.get_eop())
    have_resp = false;

  // Write output chunk 1.
  static nanotube_tap_packet_write_state write_state = nanotube_tap_packet_write_state_init;
  uint8_t chunk_mask[(sizeof(chunk)+7)/8];
  memset(chunk_mask, 0xff, sizeof(chunk_mask));

  simple_bus::word word_out;
  nanotube_tap_packet_write_req write_req = {0,};

  write_req.valid = true;
  write_req.write_offset = sizeof(simple_bus::header) + 1*sizeof(chunk);
  write_req.write_length = sizeof(chunk);

  nanotube_tap_packet_write_sb(
    sizeof(chunk), chunk_bytes_index_bits,
    &word_out, &write_state,
    &word_in, &write_req,
    (uint8_t*)&chunk, chunk_mask);

  // Write the packet word.
  nanotube_channel_write(context, PACKETS_OUT, &word_out, sizeof(word_out));
}

extern "C"
void nanotube_setup()
{
  nanotube_channel_t *packets_in =
    nanotube_channel_create("packets_in", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_0_to_1 =
    nanotube_channel_create("packets_0_to_1", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_1_to_2 =
    nanotube_channel_create("packets_1_to_2", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_out =
    nanotube_channel_create("packets_out", simple_bus::total_bytes, 16);

  if (simple_bus::sideband_bytes != 0) {
    nanotube_channel_set_attr(packets_in, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
    nanotube_channel_set_attr(packets_0_to_1, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
    nanotube_channel_set_attr(packets_1_to_2, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
    nanotube_channel_set_attr(packets_out, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
  }

  nanotube_channel_export(packets_in,
                          NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET,
                          NANOTUBE_CHANNEL_WRITE);
  nanotube_channel_export(packets_out,
                          NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET,
                          NANOTUBE_CHANNEL_READ);

  int num_clients = 2;
  auto map = nanotube_tap_map_create(NANOTUBE_MAP_TYPE_HASH, key_length,
                                     data_length, capacity, num_clients);
  auto context_0 = nanotube_context_create();
  auto context_1 = nanotube_context_create();
  auto context_2 = nanotube_context_create();
  nanotube_tap_map_add_client(map, key_length, data_length, true, data_length,
                              context_0, MAP_REQ, context_1, MAP_RESP);
  nanotube_tap_map_add_client(map, key_length, data_length, true, data_length,
                              context_1, MAP_REQ, context_2, MAP_RESP);
  nanotube_tap_map_build(map);

  nanotube_context_add_channel(context_0, PACKETS_IN, packets_in, NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context_0, PACKETS_OUT, packets_0_to_1, NANOTUBE_CHANNEL_WRITE);
  nanotube_context_add_channel(context_1, PACKETS_IN, packets_0_to_1, NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context_1, PACKETS_OUT, packets_1_to_2, NANOTUBE_CHANNEL_WRITE);
  nanotube_context_add_channel(context_2, PACKETS_IN, packets_1_to_2, NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context_2, PACKETS_OUT, packets_out, NANOTUBE_CHANNEL_WRITE);

  info_t info = {0};
  info.map = map;

  nanotube_thread_create(context_0, "stage_0", &stage_0, &info, sizeof(info));
  nanotube_thread_create(context_1, "stage_1", &stage_1, &info, sizeof(info));
  nanotube_thread_create(context_2, "stage_2", &stage_2, &info, sizeof(info));
}
