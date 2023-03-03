/**************************************************************************\
*//*! \file tap_packet_length.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  A packet passing test for the packet length tap.
**   \date  2021-09-27
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_api.h"
#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_sb.h"
#include "simple_bus.hpp"

enum channel {
  PACKETS_IN,
  PACKETS_OUT,
};

extern "C"
void logic_0(nanotube_context_t* context, void *arg)
{
  static nanotube_tap_packet_length_state s_state =
    nanotube_tap_packet_length_state_init;
  static bool s_sop = true;
  static uint16_t s_max_length = 0;
  static uint16_t s_req_offset = 0;
  static uint16_t s_packet_offset = 0;
  nanotube_tap_packet_length_req req;
  nanotube_tap_packet_length_resp resp;

  simple_bus::word word;
  if (!nanotube_channel_try_read(context, PACKETS_IN, &word, sizeof(word))) {
    nanotube_thread_wait();
    return;
  }

  bool eop = word.get_eop();
  bool sop = s_sop;
  s_sop = eop;

  // Read the request from the packet.
  if (sop) {
    assert(simple_bus::data_bytes >= sizeof(simple_bus::header) + 4);
    auto *data = word.data_ptr(sizeof(simple_bus::header));
    // The maximum length parameter in the request.
    s_max_length = ( ( uint16_t(data[0]) << 0 ) +
                     ( uint16_t(data[1]) << 8 ) );
    // The request offset allows us to stress the tap by asserting
    // req.valid part way through the packet.
    s_req_offset = ( ( uint16_t(data[2]) << 0 ) +
                     ( uint16_t(data[3]) << 8 ) );
  }
  s_packet_offset = sop ? 0 : s_packet_offset + simple_bus::data_bytes;

  // Prepare the request for the tap.
  if (s_packet_offset >= s_req_offset) {
    req.valid = true;
    req.max_length = s_max_length;
  } else {
    req.valid = false;
    req.max_length = 0;
  }

  // Invoke the packet length tap.
  nanotube_tap_packet_length_sb(
    &resp,
    &s_state,
    &word, &req);

  // Write the response into the packet.
  auto *data = word.data_ptr(sizeof(simple_bus::header));
  if (resp.valid) {
    data[0] = ((resp.result_length >> 0) & 0xff) ^ 0x80;
    data[1] = ((resp.result_length >> 8) & 0xff) ^ 0x80;
  } else {
    data[0] = 0xff;
    data[1] = 0xff;
  }

  nanotube_channel_write(context, PACKETS_OUT, &word, sizeof(word));
}

extern "C"
void nanotube_setup()
{
  nanotube_channel_t *packets_in =
    nanotube_channel_create("packets_in", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_out =
    nanotube_channel_create("packets_out", simple_bus::total_bytes, 16);

  if (simple_bus::sideband_bytes != 0) {
    nanotube_channel_set_attr(packets_in, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
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

  nanotube_context_t *context = nanotube_context_create();
  nanotube_context_add_channel(context, PACKETS_IN, packets_in, NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context, PACKETS_OUT, packets_out, NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(context, "stage_0", &logic_0, nullptr, 0);
}
