/**************************************************************************\
*//*! \file nanotube_packet_taps_core.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  Interface for packet tap core logic functions.
**   \date  2020-09-23
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PACKET_TAPS_CORE_H
#define NANOTUBE_PACKET_TAPS_CORE_H

#include "nanotube_packet_taps.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////

/*! Copy data between two buffers while rotating the order of the
**  bytes.
**
** \param buffer_out_length  The number of bytes in the output buffer.
** This must be a constant expression for synthesis.
**
** \param rot_buffer_length The number of bytes to rotate.  This must
** be at least as large as the input buffer length to avoid the input
** being truncated.  This must be a constant expression for synthesis.
**
** \param buffer_in_length  The number of bytes in the input buffer.
** This must be a constant expression for synthesis.
**
** \param rot_amount_bits  The number of bits required to specify
** the rotation amount.  This must be a constant expression
** for synthesis.
**
** \param buffer_out  The output buffer.
**
** \param buffer_in   The input buffer.
**
** \param rot_amount  The rotation amount as a number of bytes.
**
** Reads buffer_in_length bytes from buffer_in, pads them up to
** rot_buffer_length, rotates them by rot_amount bytes and writes the
** first buffer_out_length of them to buffer_out.
**
** The value written to rot_buffer[i] is:
**   rot_buffer[i] = (i >= buffer_in_length ? 0 : buffer_in[i])
**
** The rotate amount used is:
**    rot_amount_masked = ((1 << rot_amount_bits)-1) & rot_amount
**
** The value written to buffer_out[i] is:
**   rot_buffer[(i+rot_amount_masked)%rot_buffer_length].
**
** The rotate amount can be any value, but only the rot_amount_bits
** least significant bits are used.
*/
void nanotube_rotate_down(
  /* Constant parameters. */
  uint16_t buffer_out_length,
  uint16_t rot_buffer_length,
  uint16_t buffer_in_length,
  uint8_t rot_amount_bits,

  /* Outputs. */
  uint8_t *buffer_out,

  /* Inputs. */
  const uint8_t *buffer_in,
  uint16_t rot_amount);

///////////////////////////////////////////////////////////////////////////

/*! Copy bytes between buffers while shifting.
**
** \param buffer_out_length The number of bytes in the output buffer.
**
** \param buffer_out The output buffer.
**
** \param buffer_in The input buffer.
**
** \param shift_amount The shift amount in bits.
**
** Copy buffer_out_length bytes into buffer_out by shifting the bytes
** from buffer_in right and down in index.  The number of bytes in
** buffer_in is buffer_out_length+1 since some bits from input byte
** buffer_out_length might be shifted down into output byte
** buffer_out_length-1.  The maximum shift amount is 7.
**
*/
void nanotube_shift_down_bits(uint32_t buffer_out_length,
                              uint8_t *buffer_out,
                              const uint8_t *buffer_in,
                              uint8_t shift_amount);

///////////////////////////////////////////////////////////////////////////

/*! Pad a bit vector and concatenate it with itself.
**
** \param dup_bits_out The output vector.
**
** \param bit_vec_in The input vector.
**
** \param input_bit_length The number of bits in the input vector.
**
** \param padded_bit_length The number of bits in the padded vector.
**
** Produces a vector which includes two copies of the input vector,
** padded with zeros to the end of the byte.  The number of bytes in
** the output vector is the minimum reqired to hold 2*input_bit_length
** bits.
*/
void nanotube_duplicate_bits(uint8_t *dup_bits_out,
                             const uint8_t *bit_vec_in,
                             uint32_t input_bit_length,
                             uint32_t padded_bit_length);

///////////////////////////////////////////////////////////////////////////

/*! Determine the length of the packet, upto the specified maximum.
**
** \param resp_out The response structure, written by the tap with the
** result of the operation.
**
** \param state_inout An opaque state structure which should be
** initialised to nanotube_tap_packet_length_state_init and then
** passed to every call of the length tap.
**
** \param packet_word_eop A flag which indicates that the end of
** packet has arrived.
**
** \param packet_word_length The number of bytes in this packet word.
**
** \param req_in The request which is provided to the length tap.
**
** The packet length tap determines the length of the packet, bounding
** the result up to a specified parameter.  The request valid bit
** should be set as soon as the required maximum length is known and
** should remain set until the end of the packet.  If the request
** valid bit is never set, the full packet length will be provided at
** the end of the packet.
**
** The response valid bit will be set for exactly one word of the
** packet.  If the request valid bit is ever set for a packet then the
** response valid bit will be set no sooner than the request valid
** bit.  If the request valid bit is not set for a packet then the
** response valid bit will be set during the call for the end of
** packet word.
*/

void nanotube_tap_packet_length_core(
  /* Outputs. */
  struct nanotube_tap_packet_length_resp *resp_out,

  /* State. */
  struct nanotube_tap_packet_length_state *state_inout,

  /* Inputs. */
  bool packet_word_eop,
  uint16_t packet_word_length,
  const struct nanotube_tap_packet_length_req *req_in);


///////////////////////////////////////////////////////////////////////////

/*! Read a series of bytes from a packet.
**
** \param result_buffer_length  The size of the result buffer in bytes.
** This must be a constant expression for synthesis.
**
** \param result_buffer_index_bits The number of bits required for an
** index into the result buffer.  This must be a constant expression
** for synthesis.
**
** \param packet_buffer_length  The size of the packet buffer in bytes.
** This must be a constant expression for synthesis.
**
** \param packet_buffer_index_bits  The number of bits required for an
** index into the packet buffer.  This must be a constant expression
** for synthesis.
**
** \param resp_out The response structure, written by the tap with
** information about the read operation.
**
** \param result_buffer_inout  The result buffer, written by the
** tap with the bytes which were read.  The buffer is written
** incrementally, so the same buffer should be supplied to every call.
**
** \param state_inout  An opaque state structure which should be
** initialised to nanotube_tap_packet_read_state_init and then passed
** to every call to the read tap.
**
** \param packet_buffer_in  The input packet buffer.  Each call must
** supply packet_buffer_length packet bytes except the final call for
** a packet (with packet_word_eop set) which can be shorter.
**
** \param packet_word_eop Indicates whether this is the last call for
** a packet.
**
** \param req_in A non-null pointer to the read request.  The request
** is only acted on if the "valid" member is set.  After "valid" has
** been set, the request structure must remain constant until after
** the end of packet (the next call with packet_word_eop set).  The
** "valid" member must be set in the call for the word which contains
** the first byte to be read.
*/
void nanotube_tap_packet_read_core(
  /* Constant parameters */
  uint16_t result_buffer_length,
  uint8_t result_buffer_index_bits,
  uint16_t packet_buffer_length,
  uint8_t packet_buffer_index_bits,

  /* Outputs. */
  struct nanotube_tap_packet_read_resp *resp_out,
  uint8_t *result_buffer_inout,

  /* State. */
  struct nanotube_tap_packet_read_state *state_inout,

  /* Inputs. */
  const uint8_t *packet_buffer_in,
  bool packet_word_eop,
  uint16_t packet_word_length,
  const struct nanotube_tap_packet_read_req *req_in);


///////////////////////////////////////////////////////////////////////////

/*! Write a series of bytes to a packet with masking.
**
** \param request_buffer_length The size of the request buffer in
** bytes.  This must be a constant expression for synthesis.
**
** \param request_buffer_index_bits The number of bits required for an
** index into the request buffer.  This must be a constant expression
** for synthesis.
**
** \param packet_buffer_length The size of the packet buffer in bytes.
** This must be a constant expression for synthesis.
**
** \param packet_buffer_index_bits The number of bits required for an
** index into the packet buffer.  This must be a constant expression
** for synthesis.
**
** \param packet_buffer_out The output packet buffer.  The packet
** length is not modified, so the end of packet indicators are the
** same as for the input packet.
**
** \param state_inout An opaque state structure which should be
** initialised to nanotube_tap_packet_write_state_init and then passed
** to every call to the write tap.
**
** \param packet_buffer_in The input packet buffer.  Each call must
** supply packet_buffer_length packet bytes except the final call for
** a packet (with packet_word_eop set) which can be shorter.
**
** \param packet_word_eop Indicates whether this is the last call for
** a packet.
**
** \param packet_word_length The number of valid bytes in the input
** packet buffer.  This must be equal to packet_buffer_length except
** when packet_word_eop is set.
**
** \param req_in A non-null pointer to the write request.  The request
** is only acted on if the "valid" member is set.  After "valid" has
** been set, the request structure must remain constant until after
** the end of packet (the next call with packet_word_eop set).  The
** "valid" member must be set in the call for the word which contains
** the first byte to be written.
**
** \param request_bytes_in A non-null pointer to the bytes to be
** written to the packet.  The maximum number of bytes is given in the
** request structure.
**
** \param request_mask_in A non-null pointer to the byte enable bits.
** Each set bit in the mask indicates a byte to be written.  Each mask
** byte except the last refers to 8 data bytes with the least
** significant bit referring to the byte with the lowest offset.
*/
void nanotube_tap_packet_write_core(
  /* Constant parameters */
  uint16_t request_buffer_length,
  uint8_t request_buffer_index_bits,
  uint16_t packet_buffer_length,
  uint8_t packet_buffer_index_bits,

  /* Outputs. */
  uint8_t *packet_buffer_out,

  /* State. */
  struct nanotube_tap_packet_write_state *state_inout,

  /* Inputs. */
  const uint8_t *packet_buffer_in,
  bool packet_word_eop,
  uint16_t packet_word_length,
  const struct nanotube_tap_packet_write_req *req_in,
  const uint8_t *request_bytes_in,
  const uint8_t *request_mask_in);

///////////////////////////////////////////////////////////////////////////

/*! The core of the resize tap ingress stage.
**
** \param packet_word_length The number of bytes per packet word.
** This must be a constant expression for synthesis.
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
** \param word_length_in The number of bytes in the current packet
** word.
**
** \param word_eop_in A flag indicating that the current word is the
** last word of the packet.
**
** This is the inner core of the resize tap ingress stage.  It is
** expected that users will use a wrapper for the appropriate bus type
** instead of calling this function directly.
**/
void nanotube_tap_packet_resize_ingress_core(
  /* Parameters. */
  nanotube_tap_offset_t packet_word_length,

  /* Outputs. */
  bool *packet_done_out,
  nanotube_tap_packet_resize_cword_t *cword_out,
  nanotube_tap_offset_t *packet_length_out,

  /* State. */
  nanotube_tap_packet_resize_ingress_state_t *state,

  /* Inputs. */
  nanotube_tap_packet_resize_req_t *resize_req_in,
  nanotube_tap_offset_t word_length_in,
  bool word_eop_in);


/*! The core of the resize tap egress stage.
**
** \param packet_word_length The number of bytes per packet word.
**  This must be a constant expression for synthesis.
**
** \param packet_word_index_bits The number of bits required for an
**  index into the packet buffer.  This must be a constant expression
**  for synthesis.
**
** \param input_done_out A pointer to an output flag which will be set
**  when the packet word has been processed.  When set, the next call
**  should use the next packet word and control word.
**
** \param word_valid_out A pointer to an output flag which will be set
**  if the stage has produced an output packet word.
**
** \param word_eop_out A pointer to an output flag which will be set
**  if the output packet word is the last word of the packet.  This
**  flag is only valid of word_valid_out is set.
**
** \param word_length_out A pointer to an output value which will
**  contain the number of bytes in the output packet word.  This will
**  be equal to packet_word_length for every word except the last.
**  This value is only valid of word_valid_out is set.
**
** \param word_data_out A pointer to an output buffer which will
** contain the data bytes of the output packet word.  The buffer is
** only valid of word_valid_out is set.
**
** \param state A pointer to the state buffer for the egress stage.
** This buffer is used to track progress through the packet.
**
** \param carried_data The bytes carried from the previous call to the
**  next call. This buffer must live across invocations of the tap, and
**  contain at least packet_word_len bytes.
**
** \param cword A pointer to an input buffer which contains the
** control word from the ingress stage.
**
** \param packet_data_in A pointer to an input bufer which contains
** the data bytes of the input packet word.
**
** This is the inner core of the resize tap ingress stage.  It is
** expected that users will use a wrapper for the appropriate bus type
** instead of calling this function directly.
*/
void nanotube_tap_packet_resize_egress_core(
  /* Parameters. */
  nanotube_tap_offset_t packet_word_length,
  unsigned int packet_word_index_bits,

  /* Outputs. */
  bool *input_done_out,
  bool *word_valid_out,
  bool *word_eop_out,
  nanotube_tap_offset_t *word_length_out,
  uint8_t *word_data_out,

  /* State. */
  nanotube_tap_packet_resize_egress_state_t *state,
  uint8_t *carried_data,

  /* Inputs. */
  nanotube_tap_packet_resize_cword_t *cword,
  uint8_t *packet_data_in);


/**@}*/

///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif // NANOTUBE_PACKET_TAPS_CORE_H
