/*******************************************************/
/*! \file test_agent.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A base class for interacting with the test harness.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef TEST_AGENT_HPP
#define TEST_AGENT_HPP

#include <memory>
#include <string>

#include "nanotube_packet.hpp"

class packet_kernel;
class test_agent;
class test_harness;

typedef std::unique_ptr<test_agent> test_agent_ptr_t;

///////////////////////////////////////////////////////////////////////////

// The test harness runs a test on a sequence of kernels.  The
// interactions with the kernels are performed by test agents.  Each
// type of agent performs a small task such as reading packets from a
// file and sending them to the kernel or capturing packets from a
// kernel and writing them to a file.
//
// This base class describes the interface each test agent exposes to
// the main part of the test harness.  There are several hook virtual
// methods with an empty default implementation.  Overriding one of
// these hooks allows the agent to take an action when an event
// occurs.

class test_agent
{
public:
  test_agent(test_harness* harness);

  test_harness* get_harness() { return m_harness; }

  // Called at the start of the test.
  virtual void start_test();

  // Called before testing a kernel.
  virtual void before_kernel(packet_kernel* kernel);

  // Called to test a kernel.
  virtual void test_kernel(packet_kernel* kernel);

  // Called after a packet has been sent.
  virtual void after_packet(unsigned packet_count);

  // Called when a packet is received.
  virtual void receive_packet(nanotube_packet_t* packet,
                              unsigned packet_index);

  // Called at the end of the test.
  virtual void end_test();

  // Handle a quit request from the user.
  virtual void handle_quit();

private:
  // The test harness object.
  test_harness* m_harness;
};

///////////////////////////////////////////////////////////////////////////

#endif // TEST_AGENT_HPP
