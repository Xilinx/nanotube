/**************************************************************************\
*//*! \file nanotube_packet_taps_bus.cpp
** \author  Kieran Mansley <kieranm@amd.com>
**  \brief  Bus selecting wrappers of Nanotube packet taps.
**   \date  2022-02-22
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_sb.h"
#include "nanotube_packet_taps_shb.h"
#include "nanotube_packet_taps_x3rx.h"
#include "nanotube_packet_taps_bus.h"

///////////////////////////////////////////////////////////////////////////

//Wrapper for nanotube_tap_packet_length_core() that selects the bus to use

void nanotube_tap_packet_length_bus(
  /* Which bus implementation to use */
  const enum nanotube_bus_id_t bus,

  /* Outputs. */
  struct nanotube_tap_packet_length_resp *resp_out,

  /* State. */
  struct nanotube_tap_packet_length_state *state_inout,

  /* Inputs. */
  const uint8_t *packet_word_in,
  nanotube_tap_offset_t packet_word_len,
  const struct nanotube_tap_packet_length_req *req_in)
#if __clang__
  __attribute__((always_inline))
#endif
{
  switch (bus) {
      case NANOTUBE_BUS_ID_SB:
        assert(packet_word_len == simple_bus::total_bytes);
        return nanotube_tap_packet_length_sb(resp_out, state_inout,
                                             packet_word_in,
                                             req_in);
        break;
      case NANOTUBE_BUS_ID_SHB:
        assert(packet_word_len == softhub_bus::total_bytes);
        return nanotube_tap_packet_length_shb(resp_out, state_inout,
                                              packet_word_in,
                                              req_in);
        break;
      case NANOTUBE_BUS_ID_X3RX:
        assert(packet_word_len == x3rx_bus::total_bytes);
        return nanotube_tap_packet_length_x3rx(resp_out, state_inout,
                                              packet_word_in,
                                              req_in);
        break;
      default:
        assert(false);
  }
  return;
}

///////////////////////////////////////////////////////////////////////////

//Wrapper for nanotube_tap_packet_read_core() that selects the bus to use

void nanotube_tap_packet_read_bus(
  /* Which bus implementation to use */
  const enum nanotube_bus_id_t bus,

  /* Constant parameters */
  uint16_t result_buffer_length,
  uint8_t result_buffer_index_bits,

  /* Outputs. */
  struct nanotube_tap_packet_read_resp *resp_out,
  uint8_t *result_buffer_inout,

  /* State. */
  struct nanotube_tap_packet_read_state *state_inout,

  /* Inputs. */
  const uint8_t *packet_word_in,
  nanotube_tap_offset_t packet_word_len,
  const struct nanotube_tap_packet_read_req *req_in)
#if __clang__
  __attribute__((always_inline))
#endif
{
  switch (bus) {
      case NANOTUBE_BUS_ID_SB:
        assert(packet_word_len == simple_bus::total_bytes);
        return nanotube_tap_packet_read_sb(result_buffer_length,
                                           result_buffer_index_bits, resp_out,
                                           result_buffer_inout, state_inout,
                                           packet_word_in,
                                           req_in);
        break;
      case NANOTUBE_BUS_ID_SHB:
        assert(packet_word_len == softhub_bus::total_bytes);
        return nanotube_tap_packet_read_shb(result_buffer_length,
                                            result_buffer_index_bits, resp_out,
                                            result_buffer_inout, state_inout,
                                            packet_word_in,
                                            req_in);
        break;
      case NANOTUBE_BUS_ID_X3RX:
        assert(packet_word_len == x3rx_bus::total_bytes);
        return nanotube_tap_packet_read_x3rx(result_buffer_length,
                                            result_buffer_index_bits, resp_out,
                                            result_buffer_inout, state_inout,
                                            packet_word_in,
                                            req_in);
        break;
      default:
        assert(false);
  }
  return;
}

///////////////////////////////////////////////////////////////////////////

//Wrapper for nanotube_tap_packet_write_core() that selects the bus to use

void nanotube_tap_packet_write_bus(
  /* Which bus implementation to use */
  const enum nanotube_bus_id_t bus,

  /* Constant parameters */
  uint16_t request_buffer_length,
  uint8_t request_buffer_index_bits,

  /* Outputs. */
  uint8_t *packet_word_out,

  /* State. */
  struct nanotube_tap_packet_write_state *state_inout,

  /* Inputs. */
  const uint8_t *packet_word_in,
  nanotube_tap_offset_t packet_word_len,
  const struct nanotube_tap_packet_write_req *req_in,
  const uint8_t *request_bytes_in,
  const uint8_t *request_mask_in)
#if __clang__
  __attribute__((always_inline))
#endif
{
  switch (bus) {
      case NANOTUBE_BUS_ID_SB:
        assert(packet_word_len == simple_bus::total_bytes);
        return nanotube_tap_packet_write_sb(request_buffer_length,
                                            request_buffer_index_bits,
                                            packet_word_out, state_inout,
                                            packet_word_in,
                                            req_in, request_bytes_in,
                                            request_mask_in);
        break;
      case NANOTUBE_BUS_ID_SHB:
        assert(packet_word_len == softhub_bus::total_bytes);
        return nanotube_tap_packet_write_shb(request_buffer_length,
                                             request_buffer_index_bits,
                                             packet_word_out, state_inout,
                                             packet_word_in,
                                             req_in, request_bytes_in,
                                             request_mask_in);
        break;
      case NANOTUBE_BUS_ID_X3RX:
        assert(packet_word_len == x3rx_bus::total_bytes);
        return nanotube_tap_packet_write_x3rx(request_buffer_length,
                                             request_buffer_index_bits,
                                             packet_word_out, state_inout,
                                             packet_word_in,
                                             req_in, request_bytes_in,
                                             request_mask_in);
        break;
      default:
        assert(false);
  }
  return;
}

///////////////////////////////////////////////////////////////////////////

//Wrapper for nanotube_tap_packet_resize_ingress_core() that selects the bus to use

void nanotube_tap_packet_resize_ingress_bus(
  /* Which bus implementation to use */
  const enum nanotube_bus_id_t bus,

  /* Outputs. */
  bool *packet_done_out,
  nanotube_tap_packet_resize_cword_t *cword_out,
  nanotube_tap_offset_t *packet_length_out,

  /* State. */
  nanotube_tap_packet_resize_ingress_state_t *state,

  /* Inputs. */
  nanotube_tap_packet_resize_req_t *resize_req_in,
  uint8_t *packet_word_in,
  nanotube_tap_offset_t packet_word_len)
#if __clang__
  __attribute__((always_inline))
#endif
{
  switch (bus) {
      case NANOTUBE_BUS_ID_SB:
        assert(packet_word_len == simple_bus::total_bytes);
        return nanotube_tap_packet_resize_ingress_sb(packet_done_out,
                                                     cword_out,
                                                     packet_length_out,
                                                     state,
                                                     resize_req_in,
                                                     packet_word_in);
        break;
      case NANOTUBE_BUS_ID_SHB:
        assert(packet_word_len == softhub_bus::total_bytes);
        return nanotube_tap_packet_resize_ingress_shb(packet_done_out,
                                                      cword_out,
                                                      packet_length_out,
                                                      state,
                                                      resize_req_in,
                                                      packet_word_in);
        break;
      case NANOTUBE_BUS_ID_X3RX:
        assert(packet_word_len == x3rx_bus::total_bytes);
        return nanotube_tap_packet_resize_ingress_x3rx(packet_done_out,
                                                      cword_out,
                                                      packet_length_out,
                                                      state,
                                                      resize_req_in,
                                                      packet_word_in);
        break;
      default:
        assert(false);
  }
  return;
}

///////////////////////////////////////////////////////////////////////////

//Wrapper for nanotube_tap_packet_resize_egress_core() that selects the bus to use

void nanotube_tap_packet_resize_egress_bus(
  /* Which bus implementation to use */
  const enum nanotube_bus_id_t bus,

  /* Outputs. */
  bool *input_done_out,
  bool *packet_done_out,
  bool *packet_valid_out,
  uint8_t *packet_word_out,

  /* State. */
  nanotube_tap_packet_resize_egress_state_t *state,
  void *state_packet_word,

  /* Inputs. */
  nanotube_tap_packet_resize_cword_t *cword,
  uint8_t *packet_word_in,
  nanotube_tap_offset_t packet_word_len,
  nanotube_tap_offset_t new_packet_len)
#if __clang__
  __attribute__((always_inline))
#endif
{
  switch (bus) {
      case NANOTUBE_BUS_ID_SB:
        assert(packet_word_len == simple_bus::total_bytes);
        return nanotube_tap_packet_resize_egress_sb(input_done_out,
                                                    packet_done_out,
                                                    packet_valid_out,
                                                    packet_word_out, state,
                                                    state_packet_word,
                                                    cword, packet_word_in,
                                                    new_packet_len);
        break;
      case NANOTUBE_BUS_ID_SHB:
        assert(packet_word_len == softhub_bus::total_bytes);
        return nanotube_tap_packet_resize_egress_shb(input_done_out,
                                                     packet_done_out,
                                                     packet_valid_out,
                                                     packet_word_out, state,
                                                     state_packet_word,
                                                     cword, packet_word_in,
                                                     new_packet_len);
        break;
      case NANOTUBE_BUS_ID_X3RX:
        assert(packet_word_len == x3rx_bus::total_bytes);
        return nanotube_tap_packet_resize_egress_x3rx(input_done_out,
                                                     packet_done_out,
                                                     packet_valid_out,
                                                     packet_word_out, state,
                                                     state_packet_word,
                                                     cword, packet_word_in,
                                                     new_packet_len);
        break;
      default:
        assert(false);
  }
  return;
}

///////////////////////////////////////////////////////////////////////////
