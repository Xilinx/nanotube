 /**************************************************************************\
*//*! \file nanotube_packet_taps_bus.h
** \author  Kieran Mansley <kieranm@amd.com>
**  \brief  Bus selecting wrappers of Nanotube packet taps.
**   \date  2022-02-22
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
** These declarations mirror those in nanotube_packet_taps.h but provide an
** additional parameter bus_type which selects a particular implementation.
** See there for a more detailed decription and documentation of the interface.
**/

#ifndef NANOTUBE_PACKET_TAPS_BUS_H
#define NANOTUBE_PACKET_TAPS_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bus_id.h"

///////////////////////////////////////////////////////////////////////////

void nanotube_tap_packet_length_bus(
  /* Constant parameters */
  nanotube_bus_id_t bus_type,

  /* Outputs. */
  struct nanotube_tap_packet_length_resp *resp_out,

  /* State. */
  struct nanotube_tap_packet_length_state *state_inout,

  /* Inputs. */
  const void *packet_word_in,
  nanotube_tap_offset_t packet_word_len,
  const struct nanotube_tap_packet_length_req *req_in);

///////////////////////////////////////////////////////////////////////////

void nanotube_tap_packet_read_bus(
  /* Constant parameters */
  nanotube_bus_id_t bus_type,
  uint16_t result_buffer_length,
  uint8_t result_buffer_index_bits,

  /* Outputs. */
  struct nanotube_tap_packet_read_resp *resp_out,
  uint8_t *result_buffer_inout,

  /* State. */
  struct nanotube_tap_packet_read_state *state_inout,

  /* Inputs. */
  const void *packet_word_in,
  nanotube_tap_offset_t packet_word_len,
  const struct nanotube_tap_packet_read_req *req_in);

///////////////////////////////////////////////////////////////////////////

void nanotube_tap_packet_write_bus(
  /* Constant parameters */
  nanotube_bus_id_t bus_type,
  uint16_t request_buffer_length,
  uint8_t request_buffer_index_bits,

  /* Outputs. */
  void *packet_word_out,

  /* State. */
  struct nanotube_tap_packet_write_state *state_inout,

  /* Inputs. */
  const void *packet_word_in,
  nanotube_tap_offset_t packet_word_len,
  const struct nanotube_tap_packet_write_req *req_in,
  const uint8_t *request_bytes_in,
  const uint8_t *request_mask_in);

///////////////////////////////////////////////////////////////////////////

void nanotube_tap_packet_resize_ingress_bus(
  /* Constant parameters */
  nanotube_bus_id_t bus_type,

  /* Outputs. */
  bool *packet_done_out,
  nanotube_tap_packet_resize_cword_t *cword_out,
  nanotube_tap_offset_t *packet_length_out,
  
  /* State. */
  nanotube_tap_packet_resize_ingress_state_t *state,

  /* Inputs. */
  nanotube_tap_packet_resize_req_t *resize_req_in,
  void *packet_word_in,
  nanotube_tap_offset_t packet_word_len);

///////////////////////////////////////////////////////////////////////////

void nanotube_tap_packet_resize_egress_bus(
  /* Constant parameters */
  nanotube_bus_id_t bus_type,

  /* Outputs. */
  bool *input_done_out,
  bool *packet_done_out,
  bool *packet_valid_out,
  void *packet_word_out,

  /* State. */
  nanotube_tap_packet_resize_egress_state_t *state,
  void *state_packet_word,

  /* Inputs. */
  nanotube_tap_packet_resize_cword_t *cword,
  void *packet_word_in,
  nanotube_tap_offset_t packet_word_len,
  nanotube_tap_offset_t new_packet_len);

///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif // NANOTUBE_PACKET_TAPS_BUS_H
