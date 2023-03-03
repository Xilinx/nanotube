/*******************************************************/
/*! \file test_agent.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A base class for interacting with the test harness.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "test_agent.hpp"

///////////////////////////////////////////////////////////////////////////

test_agent::test_agent(test_harness* harness):
  m_harness(harness)
{
}

void test_agent::start_test()
{
}

void test_agent::before_kernel(packet_kernel* kernel)
{
}

void test_agent::test_kernel(packet_kernel* kernel)
{
}

void test_agent::after_packet(unsigned packet_count)
{
}

void test_agent::receive_packet(nanotube_packet_t* packet,
                                unsigned packet_index)
{
}

void test_agent::end_test()
{
}

void test_agent::handle_quit()
{
}

///////////////////////////////////////////////////////////////////////////
