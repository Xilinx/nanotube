/**
 * A simple example that tests packet access conversion with a non-trivial
 * control flow.
 * Author: Stephan Diestelhorst <stephand@amd.com>
 * Date:   2020-02-10
 */

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>
#include "nanotube_api.h"


static const nanotube_map_id_t map_A = 0;
static const nanotube_map_id_t map_B = 1;
static const nanotube_map_id_t map_C = 2;
static const nanotube_map_id_t map_D = 3;

nanotube_kernel_rc_t cycle_switch_map_test(nanotube_context_t *nt_ctx,
                                           nanotube_packet_t *packet)
{
    uint8_t bogus;
    nanotube_packet_read(packet, &bogus, 42, 1);

    uint8_t  d[2] = {255, 255};
    typedef const uint8_t *u8p;
    switch (bogus) {
        case 0:
            nanotube_map_read(nt_ctx, map_A, (u8p)"foo", 3, &d[0], 0, 1);
            nanotube_map_read(nt_ctx, map_B, (u8p)"bar", 3, &d[1], 1, 1);
            break;
        case 1:
            // Different offsets and keys
            nanotube_map_read(nt_ctx, map_A, (u8p)"cod", 3, &d[0], 2, 1);
            nanotube_map_read(nt_ctx, map_B, (u8p)"wip", 3, &d[1], 3, 1);
            break;
        case 2:
            // Different order, too
            nanotube_map_read(nt_ctx, map_B, (u8p)"cat", 3, &d[1], 4, 1);
            nanotube_map_read(nt_ctx, map_A, (u8p)"hat", 3, &d[0], 5, 1);
            break;
        case 3:
            // Just a single access
            nanotube_map_read(nt_ctx, map_A, (u8p)"cab", 3, &d[0], 6, 1);
            break;
    }
    nanotube_map_write(nt_ctx, map_C, (u8p)"res", 3, d, 0, 2);
    return NANOTUBE_PACKET_PASS;
}

nanotube_kernel_rc_t dag_switch_test(nanotube_context_t *nt_ctx,
                                     nanotube_packet_t *packet)
{
    uint8_t bogus;
    uint8_t phi_test = 17;
    uint8_t  d[2] = {255, 255};
    nanotube_packet_read(packet, &bogus, 0, 1);
    goto test;
bar: ;

    typedef const uint8_t *u8p;
    switch (bogus) {
        case 0:
            nanotube_map_read(nt_ctx, map_A, (u8p)"foo", 3, &d[0], 0, 1);
            nanotube_map_read(nt_ctx, map_B, (u8p)"bar", 3, &d[1], 1, 1);
            phi_test = 42;
            break;
        case 1:
            // Different offsets and keys
            nanotube_map_read(nt_ctx, map_A, (u8p)"cod", 3, &d[0], 2, 1);
            nanotube_map_read(nt_ctx, map_B, (u8p)"wip", 3, &d[1], 3, 1);
            phi_test = 23;
            break;
        case 2:
            // Different order, too
            nanotube_map_read(nt_ctx, map_D, (u8p)"cat", 3, &d[1], 4, 1);
            nanotube_map_read(nt_ctx, map_A, (u8p)"hat", 3, &d[0], 5, 1);
            phi_test = 32;
            break;
        case 3:
            // Just a single access
            nanotube_map_read(nt_ctx, map_A, (u8p)"cab", 3, &d[0], 6, 1);
            phi_test = 1;
            break;
    }
    nanotube_map_write(nt_ctx, map_C, (u8p)"res", 3, d, phi_test, 2);
    return NANOTUBE_PACKET_PASS;

test:
    nanotube_packet_read(packet, &bogus, 42, 1);
    goto bar;
}


void nanotube_setup() {
  nanotube_context_t* ctx = nanotube_context_create();
  nanotube_map_t* map_A_  = nanotube_map_create(map_A,
                              NANOTUBE_MAP_TYPE_HASH, 3, 8);
  nanotube_context_add_map(ctx, map_A_);
  nanotube_map_t* map_B_  = nanotube_map_create(map_B,
                              NANOTUBE_MAP_TYPE_HASH, 3, 8);
  nanotube_context_add_map(ctx, map_B_);
  nanotube_map_t* map_C_  = nanotube_map_create(map_C,
                              NANOTUBE_MAP_TYPE_HASH, 3, 64);
  nanotube_context_add_map(ctx, map_C_);
  nanotube_map_t* map_D_  = nanotube_map_create(map_D,
                              NANOTUBE_MAP_TYPE_HASH, 3, 8);
  nanotube_context_add_map(ctx, map_D_);

  nanotube_add_plain_packet_kernel("cycle_switch_map_test", cycle_switch_map_test, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("dag_switch_test", dag_switch_test, -1, 0 /*false*/);
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
