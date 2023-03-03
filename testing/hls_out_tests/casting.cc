/**************************************************************************\
*//*! \file casting.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  A casting test for the HLS output pass.
**   \date  2020-08-11
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
  uint32_t u0 = word.bytes[0];
  uint32_t u1 = word.bytes[1];
  uint32_t u2 = u0 + u1;
  if (u2 < 16)
    word.bytes[2] ^= 1;
  int32_t i0 = int8_t(word.bytes[0]);
  int32_t i1 = int8_t(word.bytes[1]);
  int32_t i2 = i0 + i1;
  if (i2 < 16)
    word.bytes[3] ^= 1;
  word.bytes[4] = i2;
  
  nanotube_channel_write(context, PACKETS_OUT, &word,
                         sizeof(word));
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
