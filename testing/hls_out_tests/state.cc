/**************************************************************************\
*//*! \file state.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  A state variable test for the HLS output pass.
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

static uint8_t s8_state = 10;
static uint8_t a8_state[2] = { 20, 30 };
static uint64_t s64_state = 40;
static uint64_t a64_state[2] = { 50, 60 };

// This is here to stop Clang flattening the arrays.
extern "C"
void shuffle(int index, uint8_t *p8, uint64_t *p64)
{
  uint8_t tmp8 = *p8;
  *p8 = a8_state[index];
  a8_state[index] = s8_state;
  s8_state = tmp8;
  uint64_t tmp64 = *p64;
  *p64 = a64_state[index];
  a64_state[index] = s64_state;
  s64_state = tmp64;
}

extern "C"
void logic_0(nanotube_context_t* context, void *arg)
{
  simple_bus::word word;
  if (!nanotube_channel_try_read(context, PACKETS_IN, &word,
                                 sizeof(word))) {
    nanotube_thread_wait();
    return;
  }
  s8_state += word.bytes[0];
  word.bytes[0] = s8_state;
  a8_state[1] += word.bytes[1];
  word.bytes[1] = a8_state[1];
  s64_state += word.bytes[2];
  word.bytes[2] = s64_state;
  a64_state[1] += word.bytes[3];
  word.bytes[3] = a64_state[1];
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
