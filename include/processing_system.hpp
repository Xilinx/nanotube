/*******************************************************/
/*! \file processing_system.hpp
** \author Neil Turton <neilt@amd.com>
**  \brief A class representing a Nanotube processing system.
**   \date 2020-08-27
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef PROCESSING_SYSTEM_HPP
#define PROCESSING_SYSTEM_HPP

#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <vector>

#include "nanotube_api.h"
#include "nanotube_context.hpp"
#include "nanotube_map.hpp"
#include "nanotube_thread.hpp"
#include "packet_kernel.hpp"

class nanotube_thread;

class ps_client
{
public:
  // Process a received packet.  The packet can be modified, but
  // ownership is not transferred.
  virtual void receive_packet(nanotube_packet_t *packet, nanotube_kernel_rc_t rc) = 0;
};

class dummy_ps_client: public ps_client
{
public:
  // Process a received packet.  The packet can be modified, but
  // ownership is not transferred.
  void receive_packet(nanotube_packet_t *packet, nanotube_kernel_rc_t rc);
};

class processing_system
{
public:
  typedef std::unique_ptr<processing_system> ptr_t;
  typedef std::unique_ptr<nanotube_context> context_ptr_t;
  typedef std::vector<context_ptr_t>        context_vec_t;
  typedef std::unique_ptr<nanotube_channel> channel_ptr_t;
  typedef std::unique_ptr<packet_kernel>    packet_kernel_ptr_t;
  typedef std::vector<packet_kernel_ptr_t>  packet_kernel_vec_t;
  typedef std::unique_ptr<nanotube_thread>  thread_ptr_t;
  typedef std::vector<thread_ptr_t>         thread_vec_t;
  typedef std::unique_ptr<nanotube_map>                    map_ptr_t;
  typedef std::unordered_map<nanotube_map_id_t, map_ptr_t> id_to_map_t;

  static ptr_t attach(ps_client &client);
  static void detach(ptr_t &ptr);

  static processing_system &get_current();

  ~processing_system();

  const packet_kernel_vec_t& kernels() const { return m_kernels; }
  const thread_vec_t&        threads() const { return m_threads; }
  const context_vec_t&  contexts() const { return m_contexts; }
  const id_to_map_t&         maps() const { return m_id2map; }

  nanotube_context *get_main_context() { return &m_main_context; }
  nanotube_thread *get_main_thread() { return &m_main_thread; }
  void read_maps(std::istream &is, std::ostream *debug_out = nullptr);
  void init_context(nanotube_context_t &ctx);
  void dump_maps(std::ostream &os);
  void add_malloc(void *p);
  void add_thread(thread_ptr_t thread);
  void add_context(context_ptr_t context_ptr);
  void add_map(map_ptr_t map);
  void add_channel(const std::string &name, channel_ptr_t channel);
  void channel_export(nanotube_channel *channel,
                      nanotube_channel_flags_t flags);
  void add_plain_packet_kernel(const char* name, nanotube_kernel_t* kernel,
                               enum nanotube_bus_id_t bus_type,
                               int capsules);

  // Process a received packet.  The packet may be modified but
  // ownership is not transferred.
  void receive_packet(nanotube_packet_t *packet, nanotube_kernel_rc_t rc) {
    m_client.receive_packet(packet, rc);
  }

private:
  processing_system(ps_client &client);

  void start_threads();

  void make_channel_kernels();

  static processing_system *s_current;

  ps_client &m_client;

  packet_kernel_vec_t m_kernels;

  id_to_map_t m_id2map;

  nanotube_context m_main_context;
  nanotube_thread m_main_thread;

  context_vec_t m_contexts;

  std::vector<void *> m_mallocs;

  std::multimap<std::string, channel_ptr_t> m_channels;

  std::vector<nanotube_channel *> m_packet_read_channels;
  std::vector<nanotube_channel *> m_packet_write_channels;
  thread_vec_t m_threads;
};

#endif // PROCESSING_SYSTEM_HPP
