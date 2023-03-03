/**************************************************************************\
*//*! \file nanotube_channel.hpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Implementation of Nanotube channels.
**   \date  2020-09-01
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_CHANNEL_HPP
#define NANOTUBE_CHANNEL_HPP

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "nanotube_context.hpp"

///////////////////////////////////////////////////////////////////////////

struct nanotube_channel
{
public:
  typedef std::unique_ptr<nanotube_channel> ptr_t;

  static const size_t ptr_mask = size_t(-1) >> 1;
  static const size_t wrap_bit = ~ptr_mask;

  /*! Create a nanotube_channel.
  //
  // \param name       The name of the channel.
  // \param elem_size  The size of each element.
  // \param num_elem   The capacity of the channel in elements.
  */
  nanotube_channel(const std::string &name, size_t elem_size,
                   size_t num_elem);

  /*! Get the context which can read the channel. */
  nanotube_context *get_reader() const { return m_reader; }

  /*! Set the context which can read the channel. */
  void set_reader(nanotube_context *context);

  /*! Get the context which can write the channel. */
  nanotube_context *get_writer() const { return m_writer; }

  /*! Set the context which can read the channel. */
  void set_writer(nanotube_context *context);

  /*! Mark the channel as being exported.
  //
  // \param type    The bus type for the export.
  // \param flags   The access type being exported.
  */
  void set_export_info(nanotube_channel_type_t type,
                       nanotube_channel_flags_t flags);

  /*! Set the size of the sideband */
  void set_sideband_size(size_t sideband_size);

  /*! Set the size of the sideband signals */
  void set_sideband_signals_size(size_t sideband_signals);

  /*! Get the name of the channel. */
  const std::string &get_name() const { return m_name; }

  /*! Get the element size of the channel. */
  size_t get_elem_size() const { return m_elem_size; }

  /*! Get the read pointer of the channel.
  //
  // The channel is implemented as a circular buffer.  This call
  // returns the current read offset into the buffer, in bytes.  It is
  // used for testing.
  */
  size_t get_read_ptr() const { return m_read_ptr.load() & ptr_mask; }

  /*! Get the write pointer of the channel.
  //
  // The channel is implemented as a circular buffer.  This call
  // returns the current write offset into the buffer, in bytes.  It
  // is used for testing.
  */
  size_t get_write_ptr() const { return m_write_ptr.load() & ptr_mask; }

  /*! Get the read export type.
  //
  // This is the value of the type parameter of the call to
  // set_export_info with flags indicating that read access is being
  // exported.
  //
  // \returns The read export type or NANOTUBE_CHANNEL_TYPE_NONE.
  */
  nanotube_channel_type_t get_read_export_type() const {
    return m_read_export_type;
  }

  /*! Get the write export type.
  //
  // This is the value of the type parameter of the call to
  // set_export_info with flags indicating that write access is being
  // exported.
  //
  // \returns The write export type or NANOTUBE_CHANNEL_TYPE_NONE.
  */
  nanotube_channel_type_t get_write_export_type() const {
    return m_write_export_type;
  }

  /*! Try to read an element from the channel.
  //
  // \param data       The start of the buffer to receive the element.
  // \param data_size  The size of the element.
  //
  // \returns a flag indicating whether an element was read.
  */
  bool try_read(void* data, size_t data_size);

  /*! Find out whether the channel has space to write an element.
  //
  // \returns a flag indicating whether there is space for an element.
  */
  bool has_space();

  /*! Try to write an element to the channel.
  //
  // \param data       The start of the buffer containing the element.
  // \param data_size  The size of the element.
  //
  // \returns a flag indicating whether an element was written.
  */
  bool try_write(const void* data, size_t data_size);

private:
  // The name of the channel.
  std::string m_name;

  // The size of each element in the vector.
  size_t m_elem_size;

  // The size of sideband data in the vector (included in m_elem_size)
  size_t m_sideband_size;

  // The size of sideband signals in the vector (included in m_elem_size)
  size_t m_sideband_signals_size;

  // The lower bits, covered by ptr_mask, are the byte offset of the
  // next element to read.  The top bit flips every time the read
  // pointer wraps to distinguish between a full channel and an empty
  // channel.
  std::atomic<size_t> m_read_ptr;

  // The lower bits, covered by ptr_mask, are the byte offset of the
  // next element to write.  The top bit flips every time the write
  // pointer wraps to distinguish between a full channel and an empty
  // channel.
  std::atomic<size_t> m_write_ptr;

  // Flags indicater whether the read or writer is waiting.  A flag is
  // set when reading from an empty channel or writing to a full
  // channel.  A flag is tested and cleared when reading from a full
  // channel or writing to an empty channel.  If the flag was set, the
  // wake flag is set on the waiting thread.
  std::atomic<uint_fast8_t> m_wait_flags;
  static const uint_fast8_t wait_flag_reader = (1<<0);
  static const uint_fast8_t wait_flag_writer = (1<<1);

  // A vector holding the elements which have been written to the
  // channel but not read.
  std::vector<uint8_t> m_contents;

  // The context which is allowed to read the channel.
  nanotube_context *m_reader;

  // The context which is allowed to write the channel.
  nanotube_context *m_writer;

  // The type exported for reading. */
  nanotube_channel_type_t m_read_export_type;

  // The type exported for writing. */
  nanotube_channel_type_t m_write_export_type;

  /**
   * Print debug info of data currently accessed
   */
  void print_data_debug(const void* data, size_t data_size, std::string activity) const;
};

///////////////////////////////////////////////////////////////////////////

#endif // NANOTUBE_CHANNEL_HPP
