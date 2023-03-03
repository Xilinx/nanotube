/*******************************************************/
/*! \file test_harness.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**         Neil Turton <neilt@amd.com>
**  \brief Test harness for testing packet kernels.
**   \date 2020-03-20
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "test_harness.hpp"

#include <cassert>
#include <cmath>
#include <csignal>
#include <iostream>
#include <vector>

#include "boost/program_options.hpp"

#include "nanotube_api.h"
#include "nanotube_packet.hpp"
#include "packet_kernel.hpp"

#include "map_dump_agent.hpp"
#include "map_load_agent.hpp"
#include "pcap_expect_agent.hpp"
#include "pcap_in_agent.hpp"
#include "pcap_out_agent.hpp"
#include "socket_agent.hpp"
#include "tap_agent.hpp"
#include "test_agent.hpp"
#include "test_harness_run.h"
#include "verbose_agent.hpp"

namespace asio = boost::asio;
namespace po = boost::program_options;

///////////////////////////////////////////////////////////////////////////

test_harness::test_harness():
  m_signal_set(m_asio_context, SIGINT),
  m_test_failed(false),
  // Use a 10 second timeout to handle NFS problems.
  m_timeout_ns(10*uint64_t(1000*1000*1000)),
  m_current_kernel(nullptr),
  m_p_count(0),
  m_p_sent(0),
  m_quit_flag(false)
{
}

void test_harness::add_agent(test_agent_ptr_t agent)
{
  m_agents.push_back(std::move(agent));
}

void test_harness::wake_io_thread()
{
  if (m_asio_context.stopped()) {
    asio_restart(&m_asio_context);
    m_io_thread->wake();
  }
}

void test_harness::test_kernel(packet_kernel &kernel)
{
  m_p_count = 0;
  m_p_sent = 0;
  kernel.set_timeout_ns(m_timeout_ns);

  for (test_agent_ptr_t &agent : m_agents) {
    agent->before_kernel(&kernel);
  }

  for (test_agent_ptr_t &agent : m_agents) {
    agent->test_kernel(&kernel);
  }

  kernel.flush();
}

void test_harness::send_packet(nanotube_packet_t *packet)
{
  m_current_kernel->process(packet);
  m_current_kernel->flush();
  ++m_p_sent;
  for (test_agent_ptr_t &agent : m_agents) {
    agent->after_packet(m_p_sent);
  }
}

void test_harness::receive_packet(nanotube_packet_t *packet, nanotube_kernel_rc_t rc)
{
  /* Skip dropped packets */
  if (rc == NANOTUBE_PACKET_DROP)
    return;

  // Inform the agents that a packet arrived.
  for (test_agent_ptr_t &agent : m_agents)
    agent->receive_packet(packet, m_p_count);
  ++m_p_count;
}

template<class AGENT>
class agent_val_sem: public po::value_semantic
{
public:
  agent_val_sem(test_harness* harness, const std::string &name):
    m_harness(harness), m_name(name)
  {
  }

  std::string name() const { return m_name; }
  unsigned min_tokens() const { return 1; }
  unsigned max_tokens() const { return 1; }
  bool is_composing() const { return true; }
  bool is_required() const { return false; }
  bool apply_default(boost::any & value_store) const { return true; }
  void notify(const boost::any &value_store) const { }

  void parse(boost::any & value_store, const std::vector< std::string > &new_tokens,  bool utf8) const
  {
    assert(new_tokens.size() == 1);
    test_agent_ptr_t a(new AGENT(m_harness, new_tokens[0]));
    m_harness->add_agent(std::move(a));
  }

private:
  test_harness* m_harness;
  std::string m_name;
};

void test_harness::set_test_failure()
{
  m_test_failed = true;
}

int test_harness::parse_arguments(int argc, char* argv[])
{
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "Produce a help message.")
    ("quiet", "Produce less output.")
    ("timeout,t", po::value<std::string>(),
     "Set the test timeout with units of s/us/ms/ns.")
    ("pcap-in", new agent_val_sem<pcap_in_agent>(this, "FILENAME"),
     "Add input packets from a pcap file.")
    ("pcap-out", new agent_val_sem<pcap_out_agent>(this, "FILENAME"),
     "Capture output packets to a pcap file.")
    ("pcap-expect", new agent_val_sem<pcap_expect_agent>(this, "FILENAME"),
     "Compare output packets with a pcap file.")
    ("socket", new agent_val_sem<socket_agent>(this, "PARAMS"),
     "Transport packets over a socket.")
    ("map-load", new agent_val_sem<map_load_agent>(this, "FILENAME"),
     "Load maps from a text file.")
    ("map-dump", new agent_val_sem<map_dump_agent>(this, "FILENAME"),
     "Dump maps to a text file after each packet.")
    ("tap", new agent_val_sem<tap_agent>(this, "NAME"),
     "Use a Linux TAP interface.")
    ;

  po::positional_options_description pos;
  pos.add("map-load", 1);
  pos.add("pcap-in", 1);
  pos.add("map-dump", 1);
  pos.add("pcap-out", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv)
            .options(desc).positional(pos).run(), vm);
  po::notify(vm);

  if (vm.count("timeout") != 0) {
    std::string val = vm["timeout"].as<std::string>();
    double timeout_ns = 100*1000*1000;

    // Parse the number and strip it from the string.
    int n_char = 0;
    int n_field = sscanf(val.c_str(), "%lf%n", &timeout_ns, &n_char);
    if (n_field < 1) {
      std::cerr << "Invalid timeout '" << val << "'.\n";
      return 1;
    }
    val.erase(0, n_char);

    // Check the suffix.
    if (val == "s" || val == "") {
      timeout_ns *= 1000*1000*1000;

    } else if (val == "ms") {
      timeout_ns *= 1000*1000;

    } else if (val == "us") {
      timeout_ns *= 1000;

    } else if (val == "ns") {
      timeout_ns *= 1;

    } else {
      std::cerr << "Invalid timeout suffix '" << val << "'."
                << "  Expected s, ms, us or ns.\n";
      exit(1);
    }
    m_timeout_ns = ceil(timeout_ns);
  }

  if (vm.count("help") != 0) {
    std::cout << desc << "\n";
    exit(0);
  }

  // Create the verbose test agent if necessary.
  if (vm.count("quiet") == 0) {
    test_agent_ptr_t verbose(new verbose_agent(this));
    m_agents.insert(m_agents.begin(), std::move(verbose));
  }

  return 0;
}

void test_harness::start_io_thread()
{
  // Create the thread.
  test_harness *self = this;
  m_io_thread.reset(new nanotube_thread(&m_io_thread_context, "io_thread",
                                        &io_thread_func, (void*)&self,
                                        sizeof(self)));

  // Wait for a signal.
  m_signal_set.async_wait([this](const boost::system::error_code &error,
                                 int sig_num) {
                            handle_signal(error, sig_num);
                          });

  // Start the thread.
  m_io_thread->start();
}

void test_harness::stop_io_thread()
{
  // Make sure io_context::run() returns.
  m_asio_context.stop();

  // Stop waiting for signals.
  m_signal_set.cancel();

  // Stop the thread when it reaches a safe point.
  m_io_thread->stop();

  // Delete the thread.
  m_io_thread.reset();
}

void test_harness::io_thread_func(nanotube_context_t *context,
                                  void *data)
{
  test_harness *self = *(test_harness**)data;

  while (true) {
    // Start processing I/O requests.
    self->m_asio_context.run();

    // Wait for more I/O operations.
    nanotube_thread_wait();
  }
}

void test_harness::handle_signal(const boost::system::error_code &error,
                                 int sig_num)
{
  if (!error) {
    assert(sig_num == SIGINT);
    handle_quit();
    return;
  }

  if (error == asio::error::operation_aborted) {
    // This is expected at the end of the test.
    return;
  }

  std::cerr << "Unknown error waiting for signal: " << error << "\n";
  exit(1);
}

void test_harness::handle_quit()
{
  if (m_quit_flag.load()) {
    std::cerr << "Could not shutdown cleanly.  Exiting anyway.\n";
    exit(1);
  }
  m_quit_flag.store(true);

  std::cerr << "\nQuitting...\n";

  for (test_agent_ptr_t &agent : m_agents) {
    agent->handle_quit();
  }
}

int test_harness::run(int argc, char* argv[])
{
  int rc = parse_arguments(argc, argv);
  if (rc != 0)
    return rc;

  m_system = processing_system::attach(*this);

  // Start the IO thread.
  start_io_thread();

  // Inform the agents that the test is starting.
  for (test_agent_ptr_t &agent : m_agents) {
    agent->start_test();
  }

  /* Run the kernels on all packets, dumping state afterwards */
  for (auto &kernel: m_system->kernels()) {
    m_current_kernel = kernel.get();
    test_kernel(*kernel);
  }
  m_current_kernel = nullptr;

  // Inform the agents that the test is ending.
  for (test_agent_ptr_t &agent : m_agents) {
    agent->end_test();
  }

  // Stop the IO thread.
  stop_io_thread();

  processing_system::detach(m_system);

  if (m_test_failed) {
    std::cout << "Test failed!\n";
    return 1;
  }

  std::cout << "Test passed!\n";
  return 0;
}

///////////////////////////////////////////////////////////////////////////

int test_harness_run(int argc, char *argv[])
{
  try {
    test_harness t;
    return t.run(argc, argv);

  } catch (std::exception &e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
}

///////////////////////////////////////////////////////////////////////////

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
