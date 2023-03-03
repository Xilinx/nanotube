/*******************************************************/
/*! \file map_dump_agent.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for dumping maps to a file.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "map_dump_agent.hpp"

#include "nanotube_map.hpp"
#include "test_harness.hpp"

///////////////////////////////////////////////////////////////////////////

map_dump_agent::map_dump_agent(test_harness* harness,
                               const std::string &filename):
  test_agent(harness)
{
  m_map_out.open(filename);
}

void map_dump_agent::before_kernel(packet_kernel *kernel)
{
  const std::string &kernel_name = kernel->get_name();
  m_map_out    << "# Testing kernel " << kernel_name << '\n';
}

void map_dump_agent::after_packet(unsigned packet_count)
{
  if (packet_count == 0)
    m_map_out << "# Maps before packet.\n";
  else
    m_map_out << "# Maps after packet " << packet_count-1 <<'\n';
  get_harness()->get_system()->dump_maps(m_map_out);
}

void map_dump_agent::end_test()
{
  m_map_out.close();
}

///////////////////////////////////////////////////////////////////////////
