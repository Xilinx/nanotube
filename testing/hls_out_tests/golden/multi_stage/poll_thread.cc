///////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////
#include "stages.hh"
#include "nanotube_api.h"

static void poll_thread(nanotube_context_t* context, void *arg)
{
  hls::stream<bytes<65> > stage0_port0_stream("stage0_port0");
  bytes<65>               stage0_port0_buffer;
  hls::stream<bytes<65> > stage0_port1_stream("stage0_port1");
  bytes<65>               stage0_port1_buffer;
  hls::stream<bytes<65> > stage1_port0_stream("stage1_port0");
  bytes<65>               stage1_port0_buffer;
  hls::stream<bytes<65> > stage1_port1_stream("stage1_port1");
  bytes<65>               stage1_port1_buffer;

  while (true) {
    bool active = false;

    if (stage0_port0_stream.empty()) {
      if (nanotube_channel_try_read(context, 0, &stage0_port0_buffer, 65)) {
        active = true;
        stage0_port0_stream.write(stage0_port0_buffer);
      }
    }
    if (stage0_port1_stream.empty())
      stage_0(
        stage0_port0_stream,
        stage0_port1_stream);
    if (!stage0_port1_stream.empty()) {
      if (nanotube_channel_has_space(context, 1)) {
        active = true;
        stage0_port1_stream.read(stage0_port1_buffer);
        nanotube_channel_write(context, 1, &stage0_port1_buffer, 65);
      }
    }

    if (stage1_port0_stream.empty()) {
      if (nanotube_channel_try_read(context, 1, &stage1_port0_buffer, 65)) {
        active = true;
        stage1_port0_stream.write(stage1_port0_buffer);
      }
    }
    if (stage1_port1_stream.empty())
      stage_1(
        stage1_port0_stream,
        stage1_port1_stream);
    if (!stage1_port1_stream.empty()) {
      if (nanotube_channel_has_space(context, 2)) {
        active = true;
        stage1_port1_stream.read(stage1_port1_buffer);
        nanotube_channel_write(context, 2, &stage1_port1_buffer, 65);
      }
    }

    if (!active)
      nanotube_thread_wait();
  }
}

extern "C"
void nanotube_setup()
{
  nanotube_context *context = nanotube_context_create();
  nanotube_channel_t *channels[3];

  channels[0] = nanotube_channel_create("packets_in", 65, 16);
  nanotube_context_add_channel(context, 0, channels[0], NANOTUBE_CHANNEL_READ);

  channels[1] = nanotube_channel_create("packets_intra", 65, 16);
  nanotube_context_add_channel(context, 1, channels[1], NANOTUBE_CHANNEL_READ | NANOTUBE_CHANNEL_WRITE);

  channels[2] = nanotube_channel_create("packets_out", 65, 16);
  nanotube_context_add_channel(context, 2, channels[2], NANOTUBE_CHANNEL_WRITE);

  nanotube_thread_create(context, "poll_thread", poll_thread, nullptr, 0);
}
