/**************************************************************************\
*//*! \file tap_packet_write.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  A packet passing test for the packet write tap.
**   \date  2021-01-13
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
  static const uint8_t data_buffer_index_bits = 6;

  static bool state_sop = true;
  static nanotube_tap_packet_write_state state =
    nanotube_tap_packet_write_state_init;
  static nanotube_tap_packet_write_req req = {0};
  static uint8_t data_mode = 0;
  static uint8_t data_val = 0;
  static uint8_t mask_mode = 0;
  static uint8_t mask_val = 0;

  // Read a packet word from the channel.
  simple_bus::word word_in;
  if (!nanotube_channel_try_read(context, PACKETS_IN, &word_in,
                                 sizeof(word_in))) {
    nanotube_thread_wait();
    return;
  }

  // Determine whether this is a SOP and/or EOP.
  bool sop = state_sop;
  bool eop = word_in.get_eop();
  state_sop = eop;


  // Extract the request from the first word.
  if (sop) {
    auto *data = word_in.data_ptr(sizeof(simple_bus::header));
    req.valid = true;
    req.write_offset = ( ( uint16_t(data[0]) << 0 ) +
                         ( uint16_t(data[1]) << 8 ) +
                         sizeof(simple_bus::header) );
    req.write_length = ( ( uint16_t(data[2]) << 0 ) +
                         ( uint16_t(data[3]) << 8 ) );
    data_mode = data[4];
    data_val  = data[5] ^ 0x80;
    mask_mode = data[6];
    mask_val  = data[7];
  }

  // Prepare the data and mask buffers.
  uint8_t data_buffer[simple_bus::data_bytes];
  for (unsigned i=0; i<sizeof(data_buffer); i++) {
    switch (data_mode & 1) {
    case 0: data_buffer[i] = data_val;   break;
    case 1: data_buffer[i] = data_val^i; break;
    }
  }

  uint8_t mask_buffer[(sizeof(data_buffer)+7)/8];
  for (unsigned i=0; i<sizeof(mask_buffer); i++) {
    switch (mask_mode & 1) {
    case 0: mask_buffer[i] = mask_val;   break;
    case 1: mask_buffer[i] = mask_val^i; break;
    }
  }

  /* Invoke the packet write tap. */
  static_assert(sizeof(data_buffer) <= (1U<<data_buffer_index_bits),
                "Not enough index bits.");
  simple_bus::word word_out;
  nanotube_tap_packet_write_sb(
    sizeof(data_buffer), data_buffer_index_bits,
    &word_out,
    &state,
    &word_in, &req,
    data_buffer, mask_buffer);

  nanotube_channel_write(context, PACKETS_OUT, &word_out, sizeof(word_out));
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
