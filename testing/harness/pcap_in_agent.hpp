/*******************************************************/
/*! \file pcap_in_agent.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for reading packets from a pcap file.
**   \date 2020-11-06
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef PCAP_IN_AGENT_HPP
#define PCAP_IN_AGENT_HPP

#include "test_agent.hpp"

///////////////////////////////////////////////////////////////////////////

class pcap_in_agent: public test_agent
{
public:
  pcap_in_agent(test_harness* harness, const std::string &filename);

  // Called at the start of the test.
  void start_test() override;

  // Called to test a kernel.
  void test_kernel(packet_kernel* kernel) override;

private:
  // The input filename.
  std::string m_filename;

  // The packets to pass through each kernel.
  std::vector<nanotube_packet_ptr_t> m_packets;
};

///////////////////////////////////////////////////////////////////////////

#endif // PCAP_IN_AGENT_HPP
