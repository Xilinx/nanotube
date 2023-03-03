/**************************************************************************\
*//*! \file nanotube_context.hpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Implementation of Nanotube contexts.
**   \date  2020-09-01
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_CONTEXT_HPP
#define NANOTUBE_CONTEXT_HPP

#include "nanotube_api.h"

#include <map>
#include <unordered_map>
#include <utility>

class nanotube_thread;

/*! A Nanotube context for holding resources.
 *
 * A Nanotube context is used for passing resources to a packet kernel
 * or a thread.
 *
 * This class needs to be merged with nanotube_context, but that
 * struct is currently visible to the kernel written in C.
 */
class nanotube_context
{
public:
  nanotube_context();
  ~nanotube_context();

  /*! Bind a thread to a context. */
  void bind_thread(nanotube_thread *thread);

  /*! Unbind a thread from a context. */
  void unbind_thread(nanotube_thread *thread);

  /*! Determine whether the associated thread is the current thread. */
  bool is_current();

  /*! Determine whether the associated thread is stopped. */
  bool is_stopped();

  /*! Add a channel to the context.
   *
   * \param channel_id The ID which will be used to access the channel.
   * \param channel    The channel to add.
   * \param flags      The types of access which need to be supported.
   */
  void add_channel(nanotube_channel_id_t channel_id,
                   nanotube_channel* channel,
                   nanotube_channel_flags_t flags);

  /*! Retrieve a channel from the context.
   *
   * \param channel_id  The ID which was passed to add_channel.
   * \param flags       The type of access which is requested.
   *
   * \return            A reference to the channel.
   *
   * This method raised an error on failure.
   */
  nanotube_channel &find_channel(nanotube_channel_id_t channel_id,
                                 nanotube_channel_flags_t flags);

  /*!
   * Add a map to the context.
   *
   * \param id    Numerical ID of the map to be added.
   * \param map   Pointer to the map that should be added.
   */
  void add_map(nanotube_map_id_t id, nanotube_map_t* map);

  /*!
   * Get the specified map; returns nullptr in case the id was not found.
   * \param id    The ID of the map that should be returned.
   * \return      Pointer to the map.
   */
  nanotube_map_t* get_map(nanotube_map_id_t id);

  /*! Check that this context can be used from the current thread. 
   *
   * This method raises a fatal error if the context cannot be used
   * from the current thread.
   */
  void check_thread();

  /*! Wake up the thread associated with this context. */
  void wake();

  nanotube_thread* get_thread() {return m_thread;}

private:
  typedef std::pair<nanotube_channel_id_t, nanotube_channel_flags_t>
    port_info_t;

  // The thread associated with the context, if any.
  nanotube_thread *m_thread;

  std::map<port_info_t, nanotube_channel_t *>            m_channels;
  std::unordered_map<nanotube_map_id_t, nanotube_map_t*> m_maps;
};

#endif // NANOTUBE_CONTEXT_HPP
