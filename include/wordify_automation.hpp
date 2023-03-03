/**************************************************************************\
*//*! \file wordify_automation.hpp
** \author  Stephan Diestelhorst <stephand@amd.com>
**  \brief  Manual example for the pipeline / wordify pass.
**   \date  2021-02-15
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/* Helper state that needs to be kept around */
/* The tap state buffers state of the packet tap between words, for example
 * for accessing data in the second word, or straddling multiple words. */
#define DEF_PKT_RD_TAP_STATE \
  static nanotube_tap_packet_read_state state = \
    nanotube_tap_packet_read_state_init
#define DEF_PKT_WR_TAP_STATE \
  static nanotube_tap_packet_write_state state = \
    nanotube_tap_packet_write_state_init

/* The result of the packet read; needs to stick around in case multiple
 * words are needed to construct it! */
#define DEF_STATIC_APP_BUF(n_bytes, n_bits) \
  static uint8_t app_packet_buf[n_bytes]; \
  static const uint8_t buf_index_bits = n_bits
#define DEF_APP_BUF(n_bytes, n_bits) \
  uint8_t app_packet_buf[n_bytes]; \
  const uint8_t buf_index_bits = n_bits
#define APP_BUF app_packet_buf

/* Similarly, the packet_word might have to survive multiple loop iterations,
 * in case the code ends up looping multiple times waiting for the
 * app_in_state.
 * XXX: An alternative is to always process packet words, which needs us
 * waiting for the application state, first, and then keeping it around for the
 * rest of the packet
 *
 * XXX SD: I think this is wrong, at least with the recent restructuring of the
 * code. The code now waits for the app state first, before even starting to
 * read from the packet stream.  That way, the moment the kernel pops the first
 * packet word, it must have all app state ready and can then pipe through the
 * packet words without having to keep them around for any future iterations.
 * This needs some thought, but I think I can do without keeping the packet
 * word static. The read tap will have its own static buffer in case it has a
 * packet boundary straddling read .*/
#define DEF_PKT_WORD \
  static simple_bus::word packet_word;

#define DEF_APP_CHANNEL_FLAG \
  static bool have_app_state = false;

/*
 * Map reads / writes are handled like app state: at the end of the stage when
 * the application logic has run successfully, send a map access request to the
 * special map channel.  In the next stage, wait for a response from the map
 * response channel.  The key synchornisation challenges are the same:
 * - make sure the request is only sent out once per packet (not per packet word!)
 * - make sure the request is only sent out if the application code ran successfully
 * - make sure the map response is read once per packet
 * - make sure the map response is read before the application code is run
 * - make sure the map response is present for every packet word
 *
 * That means the map request does not have to be static (but may be
 * constructed from a static packet read buffer!), but the map response buffer
 * must be static.  Also, the map request send either needs to be guarded by
 * some once-per-packet trigger (a packet_read succeeding), or must have its
 * own trigger (or could also use the app_state_sent trigger!).
 *
 * Actually: the map interface does not have predefined structs that have the
 * request / response structure in a fixed layout.  Instead, they are simple
 * function parameters, and the request sinks the required info (key, data, map
 * parameters, op) as parameters and the response only returns the value read!
 */
#define DEF_MAP_CHANNEL_FLAG \
  static bool have_map_resp = false;



#define DEFS_PKT_NOAPP_READ_STAGE(size, size_bits) \
  DEF_PKT_WORD; \
  DEF_PKT_RD_TAP_STATE; \
  DEF_STATIC_APP_BUF(size, size_bits);

#define DEFS_PKT_READ_STAGE(size, size_bits, type) \
  DEFS_PKT_NOAPP_READ_STAGE(size, size_bits) \
  DEF_APP_CHANNEL_FLAG; \
  DEF_IN_STATE(type);

#define DEFS_PKT_WRITE_STAGE(type) \
  DEF_PKT_WR_TAP_STATE; \
  DEF_PKT_WORD; \
  DEF_APP_CHANNEL_FLAG; \
  DEF_IN_STATE(type);

/**
 * NOTE: There is a careful ordering requirement between the application state
 * and the packet words; the conservative assumption is that the application
 * data is produced late by the previous stage and needed for an access in the
 * first packet word.  Without the application state available, we don't
 * necessarily know where the packet access should happen.
 *
 * In some special cases, we *might* be able to send packet words along without
 * having app state if we know that the access cannot be to the first word.
 * That requires careful dependency analysis, however, so the safe assumption
 * is to make sure that the kernel has the application state with the first
 * packet word.  If either is missing, wait, and only start the computation
 * (and forward packet words!) once both are available.
 *
 * The algorith is as follows:
 * (1) if we don't have app state: try and read application state, wait &
 *     returning if not read, if read keep in static variable
 * (2) (when we get here, we have app state!) try and read a packet word,
 *     return if nothing is there
 * (3) if this was the EOP: set a flag to read the app state again on the next round
 * (4) perform application logic including taps
 * (5) forward the packet word
 * (6) loop around again
 */
#define TRY_POP_CHANNEL(id, dest) \
  if( !nanotube_channel_try_read(context, id,     \
                                &dest,            \
                                sizeof(dest)) ) { \
    nanotube_thread_wait();                       \
    return;                                       \
  }
#define TRY_POP_PKT_CHANNEL TRY_POP_CHANNEL(PACKETS_IN, packet_word)

#define ON_EOP_CLEAR(var)              \
  {                                    \
    bool eop = packet_word.get_eop();  \
    if( eop )                          \
      var = false;                     \
  }
#define ON_EOP_CLEAR_APP_STATE ON_EOP_CLEAR(have_app_state)

// Need to pop the app state first to make sure that it is available before the
// first packet word, even if the app state is only produced late (say on the
// second packet word) by the previosu stage, we can do packet operations on
// the first word still.
#define TRY_POP_PKT_APP_CHANNEL          \
  if( !have_app_state) {                 \
    TRY_POP_CHANNEL(STATE_IN, in_state); \
    have_app_state = true;               \
  }                                      \
  TRY_POP_PKT_CHANNEL;                   \
  ON_EOP_CLEAR_APP_STATE;

/* Construct the packet read request from the application data. This is
 * literal translation of the packet_read arguments into a struct!  Rebuild
 * the request for every packet word.  This needs the required app state to
 * be constant or kept around (which it is). */
/* Read from the packet, collecting all requested data */
/* When we have all requested data (from the packet and the input state),
 * go back to the application logic and so something with it :) */
#define IF_READ_TAP_COMPLETES(offset, length)                   \
  nanotube_tap_packet_read_req req = {0};                       \
  req.read_offset = offset;                                     \
  req.read_length = length;                                     \
  req.valid       = true;                                       \
  nanotube_tap_packet_read_resp resp;                           \
  static_assert(sizeof(APP_BUF) <= (1U<<buf_index_bits),        \
                "Not enough index bits.");                      \
  nanotube_tap_packet_read_sb(                                  \
    sizeof(APP_BUF), buf_index_bits,                            \
    &resp, APP_BUF,                                             \
    &state,                                                     \
    &packet_word, &req);                   \
  if (resp.valid)

    /* Create a new output packet and copy over the meta-data from the input
     * packet.  The write tap does not adjust the size / introduce errors, etc.
     */

#define WRITE_TAP(offset, length, mask) \
    nanotube_tap_packet_write_req req = {0};                \
    req.write_offset = offset;                              \
    req.write_length = length;                              \
    req.valid        = true;                                \
    simple_bus::word packet_word_out;                       \
    nanotube_tap_packet_write_sb(                           \
      sizeof(APP_BUF), buf_index_bits,                      \
      &packet_word_out,                                     \
      &state,                                               \
      &packet_word, &req,              \
      APP_BUF, mask);

/* The state coming from the previous pipeline stage.  This might have to
 * stay around for multiple loop iterations for multi-word packets, so pull
 * it out of the loop, and mark it static */
#define DEF_IN_STATE(type) \
  static type in_state = {0};
#define IN_STATE(field) in_state.field
#define FROM_PREV_STAGE(name) \
  typeof(IN_STATE(name)) name = IN_STATE(name)

/* Write the outgoing live state to the next pipeline stage, not static, as it
 * is written out in the same iteration as it is generated */
#define DEF_OUT_STATE(type) \
  type out_state = {0};
#define OUT_STATE(field) out_state.field
#define ADD_OUT_STATE(name) \
  OUT_STATE(name) = name

#define SEND_OUT_STATE \
  nanotube_channel_write(context, STATE_OUT, &out_state, \
                         sizeof(out_state))

/* Daisy-chain the packet word to the next stage */
#define CHAIN_PKT                                            \
  nanotube_channel_write(context, PACKETS_OUT, &packet_word, \
                         sizeof(packet_word));
#define CHAIN_MOD_PKT                                            \
  nanotube_channel_write(context, PACKETS_OUT, &packet_word_out, \
                         sizeof(packet_word_out));
