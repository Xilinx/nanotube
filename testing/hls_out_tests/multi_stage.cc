/**************************************************************************\
*//*! \file multi_stage.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  A multi-stage test for the HLS output pass.
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
  PACKETS_IN = 20,
  PACKETS_INTRA,
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
  if ((word.bytes[0] & 1) == 0)
      word.bytes[0] ^= 0x02;
  nanotube_channel_write(context, PACKETS_INTRA, &word, sizeof(word));
}

extern "C"
void logic_1(nanotube_context_t* context, void *arg)
{
  simple_bus::word word;
  if (!nanotube_channel_try_read(context, PACKETS_INTRA, &word,
                                 sizeof(word))) {
    nanotube_thread_wait();
    return;
  }
  if ((word.bytes[0] & 1) == 0)
      word.bytes[0] ^= 0x02;
  nanotube_channel_write(context, PACKETS_OUT, &word, sizeof(word));
}

extern "C"
void nanotube_setup()
{
  nanotube_channel_t *packets_in =
    nanotube_channel_create("packets_in", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_intra =
    nanotube_channel_create("packets_intra", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_out =
    nanotube_channel_create("packets_out", simple_bus::total_bytes, 16);

  if (simple_bus::sideband_bytes != 0) {
    nanotube_channel_set_attr(packets_in, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
    nanotube_channel_set_attr(packets_intra, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
    nanotube_channel_set_attr(packets_out, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
  }

  nanotube_context_t *context_0 = nanotube_context_create();
  nanotube_context_add_channel(context_0, PACKETS_IN, packets_in,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context_0, PACKETS_INTRA, packets_intra,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(context_0, "stage_0", &logic_0, nullptr, 0);

  nanotube_context_t *context_1 = nanotube_context_create();
  nanotube_context_add_channel(context_1, PACKETS_INTRA, packets_intra,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context_1, PACKETS_OUT, packets_out,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(context_1, "stage_1", &logic_1, nullptr, 0);
}
