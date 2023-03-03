/**************************************************************************\
*//*! \file tap_packet_resize.cc
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
  PACKETS_THROUGH,
  CWORD_THROUGH,
  PACKETS_OUT,
};

static nanotube_tap_packet_resize_req_t logic_0_req;
static nanotube_tap_packet_resize_ingress_state_t logic_0_state;
static bool logic_0_new_req = true;

extern "C"
void logic_0(nanotube_context_t* context, void *arg)
{
  simple_bus::word packet_word;
  nanotube_tap_packet_resize_cword_t cword;
  nanotube_tap_offset_t new_length;

  if (!nanotube_channel_try_read(context, PACKETS_IN, &packet_word,
                                 sizeof(packet_word))) {
    nanotube_thread_wait();
    return;
  }

  if (logic_0_new_req) {
    uint8_t *data = packet_word.data_ptr(sizeof(simple_bus::header));
    logic_0_req.write_offset = ( ( (uint16_t(data[0]) << 0) |
                                   (uint16_t(data[1]) << 8) ) +
                                 sizeof(simple_bus::header) );
    logic_0_req.delete_length = ( (uint16_t(data[2]) << 0) |
                                  (uint16_t(data[3]) << 8) );
    logic_0_req.insert_length = ( (uint16_t(data[4]) << 0) |
                                  (uint16_t(data[5]) << 8) );
  }

  nanotube_tap_packet_resize_ingress_sb(
    &logic_0_new_req,
    &cword, &new_length,
    &logic_0_state,
    &logic_0_req,
    &packet_word);

  nanotube_channel_write(context, PACKETS_THROUGH, &packet_word,
                         sizeof(packet_word));
  nanotube_channel_write(context, CWORD_THROUGH, &cword,
                         sizeof(cword));
}

static nanotube_tap_packet_resize_egress_state_t logic_1_state;
static simple_bus::word logic_1_word_state;
static nanotube_tap_packet_resize_cword_t logic_1_cword;
static simple_bus::word logic_1_packet_word;
static bool logic_1_have_packet = false;
static bool logic_1_have_cword = false;

extern "C"
void logic_1(nanotube_context_t* context, void *arg)
{
  if (!logic_1_have_packet) {
    if (!nanotube_channel_try_read(context, PACKETS_THROUGH,
                                  &logic_1_packet_word,
                                  sizeof(logic_1_packet_word))) {
      nanotube_thread_wait();
      return;
    }
    logic_1_have_packet = true;
  }

  if (!logic_1_have_cword) {
    if (!nanotube_channel_try_read(context, CWORD_THROUGH,
                                   &logic_1_cword,
                                   sizeof(logic_1_cword))) {
      nanotube_thread_wait();
      return;
    }
    logic_1_have_cword = true;
  }

  bool input_done_out;
  bool packet_done_out;
  bool packet_valid_out;
  simple_bus::word packet_word_out;
  nanotube_tap_packet_resize_egress_sb(
    &input_done_out,
    &packet_done_out,
    &packet_valid_out,
    &packet_word_out,
    &logic_1_state,
    &logic_1_word_state,
    &logic_1_cword,
    &logic_1_packet_word,
    -1 /* unused on simple bus, should be logic_0:new_length */);

  if (input_done_out) {
    logic_1_have_packet = false;
    logic_1_have_cword = false;
  }

  if (packet_valid_out) {
    nanotube_channel_write(context, PACKETS_OUT,
                           &packet_word_out,
                           sizeof(packet_word_out));
  }
}

extern "C"
void nanotube_setup()
{
  nanotube_tap_packet_resize_ingress_state_init(&logic_0_state);
  nanotube_tap_packet_resize_egress_state_init(&logic_1_state);

  nanotube_channel_t *packets_in =
    nanotube_channel_create("packets_in", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_through =
    nanotube_channel_create("packets_through", simple_bus::total_bytes, 16);
  nanotube_channel_t *cword_through =
    nanotube_channel_create("cword_through", sizeof(nanotube_tap_packet_resize_cword_t), 16);
  nanotube_channel_t *packets_out =
    nanotube_channel_create("packets_out", simple_bus::total_bytes, 16);

  if (simple_bus::sideband_bytes != 0) {
    nanotube_channel_set_attr(packets_in, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
    nanotube_channel_set_attr(packets_through, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
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

  nanotube_context_t *context_0 = nanotube_context_create();
  nanotube_context_add_channel(context_0, PACKETS_IN, packets_in, NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context_0, PACKETS_THROUGH, packets_through, NANOTUBE_CHANNEL_WRITE);
  nanotube_context_add_channel(context_0, CWORD_THROUGH, cword_through, NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(context_0, "stage_0", &logic_0, nullptr, 0);

  nanotube_context_t *context_1 = nanotube_context_create();
  nanotube_context_add_channel(context_1, PACKETS_THROUGH, packets_through, NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context_1, CWORD_THROUGH, cword_through, NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context_1, PACKETS_OUT, packets_out, NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(context_1, "stage_1", &logic_1, nullptr, 0);
}
