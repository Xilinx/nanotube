/*******************************************************/
/*! \file pcap_expect_agent.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for comparing output packets with a pcap file.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "pcap_expect_agent.hpp"

#include "test_harness.hpp"

#include "nanotube_pcap_read.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

///////////////////////////////////////////////////////////////////////////

pcap_expect_agent::pcap_expect_agent(test_harness* harness,
                                     const std::string &filename):
  test_agent(harness),
  m_filename(filename),
  m_packet_iter(m_packets.end())
{
}

void pcap_expect_agent::start_test()
{
  nanotube_pcap_read packet_in{m_filename.c_str()};

  // Read in the packets.
  m_packets.clear();
  while (true) {
    nanotube_packet_ptr_t p = packet_in.read_next();
    if (p.get() == nullptr)
      break;
    m_packets.push_back(std::move(p));
  }

  std::cout << "Expecting " << m_packets.size() << " packets.\n";

  // Start with the first packet, if any.
  m_packet_iter = m_packets.begin();
}

static void dump_packet(nanotube_packet_t* packet)
{
  uint32_t packet_length = packet->size(NANOTUBE_SECTION_WHOLE);
  uint8_t *data = packet->begin(NANOTUBE_SECTION_WHOLE);
  std::cout << std::hex << std::setfill('0');
  for (uint32_t i=0; i<packet_length; i+=16) {
    std::cout << "  " << std::setw(4) << i << std::setw(0) << ':';
    uint32_t frag_length = std::min(16U, packet_length-i);
    for (uint32_t j=0; j<frag_length; j++) {
      std::cout << ' ' << std::setw(2) << unsigned(data[i+j])
                << std::setw(0);
    }
    std::cout << '\n';
  }
  std::cout << std::dec << std::setfill(' ');
}

static void dump_packet_compare(nanotube_packet_t* left,
                                nanotube_packet_t* right)
{
  auto sec = NANOTUBE_SECTION_WHOLE;
  uint32_t left_length = left->size(sec);
  uint32_t right_length = right->size(sec);
  uint32_t full_length = std::max(right_length, left_length);
  uint8_t *left_data = left->begin(sec);
  uint8_t *right_data = right->begin(sec);

  std::cout << std::hex << std::setfill('0');
  for (uint32_t i=0; i<full_length; i+=16) {
    uint32_t left_frag_end = std::min(i+16, left_length);
    uint32_t right_frag_end = std::min(i+16, right_length);

    if (i < left_length) {
      std::cout << "  " << std::setw(4) << i << std::setw(0) << ':';
      for (uint32_t j=i; j<i+16; j++) {
        if (j < left_length) {
          std::cout << ' ' << std::setw(2) << unsigned(left_data[j])
                    << std::setw(0);
        } else {
          std::cout << "   ";
        }
      }
    } else {
      for (uint32_t j=0; j<2+4+1+16*3; j++)
        std::cout << ' ';
    }

    if (left_frag_end != right_frag_end ||
        memcmp(left_data+i, right_data+i, left_frag_end - i) != 0) {
      std::cout << "  |";
    } else if (i < right_length) {
      std::cout << "   ";
    }

    if (i < right_length) {
      std::cout << "  " << std::setw(4) << i << std::setw(0) << ':';
      for (uint32_t j=i; j<right_frag_end; j++) {
        std::cout << ' ' << std::setw(2) << unsigned(right_data[j])
                  << std::setw(0);
      }
    }

    std::cout << '\n';
  }
  std::cout << std::dec << std::setfill(' ');
}

void pcap_expect_agent::receive_packet(nanotube_packet_t* packet,
                                       unsigned packet_index)
{
  // Report unexpected packets.
  if (m_packet_iter == m_packets.end()) {
    std::cout << "Received unexpected packet " << packet_index << ":\n";
    dump_packet(packet);
    std::cout << '\n';
    get_harness()->set_test_failure();
    return;
  }

  nanotube_packet_t* expected = m_packet_iter->get();

  auto sec = NANOTUBE_SECTION_WHOLE;
  uint32_t packet_length = packet->size(sec);
  uint32_t expected_length = expected->size(sec);
  uint8_t *packet_data = packet->begin(sec);
  uint8_t *expected_data = expected->begin(sec);

  if (packet_length != expected_length ||
      memcmp(expected_data, packet_data, packet_length) != 0) {
    std::cout << "Mismatch at packet " << packet_index
              << " (expected " << expected_length << " bytes and got "
              << packet_length << " bytes):\n";
    dump_packet_compare(expected, packet);
    std::cout << '\n';
    get_harness()->set_test_failure();
  }

  // Move on to the next packet.
  ++m_packet_iter;
}

void pcap_expect_agent::end_test()
{
  while (m_packet_iter != m_packets.end()) {
    unsigned index = m_packet_iter - m_packets.begin();
    std::cout << "Missing packet " << index << " at end of test:\n";
    dump_packet(m_packet_iter->get());
    std::cout << '\n';
    get_harness()->set_test_failure();
    m_packet_iter++;
  }
}

///////////////////////////////////////////////////////////////////////////
