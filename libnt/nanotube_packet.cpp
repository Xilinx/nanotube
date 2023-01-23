/*******************************************************/
/*! \file nanotube_packet.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Implementations of Nanotube packets.
**   \date 2020-03-03
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unordered_map>

#include "nanotube_packet.hpp"

#include "nanotube_api.h"
#include "simple_bus.hpp"
#include "softhub_bus.hpp"

uint8_t* nanotube_packet_data(nanotube_packet_t* packet) {
  return packet->begin(NANOTUBE_SECTION_PAYLOAD);
}

uint8_t* nanotube_packet_end(nanotube_packet_t* packet) {
  return packet->end(NANOTUBE_SECTION_PAYLOAD);
}

uint8_t* nanotube_packet_meta(nanotube_packet_t* packet) {
  return packet->begin(NANOTUBE_SECTION_METADATA);
}

size_t nanotube_packet_bounded_length(nanotube_packet_t* packet,
                                      size_t max) {
  auto pkt_len = packet->size(NANOTUBE_SECTION_PAYLOAD);
  return std::min(pkt_len, max);
}


nanotube_packet_port_t
nanotube_packet_get_port(nanotube_packet_t* packet)
{
  return packet->get_port();
}


void
nanotube_packet_set_port(nanotube_packet_t* packet,
                         nanotube_packet_port_t port)
{
  packet->set_port(port);
}


size_t nanotube_packet_read(nanotube_packet_t* packet, uint8_t* buffer,
                            size_t offset, size_t length) {
  /* Always write the whole buffer */
  if( buffer != nullptr )
    memset(buffer, 0, length);

  if( buffer == nullptr || packet == nullptr )
    return 0;

  auto pkt_len = packet->size(NANOTUBE_SECTION_PAYLOAD);
  if( offset > pkt_len )
    return 0;

  /* Truncate long reads */
  if( offset + length > pkt_len )
    length = pkt_len - offset;

  assert(length <= pkt_len);
  assert(offset + length <= pkt_len);

  memcpy(buffer, nanotube_packet_data(packet) + offset, length);
  return length;
}

size_t nanotube_packet_write_masked(nanotube_packet_t* packet,
                                    const uint8_t* data_in,
                                    const uint8_t* mask,
                                    size_t offset, size_t length) {
  if( data_in == nullptr || mask == nullptr || packet == nullptr)
    return 0;

  auto pkt_len = packet->size(NANOTUBE_SECTION_PAYLOAD);
  if( offset > pkt_len )
    return 0;

  /* Truncate long writes */
  if( offset + length > pkt_len )
    length = pkt_len - offset;

  assert(length <= pkt_len);
  assert(offset + length <= pkt_len);

  uint8_t* data = nanotube_packet_data(packet);

  for( size_t i = 0; i < length; ++i ) {
    if( mask[i / 8] & (1u << (i % 8)) )
      data[offset + i] = data_in[i];
  }
  return length;
}

int32_t nanotube_packet_resize(nanotube_packet_t *packet, size_t offset,
                               int32_t adjust)
{
  auto pkt_len = packet->size(NANOTUBE_SECTION_PAYLOAD);
  if (offset > pkt_len)
    return 0;

  packet->resize(NANOTUBE_SECTION_PAYLOAD, offset, adjust);
  return 1;
}

int32_t nanotube_packet_meta_resize(nanotube_packet_t *packet, size_t offset,
                               int32_t adjust)
{
  auto pkt_len = packet->size(NANOTUBE_SECTION_METADATA);
  if (offset > pkt_len)
    return 0;

  packet->resize(NANOTUBE_SECTION_METADATA, offset, adjust);
  return 1;
}

void nanotube_packet_drop(nanotube_packet_t *packet,
                          int32_t drop)
{
  if( drop ) {
    packet->resize(NANOTUBE_SECTION_WHOLE, 0);
  }
}

void nanotube_packet::reset(enum nanotube_bus_id_t bus_type)
{
  m_bus_type = bus_type;
  m_port = 0;
  m_meta_size = 0;
  contents.clear();
}

int nanotube_packet::convert_bus_type(enum nanotube_bus_id_t bus_type)
{
  // Quick return if no conversion is necessary.
  if (m_bus_type == bus_type)
    return 0;

  // Check the target bus format to fail early.
  switch (bus_type) {
  case NANOTUBE_BUS_ID_ETH:
  case NANOTUBE_BUS_ID_SB:
  case NANOTUBE_BUS_ID_SHB:
    break;

  default:
    // Format not supported.
    return EPROTONOSUPPORT;
  }

  // Convert to Ethernet if necessary.
  switch (m_bus_type) {
  case NANOTUBE_BUS_ID_ETH:
    // No conversion necessary.
    break;

  case NANOTUBE_BUS_ID_SB: {
    // Extract the header fields.
    simple_bus::header sb_header = {0};
    read(NANOTUBE_SECTION_WHOLE, (uint8_t*)&sb_header,
         0, sizeof(sb_header));
    this->m_port = sb_header.port;

    // Remove the header.
    resize(NANOTUBE_SECTION_WHOLE, 0, -(ssize_t)sizeof(sb_header));
    break;
  }

  case NANOTUBE_BUS_ID_SHB: {
    // Extract the header fields.
    softhub_bus::header shb_header = {0};
    read(NANOTUBE_SECTION_WHOLE, (uint8_t*)&shb_header,
         0, sizeof(shb_header));
    m_port = softhub_bus::get_ch_route_raw((uint8_t*)&shb_header);

    // Remove the header.
    resize(NANOTUBE_SECTION_WHOLE, 0, -(ssize_t)sizeof(shb_header));
    break;
  }

  default:
    // Format not supported.
    return EPROTONOSUPPORT;
  }

  // Convert from Ethernet if necessary.  Note: This cannot fail, so
  // there is no need to update m_bus_type yet.
  switch (bus_type) {
  case NANOTUBE_BUS_ID_ETH:
    // No conversion necessary.
    break;

  case NANOTUBE_BUS_ID_SB: {
    // Insert the header at the front of the packet.
    simple_bus::header sb_header = {0};
    sb_header.port = this->m_port;
    insert(NANOTUBE_SECTION_WHOLE, (uint8_t*)&sb_header,
           0, sizeof(sb_header));
    break;
  }

  case NANOTUBE_BUS_ID_SHB: {
    // Insert the header at the front of the packet.
    softhub_bus::header shb_header = {0};
    softhub_bus::set_ch_route_raw((uint8_t*)&shb_header, m_port);
    insert(NANOTUBE_SECTION_WHOLE, (uint8_t*)&shb_header,
           0, sizeof(shb_header));
    break;
  }

  default:
    assert(false);
  }

  m_bus_type = bus_type;
  return 0;
}

uint8_t *
nanotube_packet::begin(nanotube_packet_section_t sec)
{
  // We only support metadata & payload for now.
  if (sec == NANOTUBE_SECTION_WHOLE || sec == NANOTUBE_SECTION_METADATA)
    return &(contents.front());
  else if (sec == NANOTUBE_SECTION_PAYLOAD)
    return &(contents.front()) + m_meta_size;
  else {
    std::cerr << "ERROR: Unsupported section " << sec << ", aborting!\n";
    abort();
  }
}

uint8_t *nanotube_packet::end(nanotube_packet_section_t sec)
{
  // We only support metadata & payload for now.
  if (sec == NANOTUBE_SECTION_WHOLE || sec == NANOTUBE_SECTION_PAYLOAD)
    return &(contents.front()) + contents.size();
  else if (sec == NANOTUBE_SECTION_METADATA)
    return &(contents.front()) + m_meta_size;
  else {
    std::cerr << "ERROR: Unsupported section " << sec << ", aborting!\n";
    abort();
  }
}

std::size_t nanotube_packet::size(nanotube_packet_section_t sec) const
{
  return end(sec) - begin(sec);
}

void nanotube_packet::read(nanotube_packet_section_t sec,
                           uint8_t *buffer, std::size_t offset,
                           std::size_t length)
{
  auto sec_size = size(sec);
  assert(offset <= sec_size);
  assert(length <= sec_size - offset);
  const uint8_t *data = begin(sec) + offset;
  memcpy(buffer, data, length);
}

void nanotube_packet::write(nanotube_packet_section_t sec,
                            const uint8_t *buffer, std::size_t offset,
                            std::size_t length)
{
  auto sec_size = size(sec);
  assert(offset <= sec_size);
  assert(length <= sec_size - offset);
  uint8_t *data = begin(sec) + offset;
  memcpy(data, buffer, length);
}

void nanotube_packet::resize(nanotube_packet_section_t sec,
                             std::size_t size)
{
  // TODO support other sections.
  assert(sec == NANOTUBE_SECTION_WHOLE);
  contents.resize(size);
}

void nanotube_packet::resize(nanotube_packet_section_t sec,
                             std::size_t offset,
                             int32_t adjustment)
{
  assert(sec == NANOTUBE_SECTION_WHOLE
      || sec == NANOTUBE_SECTION_METADATA
      || sec == NANOTUBE_SECTION_PAYLOAD);
  if (sec == NANOTUBE_SECTION_PAYLOAD)
    offset += m_meta_size;
  else if (sec == NANOTUBE_SECTION_METADATA)
    m_meta_size += adjustment;
  if (adjustment > 0) {
    contents.insert(contents.begin() + offset, adjustment, 0);
  } else if (adjustment < 0) {
    if (contents.size() - offset < (std::size_t)-adjustment)
      adjustment = -(contents.size() - offset);
    contents.erase(contents.begin() + offset,
                   contents.begin() + offset - adjustment);
  }
}

void nanotube_packet::insert(nanotube_packet_section_t sec,
                             const uint8_t *buffer, std::size_t offset,
                             std::size_t length)
{
  resize(sec, offset, length);
  write(sec, buffer, offset, length);
}

nanotube_packet_port_t nanotube_packet::get_port() const
{
  return m_port;
}

void nanotube_packet::set_port(nanotube_packet_port_t port)
{
  m_port = port;
}

bool
nanotube_packet::get_bus_word(uint8_t *buffer, std::size_t buf_size,
                              std::size_t *iter)
{
  std::size_t offset = *iter;

  switch (m_bus_type) {
  default:
    assert(false); // Unsupported.

  case NANOTUBE_BUS_ID_SB: {
    assert(buf_size == simple_bus::total_bytes);
    size_t total_size = contents.size();

    assert(offset < total_size);
    size_t remaining = contents.size() - offset;
    uint8_t *data = &(contents[offset]);

    // Handle a word which is not the last.
    if (remaining > simple_bus::data_bytes) {
      memcpy(buffer, data, simple_bus::data_bytes);
      buffer[simple_bus::control_offset()] =
        simple_bus::control_value(false, false, 0);
      *iter = offset + simple_bus::data_bytes;
      return true;
    }

    // Handle the last (potentially partial) word.
    assert(remaining > 0);
    memcpy(buffer, data, remaining);
    auto empty = simple_bus::data_bytes-remaining;
    if (remaining < simple_bus::data_bytes)
      memset(buffer+remaining, 0, empty);
    buffer[simple_bus::control_offset()] =
      simple_bus::control_value(true, false, empty);
    return false;
  }

  case NANOTUBE_BUS_ID_SHB: {
    assert(buf_size == softhub_bus::total_bytes);
    size_t total_size = contents.size();

    assert(offset < total_size);
    size_t remaining = contents.size() - offset;
    uint8_t *data = &(contents[offset]);

    // Handle a word which is not the last.
    if (remaining > softhub_bus::data_bytes) {
      memcpy(buffer, data, softhub_bus::data_bytes);
      *iter = offset + softhub_bus::data_bytes;
      return true;
    }

    // Handle the final (potentially partial) word.
    assert(remaining > 0);
    memcpy(buffer, data, remaining);
    if (remaining < softhub_bus::data_bytes)
      memset(buffer+remaining, 0, softhub_bus::data_bytes-remaining);
    return false;
  }
  }
}

bool
nanotube_packet::add_bus_word(uint8_t *buffer, std::size_t buf_size)
{
  switch (m_bus_type) {
  default:
    assert(false); // Unsupported.

  case NANOTUBE_BUS_ID_SB: {
    assert(buf_size == simple_bus::total_bytes);

    // The number of bytes to append.
    uint8_t *data = buffer + simple_bus::data_offset(0);
    std::size_t num_bytes = simple_bus::data_bytes;

    // If EOP is not set then just append the whole word.
    auto control = buffer[simple_bus::control_offset()];
    if ((control & simple_bus::control_eop) == 0) {
      contents.insert(contents.end(), data, data+num_bytes);
      // Indicate that there are more words to come.
      return true;
    }

    // If EOP is set then append the bytes which are not empty.
    std::size_t empty = simple_bus::get_control_empty(control);
    assert(empty < num_bytes);
    num_bytes -= empty;
    contents.insert(contents.end(), data, data+num_bytes);

    // Indicate that this is the last word.
    return false;
  }

  case NANOTUBE_BUS_ID_SHB: {
    assert(buf_size == softhub_bus::total_bytes);

    uint8_t *data = buffer+softhub_bus::data_offset(0);
    std::size_t num_bytes = softhub_bus::data_bytes;

    // Insert the bytes into the packet.
    contents.insert(contents.end(), data, data+num_bytes);

    // Check whether we reached the end of the packet.  The header is
    // already present because the first word has been read.
    static_assert(sizeof(softhub_bus::header) <= softhub_bus::total_bytes);

    uint8_t *p_data = &(contents.front());
    auto sec_len = contents.size();
    auto cap_len = softhub_bus::get_ch_length_raw(p_data);

    // Indicate that there is more to come if the packet is not
    // complete.
    if (sec_len < cap_len) {
      return true;
    }

    // Otherwise end of packet reached so strip the excess bytes.
    if (cap_len < sec_len) {
      contents.resize(cap_len);
    }

    // Indicate that this was the last word.
    return false;
  }
  }
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
