/**************************************************************************\
*//*! \file test_tap_packet_write.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A simple test for the packet write tap core logic.
**   \date  2020-12-02
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

void test_tap_packet_write_core_case(
  struct nanotube_tap_packet_write_state *state,
  uint16_t request_buffer_length,
  uint8_t request_buffer_index_bits,
  uint16_t packet_buffer_length,
  uint8_t packet_buffer_index_bits,
  uint16_t write_offset,
  uint16_t write_length,
  uint16_t req_valid_offset,
  uint16_t packet_length)
{
  // The amount of padding to make sure there is no buffer over-run.
  uint16_t pad = 8;

  // The lengths of the buffers passed to the packet tap.
  uint16_t rb_len = request_buffer_length;
  uint16_t pb_len = packet_buffer_length;

  // The word index at which req.valid is set.
  uint16_t req_valid_index = ( req_valid_offset / pb_len );

  // The index of the word containing the first byte written.
  uint16_t write_start_index = ( write_offset / pb_len );

  // The byte after the last one written.
  uint16_t write_end_offset = (write_offset +
                               std::min(request_buffer_length, write_length));

  // The number of the packet words required.
  uint16_t packet_num_words = ( (packet_length + (pb_len-1)) / pb_len );

  if (test_verbose) {
    std::cout << "  Parameters: rbl=" << request_buffer_length
              << " pbl=" << packet_buffer_length
              << " wo=" << write_offset
              << " wl=" << write_length
              << " rvo=" << req_valid_offset
              << " pl=" << packet_length
              << "\n";
  }

  assert(req_valid_index <= write_start_index);

  // Variables to pass to the packet tap.
  uint8_t request_buffer[rb_len+2*pad];
  uint8_t mask_buffer[(rb_len+7)/8 + 2*pad];
  uint8_t packet_in_buffer[pb_len+2*pad];
  uint8_t packet_out_buffer[pb_len+2*pad];
  struct nanotube_tap_packet_write_req req;

  // The expected packet out buffer.
  uint8_t packet_exp_buffer[pb_len+2*pad];

  // Arrays of random data for initializing input buffers.
  uint8_t rd_rb[sizeof(request_buffer)];
  uint8_t rd_mb[sizeof(mask_buffer)];
  uint8_t rd_pid[sizeof(packet_in_buffer)];
  uint8_t rd_pod[sizeof(packet_out_buffer)];

  for (uint16_t index=0; index<packet_num_words; index++) {
    // Fill in the request, setting the valid bit at the requested
    // time.
    if (index >= req_valid_index) {
      req.valid = 1;
      req.write_offset = write_offset;
      req.write_length = write_length;
    } else {
      req.valid = 0;
      req.write_offset = rand() & 0xffff;
      req.write_length = rand() & 0xffff;
    }

    // Fill in random arrays.  The request buffer values stay constant
    // after the valid bit has gone high since that is what the tap
    // expects.
    if (index <= req_valid_index) {
      for (size_t i = 0; i < sizeof(rd_rb); i++)
        rd_rb[i] = rand() & 0xff;
      for (size_t i = 0; i < sizeof(rd_mb); i++)
        rd_mb[i] = rand() & 0xff;
    }
    for (size_t i = 0; i < sizeof(rd_pid); i++)
      rd_pid[i] = rand() & 0xff;
    for (size_t i = 0; i < sizeof(rd_pod); i++)
      rd_pod[i] = rand() & 0xff;

    // Initialize the buffers from the random arrays.
    memcpy(request_buffer, rd_rb, sizeof(request_buffer));
    memcpy(mask_buffer, rd_mb, sizeof(mask_buffer));
    memcpy(packet_in_buffer, rd_pid, sizeof(packet_in_buffer));
    memcpy(packet_out_buffer, rd_pod, sizeof(packet_out_buffer));

    if (test_verbose) {
      test_dump_buffer("request_buffer", request_buffer, sizeof(request_buffer));
      test_dump_buffer("mask_buffer", mask_buffer, sizeof(mask_buffer));
      test_dump_buffer("packet_in_buffer", packet_in_buffer, sizeof(packet_in_buffer));
      test_dump_buffer("packet_out_buffer", packet_out_buffer, sizeof(packet_out_buffer));
    }

    // Determine the packet arguments provided to the packet tap.
    bool packet_word_eop = (index == packet_num_words-1);
    uint16_t packet_word_length = pb_len;
    if (packet_word_eop)
      packet_word_length = packet_length - (packet_num_words-1)*pb_len;

    if (test_verbose) {
      std::cout << "Invoking tap: i=" << uint16_t(index)
                << " rbl=" << uint32_t(request_buffer_length)
                << " rbib=" << uint32_t(request_buffer_index_bits)
                << " pbl=" << uint32_t(packet_buffer_length)
                << " pbib=" << uint32_t(packet_buffer_index_bits)
                << " eop=" << uint32_t(packet_word_eop)
                << " pwl=" << uint32_t(packet_word_length)
                << " rv=" << uint32_t(req.valid)
                << " wo=" << uint32_t(req.write_offset)
                << " wl=" << uint32_t(req.write_length)
                << "\n";
    }

    // Call the packet tap.
    nanotube_tap_packet_write_core(
      /* Parameters. */
      request_buffer_length,
      request_buffer_index_bits,
      packet_buffer_length,
      packet_buffer_index_bits,
      /* Outputs. */
      packet_out_buffer + pad,
      /* State. */
      state,
      /* Inputs. */
      packet_in_buffer + pad,
      packet_word_eop, packet_word_length,
      &req,
      request_buffer + pad,
      mask_buffer + pad);

    // Create the expected contents of the packet out array.
    for (unsigned i=0; i<sizeof(packet_exp_buffer); i++) {
      uint8_t val;
      if (i < pad || i >= pb_len + pad) {
        // This byte is padding.
        val = rd_pod[i];
      } else {
        // This byte is part of the packet word.
        uint16_t packet_offset = index*pb_len + (i-pad);

        // Check whether this byte should be modified by the tap.
        if (packet_offset < packet_length &&
            packet_offset >= write_offset &&
            packet_offset < write_end_offset) {
          // This byte is part of the written region.
          uint16_t req_offset = packet_offset - write_offset;
          uint8_t mask_byte = rd_mb[req_offset/8 + pad];
          uint8_t mask_bit = (mask_byte >> (req_offset%8)) & 1;
          if (mask_bit != 0) {
            // This byte should be written from the request buffer.
            val = rd_rb[req_offset + pad];
          } else {
            // This byte should be preserved.
            val = rd_pid[i];
          }
        } else {
          // This byte is not part of the written region, so it should
          // be preserved.
          val = rd_pid[i];
        }
      }
      packet_exp_buffer[i] = val;
    }

    if (test_verbose) {
      test_dump_buffer("request_buffer", request_buffer, sizeof(request_buffer));
      test_dump_buffer("mask_buffer", mask_buffer, sizeof(mask_buffer));
      test_dump_buffer("packet_in_buffer", packet_in_buffer, sizeof(packet_in_buffer));
      test_dump_buffer("packet_out_buffer", packet_out_buffer, sizeof(packet_out_buffer));
      test_dump_buffer("packet_exp_buffer", packet_exp_buffer, sizeof(packet_exp_buffer));
    }

    // Check all the arrays to make sure they contain the expected
    // contents.
    assert_array_eq(request_buffer, rd_rb, sizeof(rd_rb));
    assert_array_eq(mask_buffer, rd_mb, sizeof(rd_mb));
    assert_array_eq(packet_in_buffer, rd_pid, sizeof(rd_pid));
    assert_array_eq(packet_out_buffer, packet_exp_buffer, sizeof(packet_exp_buffer));
  }
}

void test_tap_packet_write_core()
{
  struct nanotube_tap_packet_write_state state;

  // Case description codes indicate the use of each packet byte
  // assuming a 4 byte word size.  The actual buffer size varies so
  // the description code only shows the kind of case being tested,
  // indicating whether the write begins at a word boundary etc...
  //
  // The codes are:
  //   Be - The byte is before the write.
  //   Rf - The byte is the full write.
  //   Rs - The byte is the start of the write.
  //   Rm - The byte is mid-write.
  //   Re - The byte is the end of the write.
  //   Af - The byte is after the write.

  std::cout << "Case  1: Rf Af Af Af\n";

  state = nanotube_tap_packet_write_state_init;

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   8, 3,
                                 /* Packet buffer */   8, 3,
                                 /* Write offset/len */ 0, 1,
                                 /* Req valid */       0,
                                 /* Packet length */   8);

  std::cout << "Case  2: Rs Rm Re Af\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   7, 3,
                                 /* Packet buffer */   7, 3,
                                 /* Write offset/len */ 7, 4,
                                 /* Req valid */       0,
                                 /* Packet length */   17);

  std::cout << "Case  3: Rs Rm Rm Re\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   29, 5,
                                 /* Packet buffer */   9, 4,
                                 /* Write offset/len */ 9, 9,
                                 /* Req valid */       9,
                                 /* Packet length */   18);

  std::cout << "Case  4: Rs Rm Rm Rm | Rm Re Af Af\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   35, 6,
                                 /* Packet buffer */   13, 4,
                                 /* Write offset/len */ 0, 21,
                                 /* Req valid */       0,
                                 /* Packet length */   96);

  std::cout << "Case  5: Rs Rm Rm Rm | Rm Rm Rm Re\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   31, 6,
                                 /* Packet buffer */   13, 4,
                                 /* Write offset/len */ 0, 26,
                                 /* Req valid */       0,
                                 /* Packet length */   96);

  std::cout << "Case  6: Rs Rm Rm Rm | Rm Rm Rm Rm | Re Af Af Af\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   43, 6,
                                 /* Packet buffer */   15, 4,
                                 /* Write offset/len */ 0, 37,
                                 /* Req valid */       0,
                                 /* Packet length */   73);

  std::cout << "Case  7: Be Rf Af Af\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   35, 6,
                                 /* Packet buffer */   13, 4,
                                 /* Write offset/len */ 0, 31,
                                 /* Req valid */       0,
                                 /* Packet length */   73);

  std::cout << "Case  8: Be Rs Re Af\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   27, 5,
                                 /* Packet buffer */   11, 4,
                                 /* Write offset/len */ 5, 4,
                                 /* Req valid */       0,
                                 /* Packet length */   30);

  std::cout << "Case  9: Be Rs Rm Re\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   29, 5,
                                 /* Packet buffer */   11, 4,
                                 /* Write offset/len */ 16, 6,
                                 /* Req valid */       16,
                                 /* Packet length */   26);

  std::cout << "Case 10: Be Rs Rm Rm | Rm Re Af Af\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   29, 5,
                                 /* Packet buffer */   12, 4,
                                 /* Write offset/len */ 8, 6,
                                 /* Req valid */       0,
                                 /* Packet length */   20);

  std::cout << "Case 11: Rs Rm <EOP>\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   31, 5,
                                 /* Packet buffer */   18, 5,
                                 /* Write offset/len */ 0, 12,
                                 /* Req valid */       0,
                                 /* Packet length */   10);

  std::cout << "Case 12: Rs Rm Rm Rm <EOP>\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   31, 5,
                                 /* Packet buffer */   17, 5,
                                 /* Write offset/len */ 0, 12,
                                 /* Req valid */       0,
                                 /* Packet length */   10);

  std::cout << "Case 13: Rs Rm Rm Rm | Rm <EOP>\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   26, 5,
                                 /* Packet buffer */   19, 5,
                                 /* Write offset/len */ 19, 26,
                                 /* Req valid */       0,
                                 /* Packet length */   40);

  std::cout << "Case 14: Be Rs Rm <EOP>\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   26, 5,
                                 /* Packet buffer */   21, 5,
                                 /* Write offset/len */ 11, 26,
                                 /* Req valid */       0,
                                 /* Packet length */   19);

  std::cout << "Case 15: Be Rs Rm Rm <EOP>\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   24, 5,
                                 /* Packet buffer */   21, 5,
                                 /* Write offset/len */ 16, 23,
                                 /* Req valid */       0,
                                 /* Packet length */   21);

  std::cout << "Case 16: Be Rs Rm Rm | Rm <EOP>\n";

  test_tap_packet_write_core_case(&state,
                                 /* Result buffer */   25, 5,
                                 /* Packet buffer */   29, 5,
                                 /* Write offset/len */ 26, 20,
                                 /* Req valid */       0,
                                 /* Packet length */   32);

  std::cout << "Random cases.\n";
  for (int i=0; i<10000; i++) {
    uint16_t request_buffer_index_bits = (rand() % 8);
    uint16_t request_buffer_length =
      (rand() % (1<<request_buffer_index_bits)) + 1;
    uint16_t packet_buffer_index_bits = (rand() % 8);
    uint16_t packet_buffer_length =
      (rand() % (1<<packet_buffer_index_bits)) + 1;
    uint16_t write_offset = (rand() % 128);
    uint16_t write_length = (rand() % 16);
    uint16_t req_valid_offset = (rand() % (write_offset+1));
    uint16_t packet_length = (rand() % 128) + 1;

    if (test_verbose)
      std::cout << "Random case " << i << "\n";

    test_tap_packet_write_core_case(&state,
                                    request_buffer_length,
                                    request_buffer_index_bits,
                                    packet_buffer_length,
                                    packet_buffer_index_bits,
                                    write_offset, write_length,
                                    req_valid_offset,
                                    packet_length);
  }
}

int main(int argc, char *argv[])
{
  test_init(argc, argv);
  test_tap_packet_write_core();
  return test_fini();
}

///////////////////////////////////////////////////////////////////////////
