/**************************************************************************\
*//*! \file test_rotate_down.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A simple test for nanotube_rotate_down.
**   \date  2020-09-23
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_packet_taps_core.h"
#include "test.hpp"

#include <cassert>
#include <cstring>
#include <iostream>

///////////////////////////////////////////////////////////////////////////

void test_rotate_down_case(size_t out_length,
                           size_t rot_length,
                           size_t in_length,
                           size_t rot_amount_bits,
                           size_t rot_amount)
{
  size_t pad = 16;
  uint8_t rd_in_buf[in_length + 2*pad];
  uint8_t rd_out_buf[out_length + 2*pad];
  uint8_t in_buf[in_length + 2*pad];
  uint8_t pad_buf[rot_length];
  uint8_t exp_buf[out_length + 2*pad];
  uint8_t out_buf[out_length + 2*pad];

  size_t rot_amount_mask = (size_t(1) << rot_amount_bits) - 1;
  size_t rot_amount_masked = rot_amount & rot_amount_mask;

  assert(in_length <= rot_length);
  assert(out_length <= rot_length);

  for (size_t i = 0; i < sizeof(rd_in_buf); i++)
    rd_in_buf[i] = rand() & 0xff;
  for (size_t i = 0; i < sizeof(rd_out_buf); i++)
    rd_out_buf[i] = rand() & 0xff;
  memcpy(in_buf, rd_in_buf, sizeof(in_buf));
  memcpy(out_buf, rd_out_buf, sizeof(out_buf));
  memcpy(exp_buf, rd_out_buf, sizeof(out_buf));

  for (size_t i = 0; i < sizeof(pad_buf); i++) {
    pad_buf[i] = (i >= in_length ? 0 : rd_in_buf[i+pad]);
  }

  for (size_t i = 0; i < out_length; i++) {
    size_t rot_idx = (i+rot_amount_masked) % rot_length;
    exp_buf[i + pad] = pad_buf[rot_idx];
  }

  nanotube_rotate_down(out_length, rot_length, in_length, rot_amount_bits,
                       out_buf + pad, in_buf + pad, rot_amount);

  if (test_verbose) {
    std::cout << "Test case: in_len=" << in_length
              << ", rot_len=" << rot_length
              << ", out_len=" << out_length
              << ", rot_amount_bits=" << rot_amount_bits
              << ", rot_amount=" << rot_amount
              << "\n";
    test_dump_buffer("rd_in_buf", rd_in_buf, sizeof(rd_in_buf));
    test_dump_buffer("pad_buf", pad_buf, sizeof(pad_buf));
    test_dump_buffer("exp_buf", exp_buf, sizeof(exp_buf));
    test_dump_buffer("out_buf", out_buf, sizeof(out_buf));
  }

  assert_array_eq(in_buf, rd_in_buf, sizeof(rd_in_buf));
  assert_array_eq(out_buf, exp_buf, sizeof(exp_buf));
}

void test_rotate_down()
{
  static const uint8_t zeros[128] = {0};
  uint8_t buffer_in[128];
  uint8_t buffer_out[128];
  size_t n_in = sizeof(buffer_in);
  size_t n_out = sizeof(buffer_out);
  uint8_t rand_data[n_in + n_out];
  const uint8_t *const rd_buffer_in = rand_data + 0;
  const uint8_t *const rd_buffer_out = rand_data + n_in;

  for (size_t i = 0; i < sizeof(rand_data); i++)
    rand_data[i] = rand() & 0xff;

  std::cout << "Case 0: No call to rotate_down.\n";

  memcpy(buffer_in, rd_buffer_in, n_in);
  memcpy(buffer_out, rd_buffer_out, n_out);
  assert_array_eq(buffer_in, rd_buffer_in, n_in);
  assert_array_eq(buffer_out, rd_buffer_out, n_out);

  std::cout << "Case 1: 8 byte input, 8 byte rot_buf, 8 byte output, rotate by 0.\n";

  nanotube_rotate_down(8, 8, 8, 3, buffer_out+32, buffer_in+32, 0);
  assert_array_eq(buffer_in, rd_buffer_in, n_in);
  assert_array_eq(buffer_out, rd_buffer_out, 32);
  assert_array_eq(buffer_out+32, rd_buffer_in+32, 8);
  assert_array_eq(buffer_out+40, rd_buffer_out+40, n_out-40);

  test_rotate_down_case(8, 8, 8, 3, 0);

  std::cout << "Case 2: 8 byte input, 8 byte rot_buf, 8 byte output, rotate by 5.\n";

  memcpy(buffer_in, rd_buffer_in, n_in);
  memcpy(buffer_out, rd_buffer_out, n_out);
  nanotube_rotate_down(8, 8, 8, 3, buffer_out+32, buffer_in+32, 5);
  assert_array_eq(buffer_in, rd_buffer_in, n_in);
  assert_array_eq(buffer_out, rd_buffer_out, 32);
  assert_array_eq(buffer_out+32, rd_buffer_in+32+5, 8-5);
  assert_array_eq(buffer_out+32+8-5, rd_buffer_in+32, 5);
  assert_array_eq(buffer_out+32+8, rd_buffer_out+32+8, n_out-(32+8));

  test_rotate_down_case(8, 8, 8, 3, 5);

  std::cout << "Case 3: 13 byte input, 13 byte rot_buf, 13 byte output, rotate by 11.\n";

  memcpy(buffer_in, rd_buffer_in, n_in);
  memcpy(buffer_out, rd_buffer_out, n_out);
  nanotube_rotate_down(13, 13, 13, 4, buffer_out+32, buffer_in+32, 11);
  assert_array_eq(buffer_in, rd_buffer_in, n_in);
  assert_array_eq(buffer_out, rd_buffer_out, 32);
  assert_array_eq(buffer_out+32, rd_buffer_in+32+11, 13-11);
  assert_array_eq(buffer_out+32+13-11, rd_buffer_in+32, 11);
  assert_array_eq(buffer_out+32+13, rd_buffer_out+32+13, n_out-(32+13));

  test_rotate_down_case(13, 13, 13, 4, 11);

  std::cout << "Case 4: 21 byte input, 21 byte rot_buf, 13 byte output, rotate by 10.\n";

  memcpy(buffer_in, rd_buffer_in, n_in);
  memcpy(buffer_out, rd_buffer_out, n_out);
  nanotube_rotate_down(13, 21, 21, 5, buffer_out+32, buffer_in+32, 10);
  assert_array_eq(buffer_in, rd_buffer_in, n_in);
  assert_array_eq(buffer_out, rd_buffer_out, 32);
  assert_array_eq(buffer_out+32, rd_buffer_in+32+10, 21-10);
  assert_array_eq(buffer_out+32+11, rd_buffer_in+32, 2);
  assert_array_eq(buffer_out+32+13, rd_buffer_out+32+13, n_out-(32+13));

  test_rotate_down_case(13, 21, 21, 5, 10);

  std::cout << "Case 5: 21 byte input, 25 byte rot_buf, 25 byte output, rotate by 10.\n";

  memcpy(buffer_in, rd_buffer_in, n_in);
  memcpy(buffer_out, rd_buffer_out, n_out);
  nanotube_rotate_down(25, 25, 21, 5, buffer_out+32, buffer_in+32, 10);

  if (test_verbose) {
    test_dump_buffer("rd_buffer_in", rd_buffer_in, n_in);
    test_dump_buffer("buffer_in", buffer_in, sizeof(buffer_in));
    test_dump_buffer("buffer_out", buffer_out, sizeof(buffer_out));
  }

  assert_array_eq(buffer_in, rd_buffer_in, n_in);
  assert_array_eq(buffer_out, rd_buffer_out, 32);
  assert_array_eq(buffer_out+32, rd_buffer_in+32+10, 21-10);
  assert_array_eq(buffer_out+32+21-10, zeros, 25-21);
  assert_array_eq(buffer_out+32+25-10, rd_buffer_in+32, 10);
  assert_array_eq(buffer_out+32+25, rd_buffer_out+32+25, n_out-32-25);

  test_rotate_down_case(25, 25, 21, 5, 10);
}

int main(int argc, char *argv[])
{
  test_init(argc, argv);
  test_rotate_down();
  return test_fini();
}

///////////////////////////////////////////////////////////////////////////
