/**************************************************************************\
*//*! \file test_duplicate_bits.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A simple test for nanotube_duplicate_bits.
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

void test_duplicate_bits_case(uint32_t input_bit_length,
                              uint32_t padded_bit_length)
{
  uint32_t input_byte_length = (input_bit_length+7)/8;
  uint32_t output_byte_length = (2*padded_bit_length+7)/8;
  uint32_t pad = 8;
  uint8_t buffer_in[input_byte_length + 2*pad];
  uint8_t buffer_out[output_byte_length + 2*pad];
  uint8_t buffer_exp[output_byte_length + 2*pad];
  uint8_t rd_in[sizeof(buffer_in)];
  uint8_t rd_out[sizeof(buffer_out)];

  for (size_t i = 0; i < sizeof(rd_in); i++)
    rd_in[i] = rand() & 0xff;
  for (size_t i = 0; i < sizeof(rd_out); i++)
    rd_out[i] = rand() & 0xff;

  memcpy(buffer_in, rd_in, sizeof(buffer_in));
  memcpy(buffer_out, rd_out, sizeof(buffer_out));
  memcpy(buffer_exp, rd_out, sizeof(buffer_exp));

  if (test_verbose) {
    std::cout << "Invoking nanotube_duplicate_bits with"
              << " input_bit_length=" << input_bit_length
              << ", padded_bit_length=" << padded_bit_length
              << "\n";
  }

  nanotube_duplicate_bits(buffer_out+pad, buffer_in+pad,
                          input_bit_length, padded_bit_length);

  // Set the expected contents of the output buffer.
  memset(buffer_exp+pad, 0, output_byte_length);
  for (size_t i = 0; i<input_bit_length; i++) {
    uint8_t byte_in = rd_in[i/8 + pad];
    uint8_t bit_in = (byte_in >> (i%8)) & 1;
    size_t out_idx_0 = i;
    buffer_exp[out_idx_0/8 + pad] |= (bit_in << (out_idx_0%8));
    size_t out_idx_1 = i + padded_bit_length;
    buffer_exp[out_idx_1/8 + pad] |= (bit_in << (out_idx_1%8));
  }

  if (test_verbose) {
    test_dump_buffer("buffer_in", buffer_in, sizeof(buffer_in));
    test_dump_buffer("buffer_out", buffer_out, sizeof(buffer_out));
    test_dump_buffer("buffer_exp", buffer_exp, sizeof(buffer_exp));
  }

  assert_array_eq(buffer_in, rd_in, sizeof(buffer_in));
  assert_array_eq(buffer_out, buffer_exp, sizeof(buffer_exp));
}

void test_duplicate_bits()
{
  std::cout << "Case 1: 40 bits of input, no padding.\n";
  test_duplicate_bits_case(40, 40);

  std::cout << "Case 2: 27 bits of input, no padding.\n";
  test_duplicate_bits_case(27, 27);

  std::cout << "Case 3: 19 bits of input padded to 23.\n";
  test_duplicate_bits_case(19, 23);

  for (uint32_t ibl=1; ibl<=32; ibl++) {
    for (uint32_t pbl=ibl; pbl<=32; pbl++) {
      test_duplicate_bits_case(ibl, pbl);
    }
  }
}

int main(int argc, char *argv[])
{
  test_init(argc, argv);
  test_duplicate_bits();
  return test_fini();
}

///////////////////////////////////////////////////////////////////////////
