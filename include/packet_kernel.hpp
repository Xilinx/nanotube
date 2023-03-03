/*******************************************************/
/*! \file packet_kernel.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief An interface to a packet kernel.
**   \date 2020-08-27
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef PACKET_KERNEL_HPP
#define PACKET_KERNEL_HPP

#include "nanotube_api.h"
#include "nanotube_packet.hpp"

#include <string>

class processing_system;

class packet_kernel
{
public:
  // Create a packet kernel.
  packet_kernel(processing_system &system, const std::string &name);

  // Get the name of the kernel.
  const std::string &get_name() const { return m_name; }

  // Get the bus type.
  enum nanotube_bus_id_t get_bus_type() const { return m_bus_type; }

  // Get the timeout in nanoseconds.
  uint64_t get_timeout_ns() const { return m_timeout_ns; }

  // Set the timeout in nanoseconds.
  void set_timeout_ns(uint64_t timeout_ns) { m_timeout_ns = timeout_ns; }

  // Process a packet.  The packet may be modified but ownership is
  // not transferred.
  virtual void process(nanotube_packet_t *packet) = 0;

  // Try to receive a packet from the kernel.  Returns true if the
  // caller needs to poll again before calling nanotube_thread_wait.
  virtual bool poll();

  // Flush the kernel to make sure all outstanding packets are
  // received.
  virtual void flush();

  // Set the context of the thread which is allowed to send packets to
  // the kernel.
  virtual void set_sender(nanotube_context *sender);

  // Set the context of the thread which is allowed to receive packets
  // from the kernel.
  virtual void set_receiver(nanotube_context *receiver);

protected:
  // The processing system.
  processing_system &m_system;

  // Format of capsules required
  enum nanotube_bus_id_t m_bus_type;

private:
  // The kernel name.
  std::string m_name;

  // The timeout in nanoseconds.
  uint64_t m_timeout_ns;
};

/**
 * A packet kernel that operates on the granularity of packets / capsules, with
 * the user function being called once for the entire packet.
 *
 * If capsules = true, the system will inline header data and a control footer
 * from the bus into the packet thereby turning it into a capsule.  That means
 * that application offsets need to be adjusted.  The "platform" pass will do
 * that.
 */
class func_packet_kernel: public packet_kernel {
public:
  // Create the packet_kernel object.
  func_packet_kernel(processing_system &system,
                     const char* name,
                     nanotube_kernel_t* info,
                     enum nanotube_bus_id_t bus_type,
                     int capsules);

  // Process a packet.
  void process(nanotube_packet_t *packet) override;

private:
  // The kernel function.
  nanotube_kernel_t *m_kernel;
  // Does this kernel handle capsules (or packets)?
  bool m_capsules;
};

/**
 * A kernel type that operates on capsules (bus header information prepended to
 * the packet data) and on a word granularity.  The user kernel gets called for
 * every packet / capsule word, rather than just packets.
 *
 * The wordify / pipeline pass will convert from func_capsule_kernel to
 * channel_packet_kernel abstraction levels.
 */
class channel_packet_kernel: public packet_kernel
{
public:
  channel_packet_kernel(processing_system &system,
                        nanotube_channel &packet_write_channel,
                        nanotube_channel &packet_read_channel);

  // Process a packet.
  void process(nanotube_packet_t *packet) override;

  // Try to receive a packet.
  bool poll();

  // Flush the kernel.
  void flush() override;

  // Set the context of the thread which is allowed to send packets to
  // the kernel.
  void set_sender(nanotube_context *sender) override;

  // Set the context of the thread which is allowed to receive packets
  // from the kernel.
  void set_receiver(nanotube_context *receiver) override;

private:
  // Write a packet on a simple packet interface.
  void write_simple_packet(nanotube_packet_t *packet);
  void write_softhub_packet(nanotube_packet_t *packet);

  // Write a word to m_packets_in.
  void write_word(const uint8_t *data, size_t data_size);

  // Try to read and process a word from m_packets_in.
  bool try_read_word();
  bool try_read_simple_word();
  bool try_read_softhub_word();

  nanotube_channel &m_packet_write_channel;
  nanotube_channel &m_packet_read_channel;
  nanotube_packet  m_read_packet;
};

#endif // PACKET_KERNEL_HPP
