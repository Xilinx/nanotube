/*******************************************************/
/*! \file packet_kernel.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief An interface to a packet kernel.
**   \date 2020-08-28
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "bus_id.h"

#include "packet_kernel.hpp"

#include "processing_system.hpp"
#include "nanotube_channel.hpp"
#include "nanotube_thread.hpp"

#include "simple_bus.hpp"
#include "softhub_bus.hpp"
#include "x3rx_bus.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <memory>

///////////////////////////////////////////////////////////////////////////

packet_kernel::packet_kernel(processing_system &system,
                             const std::string &name):
  m_system(system),
  m_bus_type(NANOTUBE_BUS_ID_ETH),
  m_name(name),
  m_timeout_ns(100*1000*1000)
{
}

bool packet_kernel::poll()
{
  return false;
}

void packet_kernel::flush()
{
}

void packet_kernel::set_sender(nanotube_context *sender)
{
}

void packet_kernel::set_receiver(nanotube_context *receiver)
{
}

///////////////////////////////////////////////////////////////////////////

func_packet_kernel::func_packet_kernel(processing_system &system,
                                       const char* name,
                                       nanotube_kernel_t* kernel,
                                       enum nanotube_bus_id_t bus_type,
                                       int capsules):
  packet_kernel(system, name),
  m_kernel(kernel),
  m_capsules(capsules)
{
  // If the function is not processing capsules then force the bus
  // type to Ethernet so they do not receive a capsule header.
  m_bus_type = capsules ? bus_type : NANOTUBE_BUS_ID_ETH;
}

void func_packet_kernel::process(nanotube_packet_t *packet)
{
  nanotube_kernel_rc_t rc;
  auto* contexts = &m_system.contexts();
  if( contexts->size() == 0 ) {
    /* The system does not yet have any contexts; create one here, and copy
     * over the global maps for this context usage. NOTE: This code should
     * probably live somewhere else, and will not be needed anymore, once all
     * tests have been converted to do this themselves by using nanotube_setup.
     *
     * We rely on the maps being pulled into the system, and simply map them
     * into our temporary context 1:1
     */
    std::unique_ptr<nanotube_context> ctx(new nanotube_context());
    for( auto& id_map : m_system.maps() )
      ctx->add_map(id_map.first, id_map.second.get());
    m_system.add_context(std::move(ctx));
  }
  assert(contexts->size() > 0);

  /* Convert the packet to the required format. */
  int err = packet->convert_bus_type(m_bus_type);
  assert(err == 0);

  /* Initialize the capsule flag. */
  packet->set_is_capsule(m_capsules);

  /* Call the application kernel */
  rc = m_kernel(contexts->begin()->get(), packet);

  /* Reset the capsule flag. */
  packet->set_is_capsule(false);

  err = packet->convert_bus_type(NANOTUBE_BUS_ID_ETH);
  assert(err == 0);

  m_system.receive_packet(packet, rc);
}

///////////////////////////////////////////////////////////////////////////
channel_packet_kernel::channel_packet_kernel(
    processing_system &system,
    nanotube_channel &packet_write_channel,
    nanotube_channel &packet_read_channel):
  packet_kernel(system, packet_write_channel.get_name()),
  m_packet_write_channel(packet_write_channel),
  m_packet_read_channel(packet_read_channel)
{
  nanotube_channel_type_t write_export_type = packet_write_channel.get_write_export_type();
  switch(write_export_type) {
  case NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET:
    assert(packet_read_channel.get_read_export_type() ==
           NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET);
    assert(packet_write_channel.get_elem_size() == simple_bus::total_bytes);
    assert(packet_read_channel.get_elem_size() == simple_bus::total_bytes);
    m_bus_type = NANOTUBE_BUS_ID_SB;
    break;
  case NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET:
    assert(packet_read_channel.get_read_export_type() ==
           NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET);
    assert(packet_write_channel.get_elem_size() == softhub_bus::total_bytes);
    assert(packet_read_channel.get_elem_size() == softhub_bus::total_bytes);
    m_bus_type = NANOTUBE_BUS_ID_SHB;
    break;
  case NANOTUBE_CHANNEL_TYPE_X3RX_PACKET:
    assert(packet_read_channel.get_read_export_type() ==
           NANOTUBE_CHANNEL_TYPE_X3RX_PACKET);
    assert(packet_write_channel.get_elem_size() == x3rx_bus::total_bytes);
    assert(packet_read_channel.get_elem_size() == x3rx_bus::total_bytes);
    m_bus_type = NANOTUBE_BUS_ID_X3RX;
    break;
  default:
    assert(write_export_type == NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET ||
           write_export_type == NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET ||
           write_export_type == NANOTUBE_CHANNEL_TYPE_X3RX_PACKET);
  }

  m_read_packet.reset(m_bus_type);
}

void channel_packet_kernel::process(nanotube_packet_t *packet)
{
  switch (m_packet_write_channel.get_write_export_type()) {
  default:
  case NANOTUBE_CHANNEL_TYPE_NONE:
    assert(false);
    break;

  case NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET:
    write_simple_packet(packet);
    break;

  case NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET:
    write_softhub_packet(packet);
    break;

  case NANOTUBE_CHANNEL_TYPE_X3RX_PACKET:
    write_x3rx_packet(packet);
    break;
  }
}

bool channel_packet_kernel::poll()
{
  bool active = false;
  while (try_read_word())
    active = true;
  return active;
}

void
channel_packet_kernel::write_simple_packet(nanotube_packet_t *packet)
{
  // Make sure the correct metadata is present.
  packet->convert_bus_type(NANOTUBE_BUS_ID_SB);

  simple_bus::word w = {0};
  std::size_t iter = 0;

  assert(m_packet_write_channel.get_elem_size() == simple_bus::total_bytes);

  bool more = true;
  while (more) {
    // Get a word from the packet.
    more = packet->get_bus_word(w.bytes, simple_bus::total_bytes,
                                &iter);

    // Write it to the channel.
    write_word(w.bytes, simple_bus::total_bytes);
  }
}

void 
channel_packet_kernel::write_softhub_packet(nanotube_packet_t *packet)
{
  // Make sure the correct metadata is present.
  packet->convert_bus_type(NANOTUBE_BUS_ID_SHB);

  softhub_bus::word w = {0};
  size_t sec_size = packet->size(NANOTUBE_SECTION_WHOLE);
  uint8_t *data = packet->begin(NANOTUBE_SECTION_WHOLE);
  std::size_t iter = 0;

  assert(m_packet_write_channel.get_elem_size() == softhub_bus::total_bytes);

  // Make sure the header fields are correct.
  softhub_bus::set_ch_route_raw(data, packet->get_port());
  softhub_bus::set_ch_length_raw(data, sec_size);

  bool more = true;
  while (more) {
    // Get a word from the packet.
    more = packet->get_bus_word(w.bytes, softhub_bus::total_bytes,
                                &iter);

    // Write it to the channel.
    write_word(w.bytes, softhub_bus::total_bytes);
  }
}


void
channel_packet_kernel::write_x3rx_packet(nanotube_packet_t *packet)
{
  // Make sure the correct metadata is present.
  packet->convert_bus_type(NANOTUBE_BUS_ID_X3RX);

  x3rx_bus::word w = {0};
  size_t iter = 0;

  assert(m_packet_write_channel.get_elem_size() == x3rx_bus::total_bytes);

  bool more = true;
  while (more) {
    // Get a word from the packet
    more = packet->get_bus_word(w.bytes, x3rx_bus::total_bytes, &iter);

    // Write it to the channel
    write_word(w.bytes, x3rx_bus::total_bytes);
  } 
}

void
channel_packet_kernel::write_word(const uint8_t *data,
                                  size_t data_size)
{
  bool done = false;
  while (!done) {
    bool active = false;

    // Try to write the input word.
    done = m_packet_write_channel.try_write(data, data_size);
    active |= done;

    // Try to read an output word if this thread owns the read side.
    if (m_packet_read_channel.get_reader()->is_current())
      active |= try_read_word();

    // Wait if there was nothing to do.
    if (!active)
      nanotube_thread_wait();
  }
}

bool channel_packet_kernel::try_read_word()
{
  nanotube_channel_type_t read_export_type = m_packet_read_channel.get_read_export_type();
  switch(read_export_type) {
  case NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET:
    return try_read_simple_word();
    break;
  case NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET:
    return try_read_softhub_word();
    break;
  case NANOTUBE_CHANNEL_TYPE_X3RX_PACKET:
    return try_read_x3rx_word();
    break;
  default:
    assert(read_export_type == NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET ||
           read_export_type == NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET ||
           read_export_type == NANOTUBE_CHANNEL_TYPE_X3RX_PACKET);
  }
  return false;
}

bool channel_packet_kernel::try_read_simple_word()
{
  simple_bus::word word_read_buffer;

  // Try to read a word from the channel.
  bool success = m_packet_read_channel.try_read(
    word_read_buffer.bytes, simple_bus::total_bytes);
  if (!success)
    return false;

  // Add the word to the packet.
  bool more = m_read_packet.add_bus_word(word_read_buffer.bytes,
                                         simple_bus::total_bytes);
  if (more)
    // Indicate that a word was read.
    return true;

  // Convert the packet to Ethernet.
  m_read_packet.convert_bus_type(NANOTUBE_BUS_ID_ETH);

  // Process the packet.
  m_system.receive_packet(&m_read_packet, NANOTUBE_PACKET_PASS);

  // Clear the buffer for the next packet.
  m_read_packet.reset(m_bus_type);

  // Indicate that a word was read.
  return true;
}

bool channel_packet_kernel::try_read_softhub_word()
{
  softhub_bus::word word_read_buffer;

  // Try to read a word from the channel.
  bool success = m_packet_read_channel.try_read(
    word_read_buffer.bytes, softhub_bus::total_bytes);
  if (!success)
    return false;

  // Add the word to the packet.
  bool more = m_read_packet.add_bus_word(word_read_buffer.bytes,
                                         softhub_bus::total_bytes);
  if (more)
    // Indicate that a word was read.
    return true;

  // Convert the packet to Ethernet.
  m_read_packet.convert_bus_type(NANOTUBE_BUS_ID_ETH);

  // Process the packet.
  m_system.receive_packet(&m_read_packet, NANOTUBE_PACKET_PASS);

  // Clear the buffer for the next packet.
  m_read_packet.reset(m_bus_type);

  // Indicate that a word was read.
  return true;
}


bool channel_packet_kernel::try_read_x3rx_word()
{
  x3rx_bus::word word_read_buffer;

  // Try to read a word from the channel.
  bool success = m_packet_read_channel.try_read(
    word_read_buffer.bytes, x3rx_bus::total_bytes);
  if (!success)
    return false;

  bool more = m_read_packet.add_bus_word(word_read_buffer.bytes,
                                         x3rx_bus::total_bytes);
  if (more)
    // Indicate that a word was read.
    return true;

  // Convert the packet to Ethernet.
  m_read_packet.convert_bus_type(NANOTUBE_BUS_ID_ETH);

  // Process the packet.
  m_system.receive_packet(&m_read_packet, NANOTUBE_PACKET_PASS);

  // Clear the buffer for the next packet.
  m_read_packet.reset(m_bus_type);

  // Indicate that a word was read.
  return true;
}


void channel_packet_kernel::flush()
{
  nanotube_thread::time_point_t start_time;
  nanotube_thread::time_point_t timer;
  uint64_t ns_per_ms = 1000*1000;
  uint64_t timeout_ms = (get_timeout_ns() + (ns_per_ms-1)) / ns_per_ms;
  auto timeout_pt = boost::posix_time::millisec(timeout_ms);

  start_time = nanotube_thread::get_current_time();
  nanotube_thread::init_timer(timer, timeout_pt);

  nanotube_thread_idle_waiter waiter;
  for (auto &thread_ptr: m_system.threads()) {
    waiter.monitor_thread(*thread_ptr);
  }

  while (true) {
    while (try_read_word())
      ;

    // Check the timeout before checking whether the threads are idle
    // to correctly handle the case where this thread is interrupted
    // between the two.
    bool timed_out = nanotube_thread::check_timer(timer);

    if (waiter.is_idle()) {
      auto time_now = nanotube_thread::get_current_time();
      auto elapsed = time_now - start_time;
      if (elapsed*4 >= timeout_pt) {
        int64_t usec = elapsed.total_microseconds();
        int64_t sec = usec / 1000000;
        usec -= sec*1000000;
        std::cerr << "WARNING: Flush completed after "
                  << sec << "."
                  << std::setfill('0') << std::setw(6) << usec
                  << std::setfill(' ') << std::setw(0)
                  << " seconds.\n";
      }
      break;
    }

    if (timed_out) {
      std::cerr << "packet_kernel: Flush timed out.\n";
      exit(1);
    }

    nanotube_thread_wait();
  }
}

void channel_packet_kernel::set_sender(nanotube_context *sender)
{
  m_packet_write_channel.set_writer(sender);
}

void channel_packet_kernel::set_receiver(nanotube_context *receiver)
{
  m_packet_read_channel.set_reader(receiver);
}


///////////////////////////////////////////////////////////////////////////
