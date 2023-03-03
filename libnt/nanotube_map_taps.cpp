/**************************************************************************\
*//*! \file nanotube_map_taps.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  The implementation of Nanotube map taps.
**   \date  2021-06-03
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_map_taps.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

///////////////////////////////////////////////////////////////////////////

namespace {
struct array_map_params
{
  array_map_params(
    nanotube_map_width_t key_length,
    nanotube_map_width_t data_length,
    nanotube_map_depth_t capacity):
    m_key_length(key_length),
    m_data_length(data_length),
    m_capacity(capacity) {
  }

  nanotube_map_width_t elem_size() const {
    return m_data_length;
  }

  uint8_t *data(uint8_t *elem) const { return elem; }

  nanotube_map_width_t m_key_length;
  nanotube_map_width_t m_data_length;
  nanotube_map_depth_t m_capacity;
};
}

uint8_t *
nanotube_tap_map_array_core_alloc(
  nanotube_map_width_t key_length,
  nanotube_map_width_t data_length,
  nanotube_map_depth_t capacity)
#if __clang__
  __attribute__((always_inline))
#endif
{
  array_map_params params(key_length, data_length, capacity);
  return (uint8_t*)nanotube_malloc(capacity*params.elem_size());
}

void
nanotube_tap_map_array_core(
  /* Parameters */
  nanotube_map_width_t key_length,
  nanotube_map_width_t data_length,
  nanotube_map_depth_t capacity,

  /* Outputs */
  uint8_t *data_out,
  nanotube_map_result_t *result_out,

  /* State */
  uint8_t *map_state,

  /* Inputs */
  uint8_t *key_in,
  uint8_t *data_in,
  enum map_access_t access)
#if __clang__
  __attribute__((always_inline))
#endif
{
  array_map_params params(key_length, data_length, capacity);

  nanotube_map_depth_t index = 0;
  for (nanotube_map_width_t i=key_length-1; i>0;) {
    --i;
    index <<= 8;
    index |= key_in[i];
  }

  // NANO-274: the high-level map_op only reads for actual read commands, so
  // one side has to be adjusted.  The line below will change this low-level
  // implementation, but change the behaviour in the high-level map for now.
  //bool do_read   = ( access == NANOTUBE_MAP_READ );
  const bool do_read = true;
  bool do_update = ( access == NANOTUBE_MAP_UPDATE ||
                     access == NANOTUBE_MAP_WRITE );

  // Start with no data.
  memset(data_out, 0, data_length);

  nanotube_map_result_t result = NANOTUBE_MAP_RESULT_ABSENT;
  if (index < params.m_capacity) {
    uint8_t *elem = map_state + index*params.elem_size();
    uint8_t *elem_data = params.data(elem);

    if (do_read) {
      memcpy(data_out, elem_data, data_length);
    }

    if (do_update) {
      memcpy(elem_data, data_in, data_length);
    }
    result = NANOTUBE_MAP_RESULT_PRESENT;
  } else {
    result = NANOTUBE_MAP_RESULT_ABSENT;
  }

  *result_out = result;
}

///////////////////////////////////////////////////////////////////////////

namespace {
struct cam_map_params
{
  cam_map_params(
    nanotube_map_width_t key_length,
    nanotube_map_width_t data_length,
    nanotube_map_depth_t capacity):
    m_key_length(key_length),
    m_data_length(data_length),
    m_capacity(capacity) {
  }

  nanotube_map_width_t elem_size() const {
    return 3 + m_key_length + m_data_length;
  }

  uint8_t *valid(uint8_t *elem) const { return elem+0; }
  uint8_t *match(uint8_t *elem) const { return elem+1; }
  uint8_t *target(uint8_t *elem) const { return elem+2; }
  uint8_t *key(uint8_t *elem) const { return elem+3; }
  uint8_t *data(uint8_t *elem) const { return elem+3+m_key_length; }

  nanotube_map_width_t m_key_length;
  nanotube_map_width_t m_data_length;
  nanotube_map_depth_t m_capacity;
};
}

uint8_t *
nanotube_tap_map_cam_core_alloc(
  nanotube_map_width_t key_length,
  nanotube_map_width_t data_length,
  nanotube_map_depth_t capacity)
#if __clang__
  __attribute__((always_inline))
#endif
{
  cam_map_params params(key_length, data_length, capacity);
  return (uint8_t*)nanotube_malloc(capacity*params.elem_size());
}

void
nanotube_tap_map_cam_core(
  /* Parameters */
  nanotube_map_width_t key_length,
  nanotube_map_width_t data_length,
  nanotube_map_depth_t capacity,

  /* Outputs */
  uint8_t *data_out,
  nanotube_map_result_t *result_out,

  /* State */
  uint8_t *map_state,

  /* Inputs */
  uint8_t *key_in,
  uint8_t *data_in,
  enum map_access_t access)
#if __clang__
  __attribute__((always_inline))
#endif
{
  cam_map_params params(key_length, data_length, capacity);

  // NANO-274: the high-level map_op only reads for actual read commands, so
  // one side has to be adjusted.  The line below will change this low-level
  // implementation, but change the behaviour in the high-level map for now.
  //bool do_read   = ( access == NANOTUBE_MAP_READ );
  const bool do_read = true;
  bool do_insert = ( access == NANOTUBE_MAP_INSERT ||
                     access == NANOTUBE_MAP_WRITE );
  bool do_update = ( access == NANOTUBE_MAP_UPDATE ||
                     access == NANOTUBE_MAP_WRITE );
  bool do_remove = ( access == NANOTUBE_MAP_REMOVE );

  // Start with no data.
  memset(data_out, 0, data_length);

  // Determine which entry, if any, matches the key.
  bool match_any = false;

  // Determine which entry, if any, can host a new entry.
  bool target_any = false;

  for (unsigned i=0; i<capacity; i++) {
    uint8_t *elem = map_state + i*params.elem_size();
    uint8_t *elem_key = params.key(elem);
    bool valid = (*params.valid(elem) & 1) != 0;
    bool match = (valid && memcmp(elem_key, key_in, key_length) == 0);

    *params.match(elem) = match;
    match_any |= match;

    *params.target(elem) = !valid && !target_any;
    target_any |= !valid;
  }

  // Only insert if there was no match.
  bool need_insert = do_insert && !match_any;

  for (unsigned i=0; i<capacity; i++) {
    uint8_t *elem = map_state + i*params.elem_size();
    uint8_t *elem_key = params.key(elem);
    uint8_t *elem_data = params.data(elem);
    bool match = *params.match(elem);
    bool target = *params.target(elem);
    bool read_elem   = (do_read && match);
    bool insert_elem = (need_insert && target);
    bool update_elem = (do_update && match);
    bool remove_elem = (do_remove && match);

    if (read_elem) {
      memcpy(data_out, elem_data, data_length);
    }

    if (insert_elem) {
      *params.valid(elem) = 1;
    }

    if (insert_elem || update_elem) {
      memcpy(elem_key, key_in, key_length);
      memcpy(elem_data, data_in, data_length);
    }

    if (remove_elem) {
      *params.valid(elem) = 0;
    }
  }

  nanotube_map_result_t result = NANOTUBE_MAP_RESULT_ABSENT;
  if (match_any) {
    if (do_remove)
      result = NANOTUBE_MAP_RESULT_REMOVED;
    else
      result = NANOTUBE_MAP_RESULT_PRESENT;
  } else {
    if (do_insert && target_any)
      result = NANOTUBE_MAP_RESULT_INSERTED;
    else
      result = NANOTUBE_MAP_RESULT_ABSENT;
  }
  *result_out = result;
}

///////////////////////////////////////////////////////////////////////////


uint8_t *
nanotube_tap_map_core_alloc(
  /* Parameters */
  enum map_type_t      map_type,
  nanotube_map_width_t key_length,
  nanotube_map_width_t data_length,
  nanotube_map_depth_t capacity)
#if __clang__
  __attribute__((always_inline))
#endif
{
  switch (map_type) {
  case NANOTUBE_MAP_TYPE_HASH:
  case NANOTUBE_MAP_TYPE_LRU_HASH:
    return nanotube_tap_map_cam_core_alloc(
      key_length,
      data_length,
      capacity);

  case NANOTUBE_MAP_TYPE_ARRAY_LE:
    return nanotube_tap_map_array_core_alloc(
      key_length,
      data_length,
      capacity);

  default:
    fprintf(stderr, "Unknown Nanotube map type %d\n", map_type);
    exit(1);
  }
}

void
nanotube_tap_map_core(
  /* Parameters */
  enum map_type_t      map_type,
  nanotube_map_width_t key_length,
  nanotube_map_width_t data_length,
  nanotube_map_depth_t capacity,

  /* Outputs */
  uint8_t *data_out,
  nanotube_map_result_t *result_out,

  /* State */
  uint8_t *map_state,

  /* Inputs */
  uint8_t *key_in,
  uint8_t *data_in,
  enum map_access_t access)
#if __clang__
  __attribute__((always_inline))
#endif
{
  switch (map_type) {
  case NANOTUBE_MAP_TYPE_HASH:
  case NANOTUBE_MAP_TYPE_LRU_HASH:
    return nanotube_tap_map_cam_core(
      key_length,
      data_length,
      capacity,
      data_out,
      result_out,
      map_state,
      key_in,
      data_in,
      access);

  case NANOTUBE_MAP_TYPE_ARRAY_LE:
    return nanotube_tap_map_array_core(
      key_length,
      data_length,
      capacity,
      data_out,
      result_out,
      map_state,
      key_in,
      data_in,
      access);

  default:
    fprintf(stderr, "Unknown Nanotube map type %d\n", map_type);
    exit(1);
  }
}

///////////////////////////////////////////////////////////////////////////

struct nanotube_tap_map_client
{
  nanotube_map_width_t   key_in_length;
  nanotube_map_width_t   data_in_length;
  bool                   need_result_out;
  nanotube_map_width_t   data_out_length;
  nanotube_channel_id_t  req_channel_id;
  nanotube_channel_id_t  resp_channel_id;
  uint8_t               *map_resp_buffer;
  uint8_t               *client_req_buffer;
  uint8_t               *client_resp_buffer;

  size_t req_access_offset() const {
    return 0;
  }
  size_t req_key_offset() const {
    return sizeof(enum map_access_t);
  }
  size_t req_data_offset() const {
    return req_key_offset() + key_in_length;
  }
  size_t req_size() const {
    return req_data_offset() + data_in_length;
  }
  enum map_access_t *req_access(uint8_t *req) const {
    return (enum map_access_t *)(req + req_access_offset());
  }
  uint8_t *req_key(uint8_t *req) const {
    return (req + req_key_offset());
  }
  uint8_t *req_data(uint8_t *req) const {
    return (req + req_data_offset());
  }

  size_t state_flag_offset() const {
    return 0;
  }
  size_t state_req_offset() const {
    return 1;
  }
  size_t state_size() const {
    return state_req_offset() + req_size();
  }
  uint8_t *state_flag(uint8_t *state) const {
    return state + state_flag_offset();
  }
  uint8_t *state_req(uint8_t *state) const {
    return state + state_req_offset();
  }

  size_t resp_result_offset() const {
    return 0;
  }
  size_t resp_data_offset() const {
    return ( need_result_out ? sizeof(nanotube_map_result_t) : 0 );
  }
  size_t resp_size() const {
    return ( resp_data_offset() + data_out_length );
  }
  nanotube_map_result_t *resp_result(uint8_t *resp) const {
    return (nanotube_map_result_t *)(resp + resp_result_offset());
  }
  uint8_t *resp_data(uint8_t *resp) const {
    return resp + resp_data_offset();
  }
};

struct nanotube_tap_map
{
  enum map_type_t          map_type;
  nanotube_map_width_t     key_length;
  nanotube_map_width_t     data_length;
  nanotube_map_depth_t     capacity;
  unsigned int             num_clients;
  nanotube_context_t      *map_context;
  unsigned int             client_index;
  nanotube_tap_map_client *clients;
  uint8_t                 *arb_state;
  uint8_t                 *map_state;
  bool                    *active_buffer;
  uint8_t                 *key_in_buffer;
  uint8_t                 *data_in_buffer;
  uint8_t                 *data_out_buffer;
};

nanotube_tap_map_t *
nanotube_tap_map_create(
  enum map_type_t        map_type,
  nanotube_map_width_t   key_length,
  nanotube_map_width_t   data_length,
  nanotube_map_depth_t   capacity,
  unsigned int           num_clients)
#if __clang__
  __attribute__((always_inline))
#endif
{
  nanotube_tap_map_t *result;
  size_t result_size = sizeof(*result);
  result = (nanotube_tap_map_t *)nanotube_malloc(result_size);
  memset(result, 0, result_size);

  nanotube_tap_map_client *clients;
  size_t clients_size = num_clients*sizeof(*clients);
  clients = (nanotube_tap_map_client*)nanotube_malloc(clients_size);
  memset(clients, 0, clients_size);

  void *active_buffer = nanotube_malloc(num_clients*sizeof(bool));
  void *key_in_buffer = nanotube_malloc(key_length*sizeof(uint8_t));
  void *data_in_buffer = nanotube_malloc(data_length*sizeof(uint8_t));
  void *data_out_buffer = nanotube_malloc(data_length*sizeof(uint8_t));

  auto map_state = nanotube_tap_map_core_alloc(
    map_type,
    key_length,
    data_length,
    capacity);

  result->map_type = map_type;
  result->key_length = key_length;
  result->data_length = data_length;
  result->capacity = capacity;
  result->num_clients = num_clients;
  result->map_context = nanotube_context_create();
  result->client_index = 0;
  result->clients = clients;
  result->arb_state = nullptr;
  result->map_state = map_state;
  result->active_buffer = (bool*)active_buffer;
  result->key_in_buffer = (uint8_t*)key_in_buffer;
  result->data_in_buffer = (uint8_t*)data_in_buffer;
  result->data_out_buffer = (uint8_t*)data_out_buffer;

  return result;
}

void
nanotube_tap_map_add_client(
  nanotube_tap_map_t    *map,
  nanotube_map_width_t   key_in_length,
  nanotube_map_width_t   data_in_length,
  bool                   need_result_out,
  nanotube_map_width_t   data_out_length,
  nanotube_context_t    *req_context,
  nanotube_channel_id_t  req_channel_id,
  nanotube_context_t    *resp_context,
  nanotube_channel_id_t  resp_channel_id)
#if __clang__
  __attribute__((always_inline))
#endif
{
  static const size_t req_count = 8;
  static const size_t resp_count = 16;

  assert(map->client_index < map->num_clients);

  nanotube_tap_map_client *c_info = map->clients + map->client_index;
  c_info->key_in_length = key_in_length;
  c_info->data_in_length = data_in_length;
  c_info->need_result_out = need_result_out;
  c_info->data_out_length = data_out_length;
  c_info->req_channel_id = req_channel_id;
  c_info->resp_channel_id = resp_channel_id;
  c_info->map_resp_buffer = (uint8_t*)nanotube_malloc(c_info->resp_size());
  c_info->client_req_buffer = (uint8_t*)nanotube_malloc(c_info->req_size());
  c_info->client_resp_buffer = (uint8_t*)nanotube_malloc(c_info->resp_size());

  nanotube_context_t *map_context = map->map_context;
  size_t req_size = c_info->req_size();
  size_t resp_size = c_info->resp_size();

  auto req_channel = nanotube_channel_create("map_req", req_size,
                                             req_count);
  nanotube_context_add_channel(req_context, req_channel_id,
                               req_channel, NANOTUBE_CHANNEL_WRITE);
  nanotube_context_add_channel(map_context, 2*map->client_index+0,
                               req_channel, NANOTUBE_CHANNEL_READ);

  if (resp_size != 0) {
    auto resp_channel = nanotube_channel_create("map_resp", resp_size,
                                                resp_count);
    nanotube_context_add_channel(map_context, 2*map->client_index+1,
                                 resp_channel, NANOTUBE_CHANNEL_WRITE);
    nanotube_context_add_channel(resp_context, resp_channel_id,
                                 resp_channel, NANOTUBE_CHANNEL_READ);
  }

  map->client_index++;
}

void
nanotube_tap_map_func(nanotube_context_t *context, void *data)
{
  nanotube_tap_map_t *map = (nanotube_tap_map_t*)data;
  auto key_length = map->key_length;
  auto data_length = map->data_length;
  unsigned int num_clients = map->num_clients;

  // Determine whether there are any requests waiting.
  uint8_t *client_state = map->arb_state;
  bool any_ready = false;
  for (unsigned int i=0; i<num_clients; i++) {
    const nanotube_tap_map_client *c_info = map->clients + i;
    uint8_t *c_flag = c_info->state_flag(client_state);
    if (*c_flag != 0)
      any_ready = true;
    client_state += c_info->state_size();
  }  

  bool *active = map->active_buffer;
  enum map_access_t access = NANOTUBE_MAP_NOP;
  uint8_t *key_in = map->key_in_buffer;
  uint8_t *data_in = map->data_in_buffer;
  bool accepted = false;
  bool any_succ = false;

  memset(active, 0, num_clients*sizeof(bool));
  memset(key_in, 0, key_length);
  memset(data_in, 0, data_length);

  client_state = map->arb_state;
  for (unsigned int i=0; i<num_clients; i++) {
    const nanotube_tap_map_client *c_info = map->clients + i;
    size_t req_size = c_info->req_size();
    uint8_t *c_flag = c_info->state_flag(client_state);
    uint8_t *c_req = c_info->state_req(client_state);
    enum map_access_t *c_access = c_info->req_access(c_req);
    uint8_t *c_key = c_info->req_key(c_req);
    uint8_t *c_data = c_info->req_data(c_req);
    client_state += c_info->state_size();

    if (!any_ready) {
      int rc = nanotube_channel_try_read(context, 2*i+0, c_req,
                                         req_size);
      *c_flag = (rc != 0);
      any_succ |= (rc != 0);
    } else {
      any_succ = true;
    }

    if (*c_flag != 0 && !accepted) {
      *c_flag = 0;
      active[i] = true;
      accepted = true;
      access = map_access_t(*c_access);

      if (map->key_length <= c_info->key_in_length) {
        memcpy(key_in, c_key, map->key_length);
      } else {
        memcpy(key_in, c_key, c_info->key_in_length);
        memset(key_in + c_info->key_in_length, 0,
               map->key_length - c_info->key_in_length);
      }

      if (map->data_length <= c_info->data_in_length) {
        memcpy(data_in, c_data, map->data_length);
      } else {
        memcpy(data_in, c_data, c_info->data_in_length);
        memset(data_in + c_info->data_in_length, 0,
               map->data_length - c_info->data_in_length);
      }
    }
  }

  if (!any_succ) {
    nanotube_thread_wait();
    return;
  }

  uint8_t *data_out = map->data_out_buffer;
  nanotube_map_result_t result_out;

  memset(data_out, 0, data_length);

  nanotube_tap_map_core(
    map->map_type,
    map->key_length,
    map->data_length,
    map->capacity,
    data_out,
    &result_out,
    map->map_state,
    key_in,
    data_in,
    access);

  for (unsigned int i=0; i<num_clients; i++) {
    const nanotube_tap_map_client *c_info = map->clients + i;
    size_t resp_size = c_info->resp_size();
    client_state += c_info->state_size();

    if (!active[i])
      continue;

    uint8_t *resp_out = c_info->map_resp_buffer;
    memset(resp_out, 0, resp_size);

    nanotube_map_result_t *r_result = c_info->resp_result(resp_out);
    uint8_t *r_data = c_info->resp_data(resp_out);
    if (c_info->need_result_out) {
      *r_result = result_out;
    }
    if (c_info->data_out_length <= map->data_length) {
      memcpy(r_data, data_out, c_info->data_out_length);
    } else {
      memcpy(r_data, data_out, map->data_length);
      memset(r_data + map->data_length, 0,
             c_info->data_out_length - map->data_length);
    }

    nanotube_channel_write(context, 2*i+1, resp_out, resp_size);
  }
}

void
nanotube_tap_map_build(nanotube_tap_map_t *map)
#if __clang__
  __attribute__((always_inline))
#endif
{
  /* We need the right number of clients added in the thread function when
   * polling channels and elsewhere; so ensure enough clients were added */
  if (map->client_index != map->num_clients) {
    fprintf(stderr, "Map (key_sz: %i value_sz: %i) declared wth %i clients, "
            "but instead %i were added with nanotube_tap_map_add_client(). "
            "Aborting.\n", map->key_length, map->data_length,
            map->num_clients, map->client_index);
    exit(1);
  }

  // Do not create a thread if there are no clients.  This is to avoid
  // creating a thread with no ports.
  if (map->num_clients == 0)
    return;

  size_t arb_state_size = 0;
  for (unsigned int i=0; i<map->num_clients; i++) {
    const nanotube_tap_map_client *c_info = map->clients + i;
    arb_state_size += c_info->state_size();
  }

  uint8_t *arb_state = (uint8_t*)nanotube_malloc(arb_state_size);
  memset(arb_state, 0, arb_state_size);
  map->arb_state = arb_state;

  nanotube_thread_create(map->map_context,
                         "map_tap",
                         nanotube_tap_map_func,
                         (void*)map,
                         sizeof(*map));
}

void
nanotube_tap_map_send_req(
  /* Parameters */
  nanotube_context_t *context,
  nanotube_tap_map_t *map,
  unsigned int client_id,

  /* Inputs */
  enum map_access_t access,
  uint8_t *key_in,
  uint8_t *data_in)
#if __clang__
  __attribute__((always_inline))
#endif
{
  assert(client_id < map->num_clients);
  nanotube_tap_map_client *c_info = map->clients + client_id;
  size_t req_size = c_info->req_size();

  uint8_t *req = c_info->client_req_buffer;
  *(c_info->req_access(req)) = access;
  memcpy(c_info->req_key(req), key_in, c_info->key_in_length);
  memcpy(c_info->req_data(req), data_in, c_info->data_in_length);
  nanotube_channel_write(context, c_info->req_channel_id,
                         req, req_size);
}

bool
nanotube_tap_map_recv_resp(
  /* Parameters */
  nanotube_context_t *context,
  nanotube_tap_map_t *map,
  unsigned int client_id,

  uint8_t *data_out,
  nanotube_map_result_t *result_out)
#if __clang__
  __attribute__((always_inline))
#endif
{
  assert(client_id < map->num_clients);
  nanotube_tap_map_client *c_info = map->clients + client_id;
  size_t resp_size = c_info->resp_size();

  uint8_t *resp = c_info->client_resp_buffer;
  if (!nanotube_channel_try_read(context,
                                 c_info->resp_channel_id,
                                 resp, resp_size)) {
    return false;
  }

  if (c_info->need_result_out)
    *result_out = *(c_info->resp_result(resp));
  memcpy(data_out, c_info->resp_data(resp), c_info->data_out_length);
  return true;
}

///////////////////////////////////////////////////////////////////////////
