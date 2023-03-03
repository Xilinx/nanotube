/*******************************************************/
/*! \file verbose_agent.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for producing verbose output.
**   \date 2020-09-01
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "verbose_agent.hpp"

#include "packet_kernel.hpp"

#include <iostream>

///////////////////////////////////////////////////////////////////////////

verbose_agent::verbose_agent(test_harness* harness):
  test_agent(harness)
{
}

void verbose_agent::before_kernel(packet_kernel* kernel)
{
  const std::string &kernel_name = kernel->get_name();
  std::cout  << "Testing kernel "   << kernel_name << '\n';
}

void verbose_agent::receive_packet(nanotube_packet_t* packet,
                                   unsigned packet_index)
{
  int res = nanotube_packet_get_port(packet);
  if (res >= 0x80)
    res -= 0x100;

  auto sec = NANOTUBE_SECTION_WHOLE;
  std::cout << "  Packet " << packet_index
            << " Length " << packet->size(sec)
            << " Result " << res << '\n';
}

///////////////////////////////////////////////////////////////////////////
