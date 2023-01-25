/*******************************************************/
/*! \file pcap_out_agent.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for writing packets to a pcap file.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "pcap_out_agent.hpp"

#include "nanotube_pcap_dump.hpp"

///////////////////////////////////////////////////////////////////////////

pcap_out_agent::pcap_out_agent(test_harness* harness,
                               const std::string &filename):
  test_agent(harness),
  m_filename(filename)
{

}

void pcap_out_agent::start_test()
{
  // Use rfind with pos=0 to only look for a match at the start of the
  // string.  (From https://stackoverflow.com/a/40441240)
  bool with_meta = m_filename.rfind("capsule:", 0) == 0;
  if (with_meta)
    m_filename.erase(0, 8);
  m_packet_out.reset(new nanotube_pcap_dump(m_filename.c_str(), with_meta));
}

void pcap_out_agent::receive_packet(nanotube_packet_t* packet,
                                        unsigned packet_index)
{
  m_packet_out->write(packet);
}

///////////////////////////////////////////////////////////////////////////
