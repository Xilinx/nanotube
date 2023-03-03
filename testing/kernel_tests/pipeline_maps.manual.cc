/**************************************************************************\
*//*! \file pipeline_maps.manual.cc
** \author  Neil Turton <neilt@amd.com>,
**          Stephan Diestelhorst <stephand@amd.com>
**  \brief  Manual packet + map example for the pipeline pass.
**   \date  2021-02-15
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_api.h"
#include "nanotube_map_taps.h"
#include "nanotube_packet_taps.h"
#include "simple_bus.hpp"
#include "nanotube_packet_taps_bus.h"
#include "wordify_automation.hpp"

/**
 * The simple logic of this code can be be found in pipeline_maps.cc
 * and I will do a manual translation.  Questions are:
 * - the map access lives "between" pipeline stages, meaning that the previos
 *   stage ends with a map request and the next starts with the map response.
 *   This should make it possible (trivial?) to always colocate a map access
 *   with a packet access.  Do I want to do that straight away?  I think so.
 *   That means the pipeline stages are largely defined by the packet accesses.
 *   I cannot see an obvious benefit for the approach of splitting the pipeline
 *   again before / after the access.  Instead, I'd treat the map access
 *   channels like the (an additional) app state channel.
 * - the new map interface is different: instead of a struct, it takes
 *   individual items / pointers
 * - the error codes need coordinating
 * - all interfaces operate on cnt_context* + numerical ID, while the map
 *   interface expects a pointer to the map that comes from the setup function.
 *   Why?
 * - the map wrapper also does not offer mask functionality
 * - the sizes of the access also have vanished for the specific access; I
 *   think the idea is that we always know the map key and value size and we
 *   will always access the whole thing
 * - what is the point of having different input and output data sizes??
 * - does the tap example register the same map instance between diferrent
 *   pipeline stages? Yes, it looks like it.
 * - somehow the map tap does not find its receiving channel (ID = 2) what
 *   gives?  This seems hidden behind a lot of high-level magic boilerplate!
 *   I have the map_tap trying to read from channel ID 2 which is MAP_REQ.
 *   So the channel IDs per map tap should actually be coming from the client
 *   ID, rather than the enum, which makes sense: the request channel from
 *   which the tap reads should be 2 * client_num + 0 and the channel to which
 *   it writes should be 2 * client_num + 1.  So why would the tap then read
 *   from channel 2?!?!? => Ah the problem was that I declared having two
 *   clients, but only added one... that should be an assertion in
 *   map_finalise(..) -> checking code added and it works :)
 * - and now the the code complains because the map is a NULL pointer... maybe
 *   needs the setup function to pass the right map information -> yes, and
 *   also the right size of data copied => and one cannot simply pass a pointer
 *   to the map, because the content gets copied.. instead, the wrap into a
 *   structure is necessary, or rather copying the pointer as a value and
 *   storing it in a **
 * - the test fails, every other packet is missing some map data => looks like
 *   the insert operations failed
 *   - the first insert makes it to the map tap correctly
 *   - it looks like the entry is actually getting inserted
 *   - and the map state is looking good, too
 *   - the data is also properly read inthe CAM
 *   - maybe the write is screwed up??
 *   - the application does not seem to see / get the data in the app_dat location :(
 *   - the data makes it to nanotube_tap_map_func data_out
 *   - the data also makes it into the response channel, but it looks as follows:
 *     0x3, 0, 0, 0, 0xff, 0x01, 0xff, 0x00
 *   - the map response has a large prefix due to the response enum -> that
 *     could certainly be made smaller than 32 bit
 *   - the map response channel is only read from once, because the boolean
 *     flag is never reset on packet end marker :)
 *   - the sequencing of reading from which channel is tricky, once again.  The
 *     currentl placement of the map access puts it after the packet word read
 *     which is problematic as repeat map reads will read multiple packet
 *     words.  The other challenge is EOP handling!  Placing both (or all!)
 *     per-packet channel accesses (app state, map, etc) before the packet word
 *     access will make it safe, because we never unset a have_*_state early,
 *     only when it actually has been read and the packet word is also ready!
 * - now the test still fails because for further insert / update operations,
 *   it seems that the tap returns the written value rather than zeros in data
 *   out => yes, the map tap imlplementation always reads the old entry ->
 *   NANO-274
 * - the next issue comes from trying to read entries that are pre-populated
 *   from the *_maps.in files => it seems that there is no support for that for
 *   TAP-level maps at the moment and thus a read of a pre-populated entry
 *   fails -> NANO-273
 *
 *
 * As a result, the pipeline will look as follows:
 *
 * -- packets_in --> |-------| -- packets_0_to_1 --> |-------| -- packets_1_to_2 ..
 *                   |   0   |                       |   1   |
 *                   | pktrd |                       | pktrd | -- map req ..
 *                   |       |                       |       |
 *                   |-------| -- state_0_to_1 ----> |-------| -- state_1_to_2 ..
 *
 * packets_1_to_2 -------------------> +-------+ -- packets_out -->
 *              +-----+                |       |
 * map_req -->  | map | -- map_resp -> |   2   |
 *              +-----+                | ptkwr |
 * state_1_to_2 ---------------------> +-------+
 *
 */

enum channel {
  PACKETS_IN,
  PACKETS_OUT,
  MAP_REQ,
  MAP_RESP,
  STATE_IN,
  STATE_OUT,
};

struct live_state_0_to_1 {
  uint16_t app_next_read_offs;
  uint8_t app_sel1;
} __attribute__((packed));

struct live_state_1_to_2 {
  uint8_t app_sel1;
} __attribute__((packed));

static const uint16_t platform_offset = sizeof(simple_bus::header);

struct map_A_def {
  uint32_t key;
  uint32_t value;
};
//static const nanotube_map_id_t map_A = 0;

/********** Pipeline Stage 0 **********/
extern "C"
void logic_0(nanotube_context_t* context, void *arg)
{
  DEFS_PKT_NOAPP_READ_STAGE(1, 1);

  /* No application state, yet, just read a packet word */
  TRY_POP_PKT_CHANNEL;

  /* This stage has a constant offset / length packet read */
  const uint16_t app_read_offset = 0;
  const uint16_t app_read_length = 1;

  IF_READ_TAP_COMPLETES(app_read_offset + platform_offset, app_read_length) {
    /* Application logic start */
      uint8_t  app_sel1 = APP_BUF[0];
      uint16_t app_next_read_offs;
      if( app_sel1 > 127 )
        app_next_read_offs = 2;
      else
        app_next_read_offs = 1;
      /* NOTE: I am assuming here that the two packet reads on parallel
       * branches were converged together rather than stringed behind one
       * another */
    /* Application logic end */

    /* Prepare the outgoing live state */
    DEF_OUT_STATE(live_state_0_to_1);
    ADD_OUT_STATE(app_next_read_offs);
    ADD_OUT_STATE(app_sel1);
    SEND_OUT_STATE;
  }

  /* Always daisy-chain the packet word */
  CHAIN_PKT;
}

/********** Pipeline Stage 1 **********/
extern "C"
void logic_1(nanotube_context_t* context, void *arg)
{
  struct map_req_t {
    uint32_t key;
    uint32_t data;
    uint8_t  op;
  };

  DEFS_PKT_READ_STAGE(sizeof(map_req_t), 4, live_state_0_to_1);

  TRY_POP_PKT_APP_CHANNEL;

  /* NOTE: If we make it here, we know we have some application state from the
   * previous stage and a (BUT NOTE NECESSARILY THE RIGHT!!!!!!111!!) packet
   * word */

  /* Explicit declaration of application state */
  FROM_PREV_STAGE(app_next_read_offs);
  FROM_PREV_STAGE(app_sel1);
  const uint16_t app_read_length = sizeof(map_req_t);

  IF_READ_TAP_COMPLETES(app_next_read_offs + platform_offset,
                        app_read_length) {
    /* Application logic start */
    struct map_req_t& app_req = *(struct map_req_t*)APP_BUF;
    /* Application logic end */

    DEF_OUT_STATE(live_state_1_to_2);
    ADD_OUT_STATE(app_sel1);
    SEND_OUT_STATE;

    auto* map_A = *(nanotube_tap_map_t**)arg;
    //XXX: Ignoring the mask as the interface does not provide it.
    //XXX: Sizes have also disappeared so we need to make sure that the actual
    //     access in the normal code has the same sizes as the maps.  Or change
    //     the interfaces in the other interfaces.
    nanotube_tap_map_send_req(context, map_A, /*client_id*/0,
                              (enum map_access_t)app_req.op,
                              (uint8_t*)&app_req.key, (uint8_t*)&app_req.data);
  }
  CHAIN_PKT;
}

/********** Pipeline Stage 2 **********/
extern "C"
void logic_2(nanotube_context_t* context, void *arg)
{
  DEFS_PKT_WRITE_STAGE(live_state_1_to_2);

  if( !have_app_state) {
    TRY_POP_CHANNEL(STATE_IN, in_state);
    have_app_state = true;
  }

  /* Map read */
  //XXX: Automate the map read here
  static bool have_map_resp = false;
  static uint32_t app_dat;

  if (!have_map_resp) {
    nanotube_map_result_t result_out;
    auto* map_A = *(nanotube_tap_map_t**)arg;
    app_dat = 0x00223344;
    if (!nanotube_tap_map_recv_resp(context, map_A, /*client_id*/0,
                                    (uint8_t*)&app_dat, &result_out)) {
      nanotube_thread_wait();
      return;
    }
    have_map_resp = true;
  }

  TRY_POP_PKT_CHANNEL;
  ON_EOP_CLEAR_APP_STATE;
  ON_EOP_CLEAR(have_map_resp);
  FROM_PREV_STAGE(app_sel1);


  /* Application Logic Start */

  /* Computation on the data to test the correct taps and code synthesis */
  uint8_t app_chk = 0;
  app_chk ^=  app_dat        & 255;
  app_chk ^= (app_dat >>  8) & 255;
  app_chk ^= (app_dat >> 16) & 255;
  app_chk ^= (app_dat >> 24) & 255;
  struct write_data_t {
    uint32_t dat;
    uint8_t chk;
    uint8_t sel;
  };
  DEF_APP_BUF(sizeof(write_data_t), 3);
  auto& app_write_data = *(write_data_t*)APP_BUF;
  app_write_data.dat = app_dat;
  app_write_data.chk = app_chk;
  app_write_data.sel = ~app_sel1;
  /* Write the data back to the packet */
  const uint8_t app_one_mask = 0xff;
  /* Application Logic End */
  WRITE_TAP(3 + platform_offset, sizeof(write_data_t),
            &app_one_mask);
  CHAIN_MOD_PKT;
}

extern "C"
void nanotube_setup()
{
  /* Channels for packet data - these are daisy-chained */
  nanotube_channel_t *packets_in =
    nanotube_channel_create("packets_in", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_out =
    nanotube_channel_create("packets_out", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_0_to_1 =
    nanotube_channel_create("packets_0_to_1", simple_bus::total_bytes, 16);
  nanotube_channel_t *packets_1_to_2 =
    nanotube_channel_create("packets_1_to_2", simple_bus::total_bytes, 16);

  /* Channels for application state - how much buffering should they have? */
  nanotube_channel_t *state_0_to_1 =
    nanotube_channel_create("state_0_to_1", sizeof(live_state_0_to_1), 4);
  nanotube_channel_t *state_1_to_2 =
    nanotube_channel_create("state_1_to_2", sizeof(live_state_1_to_2), 4);

  if (simple_bus::sideband_bytes != 0) {
    nanotube_channel_set_attr(packets_in, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
    nanotube_channel_set_attr(packets_out, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
    nanotube_channel_set_attr(packets_0_to_1, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
    nanotube_channel_set_attr(packets_1_to_2, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES,
                              simple_bus::sideband_bytes);
  }

  /**
   * nanotube_channel_export marks the channels that communicate with the
   * outside world.  Channels used internally only will not be exported, here.
   */
  nanotube_channel_export(packets_in,
                          NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET,
                          NANOTUBE_CHANNEL_WRITE);
  nanotube_channel_export(packets_out,
                          NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET,
                          NANOTUBE_CHANNEL_READ);

  /* Declaring one context per thread / pipeline stage needed */
  nanotube_context_t *t0 = nanotube_context_create();
  nanotube_context_t *t1 = nanotube_context_create();
  nanotube_context_t *t2 = nanotube_context_create();

  /* Create channels between pipeline stages */
  /**
   * Channels are global objects but will be accessed with a per-context
   * numerical ID.  They are basically the ports where a specific channel
   * connects to a pipeline stage.  That (implicitly) requires that each thread
   * has its own context which is the expected way of connecting them. */

  /* Context / thread 0 wrapping pipeline stage 0 */
  nanotube_context_add_channel(t0, PACKETS_IN, packets_in,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t0, PACKETS_OUT, packets_0_to_1,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_context_add_channel(t0, STATE_OUT, state_0_to_1,
                               NANOTUBE_CHANNEL_WRITE);

  /* Context / thread 1 wrapping pipeline stage 1 */
  /* NOTE: We are reusing the PACKETS_IN and PACKETS_OUT ID, so that the thread
   * can be easily copy-pasted, and it makes it clear that they are
   * conceptually the same thing.  They do point to physically different
   * channels, however because the IDs are per context.  Those channels are
   * 1:1, so daisy chaining requires separate channels for each connection */
  nanotube_context_add_channel(t1, PACKETS_IN, packets_0_to_1,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t1, STATE_IN, state_0_to_1,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t1, PACKETS_OUT, packets_1_to_2,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_context_add_channel(t1, STATE_OUT, state_1_to_2,
                               NANOTUBE_CHANNEL_WRITE);

  /* Context / thread 2 wrapping pipeline stage 2 */
  nanotube_context_add_channel(t2, PACKETS_IN, packets_1_to_2,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t2, STATE_IN, state_1_to_2,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t2, PACKETS_OUT, packets_out,
                               NANOTUBE_CHANNEL_WRITE);

  /* Map setup, between context / thread / stage 1 -> 2 */
  const int key_size    = sizeof(map_A_def::key);
  const int value_size  = sizeof(map_A_def::value);
  const int capacity    = 4;
  const int num_clients = 1;
  auto* map = nanotube_tap_map_create(NANOTUBE_MAP_TYPE_HASH,
                                      key_size,
                                      value_size,
                                      capacity, num_clients);

  nanotube_tap_map_add_client(map, key_size, value_size, true, value_size,
                              t1, MAP_REQ, t2, MAP_RESP);
  nanotube_tap_map_build(map);

  /* Finally assign thread functions with the right arguments */
  nanotube_thread_create(t0, "logic_0", &logic_0, nullptr, 0);
  nanotube_thread_create(t1, "logic_1", &logic_1, &map, sizeof(map));
  nanotube_thread_create(t2, "logic_2", &logic_2, &map, sizeof(map));

}
