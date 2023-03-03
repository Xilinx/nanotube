/*! \file processing_system.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A class representing a Nanotube processing system.
**   \date 2020-08-27
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "processing_system.hpp"

#include "nanotube_api.h"
#include "nanotube_channel.hpp"
#include "nanotube_context.hpp"
#include "nanotube_private.hpp"
#include "nanotube_thread.hpp"

#include <cassert>

typedef std::unique_ptr<func_packet_kernel> func_packet_kernel_ptr_t;

processing_system *processing_system::s_current = nullptr;

void nanotube_setup(void) __attribute__((weak));
void nanotube_setup(void)
{
}

void dummy_ps_client::receive_packet(nanotube_packet_t *packet, nanotube_kernel_rc_t rc)
{
}

processing_system::ptr_t processing_system::attach(ps_client &client)
{
  return ptr_t(new processing_system(client));
}

void processing_system::detach(ptr_t &ptr)
{
  ptr.reset(nullptr);
}

processing_system &processing_system::get_current()
{
  // Enable after fixing NANO-79.
  //assert(s_current != nullptr);
  return *s_current;
}

processing_system::processing_system(ps_client &client):
  m_client(client),
  m_main_thread(&m_main_context)
{
  // Make the processing_system available to any API calls which need
  // it.
  assert(s_current == nullptr);
  s_current = this;

  // Run the setup function.
  nanotube_setup();

  // Make the processing_system unavailable again.
  assert(s_current == this);
  s_current = nullptr;

  // Create the channel kernels, if any.
  make_channel_kernels();

  // Start all the threads now that everything has been created.
  start_threads();
}

void
processing_system::add_plain_packet_kernel(const char* name,
                                           nanotube_kernel_t* kernel,
                                           enum nanotube_bus_id_t bus_type,
                                           int capsules) {
  func_packet_kernel_ptr_t k(new func_packet_kernel(*this, name, kernel, bus_type, capsules));
  m_kernels.emplace_back(std::move(k));
}

void nanotube_add_plain_packet_kernel(const char* name,
                                      nanotube_kernel_t* kernel,
                                      int bus_type_int, int capsules) {
  auto* sys = &processing_system::get_current();
  auto bus_type = (enum nanotube_bus_id_t)bus_type_int;
  sys->add_plain_packet_kernel(name, kernel, bus_type, capsules);
}

void processing_system::start_threads()
{
  // Start all the threads.
  for (thread_ptr_t &thread: m_threads)
    thread->start();
}

void processing_system::make_channel_kernels()
{
  if (m_packet_read_channels.size() == 0 &&
      m_packet_write_channels.size() == 0)
    return;

  assert (m_packet_read_channels.size() == 1 &&
          m_packet_write_channels.size() == 1);

  std::unique_ptr<channel_packet_kernel>
    k(new channel_packet_kernel(*this,
                                *(m_packet_write_channels[0]),
                                *(m_packet_read_channels[0])));
  m_kernels.emplace_back(std::move(k));
}

processing_system::~processing_system()
{
  // Stop all the threads.
  for (thread_ptr_t &thread: m_threads)
    thread->stop();

  // Free allocated memory.
  while (!m_mallocs.empty()) {
    free(m_mallocs.back());
    m_mallocs.pop_back();
  }
}

void
processing_system::read_maps(std::istream &is, std::ostream *debug_out)
{
  /* For now, nanotube_map_read_from checks back with us whether a read map
   * exists, already, or not; so make this available!. */
  assert(s_current == nullptr);
  s_current = this;

  while( true ) {
    nanotube_map* m = nanotube_map_read_from(is);
    if( m == nullptr )
      break;
    if (debug_out != nullptr)
      *debug_out << "  Map " << m->id << '\n';

    auto it = m_id2map.find(m->id);
    if( it != m_id2map.end() ) {
      /* We already had a map with ID in the list of maps.  Ensure that the
       * code updated that, rather than creating a new map! */
      auto& map_uniqp = it->second;
      assert( map_uniqp.get() == m );
    } else {
      /* I wanted to do this with just emplace, but that messes with the
       * vtable, in case the id is already present?!?!  Very strange! */
      m_id2map.emplace(m->id, m);
    }
  }

  assert(s_current == this);
  s_current = nullptr;
}

void processing_system::init_context(nanotube_context_t &ctx)
{
  //XXX: The maps from processing_system are copied to the nanotube_ctx_t
}

void processing_system::dump_maps(std::ostream &os)
{
  /* Pull out map IDs and sort them */
  std::vector<nanotube_map_id_t> ids;
  for( auto& m : m_id2map )
    ids.push_back(m.first);
  std::sort(ids.begin(), ids.end());

  /* Print maps in sorted ID order */
  for( auto id : ids )
    os << m_id2map[id].get();
}

void
processing_system::add_malloc(void *p)
{
  m_mallocs.push_back(p);
}

void
processing_system::add_thread(thread_ptr_t thread)
{
  m_threads.push_back(std::move(thread));
}

void
processing_system::add_context(context_ptr_t context_ptr)
{
  m_contexts.push_back(std::move(context_ptr));
}

void
processing_system::add_channel(const std::string &name,
                               channel_ptr_t channel)
{
  m_channels.emplace(name, std::move(channel));
}

void
processing_system::add_map(map_ptr_t map)
{
  m_id2map.emplace(map->id, std::move(map));
}

void
processing_system::channel_export(nanotube_channel *channel,
                                  nanotube_channel_flags_t flags)
{
  assert(flags == (flags & (NANOTUBE_CHANNEL_READ | NANOTUBE_CHANNEL_WRITE)));

  if ((flags & NANOTUBE_CHANNEL_READ) != 0) {
    channel->set_reader(&m_main_context);
    m_packet_read_channels.push_back(channel);
  }

  if ((flags & NANOTUBE_CHANNEL_WRITE) != 0) {
    channel->set_writer(&m_main_context);
    m_packet_write_channels.push_back(channel);
  }
}
