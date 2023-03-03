/*******************************************************/
/*! \file map_dump_agent.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for dumping maps to a file.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef MAP_DUMP_AGENT_HPP
#define MAP_DUMP_AGENT_HPP

#include "test_agent.hpp"

#include <fstream>
#include <string>

///////////////////////////////////////////////////////////////////////////

class map_dump_agent: public test_agent
{
public:
  map_dump_agent(test_harness* harness, const std::string &filename);

  void before_kernel(packet_kernel* kernel) override;
  void after_packet(unsigned packet_count) override;
  void end_test() override;

private:
  // A stream logging the results from maps.
  std::ofstream m_map_out;
};

///////////////////////////////////////////////////////////////////////////

#endif // MAP_DUMP_AGENT_HPP
