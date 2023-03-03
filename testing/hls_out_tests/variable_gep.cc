/**************************************************************************\
*//*! \file variable_gep.cc
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
#include <cstdint>

enum channel {
  DATA_IN,
  DATA_OUT,
};

typedef struct my_data
{
  uint32_t index;
  uint16_t bytes[16];
} my_data_t;

extern "C"
void logic_0(nanotube_context_t* context, void *arg)
{
  my_data_t data;
  if (!nanotube_channel_try_read(context, DATA_IN, &data, sizeof(data))) {
    nanotube_thread_wait();
    return;
  }
  uint8_t val = data.bytes[data.index & 0xf];
  nanotube_channel_write(context, DATA_OUT, &val, 1);
}

extern "C"
void nanotube_setup()
{
  nanotube_channel_t *packets_in =
    nanotube_channel_create("packets_in", sizeof(my_data_t), 16);
  nanotube_channel_t *packets_out =
    nanotube_channel_create("packets_out", 1, 16);

  nanotube_context_t *context = nanotube_context_create();
  nanotube_context_add_channel(context, DATA_IN, packets_in, NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(context, DATA_OUT, packets_out, NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(context, "stage_0", &logic_0, nullptr, 0);
}
