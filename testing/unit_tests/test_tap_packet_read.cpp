/**************************************************************************\
*//*! \file test_tap_packet_read.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A simple test for the packet read tap core logic.
**   \date  2020-09-29
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_packet_taps_core.h"
#include "test.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

///////////////////////////////////////////////////////////////////////////

void test_tap_packet_read_core_case(
  struct nanotube_tap_packet_read_state *state,
  uint16_t result_buffer_length,
  uint8_t result_buffer_index_bits,
  uint16_t packet_buffer_length,
  uint8_t packet_buffer_index_bits,
  uint16_t read_offset,
  uint16_t read_length,
  uint16_t req_valid_offset,
  uint16_t packet_length)
{
  // The amount of padding to make sure there is no buffer over-run.
  uint16_t pad = 8;

  // The lengths of the buffers passed to the packet tap.
  uint16_t rb_len = result_buffer_length;
  uint16_t pb_len = packet_buffer_length;

  // The word index at which req.valid is set.
  uint16_t req_valid_index = ( req_valid_offset / pb_len );

  // The index of the word containing the first byte read.
  uint16_t read_start_index = ( read_offset / pb_len );

  // The byte after the last one read.
  uint16_t read_end_offset = (read_offset + read_length);

  // The offset into the packet at which the read completes.
  uint16_t resp_valid_offset =
    std::min(read_length == 0 ? req_valid_offset : read_end_offset-1U,
             packet_length-1U);

  // The word index at which the read completes and resp.valid should
  // be set.
  uint16_t resp_valid_index = (resp_valid_offset / pb_len);

  // The expected number of bytes read.
  uint16_t resp_length = ( std::min(read_end_offset, packet_length) -
                           std::min(read_offset, packet_length) );

  // The number of the packet words required.
  uint16_t packet_num_words = ( (packet_length + (pb_len-1)) / pb_len );

  // The number of bytes required to fill all the packet words.  This
  // can be larger than the packet length since this value includes
  // the padding bytes in the final word.
  uint16_t pd_len = packet_num_words * pb_len;

  if (test_verbose) {
    std::cout << "  Parameters: rbl=" << result_buffer_length
              << " pbl=" << packet_buffer_length
              << " ro=" << read_offset
              << " rl=" << read_length
              << " rvo=" << req_valid_offset
              << " pl=" << packet_length
              << "\n";
    std::cout << "  Expecting: rvo=" << uint32_t(resp_valid_offset)
              << ", rvi=" << uint32_t(resp_valid_index)
              << ", rl=" << uint32_t(resp_length)
              << "\n";
  }

  assert(req_valid_index <= read_start_index);
  assert(resp_valid_index < packet_num_words);

  // Variables to pass to the packet tap.
  struct nanotube_tap_packet_read_resp resp;
  uint8_t result_buffer[rb_len+2*pad];
  uint8_t packet_buffer[pb_len+2*pad];
  struct nanotube_tap_packet_read_req req;

  // An array of zeros for checking that the result buffer is
  // correctly padded with zeros.
  uint8_t zeros[rb_len];
  memset(zeros, 0, sizeof(zeros));

  // An array of random data for initializing input buffers.
  uint8_t rand_data[(rb_len+2*pad) + (pd_len+2*pad)];
  const uint8_t *rd_rb = rand_data + 0;
  const uint8_t *rd_pd = rand_data + (rb_len+2*pad);

  for (size_t i = 0; i < sizeof(rand_data); i++)
    rand_data[i] = rand() & 0xff;

  // Fill the result buffer with random data to make sure everything
  // gets written.
  memcpy(result_buffer, rd_rb, sizeof(result_buffer));

  for (uint16_t index=0; index<packet_num_words; index++) {
    // Fill in the request, setting the valid bit at the requested
    // time.
    if (index >= req_valid_index) {
      req.valid = 1;
      req.read_offset = read_offset;
      req.read_length = read_length;
    } else {
      req.valid = 0;
      req.read_offset = rand() & 0xffff;
      req.read_length = rand() & 0xffff;
    }

    // Determine the packet arguments provided to the packet tap.
    bool packet_word_eop = (index == packet_num_words-1);
    uint16_t packet_word_length = pb_len;
    if (packet_word_eop)
      packet_word_length = packet_length - (packet_num_words-1)*pb_len;

    // Initialise the packet buffer provided to the packet tap.
    memcpy(packet_buffer, rd_pd, pad);
    memcpy(packet_buffer+pad, rd_pd + pad + index*pb_len, pb_len);
    memcpy(packet_buffer+pad+pb_len, rd_pd + pad + pd_len, pad);

    if (test_verbose) {
      std::cout << "  Invoking tap: i=" << uint16_t(index)
                << " rbl=" << uint32_t(result_buffer_length)
                << " rbib=" << uint32_t(result_buffer_index_bits)
                << " pbl=" << uint32_t(packet_buffer_length)
                << " pbib=" << uint32_t(packet_buffer_index_bits)
                << " eop=" << uint32_t(packet_word_eop)
                << " pwl=" << uint32_t(packet_word_length)
                << " rv=" << uint32_t(req.valid)
                << " ro=" << uint32_t(req.read_offset)
                << " rl=" << uint32_t(req.read_length)
                << "\n";
    }

    // Call the packet tap.
    nanotube_tap_packet_read_core(
      result_buffer_length,
      result_buffer_index_bits,
      packet_buffer_length,
      packet_buffer_index_bits,
      &resp, result_buffer+pad,
      state,
      packet_buffer+pad, packet_word_eop, packet_word_length, &req);

    if (test_verbose) {
      std::cout << "  Response: rv=" << uint32_t(resp.valid);
      if (resp.valid)
        std::cout << " rl=" << uint32_t(resp.result_length);
      std::cout << "\n";
    }

    // Check the arrays which should not be modified.
    assert_array_eq(packet_buffer, rd_pd, pad);
    assert_array_eq(packet_buffer+pad, rd_pd + pad + index*pb_len, pb_len);
    assert_array_eq(packet_buffer+pad+pb_len, rd_pd + pad + pd_len, pad);
    assert_array_eq(result_buffer, rd_rb, pad);
    assert_array_eq(result_buffer+result_buffer_length+pad,
                    rd_rb+result_buffer_length+pad, pad);

    // Make sure the valid bit is set at the right time.
    assert_eq(resp.valid, index==resp_valid_index);

    // Check the response if it was produced and should have been
    // produced.
    if (resp.valid && index==resp_valid_index) {
      // Check the response length.
      assert_eq(resp.result_length, resp_length);

      // Check the data copied from the packet.
      if (resp_length != 0) {
        uint16_t data_length = std::min(resp_length, rb_len);
        assert(read_offset < packet_length);
        assert(resp_length <= packet_length - read_offset);
        assert_array_eq(result_buffer+pad, rd_pd+pad+read_offset,
                        data_length);
      }

      // Check the zero padding.
      if (resp_length < rb_len)
        assert_array_eq(result_buffer+pad+resp_length, zeros,
                        rb_len - resp_length);
    }
  }
}

void test_tap_packet_read_core()
{
  struct nanotube_tap_packet_read_state state;

  // Case description codes indicate the use of each packet byte
  // assuming a 4 byte word size.  The actual buffer size varies so
  // the description code only shows the kind of case being tested,
  // indicating whether the read begins at a word boundary etc...
  //
  // The codes are:
  //   Be - The byte is before the read.
  //   Rf - The byte is the full read.
  //   Rs - The byte is the start of the read.
  //   Rm - The byte is mid-read.
  //   Re - The byte is the end of the read.
  //   Af - The byte is after the read.

  std::cout << "Case  1: Rf Af Af Af\n";

  state = nanotube_tap_packet_read_state_init;

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   8, 3,
                                 /* Packet buffer */   8, 3,
                                 /* Read offset/len */ 0, 1,
                                 /* Req valid */       0,
                                 /* Packet length */   8);

  std::cout << "Case  2: Rs Rm Re Af\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   7, 3,
                                 /* Packet buffer */   7, 3,
                                 /* Read offset/len */ 7, 4,
                                 /* Req valid */       0,
                                 /* Packet length */   17);

  std::cout << "Case  3: Rs Rm Rm Re\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   29, 5,
                                 /* Packet buffer */   9, 4,
                                 /* Read offset/len */ 9, 9,
                                 /* Req valid */       9,
                                 /* Packet length */   18);

  std::cout << "Case  4: Rs Rm Rm Rm | Rm Re Af Af\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   35, 6,
                                 /* Packet buffer */   13, 4,
                                 /* Read offset/len */ 0, 21,
                                 /* Req valid */       0,
                                 /* Packet length */   96);

  std::cout << "Case  5: Rs Rm Rm Rm | Rm Rm Rm Re\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   31, 6,
                                 /* Packet buffer */   13, 4,
                                 /* Read offset/len */ 0, 26,
                                 /* Req valid */       0,
                                 /* Packet length */   96);

  std::cout << "Case  6: Rs Rm Rm Rm | Rm Rm Rm Rm | Re Af Af Af\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   43, 6,
                                 /* Packet buffer */   15, 4,
                                 /* Read offset/len */ 0, 37,
                                 /* Req valid */       0,
                                 /* Packet length */   73);

  std::cout << "Case  7: Be Rf Af Af\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   35, 6,
                                 /* Packet buffer */   13, 4,
                                 /* Read offset/len */ 0, 31,
                                 /* Req valid */       0,
                                 /* Packet length */   73);

  std::cout << "Case  8: Be Rs Re Af\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   27, 5,
                                 /* Packet buffer */   11, 4,
                                 /* Read offset/len */ 5, 4,
                                 /* Req valid */       0,
                                 /* Packet length */   30);

  std::cout << "Case  9: Be Rs Rm Re\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   29, 5,
                                 /* Packet buffer */   11, 4,
                                 /* Read offset/len */ 16, 6,
                                 /* Req valid */       16,
                                 /* Packet length */   26);

  std::cout << "Case 10: Be Rs Rm Rm | Rm Re Af Af\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   29, 5,
                                 /* Packet buffer */   12, 4,
                                 /* Read offset/len */ 8, 6,
                                 /* Req valid */       0,
                                 /* Packet length */   20);

  std::cout << "Case 11: Rs Rm <EOP>\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   31, 5,
                                 /* Packet buffer */   18, 5,
                                 /* Read offset/len */ 0, 12,
                                 /* Req valid */       0,
                                 /* Packet length */   10);

  std::cout << "Case 12: Rs Rm Rm Rm <EOP>\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   31, 5,
                                 /* Packet buffer */   17, 5,
                                 /* Read offset/len */ 0, 12,
                                 /* Req valid */       0,
                                 /* Packet length */   10);

  std::cout << "Case 13: Rs Rm Rm Rm | Rm <EOP>\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   26, 5,
                                 /* Packet buffer */   19, 5,
                                 /* Read offset/len */ 19, 26,
                                 /* Req valid */       0,
                                 /* Packet length */   40);

  std::cout << "Case 14: Be Rs Rm <EOP>\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   26, 5,
                                 /* Packet buffer */   21, 5,
                                 /* Read offset/len */ 11, 26,
                                 /* Req valid */       0,
                                 /* Packet length */   19);

  std::cout << "Case 15: Be Rs Rm Rm <EOP>\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   24, 5,
                                 /* Packet buffer */   21, 5,
                                 /* Read offset/len */ 16, 23,
                                 /* Req valid */       0,
                                 /* Packet length */   21);

  std::cout << "Case 16: Be Rs Rm Rm | Rm <EOP>\n";

  test_tap_packet_read_core_case(&state,
                                 /* Result buffer */   25, 5,
                                 /* Packet buffer */   29, 5,
                                 /* Read offset/len */ 26, 20,
                                 /* Req valid */       0,
                                 /* Packet length */   32);

  std::cout << "Random cases.\n";
  for (int i=0; i<1000; i++) {
    uint16_t result_buffer_length = (rand() % 16) + 1;
    uint16_t packet_buffer_length = (rand() % 16) + 1;
    uint16_t read_offset = (rand() % 32);
    uint16_t read_length = (rand() % 16);
    uint16_t req_valid_offset = (rand() % (read_offset+1));
    uint16_t packet_length = (rand() % 32) + 1;

    if (test_verbose)
      std::cout << "Random case " << i << "\n";

    test_tap_packet_read_core_case(&state,
                                   result_buffer_length, 6,
                                   packet_buffer_length, 6,
                                   read_offset, read_length,
                                   req_valid_offset,
                                   packet_length);
  }
}

int main(int argc, char *argv[])
{
  test_init(argc, argv);
  test_tap_packet_read_core();
  return test_fini();
}

///////////////////////////////////////////////////////////////////////////
