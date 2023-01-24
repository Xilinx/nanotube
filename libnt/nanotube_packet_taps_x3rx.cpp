/**************************************************************************\
*//*! \file nanotube_packet_taps_x3rx.cpp
** \author  Kieran Mansley <kieran.mansley@amd.com>
**  \brief  The x3rx bus specific implementation of Nanotube packet taps.
**   \date  2023-01-17
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_x3rx.h"
#include "nanotube_packet_taps_core.h"

#include <cstring>

///////////////////////////////////////////////////////////////////////////

// Decode how many bytes in this word are valid packet data

static inline
uint16_t x3rx_valid_data_bytes(const x3rx_bus::word* x3rxw_in, uint8_t *data_eop_seen)
#if __clang__
  __attribute__((always_inline))
#endif
{
  /* Reset tracking of data EOP when we see data SOP */
  if( x3rxw_in->get_data_sop() )
    *data_eop_seen = 0;
  if( x3rxw_in->get_data_eop() ) {
    int valid = x3rxw_in->get_data_eop_valid();
    /* Remember we've seen data EOP in case there are subsequent words with just meta data */
    *data_eop_seen = 1;
    /* Convert valid bit mask into number of bytes */
    if( valid == 1 ) return 1;
    valid = valid >> 1;
    if( valid == 1 ) return 2;
    valid = valid >> 1;
    if( valid == 1 ) return 3;
    valid = valid >> 1;
    if( valid == 1 ) return 4;
  }
  else if( *data_eop_seen ) {
    /* No valid data bytes after data_eop */
    return 0;
  }
  return x3rx_bus::data_bytes;
}

///////////////////////////////////////////////////////////////////////////

// X3RX bus wrapper for nanotube_tap_packet_length_core()

void nanotube_tap_packet_length_x3rx(
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
  check_type(nanotube_tap_packet_length, _x3rx);

  const auto* x3rxw_in = (x3rx_bus::word*)packet_word_in;
  uint16_t packet_word_length;
  /* meta EOP comes after data EOP so use that to track when there are no
   * more words for this packet */
  bool packet_word_eop = x3rxw_in->get_meta_eop();

  packet_word_length = x3rx_valid_data_bytes(x3rxw_in, &state_inout->data_eop_seen);

  nanotube_tap_packet_length_core(
    resp_out,
    state_inout,
    packet_word_eop, packet_word_length, req_in);
}

///////////////////////////////////////////////////////////////////////////

// X3RX bus wrapper for nanotube_tap_packet_read_core()

void nanotube_tap_packet_read_x3rx(
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
  check_type(nanotube_tap_packet_read, _x3rx);

  const uint8_t word_index_bits = 2;
  static_assert(sizeof(x3rx_bus::data_bytes) <= (1U<<word_index_bits),
                "Word index width is too small.");
  const auto* x3rxw_in = (const x3rx_bus::word*)packet_word_in;
  uint16_t packet_word_length;
  /* meta EOP comes after data EOP so use that to track when there are no
   * more words for this packet */
  bool packet_word_eop = x3rxw_in->get_meta_eop();

  packet_word_length = x3rx_valid_data_bytes(x3rxw_in, &state_inout->data_eop_seen);

  nanotube_tap_packet_read_core(
    result_buffer_length, result_buffer_index_bits,
    x3rx_bus::data_bytes, word_index_bits,
    resp_out, result_buffer_inout,
    state_inout,
    x3rxw_in->data_ptr(0), packet_word_eop, packet_word_length, req_in);
}

///////////////////////////////////////////////////////////////////////////

// X3RX bus wrapper for nanotube_tap_packet_write_core()

void nanotube_tap_packet_write_x3rx(
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
  check_type(nanotube_tap_packet_write, _x3rx);

  const uint8_t word_index_bits = 2;
  static_assert(sizeof(x3rx_bus::data_bytes) <= (1U<<word_index_bits),
                "Word index width is too small.");

  const auto *x3rxw_in = (const x3rx_bus::word*)packet_word_in;
  auto *x3rxw_out      = (x3rx_bus::word*)packet_word_out;
  uint16_t packet_word_length;
  /* meta EOP comes after data EOP so use that to track when there are no
   * more words for this packet */
  bool packet_word_eop = x3rxw_in->get_meta_eop();

  packet_word_length = x3rx_valid_data_bytes(x3rxw_in, &state_inout->data_eop_seen);

  /* Propagate any additional sideband bytes/signals */
  if( x3rx_bus::total_bytes > x3rx_bus::data_bytes ) {
    memcpy(x3rxw_out->data_ptr(x3rx_bus::data_bytes),
           x3rxw_in->data_ptr(x3rx_bus::data_bytes),
           x3rx_bus::total_bytes - x3rx_bus::data_bytes);
  }

  // Invoke the core of the tap.
  nanotube_tap_packet_write_core(
    /* Constant parameters */
    request_buffer_length,
    request_buffer_index_bits,
    x3rx_bus::data_bytes,
    word_index_bits,

    /* Outputs. */
    x3rxw_out->data_ptr(),

    /* State. */
    state_inout,

    /* Inputs. */
    x3rxw_in->data_ptr(),
    packet_word_eop,
    packet_word_length,
    req_in,
    request_bytes_in,
    request_mask_in);
}

///////////////////////////////////////////////////////////////////////////

// X3RX bus wrapper for nanotube_tap_packet_resize_ingress_core()

void nanotube_tap_packet_resize_ingress_x3rx(
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
  check_type(nanotube_tap_packet_resize_ingress, _x3rx);

  auto *x3rxw_in = (x3rx_bus::word*)packet_word_in;

  uint16_t packet_word_length;
  /* meta EOP comes after data EOP so use that to track when there are no
   * more words for this packet */
  bool packet_word_eop = x3rxw_in->get_meta_eop();

  packet_word_length = x3rx_valid_data_bytes(x3rxw_in, &state->data_eop_seen);

  nanotube_tap_packet_resize_ingress_core(
    x3rx_bus::data_bytes,
    packet_done_out,
    cword_out,
    packet_length_out,
    state,
    resize_req_in,
    packet_word_length,
    packet_word_eop);
}

///////////////////////////////////////////////////////////////////////////

// X3RX bus wrapper for nanotube_tap_packet_resize_egress_core()

void nanotube_tap_packet_resize_egress_x3rx(
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
  check_type(nanotube_tap_packet_resize_egress, _x3rx);

  auto *x3rxw_in  = (x3rx_bus::word*)packet_word_in;
  auto *x3rxw_out = (x3rx_bus::word*)packet_word_out;

  bool word_eop_out;
  nanotube_tap_offset_t word_length_out;

  bool input_eop = x3rxw_in->get_meta_eop();

  nanotube_tap_packet_resize_egress_core(
    x3rx_bus::data_bytes, x3rx_bus::log_data_bytes,
    input_done_out,
    word_valid_out, &word_eop_out,
    &word_length_out, x3rxw_out->data_ptr(),
    state, (uint8_t*)state_packet_word,
    cword, x3rxw_in->data_ptr());

  *packet_done_out = input_eop & *input_done_out;

  /* Set TLAST, TKEEP, TSTRB, if sideband signals present */
  if( *word_valid_out && x3rx_bus::sideband_signals_bytes ) {
    x3rxw_out->set_tlast(word_eop_out);
    /* Assume first word_length_out bytes are valid and rest are not */
    x3rxw_out->set_tkeep(word_length_out);
    x3rxw_out->set_tstrb(word_length_out);
  }
}

///////////////////////////////////////////////////////////////////////////

bool nanotube_tap_packet_is_eop_x3rx(
  const void *packet_word_in,
  struct nanotube_tap_packet_eop_state *state_inout
)
#if __clang__
  __attribute__((always_inline))
#endif
{
  check_type(nanotube_tap_packet_is_eop, _x3rx);

  auto* pw = (x3rx_bus::word*)packet_word_in;
  /* meta EOP comes after data EOP so use that to track when there are no
   * more words for this packet */
  return pw->get_meta_eop();
}

///////////////////////////////////////////////////////////////////////////

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
