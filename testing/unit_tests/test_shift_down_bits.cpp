/**************************************************************************\
*//*! \file test_shift_down_bits.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A simple test for nanotube_shift_down_bits.
**   \date  2020-12-02
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_packet_taps_core.h"
#include "test.hpp"

#include <cstring>
#include <iostream>

///////////////////////////////////////////////////////////////////////////

void test_shift_down_bits()
{
  uint8_t buffer_in[16];
  uint8_t buffer_out[16];
  size_t n_in = sizeof(buffer_in);
  size_t n_out = sizeof(buffer_out);
  uint8_t rand_data[n_in + n_out];
  const uint8_t *const rd_buffer_in = rand_data + 0;
  const uint8_t *const rd_buffer_out = rand_data + n_in;

  for (size_t i = 0; i < sizeof(rand_data); i++)
    rand_data[i] = rand() & 0xff;

  std::cout << "Case 0: No call to shift_down_bits.\n";

  memcpy(buffer_in, rd_buffer_in, n_in);
  memcpy(buffer_out, rd_buffer_out, n_out);
  assert_array_eq(buffer_in, rd_buffer_in, n_in);
  assert_array_eq(buffer_out, rd_buffer_out, n_out);

  std::cout << "Case 1: 8 byte output, shift by 0.\n";

  nanotube_shift_down_bits(8, buffer_out+4, buffer_in+4, 0);
  assert_array_eq(buffer_in, rd_buffer_in, n_in);
  assert_array_eq(buffer_out, rd_buffer_out, 4);
  assert_array_eq(buffer_out+4, rd_buffer_in+4, 8);
  assert_array_eq(buffer_out+12, rd_buffer_out+12, n_out-12);

  std::cout << "Case 2: 8 byte output, shift by 5.\n";

  nanotube_shift_down_bits(8, buffer_out+4, buffer_in+4, 5);
  assert_array_eq(buffer_in, rd_buffer_in, n_in);
  assert_array_eq(buffer_out, rd_buffer_out, 4);
  for (int idx=0; idx<8; idx++) {
    uint8_t byte0 = buffer_in[idx+4];
    uint8_t byte1 = buffer_in[idx+5];
    uint8_t exp_val = (byte0 >> 5) | (byte1 << 3);
    assert_eq(buffer_out[idx+4], exp_val);
  }
  assert_array_eq(buffer_out+12, rd_buffer_out+12, 4);
}

int main(int argc, char *argv[])
{
  test_init(argc, argv);
  test_shift_down_bits();
  return test_fini();
}

///////////////////////////////////////////////////////////////////////////
