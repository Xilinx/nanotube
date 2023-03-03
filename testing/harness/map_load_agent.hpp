/*******************************************************/
/*! \file map_load_agent.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for loading maps from a file.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef MAP_LOAD_AGENT_HPP
#define MAP_LOAD_AGENT_HPP

#include "test_agent.hpp"

#include <string>

///////////////////////////////////////////////////////////////////////////

class map_load_agent: public test_agent
{
public:
  map_load_agent(test_harness* harness, const std::string &filename);

  void start_test() override;

private:
  std::string m_filename;
};

///////////////////////////////////////////////////////////////////////////

#endif // MAP_LOAD_AGENT_HPP
