/**************************************************************************\
*//*! \file switch.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  A simple test for the HLS output pass.
**   \date  2020-08-08
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_api.h"
#include "simple_bus.hpp"

enum channel
{
  PACKETS_IN,
  PACKETS_OUT,
};

extern "C"
void logic_0(nanotube_context_t* context, void *arg)
{
  simple_bus::word word;
  if (!nanotube_channel_try_read(context, PACKETS_IN, &word,
                                 sizeof(word))) {
    nanotube_thread_wait();
    return;
  }
  switch (word.bytes[0] & 0x3) {
  case 0:
    // Unicast globally assigned.  Convert to a local address.
    word.bytes[0] ^= 2;
    break;
  case 1:
    // Multicast globally assigned.  Do nothing.
    break;
  case 2:
    // Unicast locally assigned.  Flip the top bit.
    word.bytes[0] ^= 0x80;
    break;
  case 3:
    // Multicast locally assigned.  Do nothing.
    break;
  default:
    // Should not happen.
    break;
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

  nanotube_context *context = nanotube_context_create();
  nanotube_context_add_channel(context, PACKETS_IN, packets_in,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context, PACKETS_OUT, packets_out,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(context, "stage_0", &logic_0, nullptr, 0);
}
