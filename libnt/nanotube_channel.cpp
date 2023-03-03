/**************************************************************************\
*//*! \file nanotube_channel.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Implementation of Nanotube channels.
**   \date  2020-09-01
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_channel.hpp"

#include "nanotube_api.h"
#include "nanotube_context.hpp"
#include "nanotube_thread.hpp"
#include "processing_system.hpp"

#include <atomic>
#include <iostream>
#include <cassert>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>

//#define CHANNEL_DATA_DEBUG

///////////////////////////////////////////////////////////////////////////

nanotube_channel::nanotube_channel(const std::string &name,
                                   size_t elem_size,
                                   size_t num_elem):
  m_name(name),
  m_elem_size(elem_size),
  m_sideband_size(0),
  m_sideband_signals_size(0),
  m_read_ptr(0),
  m_write_ptr(0),
  m_wait_flags(0),
  m_contents(elem_size*num_elem, 0),
  m_reader(nullptr),
  m_writer(nullptr),
  m_read_export_type(NANOTUBE_CHANNEL_TYPE_NONE),
  m_write_export_type(NANOTUBE_CHANNEL_TYPE_NONE)
{
}

void nanotube_channel::set_reader(nanotube_context *context)
{
  if (m_reader != nullptr) {
    assert(m_reader->is_current() || m_reader->is_stopped());
  }
  if (context != nullptr) {
    assert(context->is_current() || context->is_stopped());
  }
  m_reader = context;
}

void nanotube_channel::set_writer(nanotube_context *context)
{
  if (m_writer != nullptr) {
    assert(m_writer->is_current() || m_writer->is_stopped());
  }
  if (context != nullptr) {
    assert(context->is_current() || context->is_stopped());
  }
  m_writer = context;
}

void nanotube_channel::set_export_info(nanotube_channel_type_t type,
                                       nanotube_channel_flags_t flags)
{
  assert(type != NANOTUBE_CHANNEL_TYPE_NONE);
  assert(flags == (flags & (NANOTUBE_CHANNEL_READ | NANOTUBE_CHANNEL_WRITE)));

  if ((flags & NANOTUBE_CHANNEL_READ) != 0) {
    assert(m_read_export_type == NANOTUBE_CHANNEL_TYPE_NONE);
    m_read_export_type = type;
  }

  if ((flags & NANOTUBE_CHANNEL_WRITE) != 0) {
    assert(m_write_export_type == NANOTUBE_CHANNEL_TYPE_NONE);
    m_write_export_type = type;
  }
}

void nanotube_channel::set_sideband_size(size_t sideband_size)
{
  assert(sideband_size < m_elem_size);
  m_sideband_size = sideband_size;
}

void nanotube_channel::set_sideband_signals_size(size_t sideband_signals)
{
  assert(sideband_signals < m_elem_size);
  m_sideband_signals_size = sideband_signals;
}

bool nanotube_channel::try_read(void* data, size_t data_size)
{
  assert(m_elem_size == data_size);

  // Make sure we are executing in the reader thread.
  assert(m_reader != nullptr);
  m_reader->check_thread();

  // Set the read waiter flag and exit if the channel is empty.  The
  // load of m_read_ptr is relaxed because that location is only
  // written by this thread.  If the write pointer is not equal to the
  // read pointer then the next element can be safely accessed.
  // Otherwise, we need to set wait_flag_reader in m_wait_flags in a
  // race-free way.
  size_t read_ptr = m_read_ptr.load(std::memory_order_relaxed);
  if (read_ptr == m_write_ptr.load()) {
    // Clear the buffer.
    memset(data, 0, data_size);

    // Do not modify m_wait_flags if the flag is already set.  In this
    // case, the race condition has already been handled.
    if (m_wait_flags.load() & wait_flag_reader)
      return false;

    // Indicate that the reader is waiting.  This read-modify-write of
    // m_wait_flags and the following load from m_write_ptr are both
    // seq_cst operations which means they are both present in the
    // total order of seq_cst operations.  This is required to break a
    // race condition between the reader going to sleep and the writer
    // producing data.
    //
    // If the load from m_write_ptr here does not see the
    // corresponding store in try_write() then in the total order:
    //   The update of m_wait_flags here is ordered before
    //   the load from m_write_ptr below which is ordered before
    //   the store to m_write_ptr in try_write() which is ordered before
    //   the load from m_wait_flags in try_write()
    //
    // As a result, the load of m_wait_flags in try_write() will see
    // the update of m_wait_flags here.  That will break the race
    // condition.
    m_wait_flags.fetch_or(wait_flag_reader);

    // Double check the write pointer in case it got written between
    // the previous load and setting wait_flag_reader.
    if (read_ptr == m_write_ptr.load()) {
      return false;
    }
  }

  // There is an element in the channel.  Copy it out.
  size_t offset = read_ptr & ptr_mask;
  memcpy(data, &(m_contents[offset]), data_size);

#ifdef CHANNEL_DATA_DEBUG
  print_data_debug(data, data_size, "read");
#endif

  // Update the read pointer.
  if (data_size < m_contents.size() - offset) {
    // If there is enough space without wrapping, increment the low
    // bits.
    read_ptr += data_size;
  } else {
    // We need to wrap the pointer.  Flip the top bit and clear the
    // offset bits.
    read_ptr = (~read_ptr) & wrap_bit;
  }

  // Update the read pointer as a seq_cst operation.  See below.
  m_read_ptr.store(read_ptr);

  // Clear the wait flags.  The reader wait flag can be cleared
  // because this thread successfully read an element.  The writer
  // wait flag can be cleared and the writer woken since there is now
  // space in the channel.
  //
  // This operation and the previous store to m_read_ptr are both
  // seq_cst operations which means they are both present in the total
  // order between seq_cst operations.  This is required to break a
  // race condition between the reader freeing space and the writer
  // going to sleep.
  //
  // If the load from m_wait_flags here does not see the corresponding
  // update in has_space() then:
  //   The store to m_read_ptr above is ordered before
  //   the load of m_wait_flags here which is ordered before
  //   the update of m_wait_flags in has_space() which is ordered before
  //   the load of m_read_ptr in has_space()
  //
  // As a result, the load of m_read_ptr in has_space() will see the
  // update of m_read_ptr above.  That will break the race condition.
  if (m_wait_flags.load() != 0) {
    // Fetch and clear the flags.
    uint_fast8_t flags = m_wait_flags.exchange(0);
    // Wake the writer if necessary.
    if (flags & wait_flag_writer)
      m_writer->wake();
  }

  return true;
}

bool nanotube_channel::has_space()
{
  // Make sure we are executing in the writer thread.
  assert(m_writer != nullptr);
  m_writer->check_thread();

  // Set the write waiter flag and exit if the channel is full.  The
  // load from m_write_ptr is relaxed because that location is only
  // written by this thread.  If the read pointer does not differ from
  // the write pointer by the size of the ring then the next element
  // can be safely written.  Otherwise we need to set wait_flag_writer
  // in m_wait_flags in a race-free way.
  size_t write_ptr = m_write_ptr.load(std::memory_order_relaxed);
  size_t full_read_ptr = write_ptr ^ wrap_bit;
  if (full_read_ptr == m_read_ptr.load()) {
    // Avoid modifying m_wait_flags if the writer wait flag is already
    // set.  In this case, the race condition has already been
    // handled.
    if (m_wait_flags.load() & wait_flag_writer)
      return false;

    // Indicate that the writer is waiting.  This read-modify-write of
    // m_wait_flags and the following load from m_read_ptr are both
    // seq_cst operations which means they are both present in the
    // total order of seq_cst operations.  This is required to break a
    // race condition between the writer going to sleep and the reader
    // making more space available.
    //
    // If the load from m_read_ptr here does not see the corresponding
    // store in try_read() then in the total order:
    //   The update of m_wait_flags here is ordered before
    //   the load from m_read_ptr below which is ordered before
    //   the store to m_read_ptr in try_read() which is ordered before
    //   the load from m_wait_flags in try_read()
    //
    // As a result, the load of m_wait_flags in try_read() will see
    // the update of m_wait_flags here.  That will break the race
    // condition.
    m_wait_flags.fetch_or(wait_flag_writer);

    // Double check the read pointer in case it got written between
    // the previous load and setting wait_flag_writer.
    if (full_read_ptr == m_read_ptr.load()) {
      return false;
    }
  }

  return true;
}

bool nanotube_channel::try_write(const void* data, size_t data_size)
{
  assert(m_elem_size == data_size);

  // Return failure if there is no space.
  if (!has_space())
    return false;

#ifdef CHANNEL_DATA_DEBUG
  print_data_debug(data, data_size, "write");
#endif

  // Write the data into the channel.  This load fro m_write_ptr is
  // relaxed because that location is only written by this thread.
  size_t write_ptr = m_write_ptr.load(std::memory_order_relaxed);
  size_t offset = write_ptr & ptr_mask;
  memcpy(&(m_contents[offset]), data, data_size);

  // Update the write pointer.
  if (data_size < m_contents.size() - offset) {
    // If there is enough space without wrapping, increment the low
    // bits.
    write_ptr += data_size;
  } else {
    // We need to wrap the pointer.  Flip the top bit and clear the
    // offset bits.
    write_ptr = (~write_ptr) & wrap_bit;
  }

  // Update the write pointer as a seq_cst operation.  See below.
  m_write_ptr.store(write_ptr);

  // Clear the wait flags.  The writer wait flag can be cleared
  // because this thread successfully wrote an element.  The reader
  // wait flag can be cleared and the reader woken since there is now
  // an element in the channel.
  //
  // This operation and the previous store to m_write_ptr are both
  // seq_cst operations which means they are both present in the total
  // order of seq_cst operations.  This is required to break a race
  // condition between the writer producinng data and the reader going
  // to sleep.
  //
  // If the load from m_wait_flags here does not see the corresponding
  // update in try_read() then in the total order:
  //   The store to m_write_ptr above is ordered before
  //   the load of m_wait_flags here which is ordered before
  //   the update of m_wait_flags in try_read() which is ordered before
  //   the load of m_write_ptr in try_read()
  //
  // As a result, the load of m_write_ptr in try_read() will see the
  // update of m_write_ptr above.  That will break the race condition.
  if (m_wait_flags.load() != 0) {
    // Fetch and clear the flags.
    uint_fast8_t flags = m_wait_flags.exchange(0);
    // Wake the reader if necessary.
    if (flags & wait_flag_reader)
      m_reader->wake();
  }

  return true;
}

void
nanotube_channel::print_data_debug(const void* data, size_t data_size, std::string activity) const {
  nanotube_stderr_guard guard;
  std::cerr << "Channel " << m_name << " (" << m_writer->get_thread()->get_name()
            << " => " << m_reader->get_thread()->get_name()
            << ") " << activity << " bytes:\n  ";
  for( unsigned n = 0; n < data_size; n++ ) {
    fprintf(stderr, "%02x ", ((uint8_t*)data)[n]);
    if( (n & 15) == 15 )
      std::cerr << "\n  ";
  }
  std::cerr << '\n';
}

///////////////////////////////////////////////////////////////////////////

nanotube_channel_t*
nanotube_channel_create(const char* name,
                        size_t elem_size,
                        size_t num_elem)
{
  nanotube_channel::ptr_t
    channel_ptr{ new nanotube_channel(name, elem_size, num_elem) };

  processing_system &ps = processing_system::get_current();
  nanotube_channel *channel = channel_ptr.get();
  ps.add_channel(name, std::move(channel_ptr));
  return channel;
}

int32_t
nanotube_channel_set_attr(nanotube_channel_t *channel,
                          nanotube_channel_attr_id_t attr_id,
                          int32_t attr_val)
{
  switch(attr_id) {
    case NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES:
      channel->set_sideband_size((size_t)attr_val);
      break;
    case NANOTUBE_CHANNEL_ATTR_SIDEBAND_SIGNALS:
      channel->set_sideband_signals_size((size_t)attr_val);
      break;
    default:
      return -EINVAL;
  }
  return 0;
}

void
nanotube_channel_export(nanotube_channel_t *channel,
                        nanotube_channel_type_t type,
                        nanotube_channel_flags_t flags)
{
  processing_system &ps = processing_system::get_current();
  channel->set_export_info(type, flags);
  ps.channel_export(channel, flags);
}

void nanotube_channel_read(nanotube_context_t* context,
                           nanotube_channel_id_t channel_id,
                           void* data,
                           size_t data_size)
{
  while (!nanotube_channel_try_read(context, channel_id, data, data_size))
    nanotube_thread_wait();
}

int nanotube_channel_try_read(nanotube_context_t* context,
                              nanotube_channel_id_t channel_id,
                              void* data,
                              size_t data_size)
{
  nanotube_channel &channel =
    context->find_channel(channel_id, NANOTUBE_CHANNEL_READ);

  return channel.try_read(data, data_size);
}

void nanotube_channel_write(nanotube_context_t* context,
                            nanotube_channel_id_t channel_id,
                            const void* data,
                            size_t data_size)
{
  nanotube_channel &channel =
    context->find_channel(channel_id, NANOTUBE_CHANNEL_WRITE);

  while(!channel.try_write(data, data_size))
    nanotube_thread_wait();
}

int nanotube_channel_has_space(nanotube_context_t* context,
                               nanotube_channel_id_t channel_id)
{
  nanotube_channel &channel =
    context->find_channel(channel_id, NANOTUBE_CHANNEL_WRITE);

  return channel.has_space();
}

///////////////////////////////////////////////////////////////////////////
