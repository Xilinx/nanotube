/**************************************************************************\
*//*! \file pointers.cc
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

enum channel {
  PACKETS_IN,
  PACKETS_OUT,
};

extern "C"
void logic_0(nanotube_context_t* context, void *arg)
{
  simple_bus::word word;
  if (!nanotube_channel_try_read(context, PACKETS_IN, &word, sizeof(word))) {
    nanotube_thread_wait();
    return;
  }
  uint8_t *buffer;
  if (word.bytes[0] == 0) {
    word.bytes[1] = 42;
    buffer = word.bytes + 1;
  } else {
    buffer = word.bytes + 0;
  }
  if (word.bytes[2] == 0)
    buffer += 2;
  nanotube_channel_write(context, PACKETS_OUT, buffer, 1);
}

extern "C"
void nanotube_setup()
{
  nanotube_channel_t *packets_in =
    nanotube_channel_create("packets_in", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_out =
    nanotube_channel_create("packets_out", 1, 16);

  if (simple_bus::sideband_bytes != 0) {
    nanotube_channel_set_attr(packets_in, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
  }

  nanotube_context_t *context = nanotube_context_create();
  nanotube_context_add_channel(context, PACKETS_IN, packets_in, NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context, PACKETS_OUT, packets_out, NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(context, "stage_0", &logic_0, nullptr, 0);
}
