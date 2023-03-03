/**************************************************************************\
*//*! \file nanotube_packet_taps_shb.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  The softhub bus specific implementation of Nanotube packet taps.
**   \date  2020-09-23
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_core.h"
#include "nanotube_packet_taps_shb.h"

#include <cstring>

///////////////////////////////////////////////////////////////////////////
//Simple bus wrapper for nanotube_tap_packet_length_core()

void nanotube_tap_packet_length_shb(
  /* Outputs. */
  struct nanotube_tap_packet_length_resp *resp_out,

  /* State. */
  struct nanotube_tap_packet_length_state *state_inout,

  /* Inputs. */
  const void *packet_word_in,
  const struct nanotube_tap_packet_length_req *req_in)
#if __clang__
  __attribute__((always_inline))
#endif
{
  check_type(nanotube_tap_packet_length, _shb);

  const auto* shbw_in = (softhub_bus::word*)packet_word_in;

  if( state_inout->packet_offset == 0 ) {
    state_inout->packet_length = shbw_in->get_ch_length();
  }

  uint16_t packet_remaining = state_inout->packet_length - state_inout->packet_offset;
  bool packet_word_eop = packet_remaining <= softhub_bus::data_bytes;
  uint16_t packet_word_length = (packet_word_eop ? packet_remaining : softhub_bus::data_bytes);

  nanotube_tap_packet_length_core(
    resp_out,
    state_inout,
    packet_word_eop, packet_word_length, req_in);
}

///////////////////////////////////////////////////////////////////////////

//Simple bus wrapper for nanotube_tap_packet_read_core()

void nanotube_tap_packet_read_shb(
  /* Constant parameters */
  uint16_t result_buffer_length,
  uint8_t result_buffer_index_bits,

  /* Outputs. */
  struct nanotube_tap_packet_read_resp *resp_out,
  uint8_t *result_buffer_inout,

  /* State. */
  struct nanotube_tap_packet_read_state *state_inout,

  /* Inputs. */
  const void *packet_word_in,
  const struct nanotube_tap_packet_read_req *req_in)
#if __clang__
  __attribute__((always_inline))
#endif
{
  check_type(nanotube_tap_packet_read, _shb);

  const uint8_t word_index_bits = 6;
  static_assert(sizeof(softhub_bus::data_bytes) <= (1U<<word_index_bits),
                "Word index width is too small.");
  const auto* shbw_in = (const softhub_bus::word*)packet_word_in;
  
  if( state_inout->packet_offset == 0 ) {
    state_inout->packet_length = shbw_in->get_ch_length();
  }

  uint16_t packet_remaining = state_inout->packet_length - state_inout->packet_offset;
  bool packet_word_eop = packet_remaining <= softhub_bus::data_bytes;
  uint16_t packet_word_length = (packet_word_eop ? packet_remaining : softhub_bus::data_bytes);

  nanotube_tap_packet_read_core(
    result_buffer_length, result_buffer_index_bits,
    softhub_bus::data_bytes, word_index_bits,
    resp_out, result_buffer_inout,
    state_inout,
    shbw_in->data_ptr(0), packet_word_eop, packet_word_length, req_in);
}

///////////////////////////////////////////////////////////////////////////

//Simple bus wrapper for nanotube_tap_packet_write_core()

void nanotube_tap_packet_write_shb(
  /* Constant parameters */
  uint16_t request_buffer_length,
  uint8_t request_buffer_index_bits,

  /* Outputs. */
  void *packet_word_out,

  /* State. */
  struct nanotube_tap_packet_write_state *state_inout,

  /* Inputs. */
  const void *packet_word_in,
  const struct nanotube_tap_packet_write_req *req_in,
  const uint8_t *request_bytes_in,
  const uint8_t *request_mask_in)
#if __clang__
  __attribute__((always_inline))
#endif
{
  check_type(nanotube_tap_packet_write, _shb);

  const uint8_t word_index_bits = 6;
  static_assert(sizeof(softhub_bus::data_bytes) <= (1U<<word_index_bits),
                "Word index width is too small.");

  const auto *shbw_in = (const softhub_bus::word*)packet_word_in;
  auto *shbw_out      = (softhub_bus::word*)packet_word_out;

  if( state_inout->packet_offset == 0 ) {
    state_inout->packet_length = shbw_in->get_ch_length();
  }

  /* Propagate the sideband bytes */
  if( softhub_bus::total_bytes > softhub_bus::data_bytes ) {
    memcpy(shbw_out->data_ptr(softhub_bus::data_bytes),
           shbw_in->data_ptr(softhub_bus::data_bytes),
           softhub_bus::total_bytes - softhub_bus::data_bytes);
  }

  uint16_t packet_remaining = state_inout->packet_length - state_inout->packet_offset;
  bool packet_word_eop = packet_remaining <= softhub_bus::data_bytes;
  uint16_t packet_word_length = (packet_word_eop ? packet_remaining : softhub_bus::data_bytes);

  // Invoke the core of the tap.
  nanotube_tap_packet_write_core(
    /* Constant parameters */
    request_buffer_length,
    request_buffer_index_bits,
    softhub_bus::data_bytes,
    word_index_bits,

    /* Outputs. */
    shbw_out->data_ptr(),

    /* State. */
    state_inout,

  /* Inputs. */
    shbw_in->data_ptr(),
    packet_word_eop,
    packet_word_length,
    req_in,
    request_bytes_in,
    request_mask_in);
}

///////////////////////////////////////////////////////////////////////////

//Simple bus wrapper for nanotube_tap_packet_resize_ingress_core()

void nanotube_tap_packet_resize_ingress_shb(
  /* Outputs. */
  bool *packet_done_out,
  nanotube_tap_packet_resize_cword_t *cword_out,
  nanotube_tap_offset_t *packet_length_out,

  /* State. */
  nanotube_tap_packet_resize_ingress_state_t *state,

  /* Inputs. */
  nanotube_tap_packet_resize_req_t *resize_req_in,
  void *packet_word_in)
#if __clang__
  __attribute__((always_inline))
#endif
{
  check_type(nanotube_tap_packet_resize_ingress, _shb);

  auto *shbw_in = (softhub_bus::word*)packet_word_in;

  if( state->new_req ) {
    state->packet_length = shbw_in->get_ch_length();
  }

  auto bytes_deleted = resize_req_in->write_offset + resize_req_in->delete_length < state->packet_length ?
    resize_req_in->delete_length : state->packet_length - resize_req_in->write_offset;
  auto bytes_inserted = resize_req_in->write_offset < state->packet_length ?
    resize_req_in->insert_length : 0;

  *packet_length_out = state->packet_length - bytes_deleted + bytes_inserted;

  nanotube_tap_offset_t packet_remaining_in = state->packet_length - state->packet_offset;
  bool eop_in = packet_remaining_in <= softhub_bus::data_bytes;
  nanotube_tap_offset_t word_length_in = (eop_in ? packet_remaining_in : softhub_bus::data_bytes);

  nanotube_tap_packet_resize_ingress_core(
    softhub_bus::data_bytes,
    packet_done_out,
    cword_out,
    packet_length_out,
    state,
    resize_req_in,
    word_length_in,
    eop_in);
}

///////////////////////////////////////////////////////////////////////////

//Simple bus wrapper for nanotube_tap_packet_resize_egress_core()

void nanotube_tap_packet_resize_egress_shb(
  /* Outputs. */
  bool *input_done_out,
  bool *packet_done_out,
  bool *word_valid_out,
  void *packet_word_out,

  /* State. */
  nanotube_tap_packet_resize_egress_state_t *state,
  void *state_packet_word,

  /* Inputs. */
  nanotube_tap_packet_resize_cword_t *cword,
  void *packet_word_in,
  nanotube_tap_offset_t new_packet_len)
#if __clang__
  __attribute__((always_inline))
#endif
{
  check_type(nanotube_tap_packet_resize_egress, _shb);

  auto *shbw_in  = (softhub_bus::word*)packet_word_in;
  auto *shbw_out = (softhub_bus::word*)packet_word_out;

  bool word_eop_out;
  nanotube_tap_offset_t word_length_out;

  if( state->new_pkt ) {
    /* store the input length to calculate the input eop */
    state->packet_length = shbw_in->get_ch_length();
    /* update the header to have the new length */
    shbw_in->set_ch_length(new_packet_len);
  }
  bool input_eop = (state->packet_length - state->packet_offset) < softhub_bus::data_bytes;

  nanotube_tap_packet_resize_egress_core(
    softhub_bus::data_bytes, softhub_bus::log_data_bytes,
    input_done_out,
    word_valid_out, &word_eop_out,
    &word_length_out, shbw_out->data_ptr(),
    state, (uint8_t*)state_packet_word,
    cword, shbw_in->data_ptr());

  *packet_done_out = input_eop & *input_done_out;

  /* Set packet offset to zero if we've finished this packet and will be moving on
   * to the next one, else increment if it we've finished processing an input word */
  if( *packet_done_out ) {
    state->packet_offset = 0;
    state->new_pkt = true;
  }
  else {
    state->new_pkt = false;
    if( *input_done_out ) {
      state->packet_offset += softhub_bus::data_bytes;
    }
  }
  /* Set TLAST, TKEEP, TSTRB, if sideband signals present */
  if( *word_valid_out && softhub_bus::sideband_signals_bytes ) {
    shbw_out->set_tlast(word_eop_out);
    /* Assume first word_length_out bytes are valid and rest are not */
    shbw_out->set_tkeep(word_length_out);
    shbw_out->set_tstrb(word_length_out);
  }
}

///////////////////////////////////////////////////////////////////////////

bool nanotube_tap_packet_is_eop_shb(
  const void *packet_word_in,
  struct nanotube_tap_packet_eop_state *state_inout
)
#if __clang__
  __attribute__((always_inline))
#endif
{
  check_type(nanotube_tap_packet_is_eop, _shb);
  auto *shbw_in  = (softhub_bus::word*)packet_word_in;

  if( state_inout->packet_offset == 0) {
    //new packet
    state_inout->packet_length = shbw_in->get_ch_length();
  }
  uint16_t packet_remaining = state_inout->packet_length - state_inout->packet_offset;
  bool packet_word_eop = packet_remaining <= softhub_bus::data_bytes;

  if( packet_word_eop )
    state_inout->packet_offset = 0;
  else 
    state_inout->packet_offset += softhub_bus::data_bytes;
  
  return packet_word_eop;
}

///////////////////////////////////////////////////////////////////////////

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
