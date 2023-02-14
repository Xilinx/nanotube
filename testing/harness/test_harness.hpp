/*******************************************************/
/*! \file test_harness.hpp
** \author Neil Turton <neilt@amd.com>
**         Kieran Mansley <kieranm@amd.com>
**  \brief Test harness for testing packet kernels.
**   \date 2020-08-28
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef TEST_HARNESS_HPP
#define TEST_HARNESS_HPP

///////////////////////////////////////////////////////////////////////////

#include "test_agent.hpp"

#include "processing_system.hpp"

#include <atomic>
#include <cstdint>

#include "boost/asio/version.hpp"

// io_service was renamed to io_context somewhere between 100803 and
// 101401.  Interesting versions are:
//   RHEL-7        Boost 1.53.0 (105300)  asio 1.8.3   (100803)
//   Ubuntu 18.04  Boost 1.65.1 (106501)  asio 1.10.10 (101010)
//   Ubuntu 20.04  Boost 1.71.0 (107100)  asio 1.14.1  (101401)
#if BOOST_ASIO_VERSION >= 101401

#include "boost/asio/io_context.hpp"

typedef boost::asio::io_context asio_context_t;
static inline void asio_restart(asio_context_t *ctxt) {
  ctxt->restart();
}

#else

#include "boost/asio/io_service.hpp"

typedef boost::asio::io_service asio_context_t;
static inline void asio_restart(asio_context_t *ctxt) {
  ctxt->reset();
}

#endif
#include "boost/asio/signal_set.hpp"

///////////////////////////////////////////////////////////////////////////

class test_harness: public ps_client
{
public:
  // The default constructor;
  test_harness();

  // Get a pointer to the processing system.
  processing_system *get_system() { return m_system.get(); }

  // Get the ASIO context.
  asio_context_t *get_asio_context() { return &m_asio_context; }

  // Get the I/O thread context.
  nanotube_context *get_io_thread_context() { return &m_io_thread_context; }

  // Get the quit flag.
  bool get_quit_flag() const { return m_quit_flag.load(); }

  // Add an agent to the test harness.  Ownership is transferred.
  void add_agent(test_agent_ptr_t agent);

  // Make sure the I/O thread is servicing requests.
  void wake_io_thread();

  // Test the specified kernel by passing packets through it and
  // reporting the results.
  void test_kernel(packet_kernel &kernel);

  // Send a packet to the kernel.
  void send_packet(nanotube_packet_t *packet);

  // Handle a packet received from the processing system.
  void receive_packet(nanotube_packet_t *packet, nanotube_kernel_rc_t rc) override;

  // Indicate test failure.  The caller is expected to produce a
  // suitable error message.
  void set_test_failure();

  // Parse the command line arguments.
  int parse_arguments(int argc, char *argv[]);

  // Start the I/O thread.
  void start_io_thread();

  // Stop the I/O thread.
  void stop_io_thread();

  // The entry point to the I/O thread.
  static void io_thread_func(nanotube_context_t *context, void *data);

  // Handle a signal.
  void handle_signal(const boost::system::error_code &error,
                     int sig_num);

  // Handle a quit request from the user.
  void handle_quit();

  // Enumerate all the kernels and test them.
  int run(int argc, char *argv[]);

private:
  // The processing system being tested.
  processing_system::ptr_t m_system;

  // The async I/O context.
  asio_context_t m_asio_context;

  // The signal set.
  boost::asio::signal_set m_signal_set;

  // The context of the I/O thread.
  nanotube_context_t m_io_thread_context;

  // The I/O thread.
  std::unique_ptr<nanotube_thread> m_io_thread;

  // Indicates whether the test failed.
  std::atomic<bool> m_test_failed;

  // The test timeout in nanoseconds.
  uint64_t m_timeout_ns;
  // The current kernel being tested.
  packet_kernel *m_current_kernel;

  // The number of packets received.
  unsigned m_p_count;

  // The number of packets sent.
  unsigned m_p_sent;

  // A flag to indicate that a quit request has arrived.
  std::atomic<bool> m_quit_flag;

  // The test agents.
  std::vector<test_agent_ptr_t> m_agents;
};

///////////////////////////////////////////////////////////////////////////

#endif // TEST_HARNESS_HPP
