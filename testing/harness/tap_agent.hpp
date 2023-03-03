/*******************************************************/
/*! \file tap_agent.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for connecting to a Linux TAP device.
**   \date 2020-11-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef TAP_AGENT_HPP
#define TAP_AGENT_HPP

#include "test_agent.hpp"

#include "nanotube_context.hpp"
#include "nanotube_thread.hpp"

#include "boost/asio/posix/stream_descriptor.hpp"

///////////////////////////////////////////////////////////////////////////

class tap_agent: public test_agent
{
public:
  tap_agent(test_harness* harness, const std::string &name);
  ~tap_agent();

  // Called to test a kernel.
  void test_kernel(packet_kernel* kernel) override;

  // Called when a packet is received from the kernel.
  void receive_packet(nanotube_packet_t* packet,
                      unsigned packet_index) override;

  // Handle a quit request from the user.
  void handle_quit() override;

private:
  // The RX packet.
  nanotube_packet_t m_packet;

  // The async IO stream descriptor for the TAP device.
  boost::asio::posix::stream_descriptor m_stream;

  // The TAP file descriptor.
  int m_fd;

  // The current kernel.
  packet_kernel *m_kernel;

  // Start a TAP RX operation.
  void init_tap_rx();

  // Handle RX data from the TAP device.
  void handle_tap_rx(const boost::system::error_code &code,
                     std::size_t length);
};

///////////////////////////////////////////////////////////////////////////

#endif // TAP_AGENT_HPP
