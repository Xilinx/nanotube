/**************************************************************************\
*//*! \file nanotube_packet_taps.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  Interface for Nanotube packet taps.
**   \date  2020-09-23
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PACKET_TAPS_H
#define NANOTUBE_PACKET_TAPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////

// Holds an offset into a packet or a packet length.
typedef uint16_t nanotube_tap_offset_t;

///////////////////////////////////////////////////////////////////////////

/*! Describes a request to the packet length tap. */
struct nanotube_tap_packet_length_req {
  /*! True if the request has been supplied. */
  bool valid;

  /*! The maximum length to return. */
  uint16_t max_length;
};

/*! Describes a response from the packet length tap. */
struct nanotube_tap_packet_length_resp {
  /*! True if the response is valid.  This will be set after one call
   *  per packet. */
  bool valid;

  /*! The length of the packet. */
  uint16_t result_length;
};

/*! The state passed between calls to the packet length tap. */
struct nanotube_tap_packet_length_state {
  /*! The length of the current packet (only used by softhub_bus) */
  uint16_t packet_length;
  
  /*! The current offset into the packet. */
  uint16_t packet_offset;

  /*! A flag which indicates that the response has been generated. */
  uint8_t done;

  /*! A flag which indicates that we've seen the DATA_EOP bit set for 
   *  this packet (only used by x3rx_bus) */
  uint8_t data_eop_seen;
};

/*! The initial state to pass to the packet length tap. */
static const nanotube_tap_packet_length_state
  nanotube_tap_packet_length_state_init = { 0, 0, 0, 0 };

/*! Determine the length of the packet, upto the specified maximum.
**
** \param resp_out The response structure, written by the tap with the
** result of the operation.
**
** \param state_inout An opaque state structure which should be
** initialised to nanotube_tap_packet_length_state_init and then
** passed to every call of the length tap.
**
** \param packet_word_in  The input packet word.
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
void nanotube_tap_packet_length(
  /* Outputs. */
  struct nanotube_tap_packet_length_resp *resp_out,

  /* State. */
  struct nanotube_tap_packet_length_state *state_inout,

  /* Inputs. */
  const void *packet_word_in,
  const struct nanotube_tap_packet_length_req *req_in);

///////////////////////////////////////////////////////////////////////////

/*! Describes a request to the packet read tap. */
struct nanotube_tap_packet_read_req {
  /*! True if the request has been supplied. */
  bool valid;

  /*! The packet offset in bytes of start the read. */
  uint16_t read_offset;

  /*! The number of bytes to read. */
  uint16_t read_length;
};

/*! Describes a response from the packet read tap. */
struct nanotube_tap_packet_read_resp {
  /*! True if the response is valid.  This will be set after one call
   *  per packet. */
  bool valid;

  /*! The number of bytes read. */
  uint16_t result_length;
};

/*! The state passed between calls to the packet read tap. */
struct nanotube_tap_packet_read_state {
  /*! The length of the current packet (only used by softhub_bus) */
  uint16_t packet_length;
  
  /*! The current offset into the packet. */
  uint16_t packet_offset;

  /*! The rot_amount value to supply to nanotube_rotate_down. */
  uint16_t rotate_amount;

  /*! The current offset into the result buffer. */
  uint16_t result_offset;

  /*! A flag which indicates that the response has been produced. */
  uint8_t  done;

  /*! A flag which indicates that we've seen the DATA_EOP bit set for 
   *  this packet (only used by x3rx_bus) */
  uint8_t data_eop_seen;
};

/*! The initial state to pass to the packet read tap. */
static const nanotube_tap_packet_read_state
  nanotube_tap_packet_read_state_init = { 0, 0, 0, 0, 0, 0 };

/*! Read a series of bytes from a packet.
**
** \param result_buffer_length  The size of the result buffer in bytes.
** This must be a constant expression for synthesis.
**
** \param result_buffer_index_bits The number of bits required for an
** index into the result buffer.  This must be a constant expression
** for synthesis.
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
** \param packet_word_in  The input packet word.
**
** \param req_in A non-null pointer to the read request.  The request
** is only acted on if the "valid" member is set.  After "valid" has
** been set, the request structure must remain constant until after
** the end of packet.  The "valid" member must be set in the call for
** the word which contains the first byte to be read.
*/
void nanotube_tap_packet_read(
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

/*! Describes a request to the packet write tap. */
struct nanotube_tap_packet_write_req {
  /*! True if the request has been supplied. */
  bool valid;

  /*! The packet offset in bytes of the start of the write. */
  uint16_t write_offset;

  /*! The number of bytes to write. */
  uint16_t write_length;
};

/*! The state passed between calls to the packet write tap. */
struct nanotube_tap_packet_write_state {
  /*! The length of the current packet (only used by softhub_bus) */
  uint16_t packet_length;

  /*! The current offset into the packet. */
  uint16_t packet_offset;

  /*! The rot_amount value to supply to nanotube_rotate_down. */
  uint16_t rotate_amount;

  /*! The current offset into the request buffer. */
  uint16_t request_offset;

  /*! A flag which indicates that the response has been produced. */
  uint8_t  done;

  /*! A flag which indicates that we've seen the DATA_EOP bit set for 
   *  this packet (only used by x3rx_bus) */
  uint8_t data_eop_seen;
};

/*! The initial state to pass to the packet write tap. */
static const nanotube_tap_packet_write_state
  nanotube_tap_packet_write_state_init = { 0, 0, 0, 0, 0, 0 };

/*! Write a series of bytes to a packet with masking.
**
** \param request_buffer_length The size of the request buffer in
** bytes.  This must be a constant expression for synthesis.
**
** \param request_buffer_index_bits The number of bits required for an
** index into the request buffer.  This must be a constant expression
** for synthesis.
**
** \param packet_word_out The output packet word.  The packet
** length is not modified, so the end of packet indicators are the
** same as for the input packet, and the output buffer must hold
** packet_word_len bytes.
**
** \param state_inout An opaque state structure which should be
** initialised to nanotube_tap_packet_write_state_init and then passed
** to every call to the write tap.
**
** \param packet_word_in The input packet word.  Each call must
** supply packet_buffer_length packet bytes except the final call for
** a packet (with packet_word_eop set) which can be shorter.
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
void nanotube_tap_packet_write(
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

/*! A structure containing request parameters for the resize tap. */
typedef struct
{
  /*! The offset into the packet of the region to modify. */
  nanotube_tap_offset_t write_offset;

  /*! The number of bytes to delete. */
  nanotube_tap_offset_t delete_length;

  /*! The number of zero bytes to insert. */
  nanotube_tap_offset_t insert_length;
} nanotube_tap_packet_resize_req_t;

/*! A structure containing information about the associated input
 *  packet word. */
struct nanotube_tap_packet_resize_cword_t
{
  /*! The amount to rotate packet data by after the modified region. */
  nanotube_tap_offset_t packet_rot;

  /*! The number of bytes in the output word before the start of the
   *  modified region. */
  nanotube_tap_offset_t output_insert_start;

  /*! The number of bytes in the output word before the end of the
   *  modified region. */
  nanotube_tap_offset_t output_insert_end;

  /*! The number of bytes in the carried word before the start of the
   *  modified region. */
  nanotube_tap_offset_t carried_insert_start;

  /*! The number of bytes in the carried word before the end of the
   *  modified region. */
  nanotube_tap_offset_t carried_insert_end;

  /*! A flag which is set if the output word should contain bytes from
   *  the carried data and clear if the output word should contain
   *  unshifted bytes from the input word. */
  bool select_carried;

  /*! A flag which is set if there is at least one output word for
   *  this input word. */
  bool push_1;

  /*! A flag which is set if there are two output words for this input
   *  word. */
  bool push_2;

  /*! A flag which is set if this is the last input word of the
   *  packet. */
  bool eop;

  /*! The number of bytes in the last output word of this input
   *  word. */
  nanotube_tap_offset_t word_length;
};

/* NOTE: This struct has a non-zero initialiser! */
struct nanotube_tap_packet_resize_ingress_state_t
{
  /*! A flag which is set if the next input word is the first word of
   *  a packet. */
  bool new_req;

  /*! The length of the current (input) packet (only used by softhub_bus) */
  nanotube_tap_offset_t packet_length;

  /*! The current offset into the (input) packet. */
  nanotube_tap_offset_t packet_offset;

  /*! The amount to rotate packet data by after the modified
   *  region. */
  nanotube_tap_offset_t packet_rot_amount;

  /*! The number of bytes remaining in the unshifted region. */
  nanotube_tap_offset_t unshifted_length;

  /*! A flag which is set if the modified region has been reached. */
  bool edit_started;

  /*! The number of remaining bytes to be deleted. */
  nanotube_tap_offset_t edit_delete_length;

  /*! The number of remaining bytes to be inserted. */
  nanotube_tap_offset_t edit_insert_length;

  /*! The number of bytes carried from the edit stage to the shifted
   *  stage. */
  nanotube_tap_offset_t edit_carried_len;

  /*! A flag which is set if the shifted region has been reached. */
  bool shifted_started;

  /*! The number of bytes carried out of the shifted region into the
   *  next word. */
  nanotube_tap_offset_t shifted_carried_len;

  /*! A flag which indicates that we've seen the DATA_EOP bit set for 
   *  this packet (only used by x3rx_bus) */
  uint8_t data_eop_seen;
};


/* NOTE: This struct has a non-zero initialiser! */
struct nanotube_tap_packet_resize_egress_state_t
{
  /*! A flag which is set if the next call expects a new input
   *  word. */
  bool new_req;

  /*! A flag which is set if the next call expects a new input packet.
   *  It will be processing the first input and output word of a new packet
   *  (Currently only used by softhub_bus) */
  bool new_pkt;

  /*! The length of the current (input) packet (only used by softhub_bus) */
  uint16_t packet_length;

  /*! The current offset into the (input) packet. (only used by softhub_bus) */
  uint16_t packet_offset;
};



///////////////////////////////////////////////////////////////////////////

/*! \name nanotube_tap_packet_resize
**
**  The packet resize tap is used to insert bytes into and remove
**  bytes from a packet.  The inserted bytes are zeros and the removed
**  bytes are discarded.  To insert non-zero bytes, use a write tap
**  after the resize tap.  To examine removed bytes, use a read tap
**  before the resize tap.
**
**  The tap is split into two stages for performance reasons.  The
**  ingress stage must be called for each packet word.  Each call
**  accepts a packet word and the resize request for the packet.  The
**  request must be the same for all the words in the packet.  It
**  produces a control word which must be delivered to the egress
**  stage along side the corresponding packet word.
**
**  The egress stage must be called one or more times for each packet
**  word.  It indicates when it has finished processing one packet
**  word and expects the next call to be for the next packet word.
**
**  Both stages use a state structure to keep track of progress
**  through the packet.  The state structure must be initialised by
**  calling the corresponding init function from the Nanotube setup
**  function.
**/
/**@{*/

/*! The resize tap ingress stage.
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
** \param packet_word_in The input packet word.
**/
void nanotube_tap_packet_resize_ingress(
  /* Outputs. */
  bool *packet_done_out,
  nanotube_tap_packet_resize_cword_t *cword_out,
  nanotube_tap_offset_t *packet_length_out,

  /* State. */
  nanotube_tap_packet_resize_ingress_state_t *state,

  /* Inputs. */
  nanotube_tap_packet_resize_req_t *resize_req_in,
  void *packet_word_in);

/*! Initialise the resize tap ingress state.
**
** \param state A pointer to the state structure to initialise.
**/
void nanotube_tap_packet_resize_ingress_state_init(
  /* Outputs. */
  nanotube_tap_packet_resize_ingress_state_t *state);

/*! The resize tap egress stage.
**
** \param input_done_out A pointer to an output flag which will be set
**  when the packet word has been processed.  When set, the next call
**  should use the next packet word and control word.
**
** \param packet_done_out A pointer to an output flag which will be set if
**  the output packet word is the last word of the packet.  This flag is
**  only valid of word_valid_out is set.
**
** \param packet_valid_out A pointer to an output flag which will be set
**  if the stage has produced an output packet word.
**
** \param packet_word_out A pointer to an output buffer which will
**  contain the data bytes of the output packet word.  The buffer is
**  only valid of word_valid_out is set.
**
** \param state A pointer to the state buffer for the egress stage.
**  This buffer is used to track progress through the packet.
**
** \param state_packet_word Pointer to a packet-word sized buffer of static
**  storage for the tap.
**
** \param cword A pointer to an input buffer which contains the
** control word from the ingress stage.
**
** \param packet_word_in The input packet word.
** 
** \param new_packet_len The new packet length, as supplied by the 
** ingress resize tap (currently only used by softhub bus)
*/
void nanotube_tap_packet_resize_egress(
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

/*! Initialise the resize tap egress state.
**
** \param state A pointer to the state structure to initialise.
**/
void nanotube_tap_packet_resize_egress_state_init(
  /* Outputs. */
  nanotube_tap_packet_resize_egress_state_t *state);

/**@}*/

///////////////////////////////////////////////////////////////////////////
struct nanotube_tap_packet_eop_state {
  /*! The length of the current packet (only used by softhub_bus) */
  uint16_t packet_length;
  
  /*! The current offset into the packet. (only used by softhub_bus) */
  uint16_t packet_offset;
};

/*! The initial state to pass to the packet eop tap. */
static const nanotube_tap_packet_eop_state
  nanotube_tap_packet_eop_state_init = { 0, 0 };

/*! Generic prototpye for determining the end of packet.
**
** This function is just a prototype for returning the end of the packet;
** it will need to be specialised based on by the bus type by adding a
** suffix to the name, namely the one returned from get_bus_suffix(bus_type).
**
** \param packet_word_in A pointer to the packet in the format provided by
**                       the bus.
**
** \param state_inout Static storage for the tap
**/
bool nanotube_tap_packet_is_eop(
  const void           *packet_word_in,
  struct nanotube_tap_packet_eop_state *state_inout);

///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}

#include <type_traits>
// Simple macro that checks that the simple bus functions implement the
// same interface as the generic functions.
#define check_type(fn, suffix) \
  static_assert( \
    std::is_same< \
      decltype(fn ## suffix), \
      decltype(fn)>::value, \
    "Wrong type for " # fn # suffix );
#endif

#endif // PACKET_TAPS_H
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
