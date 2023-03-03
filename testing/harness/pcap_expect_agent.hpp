/*******************************************************/
/*! \file pcap_expect_agent.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for comparing output packets with a pcap file.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef PCAP_EXPECT_AGENT_HPP
#define PCAP_EXPECT_AGENT_HPP

#include "test_agent.hpp"

///////////////////////////////////////////////////////////////////////////

class pcap_expect_agent: public test_agent
{
public:
  pcap_expect_agent(test_harness* harness, const std::string &filename);

  // Called at the start of the test.
  void start_test() override;

  // Called when a packet is received.
  void receive_packet(nanotube_packet_t* packet,
                      unsigned packet_index) override;

  // Called at the end of the test.
  void end_test() override;

private:
  typedef std::vector<nanotube_packet_ptr_t> packet_vector_t;

  // The output filename.
  std::string m_filename;

  // The packets to pass through each kernel.
  packet_vector_t m_packets;

  // The next expected packet.
  packet_vector_t::iterator m_packet_iter;
};

///////////////////////////////////////////////////////////////////////////

#endif // PCAP_EXPECT_AGENT_HPP
