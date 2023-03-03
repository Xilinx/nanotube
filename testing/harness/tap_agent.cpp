/*******************************************************/
/*! \file tap_agent.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent for connecting to a Linux TAP device.
**   \date 2021-09-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "tap_agent.hpp"

#include "test_harness.hpp"

#include "processing_system.hpp"

#include <csignal>
#include <iostream>

extern "C"
{
  #include <fcntl.h>
  #include <netinet/in.h>
  #include <net/if.h>
  #include <linux/if_tun.h>
  #include <poll.h>
  #include <sys/ioctl.h>
}

namespace asio = boost::asio;

///////////////////////////////////////////////////////////////////////////

tap_agent::tap_agent(test_harness* harness, const std::string &name):
  test_agent(harness),
  m_stream(*(harness->get_asio_context()))
{
  // Open the TUN/TAP device.
  const char *filename = "/dev/net/tun";
  m_fd = open(filename, O_RDWR);
  if (m_fd < 0) {
    int err = errno;
    std::cerr << "Error s_opening '" << filename << "': "
              << strerror(err) << "\n";
    exit(1);
  }

  // Create the request structure.
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));

  // Use TUN to include Ethernet headers.
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_PERSIST;

  // Set the name.  It has already been zero padded.
  size_t name_len = name.size();
  if (name_len > IFNAMSIZ) {
    std::cerr << "ERROR: Interface name '" << name << "' is longer than "
              << int(IFNAMSIZ) << " characters.\n";
    exit(1);
  }
  memcpy(ifr.ifr_name, name.data(), name_len);

  // Attach to the interface.
  int err = ioctl(m_fd, TUNSETIFF, (void*)&ifr);
  if (err < 0) {
    err = errno;
    std::cerr << "Error setting up '" << name << "': "
              << strerror(err) << "\n";
    exit(1);
  }

  // Associate the ASIO stream descriptor with the file descriptor.
  m_stream.assign(m_fd);
}

tap_agent::~tap_agent()
{
}

void tap_agent::test_kernel(packet_kernel* kernel)
{
  // Set the kernel.
  m_kernel = kernel;

  // Allow the RX thread to send packets to the kernel.
  kernel->set_sender(get_harness()->get_io_thread_context());

  // Start the first RX operation.
  init_tap_rx();

  // Make sure the IO operation is noticed.
  get_harness()->wake_io_thread();

  // Keep checking for packets.
  while (get_harness()->get_quit_flag() == false) {
    if (!kernel->poll())
      nanotube_thread_wait();
  }

  // Cancel all IO operations.
  m_stream.cancel();

  // Allow the main thread to send packets to the kernel.
  processing_system *ps = get_harness()->get_system();
  kernel->set_sender(ps->get_main_context());
}

void tap_agent::receive_packet(nanotube_packet_t* packet,
                               unsigned packet_index)
{
  auto sec = NANOTUBE_SECTION_WHOLE;
  void *buffer = (void*)(packet->begin(sec));
  auto size = packet->size(sec);
  int rc = write(m_fd, buffer, size);
  if (rc < 0) {
    int err = errno;
    std::cerr << "Error writing to TAP interface: "
              << strerror(err) << "\n";
    exit(1);
  }

  if (size_t(rc) != size) {
    std::cerr << "Unexpected return value from write to TAP"
              << " interface: " << rc << " != " << size << "\n";
    exit(1);
  }
}

void tap_agent::init_tap_rx()
{
  auto sec = NANOTUBE_SECTION_WHOLE;
  std::size_t size = 10*1024;
  m_packet.resize(sec, size);
  uint8_t *data = m_packet.begin(sec);
  m_stream.async_read_some(asio::buffer(data, size),
                           [this](const boost::system::error_code &error,
                                  std::size_t length) {
                             handle_tap_rx(error, length);
                           });
}

void tap_agent::handle_tap_rx(const boost::system::error_code &error,
                              std::size_t length)
{
  if (!error) {
    // Pass the packet to the kernel.
    auto sec = NANOTUBE_SECTION_WHOLE;
    m_packet.resize(sec, length);
    m_packet.set_port(0);
    m_kernel->process(&m_packet);

    // Start the next RX operation.
    init_tap_rx();
    return;
  }

  if (error == asio::error::operation_aborted) {
    // This is expected at the end of the test.
    return;
  }

  if (error == asio::error::eof) {
    std::cerr << "Unexpected EOF reading from TAP interface.\n";
    exit(1);
  }

  std::cerr << "Unknown error from TAP interface: " << error << "\n";
  exit(1);
}

void tap_agent::handle_quit()
{
  // Wake the main thread to make sure it notices that the quit flag
  // has been set.
  test_harness *th = get_harness();
  processing_system *ps = th->get_system();
  nanotube_thread *main_thread = ps->get_main_thread();
  main_thread->wake();
}

///////////////////////////////////////////////////////////////////////////
