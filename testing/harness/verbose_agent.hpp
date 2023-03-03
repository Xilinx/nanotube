/*******************************************************/
/*! \file verbose_agent.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for producing verbose output.
**   \date 2020-11-17
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef VERBOSE_AGENT_HPP
#define VERBOSE_AGENT_HPP

#include "test_agent.hpp"

///////////////////////////////////////////////////////////////////////////

class verbose_agent: public test_agent
{
public:
  explicit verbose_agent(test_harness* harness);

  // Called to test a kernel.
  void before_kernel(packet_kernel* kernel) override;

  // Called when a packet is received.
  void receive_packet(nanotube_packet_t* packet,
                      unsigned packet_index) override;
};

///////////////////////////////////////////////////////////////////////////

#endif // VERBOSE_AGENT_HPP
