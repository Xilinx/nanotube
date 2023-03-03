/*******************************************************/
/*! \file socket_agent.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent to transport packets over a socket.
**   \date 2022-10-24
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef SOCKET_AGENT_HPP
#define SOCKET_AGENT_HPP

#include "test_agent.hpp"

#include "boost/asio/ip/tcp.hpp"

#include <memory>
#include <unordered_set>

///////////////////////////////////////////////////////////////////////////

class socket_agent: public test_agent
{
public:
  socket_agent(test_harness* harness, const std::string &params);
  ~socket_agent();

  // Called at the start of the test.
  void start_test() override;

  // Called to test a kernel.
  void test_kernel(packet_kernel* kernel) override;

  // Called when a packet is received from the kernel.
  void receive_packet(nanotube_packet_t* packet,
                      unsigned packet_index) override;

  // Called at the end of the test.
  void end_test() override;

  // Handle a quit request from the user.
  void handle_quit() override;

  // The agent implementation.
  class impl;

private:
  std::shared_ptr<impl> m_impl;
};

///////////////////////////////////////////////////////////////////////////

#endif // SOCKET_AGENT_HPP
