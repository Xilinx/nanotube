/**************************************************************************\
*//*! \file nanotube_map_taps.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  Interface for Nanotube map taps.
**   \date  2021-06-02
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_MAP_TAPS_H
#define NANOTUBE_MAP_TAPS_H

#include "nanotube_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint16_t nanotube_map_width_t;
typedef uint64_t nanotube_map_depth_t;

typedef enum {
  NANOTUBE_MAP_RESULT_ABSENT,
  NANOTUBE_MAP_RESULT_INSERTED,
  NANOTUBE_MAP_RESULT_REMOVED,
  NANOTUBE_MAP_RESULT_PRESENT,
} nanotube_map_result_t;

// The tap is split into two parts.  The core keeps track of the state
// of the map.  The wrapper arbitrates requests from different clients
// and dispatches responses to the correct client.

///////////////////////////////////////////////////////////////////////////

/*! Allocate the state for a CAM-based map tap.
**
** \param key_length The size of each key in bytes.
**
** \param data_length The size of the associated data in bytes.
**
** \param capacity The maximum number of entries in the map.
*/
uint8_t *
nanotube_tap_map_cam_core_alloc(
  nanotube_map_width_t key_length,
  nanotube_map_width_t data_length,
  nanotube_map_depth_t capacity);

/*! Perform a request on a CAM-based map tap.
**
** \param key_length The size of each key in bytes.
**
** \param data_length The size of the associated data in bytes.
**
** \param capacity The maximum number of entries in the map.
**
** \param data_out The output buffer for the associated data.
**
** \param result_out An output buffer for the result of the operation.
**
** \param map_state The map state, allocated by nanotube_tap_map_cam_core_alloc.
**
** \param key_in The input buffer for the key.
**
** \param data_in The input buffer for the associated data.
**
** \param access The operation to perform.
**
** There should be one call site to the tap per map.  The input and
** output buffer sizes are determined by the key_length and
** data_length parameters.  The parameters should match the ones
** passed to the alloc call.
*/
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
  enum map_access_t access);

///////////////////////////////////////////////////////////////////////////

/*! Allocate the state for a generic map tap.
**
** \param map_type The type of map to create.
**
** \param key_length The size of each key in bytes.
**
** \param data_length The size of the associated data in bytes.
**
** \param capacity The maximum number of entries in the map.
*/
uint8_t *
nanotube_tap_map_core_alloc(
  /* Parameters */
  enum map_type_t      map_type,
  nanotube_map_width_t key_length,
  nanotube_map_width_t data_length,
  nanotube_map_depth_t capacity);

/*! Perform a request on a generic map tap.
**
** \param map_type The type of map.
**
** \param key_length The size of each key in bytes.
**
** \param data_length The size of the associated data in bytes.
**
** \param capacity The maximum number of entries in the map.
**
** \param data_out The output buffer for the associated data.
**
** \param result_out An output buffer for the result of the operation.
**
** \param map_state The map state, allocated by nanotube_tap_map_cam_core_alloc.
**
** \param key_in The input buffer for the key.
**
** \param data_in The input buffer for the associated data.
**
** \param access The operation to perform.
**
** There should be one call site to the tap per map.  The input and
** output buffer sizes are determined by the key_length and
** data_length parameters.  The parameters should match the ones
** passed to the alloc call.
*/
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
  enum map_access_t access);

///////////////////////////////////////////////////////////////////////////

// A type which represents the map server.
typedef struct nanotube_tap_map nanotube_tap_map_t;

/*! Allocate the state for a generic map tap.
**
** \param map_type The type of map to create.
**
** \param key_length The size of each key in bytes.
**
** \param data_length The size of the associated data in bytes.
**
** \param capacity The maximum number of entries in the map.
**
** \param num_clients The number of clients which will be added.
**
** This function is called from the setup function as part of a
** sequence to create a map tap and a number of clients.  The caller
** must first call this function, then call
** nanotube_tap_map_add_client the number of times specified by the
** num_clients parameter and then call nanotube_tap_map_build.
*/
nanotube_tap_map_t *
nanotube_tap_map_create(
  /* Parameters */
  enum map_type_t        map_type,
  nanotube_map_width_t   key_length,
  nanotube_map_width_t   data_length,
  nanotube_map_depth_t   capacity,
  unsigned int           num_clients);

/*! Add a client to a map tap.
**
** \param map The return value from nanotube_tap_map_create.
**
** \param key_in_length The size of the input key buffer.
**
** \param data_in_length The size of the input data buffer.
**
** \param need_result_out A flag indicating whether a result buffer is
** required.
**
** \param data_out_length The size of the output data buffer.
**
** \param req_context The context for the requester.
**
** \param req_channel_id The channel ID to use for the requester.
**
** \param resp_context The context for the responder.
**
** \param resp_channel_id The channel ID to use for the responder.
**
** The clients are used to access the map.  Each client can be
** customized to have the buffer sizes it needs for the operations it
** performs.  The request context identifies the thread which calls
** nanotube_tap_map_send_req and always needs to be supplied.  The
** response context identifies the thread which calls
** nanotube_tap_map_recv_resp and only needs to be supplied if the
** result is needed or the output data buffer size is not zero.
*/
void
nanotube_tap_map_add_client(
  /* Parameters */
  nanotube_tap_map_t    *map,
  nanotube_map_width_t   key_in_length,
  nanotube_map_width_t   data_in_length,
  bool                   need_result_out,
  nanotube_map_width_t   data_out_length,
  nanotube_context_t    *req_context,
  nanotube_channel_id_t  req_channel_id,
  nanotube_context_t    *resp_context,
  nanotube_channel_id_t  resp_channel_id);

/*! Build a map tap.
**
** \param map The map to build.
**
** This function builds a map which has been created after all its
** clients have been added.
*/
void
nanotube_tap_map_build(
  /* Parameters */
  nanotube_tap_map_t *map);

/*! Send a request to a map tap.
**
** \param map The map which will process the request.
**
** \param client_id The index of the client which is sending the
** request.
**
** \param access The type of access to perform.
**
** \param key_in The input key buffer.
**
** \param data_in The input data buffer.
**
** There must only be one static call to this function with a given
** map and client_id.
*/
void
nanotube_tap_map_send_req(
  /* Parameters */
  nanotube_context_t *context,
  nanotube_tap_map_t *map,
  unsigned int client_id,

  enum map_access_t access,
  uint8_t *key_in,
  uint8_t *data_in);

/*! Receive a response from a map tap.
**
** \param map The map which may have produced a response.
**
** \param client_id The index of the client which is receiving the
** response.
**
** \param data_out The output data buffer.
**
** \param result_out The output result buffer.
**
** \returns true if a response was received.
**
** There must only be one static call to this function with a given
** map and client_id.
*/
bool
nanotube_tap_map_recv_resp(
  /* Parameters */
  nanotube_context_t *context,
  nanotube_tap_map_t *map,
  unsigned int client_id,

  uint8_t *data_out,
  nanotube_map_result_t *result_out);

///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif // NANOTUBE_MAP_TAPS_H
