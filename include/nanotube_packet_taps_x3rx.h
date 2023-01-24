 /**************************************************************************\
*//*! \file nanotube_packet_taps_x3rx.h
** \author  Kieran Mansley <kieran.mansley@amd.com>
**  \brief  X3RX Bus interface for Nanotube packet taps.
**   \date  2023-01-17
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PACKET_TAPS_X3RX_H
#define NANOTUBE_PACKET_TAPS_X3RX_H

#include "x3rx_bus.hpp"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////

/*! The x3rx bus wrapper of the length tap.
**/
void nanotube_tap_packet_length_x3rx(
  /* Outputs. */
  struct nanotube_tap_packet_length_resp *resp_out,

  /* State. */
  struct nanotube_tap_packet_length_state *state_inout,

  /* Inputs. */
  const void *packet_word_in,
  const struct nanotube_tap_packet_length_req *req_in);


///////////////////////////////////////////////////////////////////////////

/*! The x3rx bus wrapper of the read tap.
**/
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
  const struct nanotube_tap_packet_read_req *req_in);


///////////////////////////////////////////////////////////////////////////

/*! The x3rx bus wrapper of the write tap.
**/
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
  const uint8_t *request_mask_in);


///////////////////////////////////////////////////////////////////////////

/*! The x3rx bus wrapper of the resize tap ingress stage.
**
** \param packet_done_out A pointer to an output flag which will be
** set when the end of the packet has been processed.  When set, the
** next call should use the next request.
**
** \param cword_out A pointer to an output buffer which is written
** with the control word for this packet word.  The control word and
** packet word must be presented together to the egress stage.
**
** \param packet_length_out A pointer to a value that is set to the new
** length of the packet (after the resize operation completes).
**
** \param state The ingress stage state which is used to track
** progress through the packet.
**
** \param resize_req_in A pointer to an input buffer which contains
** the resize request for this packet.
**
** \param packet_word_in A pointer to an input buffer which contains
** the packet word.
**/
void nanotube_tap_packet_resize_ingress_x3rx(
  /* Outputs. */
  bool *packet_done_out,
  nanotube_tap_packet_resize_cword_t *cword_out,
  nanotube_tap_offset_t *packe_length_out,

  /* State. */
  nanotube_tap_packet_resize_ingress_state_t *state,

  /* Inputs. */
  nanotube_tap_packet_resize_req_t *resize_req_in,
  void *packet_word_in);


///////////////////////////////////////////////////////////////////////////

/*! The x3rx bus wrapper of the resize tap ingress stage.
**
** \param input_done_out A pointer to an output flag which will be set
**  when the packet word has been processed.  When set, the next call
**  should use the next packet word and control word.
**
** \param packet_done_out A pointer to an output flag which will be set if the
**  tap is done processing the entire packet and ready to receive the next full
**  packet.
**
** \param word_valid_out A pointer to an output flag which will be set
**  if the stage has produced an output packet word.
**
** \param packet_word_out A pointer to an output buffer which will
** contain the output packet word.  This word is only valid if
** word_valid_out is set.
**
** \param state A pointer to the state buffer for the egress stage.
** This buffer is used to track progress through the packet.
**
** \param cword A pointer to an input buffer which contains the
** control word from the ingress stage.
**
** \param packet_word_in A pointer to an input bufer which contains
** the input packet word.
**
** \param new_packet_len The new packet length, as supplied by the
** ingress resize tap (currently only used by softhub bus)
*/
void nanotube_tap_packet_resize_egress_x3rx(
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
  nanotube_tap_offset_t new_packet_len);


///////////////////////////////////////////////////////////////////////////

/*! Check whether a softhub bus packet word denotes the end of packet.
**
** This is a specialisation of the generic nanotube_tap_packet_is_eop function
** for softhub bus packet words.
**
** \param packet_word_in  The input packet word.
** \param state_inout Static storage for the tap
** \return True if this packet word is the last one of the packet.
**/
bool nanotube_tap_packet_is_eop_x3rx(
  const void *packet_word_in,
  struct nanotube_tap_packet_eop_state *state_inout
);

///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif // NANOTUBE_PACKET_TAPS_x3rx_H
