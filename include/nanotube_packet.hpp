/**************************************************************************\
*//*! \file nanotube_packet.hpp
** \author  Stephan Diestelhorst <stephand@amd.com>
**          Neil Turton <neilt@amd.com>
**  \brief  Nanotube packet internal definitions.
**   \date  2020-08-26
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PACKET_HPP
#define NANOTUBE_PACKET_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "nanotube_api.h"
#include "bus_id.h"

// Defines sections of a packet.  The interpretation of the sections
// may depend on the bus type.
// For the time being we only support WHOLE, METADATA, and PAYLOAD.
typedef enum {
  // The whole packet, including all metadata.
  NANOTUBE_SECTION_WHOLE,

  // The packet payload.  Typically Ethernet.
  NANOTUBE_SECTION_PAYLOAD,

  // All the metadata.  This may be the same as
  // NANOTUBE_SECTION_SYS_META.
  NANOTUBE_SECTION_METADATA,

  // The system metadata.  This may or may not include
  // NANOTUBE_SECTION_USER_META.
  NANOTUBE_SECTION_SYS_META,

  // The user metadata, available for application use.
  NANOTUBE_SECTION_USER_META,
} nanotube_packet_section_t;

/*!
** Implementation of a Nanotube packet.
**
** Most users should not have to care about the specific contents / layout,
** here!
*/
class nanotube_packet {
public:
  /*! Construct a packet. */
  nanotube_packet(enum nanotube_bus_id_t bus_type = NANOTUBE_BUS_ID_ETH) {
    reset(bus_type);
  }

  /*! Reset the packet contents with the specified bus type. */
  void reset(enum nanotube_bus_id_t bus_type);

  /*! Reset the packet contents without changing the bus type. */
  void reset() { reset(m_bus_type); }

  enum nanotube_bus_id_t get_bus_type() const {
    return m_bus_type;
  }

  /*! Get the flag indicating whether the packet should be treated as
   *  a capsule. */
  bool get_is_capsule() const { return m_is_capsule; }

  /*! Set the flag indicating whether the packet should be treated as
   *  a capsule. */
  void set_is_capsule(bool val) { m_is_capsule = val; }

  /*! Convert the packet to a different bus type.
   *
   * \param bus_type The target bus type.
   * \returns a Linux error code.
   */
  int convert_bus_type(enum nanotube_bus_id_t bus_type);

  /*! Get a pointer to the beginning of a section. */
  const uint8_t *begin(nanotube_packet_section_t sec) const {
    /* Call the non-const method and cast the result.  Use of a
     * non-const temporary makes it clear that this method isn't
     * calling itself recursively. */
    uint8_t *res = const_cast<nanotube_packet *>(this)->begin(sec);
    return res;
  }

  /*! Get a pointer to the beginning of a section. */
  uint8_t *begin(nanotube_packet_section_t sec);

  /*! Get a pointer to the end of a section. */
  const uint8_t *end(nanotube_packet_section_t sec) const {
    /* Call the non-const method and cast the result.  Use of a
     * non-const temporary makes it clear that this method isn't
     * calling itself recursively. */
    uint8_t *res = const_cast<nanotube_packet *>(this)->end(sec);
    return res;
  }

  /*! Get a pointer to the end of a section. */
  uint8_t *end(nanotube_packet_section_t sec);

  /*! Get the size of a section. */
  std::size_t size(nanotube_packet_section_t sec) const;

  /*! Read bytes from a section. */
  void read(nanotube_packet_section_t sec, uint8_t *buffer,
            std::size_t offset, std::size_t length);

  /*! Write bytes into a section. */
  void write(nanotube_packet_section_t sec, const uint8_t *buffer,
             std::size_t offset, std::size_t length);

  /*! Set the size of a section.  The section is truncated or filled
   *  with zeros. Only support SECTION_WHOLE for now. */
  void resize(nanotube_packet_section_t sec, std::size_t size);

  /*! Adjust the size of the section.  Invalidates all pointers to
   *  data in this packet.
   *
   * \param sec     The section to adjust.
   * \param offset  The offset at which to insert/remove bytes.
   * \param adjust  The number of bytes inserted (positive) into or
   *                removed (negative) from the packet.
   */
  void resize(nanotube_packet_section_t sec, std::size_t offset,
              int32_t adjustment);

  /*! Insert bytes into a section. */
  void insert(nanotube_packet_section_t sec, const uint8_t *buffer,
              std::size_t offset, std::size_t length);

  /*! Get the current port number. */
  nanotube_packet_port_t get_port() const;

  /*! Set the current port number. */
  void set_port(nanotube_packet_port_t port);

  /*! Get a bus word from the packet.
   *
   * \param buffer   The buffer to receive the word.
   * \param buf_size The size of the buffer in bytes.
   * \param iter     A pointer to an iterator.
   *
   * \returns true if there are more words.
   *
   * The buffer size must be correct for the bus type.  The value of
   * iter on the first call must be zero and it will be updated for
   * the next call.
   */
  bool get_bus_word(uint8_t *buffer, std::size_t buf_size,
                    std::size_t *iter);

  /*! Add a bus word to the packet.
   *
   * \param buffer   The buffer to receive the word.
   * \param buf_size The size of the buffer in bytes.
   *
   * \returns true if there are more words.
   *
   * The buffer size must be correct for the bus type.  The packet
   * should be reset before calling this method for the first word.
   * The packet will be valid after this method returns true.
   */
  bool add_bus_word(uint8_t *buffer, std::size_t buf_size);

private:
  /*! Get the size of the metadata header. */
  std::size_t get_meta_size() const;

  /*! The bus type used to represent the packet. */
  enum nanotube_bus_id_t m_bus_type;

  /*! A flag indicating whether the packet should be treated as a
   *  capsule.  This is only used for the C API. */
  bool m_is_capsule;

  /*! The contents of the packet. Contains metadata prefixed to packet data. */
  std::vector<uint8_t> m_contents;

  /*! The current destination port of the packet. */
  nanotube_packet_port_t m_port;

  /*! Byte offset to start of packet data in contents. */
  std::size_t m_meta_size;

  bool m_data_eop_seen; // Only used for x3rx bus EOP tracking
};

typedef std::unique_ptr<struct nanotube_packet> nanotube_packet_ptr_t;

#endif // NANOTUBE_PACKET_HPP
