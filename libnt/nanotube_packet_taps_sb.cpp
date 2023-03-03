/**************************************************************************\
*//*! \file nanotube_packet_taps_sb.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  The simple bus specific implementation of Nanotube packet taps.
**   \date  2020-09-23
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_sb.h"
#include "nanotube_packet_taps_core.h"

#include <cstring>

///////////////////////////////////////////////////////////////////////////
//Simple bus wrapper for nanotube_tap_packet_length_core()

void nanotube_tap_packet_length_sb(
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
  check_type(nanotube_tap_packet_length, _sb);

  const auto* sbw_in = (simple_bus::word*)packet_word_in;
  bool packet_word_eop = sbw_in->get_eop();
  uint16_t packet_word_length =
    ( simple_bus::data_bytes -
      (packet_word_eop ? sbw_in->get_empty() : 0) );

  nanotube_tap_packet_length_core(
    resp_out,
    state_inout,
    packet_word_eop, packet_word_length, req_in);
}

///////////////////////////////////////////////////////////////////////////

//Simple bus wrapper for nanotube_tap_packet_read_core()

void nanotube_tap_packet_read_sb(
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
  check_type(nanotube_tap_packet_read, _sb);

  const uint8_t word_index_bits = 6;
  static_assert(sizeof(simple_bus::data_bytes) <= (1U<<word_index_bits),
                "Word index width is too small.");
  const auto* sbw_in = (const simple_bus::word*)packet_word_in;

  bool packet_word_eop = sbw_in->get_eop();
  uint16_t packet_word_length =
    ( simple_bus::data_bytes -
      (packet_word_eop ? sbw_in->get_empty() : 0) );

  nanotube_tap_packet_read_core(
    result_buffer_length, result_buffer_index_bits,
    simple_bus::data_bytes, word_index_bits,
    resp_out, result_buffer_inout,
    state_inout,
    sbw_in->data_ptr(0), packet_word_eop, packet_word_length, req_in);
}

///////////////////////////////////////////////////////////////////////////

//Simple bus wrapper for nanotube_tap_packet_write_core()

void nanotube_tap_packet_write_sb(
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
  check_type(nanotube_tap_packet_write, _sb);

  const uint8_t word_index_bits = 6;
  static_assert(sizeof(simple_bus::data_bytes) <= (1U<<word_index_bits),
                "Word index width is too small.");

  const auto *sbw_in = (const simple_bus::word*)packet_word_in;
  auto *sbw_out      = (simple_bus::word*)packet_word_out;

  bool packet_word_eop = sbw_in->get_eop();
  uint16_t packet_word_length =
    ( simple_bus::data_bytes -
      (packet_word_eop ? sbw_in->get_empty() : 0) );

  // Copy the control value.
  sbw_out->control() = sbw_in->control();

  /* Propagate any additional sideband bytes/signals */
  if( simple_bus::total_bytes > simple_bus::data_bytes + 1 /*control byte*/ ) {
    memcpy(sbw_out->data_ptr(simple_bus::data_bytes),
           sbw_in->data_ptr(simple_bus::data_bytes),
           simple_bus::total_bytes - simple_bus::data_bytes - 1);
  }

  // Invoke the core of the tap.
  nanotube_tap_packet_write_core(
    /* Constant parameters */
    request_buffer_length,
    request_buffer_index_bits,
    simple_bus::data_bytes,
    word_index_bits,

    /* Outputs. */
    sbw_out->data_ptr(),

    /* State. */
    state_inout,

  /* Inputs. */
    sbw_in->data_ptr(),
    packet_word_eop,
    packet_word_length,
    req_in,
    request_bytes_in,
    request_mask_in);
}

///////////////////////////////////////////////////////////////////////////

//Simple bus wrapper for nanotube_tap_packet_resize_ingress_core()

void nanotube_tap_packet_resize_ingress_sb(
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
  check_type(nanotube_tap_packet_resize_ingress, _sb);

  auto *sbw_in = (simple_bus::word*)packet_word_in;

  nanotube_tap_offset_t word_length_in =
    ( simple_bus::data_bytes -
      (sbw_in->get_eop() ? sbw_in->get_empty() : 0) );
  bool eop_in = sbw_in->get_eop();

  nanotube_tap_packet_resize_ingress_core(
    simple_bus::data_bytes,
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

void nanotube_tap_packet_resize_egress_sb(
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
  check_type(nanotube_tap_packet_resize_egress, _sb);

  auto *sbw_in  = (simple_bus::word*)packet_word_in;
  auto *sbw_out = (simple_bus::word*)packet_word_out;

  bool word_eop_out;
  nanotube_tap_offset_t word_length_out;

  bool input_eop = sbw_in->get_eop();

  nanotube_tap_packet_resize_egress_core(
    simple_bus::data_bytes, simple_bus::log_data_bytes,
    input_done_out,
    word_valid_out, &word_eop_out,
    &word_length_out, sbw_out->data_ptr(),
    state, (uint8_t*)state_packet_word,
    cword, sbw_in->data_ptr());

  *packet_done_out = input_eop & *input_done_out;

  bool err = sbw_in->get_err();
  simple_bus::empty_t empty = simple_bus::data_bytes - word_length_out;
  sbw_out->set_control(word_eop_out, err, empty);

  /* Set TLAST, TKEEP, TSTRB, if sideband signals present */
  if( *word_valid_out && simple_bus::sideband_signals_bytes ) { 
    sbw_out->set_tlast(word_eop_out);
    /* Assume first word_length_out bytes are valid and rest are not */
    sbw_out->set_tkeep(word_length_out);
    sbw_out->set_tstrb(word_length_out);
  }
}

///////////////////////////////////////////////////////////////////////////

bool nanotube_tap_packet_is_eop_sb(
  const void *packet_word_in,
  struct nanotube_tap_packet_eop_state *state_inout
)
#if __clang__
  __attribute__((always_inline))
#endif
{
  check_type(nanotube_tap_packet_is_eop, _sb);

  auto* pw = (simple_bus::word*)packet_word_in;
  return pw->get_eop();
}

///////////////////////////////////////////////////////////////////////////

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
