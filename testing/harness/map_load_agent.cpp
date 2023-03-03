/*******************************************************/
/*! \file map_load_agent.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for loading maps from a file.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "map_load_agent.hpp"

#include "test_harness.hpp"

#include "nanotube_map.hpp"

#include <fstream>
#include <iostream>

///////////////////////////////////////////////////////////////////////////

map_load_agent::map_load_agent(test_harness* harness,
                               const std::string &filename):
  test_agent(harness),
  m_filename(filename)
{
}

void map_load_agent::start_test()
{
  /* Read map data from file */
  std::ifstream map_in;
  map_in.open(m_filename);

  std::cout << "Reading maps\n";
  get_harness()->get_system()->read_maps(map_in, &std::cout);
}

///////////////////////////////////////////////////////////////////////////
