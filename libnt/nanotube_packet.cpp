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
#include <limits>
#include <unordered_map>

#include "nanotube_packet.hpp"

#include "nanotube_api.h"
#include "simple_bus.hpp"
#include "softhub_bus.hpp"
#include "x3rx_bus.hpp"

uint8_t* nanotube_packet_data(nanotube_packet_t* packet) {
  auto sec = ( packet->get_is_capsule()
               ? NANOTUBE_SECTION_WHOLE
               : NANOTUBE_SECTION_PAYLOAD );
  return packet->begin(sec);
}

uint8_t* nanotube_packet_end(nanotube_packet_t* packet) {
  auto sec = ( packet->get_is_capsule()
               ? NANOTUBE_SECTION_WHOLE
               : NANOTUBE_SECTION_PAYLOAD );
  return packet->end(sec);
}

uint8_t* nanotube_packet_meta(nanotube_packet_t* packet) {
  assert(!packet->get_is_capsule());
  return packet->begin(NANOTUBE_SECTION_METADATA);
}

size_t nanotube_packet_bounded_length(nanotube_packet_t* packet,
                                      size_t max) {
  auto sec = ( packet->get_is_capsule()
               ? NANOTUBE_SECTION_WHOLE
               : NANOTUBE_SECTION_PAYLOAD );
  auto pkt_len = packet->size(sec);
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

  auto sec = ( packet->get_is_capsule()
               ? NANOTUBE_SECTION_WHOLE
               : NANOTUBE_SECTION_PAYLOAD );
  auto pkt_len = packet->size(sec);
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

  auto sec = ( packet->get_is_capsule()
               ? NANOTUBE_SECTION_WHOLE
               : NANOTUBE_SECTION_PAYLOAD );
  auto pkt_len = packet->size(sec);
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
  auto sec = ( packet->get_is_capsule()
               ? NANOTUBE_SECTION_WHOLE
               : NANOTUBE_SECTION_PAYLOAD );
  auto pkt_len = packet->size(sec);
  if (offset > pkt_len)
    return 0;

  packet->resize(sec, offset, adjust);
  return 1;
}

int32_t nanotube_packet_meta_resize(nanotube_packet_t *packet, size_t offset,
                               int32_t adjust)
{
  assert(!packet->get_is_capsule());
  auto pkt_len = packet->size(NANOTUBE_SECTION_METADATA);
  if (offset > pkt_len)
    return 0;

  packet->resize(NANOTUBE_SECTION_METADATA, offset, adjust);
  return 1;
}

void nanotube_packet_drop(nanotube_packet_t *packet,
                          int32_t drop)
{
  assert(false);
  if( drop ) {
    packet->resize(NANOTUBE_SECTION_WHOLE, 0);
  }
}

void nanotube_packet::reset(enum nanotube_bus_id_t bus_type,
                            bool empty_metadata)
{
  m_is_capsule = false;
  m_metadata_specified = false;
  m_port = 0;
  m_meta_size = 0;
  m_data_eop_seen = false;
  m_contents.clear();

  if (empty_metadata) {
    // The caller wants to add metadata explicitly, so leave the
    // contents empty.
    m_bus_type = bus_type;
  } else {
    // Converting the packet from Ethernet will add the default
    // metadata as requested.
    m_bus_type = NANOTUBE_BUS_ID_ETH;
    convert_bus_type(bus_type);
  }
}

int nanotube_packet::convert_bus_type(enum nanotube_bus_id_t bus_type)
{
  /* m_bus_type is the current type.  bus_type is the desired type */

  // Quick return if no conversion is necessary.
  if (m_bus_type == bus_type)
    return 0;

  // Check the target bus format to fail early.
  switch (bus_type) {
  case NANOTUBE_BUS_ID_ETH:
  case NANOTUBE_BUS_ID_SB:
  case NANOTUBE_BUS_ID_SHB:
  case NANOTUBE_BUS_ID_X3RX:
    break;

  default:
    // Format not supported.
    return EPROTONOSUPPORT;
  }

  // Convert to Ethernet from current bus type (m_bus_type) if necessary.
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
    resize(NANOTUBE_SECTION_WHOLE, 0, -(ptrdiff_t)sizeof(sb_header));
    break;
  }

  case NANOTUBE_BUS_ID_SHB: {
    // Extract the header fields.
    softhub_bus::header shb_header = {0};
    read(NANOTUBE_SECTION_WHOLE, (uint8_t*)&shb_header,
         0, sizeof(shb_header));
    m_port = softhub_bus::get_ch_route_raw((uint8_t*)&shb_header);

    // Remove the header.
    resize(NANOTUBE_SECTION_WHOLE, 0, -(ptrdiff_t)sizeof(shb_header));
    break;
  }

  case NANOTUBE_BUS_ID_X3RX: {
    // No header at the start of the packet
    // TODO extract the port from sideband metadata
    break;
  }

  default:
    // Format not supported.
    return EPROTONOSUPPORT;
  }

  // Convert from Ethernet to desired bus type (bus_type) if necessary.
  // Note: This cannot fail, so there is no need to update m_bus_type yet.
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

  case NANOTUBE_BUS_ID_X3RX: {
    // No header at the front of the packet for X3RX 
    // TODO set port in sideband metadata
    break;
  }

  default:
    assert(false);
  }

  // Conversion complete: update current bus type
  m_bus_type = bus_type;
  return 0;
}

uint8_t *
nanotube_packet::begin(nanotube_packet_section_t sec)
{
  // We only support metadata, payload and whole for now.
  switch (sec) {
  case NANOTUBE_SECTION_WHOLE:
  case NANOTUBE_SECTION_METADATA:
    return &(m_contents.front());

  case NANOTUBE_SECTION_PAYLOAD:
    return &(m_contents[get_meta_size()]);

  default:
    std::cerr << "ERROR: Unsupported section " << sec << ", aborting!\n";
    abort();
  }
}

uint8_t *nanotube_packet::end(nanotube_packet_section_t sec)
{
  // We only support metadata, payload and whole for now.
  switch (sec) {
  case NANOTUBE_SECTION_WHOLE:
  case NANOTUBE_SECTION_PAYLOAD:
    return &(m_contents.front()) + m_contents.size();

  case NANOTUBE_SECTION_METADATA:
    return &(m_contents[get_meta_size()]);

  default:
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
                             std::size_t new_size)
{
  std::size_t old_size = size(sec);
  int32_t adjustment = int32_t(new_size) - int32_t(old_size);
  resize(sec, old_size, adjustment);
}

void nanotube_packet::resize(nanotube_packet_section_t sec,
                             std::size_t offset,
                             int32_t adjustment)
{
  static const std::ptrdiff_t MAX_SSIZE =
    std::numeric_limits<std::ptrdiff_t>::max();

  // Check the offset and sizes.
  std::size_t old_size = size(sec);
  assert(old_size <= std::size_t(MAX_SSIZE));
  assert(offset <= old_size);

  // The amount the packet can grow.
  std::ptrdiff_t space = MAX_SSIZE - m_contents.size();
  assert(std::ptrdiff_t(adjustment) <= space);

  // The number of bytes after the adjustment point.
  std::ptrdiff_t tail = std::ptrdiff_t(old_size - offset);
  if (adjustment < -tail)
    adjustment = -tail;

  // The new size of the section.
  std::size_t new_size = old_size + adjustment;

  // Update the offset to be relative to the start of the contents.
  // Also update any internal metadata.
  switch (sec) {
  case NANOTUBE_SECTION_WHOLE:
    // No offset update needed.  Just update any internal metadata.
    switch (m_bus_type) {
    case NANOTUBE_BUS_ID_ETH:
      // Update the metadata size if it has been truncated.
      if (m_meta_size > new_size)
        m_meta_size = new_size;
      break;

    case NANOTUBE_BUS_ID_SB:
    case NANOTUBE_BUS_ID_SHB:
    case NANOTUBE_BUS_ID_X3RX:
      // None.
      break;

    default:
      std::cerr << "ERROR: Unsupported bus type " << m_bus_type
                << " for whole resize, aborting!\n";
      break;
    }
    break;

  case NANOTUBE_SECTION_PAYLOAD:
    // Resize the payload and update the metadata if necessary.
    switch (m_bus_type) {
    case NANOTUBE_BUS_ID_ETH:
      assert(m_contents.size() >= m_meta_size);
      offset += m_meta_size;
      break;

    case NANOTUBE_BUS_ID_SB:
      assert(m_contents.size() >= sizeof(simple_bus::header));
      offset += sizeof(simple_bus::header);
      break;

    case NANOTUBE_BUS_ID_SHB:
      assert(m_contents.size() >= sizeof(softhub_bus::header));
      offset += sizeof(softhub_bus::header);
      break;

    case NANOTUBE_BUS_ID_X3RX:
      break;

    default:
      std::cerr << "ERROR: Unsupported bus type " << m_bus_type
                << " for payload resize, aborting!\n";
      abort();
    }
    break;

  case NANOTUBE_SECTION_METADATA:
    switch (m_bus_type) {
    case NANOTUBE_BUS_ID_ETH:
      m_meta_size = new_size;
      break;

    case NANOTUBE_BUS_ID_SB:
    case NANOTUBE_BUS_ID_SHB:
    case NANOTUBE_BUS_ID_X3RX:
      // Not supported yet.
    default:
      std::cerr << "ERROR: Unsupported bus type " << m_bus_type
                << " for metadata resize, aborting!\n";
      abort();
    }
    break;

  default:
    std::cerr << "ERROR: Unsupported section " << sec << ", aborting!\n";
    abort();
  }

  if (adjustment > 0) {
    m_contents.insert(m_contents.begin() + offset, adjustment, 0);
  } else if (adjustment < 0) {
    m_contents.erase(m_contents.begin() + offset,
                     m_contents.begin() + offset - adjustment);
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
  switch (m_bus_type) {
  case NANOTUBE_BUS_ID_ETH:
  case NANOTUBE_BUS_ID_X3RX:
    return m_port;

  case NANOTUBE_BUS_ID_SB: {
    assert(m_contents.size() >= sizeof(simple_bus::header));
    auto *hdr = (simple_bus::header*)&(m_contents.front());
    return hdr->port;
  }

  case NANOTUBE_BUS_ID_SHB: {
    const uint8_t *data = &(m_contents.front());
    return softhub_bus::get_ch_route_raw(data);
  }

  default:
    std::cerr << "ERROR: Unsupported bus type " << m_bus_type
              << " for get_port, aborting!\n";
    assert(false);
  }
}

void nanotube_packet::set_port(nanotube_packet_port_t port)
{
  switch (m_bus_type) {
  case NANOTUBE_BUS_ID_ETH:
  case NANOTUBE_BUS_ID_X3RX:
    m_port = port;
    break;

  case NANOTUBE_BUS_ID_SB: {
    assert(m_contents.size() >= sizeof(simple_bus::header));
    auto *hdr = (simple_bus::header*)&(m_contents.front());
    hdr->port = port;
    break;
  }

  case NANOTUBE_BUS_ID_SHB: {
    uint8_t *data = &(m_contents.front());
    softhub_bus::set_ch_route_raw(data, port);
    break;
  }

  default:
    std::cerr << "ERROR: Unsupported bus type " << m_bus_type
              << " for set_port, aborting!\n";
    assert(false);
  }
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
    size_t total_size = m_contents.size();

    assert(offset < total_size);
    size_t remaining = m_contents.size() - offset;
    uint8_t *data = &(m_contents[offset]);

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
    size_t total_size = m_contents.size();

    assert(offset < total_size);
    size_t remaining = m_contents.size() - offset;
    uint8_t *data = &(m_contents[offset]);

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

  case NANOTUBE_BUS_ID_X3RX: {
    assert(buf_size == x3rx_bus::total_bytes);
    size_t total_size = m_contents.size();

    assert(offset < total_size);
    size_t remaining = m_contents.size() - offset;
    uint8_t *data = &(m_contents[offset]);

    // Handle first word (which is also not the last)
    if (offset == 0) {
      assert(remaining > x3rx_bus::data_bytes);
      x3rx_bus::set_sideband_raw(buffer+x3rx_bus::sideband_offset(0),
                                 1, 0, x3rx_bus::data_bytes, 1, 0,
                                 0 /* TODO port */);
      memcpy(buffer, data, x3rx_bus::data_bytes);
      *iter = offset + x3rx_bus::data_bytes;
      return true;
    }

    // Handle middle word (not first, not last)
    if (remaining > x3rx_bus::data_bytes) {
      x3rx_bus::set_sideband_raw(buffer+x3rx_bus::sideband_offset(0),
                                 0, 0, x3rx_bus::data_bytes, 0, 0, 0);
      memcpy(buffer, data, x3rx_bus::data_bytes);
      *iter = offset + x3rx_bus::data_bytes;
      return true;
    }

    // Handle the final (potentially partial) word
    assert(remaining > 0);
    x3rx_bus::set_sideband_raw(buffer+x3rx_bus::sideband_offset(0),
                               0, 1, remaining, 0, 1, 0);
    memcpy(buffer, data, remaining);
    if (remaining < x3rx_bus::data_bytes)
      memset(buffer+remaining, 0, x3rx_bus::data_bytes-remaining);
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
      m_contents.insert(m_contents.end(), data, data+num_bytes);
      // Indicate that there are more words to come.
      return true;
    }

    // If EOP is set then append the bytes which are not empty.
    std::size_t empty = simple_bus::get_control_empty(control);
    assert(empty < num_bytes);
    num_bytes -= empty;
    m_contents.insert(m_contents.end(), data, data+num_bytes);

    // Indicate that this is the last word.
    return false;
  }

  case NANOTUBE_BUS_ID_SHB: {
    assert(buf_size == softhub_bus::total_bytes);

    uint8_t *data = buffer+softhub_bus::data_offset(0);
    std::size_t num_bytes = softhub_bus::data_bytes;

    // Insert the bytes into the packet.
    m_contents.insert(m_contents.end(), data, data+num_bytes);

    // Check whether we reached the end of the packet.  The header is
    // already present because the first word has been read.
    static_assert(sizeof(softhub_bus::header) <= softhub_bus::total_bytes);

    uint8_t *p_data = &(m_contents.front());
    auto sec_len = m_contents.size();
    auto cap_len = softhub_bus::get_ch_length_raw(p_data);

    // Indicate that there is more to come if the packet is not
    // complete.
    if (sec_len < cap_len) {
      return true;
    }

    // Otherwise end of packet reached so strip the excess bytes.
    if (cap_len < sec_len) {
      m_contents.resize(cap_len);
    }

    // Indicate that this was the last word.
    return false;
  }

  case NANOTUBE_BUS_ID_X3RX: {
    assert(buf_size == x3rx_bus::total_bytes);

    // Get a pointer to the start of the data/sideband.
    x3rx_bus::byte_t *data = buffer+x3rx_bus::data_offset(0);
    x3rx_bus::byte_t *sideband = buffer+x3rx_bus::sideband_offset(0);

    size_t num_bytes = x3rx_bus::data_bytes;

    // If data EOP is not set then just append the whole word.
    if (!x3rx_bus::get_sideband_data_eop(sideband) && !m_data_eop_seen) {
      m_contents.insert(m_contents.end(), data, data+num_bytes);
      // Indicate that there are more words to come.
      return true;
    }
    // If data EOP is set then append the available bytes
    else if (x3rx_bus::get_sideband_data_eop(sideband)) {
      m_data_eop_seen = true;

      int valid = x3rx_bus::get_sideband_data_eop_valid(sideband);
      switch( valid ) {
        case 1:
          num_bytes = 1;
          break;
        case 3:
          num_bytes = 2;
          break;
        case 7:
          num_bytes = 3;
          break;
        case 15:
          num_bytes = 4;
          break;
        default:
          assert(false);
      }

      m_contents.insert(m_contents.end(), data, data+num_bytes);
    }
    else if (m_data_eop_seen) {
      // No data bytes after we've seen data EOP
      num_bytes = 0;
    }

    // If we get here we've run out of packet data but have to keep going if not
    // read all the metadata/sideband
    if (x3rx_bus::get_sideband_meta_eop(sideband)) {
      // We've reached the end of the packet
      return false;
    }
    else {
      return true;
    }
  }
  }
}

std::size_t nanotube_packet::get_meta_size() const
{
  switch (m_bus_type) {
  case NANOTUBE_BUS_ID_ETH:
    assert(m_contents.size() >= m_meta_size);
    return m_meta_size;

  case NANOTUBE_BUS_ID_SB:
    assert(m_contents.size() >= sizeof(simple_bus::header));
    return sizeof(simple_bus::header);

  case NANOTUBE_BUS_ID_SHB:
    assert(m_contents.size() >= sizeof(softhub_bus::header));
    return sizeof(softhub_bus::header);

  case NANOTUBE_BUS_ID_X3RX:
    return 0;

  default:
    std::cerr << "ERROR: Unsupported bus type " << m_bus_type
              << ", aborting!\n";
    abort();
  }
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
