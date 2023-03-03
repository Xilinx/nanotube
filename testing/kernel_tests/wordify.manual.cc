/**************************************************************************\
*//*! \file wordify.manual.cc
** \author  Neil Turton <neilt@amd.com>,
**          Stephan Diestelhorst <stephand@amd.com>
**  \brief  Manual example for the pipeline / wordify pass.
**   \date  2021-02-15
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_api.h"
#include "nanotube_packet_taps.h"
#include "simple_bus.hpp"
#include "nanotube_packet_taps_sb.h"
#include "wordify_automation.hpp"

/**
 * The simple logic of this code is as follows:
 *
 * void process_packet(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
 *   uint8_t sel1;
 *   nanotube_packet_read(packet, &sel1, 0 + platform_offset, 1);
 *
 *   uint16_t next_read_offs;
 *   if( sel1 > 127 ) {
 *     next_read_offs = 1;
 *   } else {
 *     next_read_offs = 2;
 *   }
 *   uint8_t dat;
 *   nanotube_packet_read(packet, &dat, next_read_offs + platform_offset, 1);
 *   dat = ~dat;
 *
 *   nanotube_packet_write(packet, &dat, 3 + platform_offset, 1);
 * }
 *
 * and my aim to do a fairly simple 1:1 translation without merging packet read
 * taps for starters.  The code has been converged (and converted by the
 * platform pass), so there are no two nanotube_packet_reads in parallel
 * conditional blocks!  Also, pretend to be a "stupid" compiler and put the
 * packet write tap into a separate pipeline stage.
 *
 * As a result, the pipeline will look as follows:
 *
 * -- packets_in --> |-------| -- packets_0_to_1 --> |-------| -- packets_1_to_2 ..
 *                   |   0   |                       |   1   |
 *                   | pktrd |                       | pktrd |
 *                   |-------| -- state_0_to_1 ----> |-------| -- state_1_to_2 ..
 *
 * packets_1_to_2 --> |-------| -- packets_out -->
 *                    |   2   |
 *                    | ptkwr |
 * state_1_to_2 ----> |-------|
 */

enum channel {
  PACKETS_IN,
  PACKETS_OUT,
  STATE_IN,
  STATE_OUT,
};

struct live_state_0_to_1 {
  uint16_t app_next_read_offs;
} __attribute__((packed));

struct live_state_1_to_2 {
  uint8_t app_dat;
} __attribute__((packed));

static const uint16_t platform_offset = sizeof(simple_bus::header);

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
      uint8_t  sel1 = APP_BUF[0];
      uint16_t app_next_read_offs;
      if( sel1 > 127 )
        app_next_read_offs = 1;
      else
        app_next_read_offs = 2;
      /* NOTE: I am assuming here that the two packet reads on parallel
       * branches were converged together rather than stringed behind one
       * another */
    /* Application logic end */

    /* Prepare the outgoing live state */
    DEF_OUT_STATE(live_state_0_to_1);
    ADD_OUT_STATE(app_next_read_offs);
    SEND_OUT_STATE;
  }

  /* Always daisy-chain the packet word */
  CHAIN_PKT;
}

/********** Pipeline Stage 1 **********/
extern "C"
void logic_1(nanotube_context_t* context, void *arg)
{
  DEFS_PKT_READ_STAGE(1, 1, live_state_0_to_1);

  TRY_POP_PKT_APP_CHANNEL;

  /* NOTE: If we make it here, we know we have some application state from the
   * previous stage and a (BUT NOTE NECESSARILY THE RIGH!!!!!!111!!) packet
   * word */

  /* Explicit declaration of application state */
  FROM_PREV_STAGE(app_next_read_offs);
  const uint16_t app_read_length = 1;

  IF_READ_TAP_COMPLETES(app_next_read_offs + platform_offset,
                        app_read_length) {
    /* Application logic start */
      uint8_t app_dat = APP_BUF[0];
      app_dat = ~app_dat;
    /* Application logic end */

    DEF_OUT_STATE(live_state_1_to_2);
    ADD_OUT_STATE(app_dat);
    SEND_OUT_STATE;
  }
  CHAIN_PKT;
}

/********** Pipeline Stage 2 **********/
/* Packet writes are tricky, as the state needed to construct them might only
 * be available on a packet input word that is after the write location.  In
 * that case, the write needs to be in a separate stage and the channels will
 * buffer enough packet data. We stay safe here, and simply move the write to a
 * separate stage. */
extern "C"
void logic_2(nanotube_context_t* context, void *arg)
{
  DEFS_PKT_WRITE_STAGE(live_state_1_to_2);

  TRY_POP_PKT_APP_CHANNEL;

  /* Collect the data that needs to be written. This is pretty simple, but I
   * am being a bit verbose here thinking about what is application code and
   * what comes from the API.  Ultimately, all the data that is needed comes
   * either straight out of the state, or is a constant in the application.
   * There is a bit of flexibility here, we could decide to push more
   * application logic from the previous stage (after the read) into here; in
   * the extreme case, the previous stage just reads and delivers the read
   * data as in_state.  For now, lets front-load the pipeline. */
  FROM_PREV_STAGE(app_dat);

  /* Creating the packet_write request every time through the loop here. This
   * (as in the read case!) relies on the necessary state being buffered
   * (static) or constant, which again is the case.  If there was heavy logic
   * processing here in this stage, would there be a benefit to storing the
   * result of said computation rather than doing the computation again every
   * cycle? XXX: Discuss this :) */

  /* Application Logic Start */
  DEF_APP_BUF(1, 1);
  APP_BUF[0] = app_dat;
  const uint16_t app_write_offset = 3;
  const uint16_t app_write_length = 1;
  const uint8_t  app_write_mask   = 0xff;
  /* Application Logic End */
  WRITE_TAP(app_write_offset + platform_offset, app_write_length,
            &app_write_mask);
  /* Note: One can actually continue with application logic here, so don't
   * need to break the tap, here */
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

  /**
   * Channels are global objects but will be accessed with a per-context
   * numerical ID.  They are basically the ports where a specific channel
   * connects to a pipeline stage.  That (implicitly) requires that each thread
   * has its own context which is the expected way of connecting them. */

  /* Context / thread 0 wrapping pipeline stage 0 */
  nanotube_context_t *t0 = nanotube_context_create();
  nanotube_context_add_channel(t0, PACKETS_IN, packets_in,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t0, PACKETS_OUT, packets_0_to_1,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_context_add_channel(t0, STATE_OUT, state_0_to_1,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(t0, "logic_0", &logic_0, nullptr, 0);

  /* Context / thread 1 wrapping pipeline stage 1 */
  /* NOTE: We are reusing the PACKETS_IN and PACKETS_OUT ID, so that the thread
   * can be easily copy-pasted, and it makes it clear that they are
   * conceptually the same thing.  They do point to physically different
   * channels, however.  Those channels are 1:1, so daisy chaining requires
   * separate channels for each connection */
  nanotube_context_t *t1 = nanotube_context_create();
  nanotube_context_add_channel(t1, PACKETS_IN, packets_0_to_1,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t1, STATE_IN, state_0_to_1,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t1, PACKETS_OUT, packets_1_to_2,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_context_add_channel(t1, STATE_OUT, state_1_to_2,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(t1, "logic_1", &logic_1, nullptr, 0);

  /* Context / thread 2 wrapping pipeline stage 2 */
  nanotube_context_t *t2 = nanotube_context_create();
  nanotube_context_add_channel(t2, PACKETS_IN, packets_1_to_2,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t2, STATE_IN, state_1_to_2,
                               NANOTUBE_CHANNEL_READ);
  nanotube_context_add_channel(t2, PACKETS_OUT, packets_out,
                               NANOTUBE_CHANNEL_WRITE);
  nanotube_thread_create(t2, "logic_2", &logic_2, nullptr, 0);
}
