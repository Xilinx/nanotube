/*******************************************************/
/*! \file pcap_in_agent.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for reading packets from a pcap file.
**   \date 2020-08-28
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "pcap_in_agent.hpp"

#include "test_harness.hpp"

#include "nanotube_pcap_read.hpp"

///////////////////////////////////////////////////////////////////////////

pcap_in_agent::pcap_in_agent(test_harness* harness,
                             const std::string &filename):
  test_agent(harness),
  m_filename(filename)
{
}

void pcap_in_agent::start_test()
{
  nanotube_pcap_read packet_in{m_filename.c_str()};

  /* Read in packets */
  m_packets.clear();
  while (true) {
    nanotube_packet_ptr_t p = packet_in.read_next();
    if (p.get() == nullptr)
      break;
    m_packets.push_back(std::move(p));
  }
  std::cout << "Read " << m_packets.size() << " packets\n";
}

void pcap_in_agent::test_kernel(packet_kernel* kernel)
{
  for (nanotube_packet_ptr_t &p : m_packets) {
    get_harness()->send_packet(p.get());
  }
}

///////////////////////////////////////////////////////////////////////////
