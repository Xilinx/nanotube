/*******************************************************/
/*! \file pcap_out_agent.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for writing packets to a pcap file.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef PCAP_OUT_AGENT_HPP
#define PCAP_OUT_AGENT_HPP

#include "test_agent.hpp"

class nanotube_pcap_dump;

///////////////////////////////////////////////////////////////////////////

class pcap_out_agent: public test_agent
{
public:
  pcap_out_agent(test_harness* harness, const std::string &filename);

  // Called at the start of the test.
  void start_test() override;

  // Called when a packet is received.
  void receive_packet(nanotube_packet_t* packet,
                      unsigned packet_index) override;

private:
  // The output filename.
  std::string m_filename;

  // The output pcap file.
  std::unique_ptr<nanotube_pcap_dump> m_packet_out;
};

///////////////////////////////////////////////////////////////////////////

#endif // PCAP_OUT_AGENT_HPP
