/**************************************************************************\
*//*! \file tap_packet_read.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  A packet passing test for the packet read tap.
**   \date  2020-09-01
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

bool logic_0_sop = true;
bool logic_0_extend_word = false;

extern "C"
void logic_0(nanotube_context_t* context, void *arg)
{
  static const uint8_t result_buffer_index_bits = 6;

  static nanotube_tap_packet_read_state state =
    nanotube_tap_packet_read_state_init;
  static nanotube_tap_packet_read_req req = {0};
  static nanotube_tap_packet_read_resp resp;
  static uint8_t result_buffer[simple_bus::data_bytes];

  simple_bus::word word;
  if (!nanotube_channel_try_read(context, PACKETS_IN, &word, sizeof(word))) {
    nanotube_thread_wait();
    return;
  }

  bool eop = word.get_eop();
  if (logic_0_sop) {
    auto *data = word.data_ptr(sizeof(simple_bus::header));
    req.valid = true;
    req.read_offset = ( ( uint16_t(data[0]) << 0 ) +
                        ( uint16_t(data[1]) << 8 ) +
                        sizeof(simple_bus::header) );
    req.read_length = ( ( uint16_t(data[2]) << 0 ) +
                        ( uint16_t(data[3]) << 8 ) );
    logic_0_extend_word = (data[4] != 0);
  }
  logic_0_sop = eop;

  /* Invoke the packet read tap. */
  static_assert(sizeof(result_buffer) <= (1U<<result_buffer_index_bits),
                "Not enough index bits.");
  nanotube_tap_packet_read_sb(
    sizeof(result_buffer), result_buffer_index_bits,
    &resp, result_buffer,
    &state,
    &word, &req);

  if (resp.valid) {
#if __clang__
#pragma clang loop unroll(enable)
#endif
    for (unsigned i=0; i<simple_bus::data_bytes; i++) {
      uint8_t val = result_buffer[i];
      if (i == sizeof(simple_bus::header)+0)
        val ^= ( (resp.result_length>>0) & 0xff );
      else if (i == sizeof(simple_bus::header)+1)
        val ^= ( (resp.result_length>>8) & 0xff );
      else
        val ^= 0x80;
      word.data_ref(i) = val;
    }

    if (logic_0_extend_word && eop)
      word.set_control(eop, word.get_err(), 0);
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
