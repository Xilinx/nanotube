/**************************************************************************\
*//*! \file nanotube_context.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Implementation of Nanotube contexts.
**   \date  2020-09-01
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_context.hpp"

#include "nanotube_api.h"
#include "nanotube_channel.hpp"
#include "nanotube_thread.hpp"
#include "processing_system.hpp"

#include <cassert>

///////////////////////////////////////////////////////////////////////////

nanotube_context::nanotube_context():
  m_thread(nullptr)
{
}

nanotube_context::~nanotube_context()
{
  assert(m_thread == nullptr);
}

void nanotube_context::bind_thread(nanotube_thread *thread)
{
  assert(m_thread == nullptr);
  m_thread = thread;
}

void nanotube_context::unbind_thread(nanotube_thread *thread)
{
  assert(m_thread == thread);
  m_thread = nullptr;
}

bool nanotube_context::is_current()
{
  if (m_thread == nullptr)
    return false;
  return m_thread->is_current();
}

bool nanotube_context::is_stopped()
{
  if (m_thread == nullptr)
    return true;
  return m_thread->is_stopped();
}

void nanotube_context::add_channel(nanotube_channel_id_t channel_id,
                                        nanotube_channel_t* channel,
                                        nanotube_channel_flags_t flags)
{
  if (flags & NANOTUBE_CHANNEL_READ) {
    auto res = m_channels.emplace(
      std::make_pair(channel_id, NANOTUBE_CHANNEL_READ), channel);
    if (!res.second)
      throw std::runtime_error("Channel is already registered for"
                               " reading.");
    channel->set_reader(this);
  }

  if (flags & NANOTUBE_CHANNEL_WRITE) {
    auto res = m_channels.emplace(
      std::make_pair(channel_id, NANOTUBE_CHANNEL_WRITE), channel);
    if (!res.second)
      throw std::runtime_error("Channel is already registered for"
                               " writing.");
    channel->set_writer(this);
  }
}

nanotube_channel_t &
nanotube_context::find_channel(nanotube_channel_id_t channel_id,
                                    nanotube_channel_flags_t flags)
{
  auto it = m_channels.find(std::make_pair(channel_id, flags));
  assert (it != m_channels.end());
  return *(it->second);
}

void
nanotube_context::add_map(nanotube_map_id_t id, nanotube_map_t* map) {
  m_maps[id] = map;
}

nanotube_map_t*
nanotube_context::get_map(nanotube_map_id_t id) {
  auto it = m_maps.find(id);
  if( it == m_maps.end() )
    return nullptr;
  return it->second;
}

void nanotube_context::check_thread()
{
  assert(m_thread != nullptr);
  m_thread->check_current();
}

void nanotube_context::wake()
{
  assert(m_thread != nullptr);
  m_thread->wake();
}

///////////////////////////////////////////////////////////////////////////

nanotube_context_t *nanotube_context_create()
{
  auto* ctx = new nanotube_context();
  std::unique_ptr<nanotube_context> context_ptr(ctx);

  processing_system &ps = processing_system::get_current();
  ps.add_context(std::move(context_ptr));
  return ctx;
}

void nanotube_context_add_channel(nanotube_context_t* context,
                                  nanotube_channel_id_t channel_id,
                                  nanotube_channel_t* channel,
                                  nanotube_channel_flags_t flags)
{
  context->add_channel(channel_id, channel, flags);
}

void nanotube_context_add_map(nanotube_context_t* context,
                              nanotube_map_t*     map) {
  context->add_map(map->id, map);
}

///////////////////////////////////////////////////////////////////////////
