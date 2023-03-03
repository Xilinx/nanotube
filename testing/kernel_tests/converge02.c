/**
 * Test converging / coalescing of a variety of constructs, and also provide
 * some meat for the alias analysis.
 * Author: Stephan Diestelhorst <stephand@amd.com>
 * Date:   2021-01-13
 */

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include "nanotube_api.h"

typedef const uint8_t *u8p;

static const nanotube_map_id_t map_A = 0;
static const nanotube_map_id_t map_B = 1;
static const nanotube_map_id_t map_C = 2;
static const nanotube_map_id_t map_D = 3;

nanotube_kernel_rc_t very_simple_separate(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t sel1;
  nanotube_packet_read(packet, &sel1, 1, 1);
  if( sel1 > 127 ) {
    /* Read some data from a map */
    uint8_t val[8];
    nanotube_map_read(nt_ctx, map_A, (u8p)"foo", 3, val, 0, 8);
    nanotube_packet_write(packet, val, 32, 8);
  } else {
    /* Read some data from the packet */
    uint8_t val[8];
    nanotube_packet_read(packet, val, 20, 8);
    nanotube_packet_write(packet, val, 32, 8);
  }
  return NANOTUBE_PACKET_PASS;
}

#ifdef STEPHAN_ADDED_HANDLING_FOR_PHI_OF_ADDR_IN_CONVERGE
void very_simple_phi_of_addr(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t sel1;
  nanotube_packet_read(packet, &sel1, 1, 1);
  /* Note: Use one per-function local var, and one alloca to trip up the
   * optimiser */
  uint8_t val_map[8];
  uint8_t* p;
  if( sel1 > 127 ) {
    /* Read some data from a map */
    nanotube_map_read(nt_ctx, map_A, (u8p)"foo", 3, val_map, 0, 8);
    p = val_map;
  } else {
    uint8_t* val_packet = alloca(8);
    /* Read some data from the packet */
    nanotube_packet_read(packet, val_packet, 20, 8);
    p = val_packet;
  }
  nanotube_packet_write(packet, p, 32, 8);
}
#endif

nanotube_kernel_rc_t very_simple_memoryphi(nanotube_context_t *nt_ctx,
                                           nanotube_packet_t *packet) {
  uint8_t sel1;
  nanotube_packet_read(packet, &sel1, 1, 1);
  uint8_t val[8];
  if( sel1 > 127 ) {
    /* Read some data from a map */
    nanotube_map_read(nt_ctx, map_A, (u8p)"foo", 3, val, 0, 8);
  } else {
    /* Read some data from the packet */
    nanotube_packet_read(packet, val, 20, 8);
  }
  nanotube_packet_write(packet, val, 32, 8);
  return NANOTUBE_PACKET_PASS;
}

/**
 * This function illustrates the case where there is a phi of the pointer,
 * but no MemoryPhi for the Def of the packet_write.  That should be
 * obvious from the name of the function: Look mom, no MemoryPhi! */
nanotube_kernel_rc_t very_simple_lmnmp(nanotube_context_t *nt_ctx,
                                       nanotube_packet_t *packet) {
  uint8_t sel1;
  nanotube_packet_read(packet, &sel1, 1, 1);
  uint8_t* p;

  /* Read some data from a map */
  uint8_t val_map[8];
  nanotube_map_read(nt_ctx, map_A, (u8p)"foo", 3, val_map, 0, 8);
  if( sel1 > 127 )
    p = val_map;

  /* Read some data from the packet */
  uint8_t val_packet[8];
  nanotube_packet_read(packet, val_packet, 20, 8);
  if( sel1 <= 127 )
    p = val_packet;

  nanotube_packet_write(packet, p, 32, 8);
  return NANOTUBE_PACKET_PASS;
}

/**
 * Test a very straightforward combination of converging accesses on separate,
 * parallel basic blocks.
 */
nanotube_kernel_rc_t simple(nanotube_context_t *nt_ctx,
                            nanotube_packet_t *packet)
{

  /* Prepare some values with a long lifetime to ensure they get passed on
   * properly */
  uint8_t live_val = 0;
  uint8_t live_mem[4];
  nanotube_packet_read(packet, live_mem, 24, 4);
  live_val = live_mem[0] - 17;
  printf("Live val: %x live mem[3]: %x\n", (unsigned)live_val, (unsigned)live_mem[3]);

  /* Two parallel identical operationa, also have a merge oportunity, read
   * both 16 & 17, and just pull out one of the two */
  uint8_t sel0;
  uint8_t dat0;
  nanotube_packet_read(packet, &sel0, 0, 1);
  if( sel0 > 127)
    nanotube_packet_read(packet, &dat0, 16, 1);
  else
    nanotube_packet_read(packet, &dat0, 17, 1);
  printf("Sel: %x Data: %x\n", (unsigned)sel0, (unsigned)dat0);


  /* A more diverse set, we hope we can merge the first, and the last
   * operation */
  uint8_t sel1;
  uint8_t dat1;
  /* Also a rearrange oportunity, here. Move this one to the front, and
   * combine with the first. */
  nanotube_packet_read(packet, &sel1, 1, 1);
  if( sel1 > 127 ) {
    /* Read some data from a map */
    uint8_t val[8];
    nanotube_packet_read(packet, &dat1, 18, 1);
    nanotube_map_read(nt_ctx, map_A, (u8p)"foo", 3, val, 0, 8);
    /* Merge oportunity: consecutive writes */
    nanotube_packet_write(packet, val, 32, 4);
    nanotube_packet_write(packet, val + 4, 36, 4);
  } else {
    /* Read some data from the packet */
    uint8_t val[8];
    nanotube_packet_read(packet, &dat1, 19, 1);
    nanotube_packet_read(packet, val, 20, 8);
    /* Merge oportunity: consecutive writes */
    nanotube_packet_write(packet, val, 32, 4);
    nanotube_packet_write(packet, val + 4, 36, 4);
  }
  printf("Sel: %x Data: %x\n", (unsigned)sel1, (unsigned)dat1);

  /* Do something with the live values from the beginning */
  uint8_t buf[1];
  buf[0] = live_val;
  nanotube_map_write(nt_ctx, map_D, (u8p)"baz", 3, buf, 0, 1);
  nanotube_map_write(nt_ctx, map_D, (u8p)"baz", 3, live_mem, 1, 4);

  return NANOTUBE_PACKET_PASS;
}

/**
 * Similar to simple, but this time around, use a single memory location
 * that has different stores in the different paths.
 */
nanotube_kernel_rc_t simple_memory_phi(nanotube_context_t *nt_ctx,
                                       nanotube_packet_t *packet)
{
  /* Prepare some values with a long lifetime to ensure they get passed on
   * properly */
  uint8_t live_val = 0;
  uint8_t live_mem[4];
  nanotube_packet_read(packet, live_mem, 24, 4);
  live_val = live_mem[0] - 17;
  printf("Live val: %x live mem[3]: %x\n", (unsigned)live_val, (unsigned)live_mem[3]);

  /* Two parallel identical operationa, also have a merge oportunity, read
   * both 16 & 17, and just pull out one of the two */
  uint8_t sel0;
  uint8_t dat0;
  nanotube_packet_read(packet, &sel0, 0, 1);
  if( sel0 > 127)
    nanotube_packet_read(packet, &dat0, 16, 1);
  else
    nanotube_packet_read(packet, &dat0, 17, 1);
  printf("Sel: %x Data: %x\n", (unsigned)sel0, (unsigned)dat0);


  /* A more diverse set, we hope we can merge the first, and the last
   * operation */
  uint8_t sel1;
  uint8_t dat1;
  /* Also a rearrange oportunity, here. Move this one to the front, and
   * combine with the first. */
  nanotube_packet_read(packet, &sel1, 1, 1);
  uint8_t val[8];
  if( sel1 > 127 ) {
    /* Read some data from a map */
    nanotube_packet_read(packet, &dat1, 18, 1);
    nanotube_map_read(nt_ctx, map_A, (u8p)"foo", 3, val, 0, 8);
  } else {
    /* Read some data from the packet */
    nanotube_packet_read(packet, &dat1, 19, 1);
    nanotube_packet_read(packet, val, 20, 8);
  }
  /* Merge oportunity: consecutive writes */
  nanotube_packet_write(packet, val, 32, 4);
  nanotube_packet_write(packet, val + 4, 36, 4);
  printf("Sel: %x Data: %x\n", (unsigned)sel1, (unsigned)dat1);

  /* Do something with the live values from the beginning */
  uint8_t buf[1];
  buf[0] = live_val;
  nanotube_map_write(nt_ctx, map_D, (u8p)"bay", 3, buf, 0, 1);
  nanotube_map_write(nt_ctx, map_D, (u8p)"bay", 3, live_mem, 1, 4);

  return NANOTUBE_PACKET_PASS;
}

nanotube_kernel_rc_t partial_memory_phi(nanotube_context_t *nt_ctx,
                                        nanotube_packet_t *packet)
{
  /* Prepare some values with a long lifetime to ensure they get passed on
   * properly */
  uint8_t live_val = 0;
  uint8_t live_mem[4];
  nanotube_packet_read(packet, live_mem, 24, 4);
  live_val = live_mem[0] - 17;
  printf("Live val: %x live mem[3]: %x\n", (unsigned)live_val, (unsigned)live_mem[3]);

  /* Two parallel identical operationa, also have a merge oportunity, read
   * both 16 & 17, and just pull out one of the two */
  uint8_t sel0;
  uint8_t dat0;
  nanotube_packet_read(packet, &sel0, 0, 1);
  if( sel0 > 127)
    nanotube_packet_read(packet, &dat0, 16, 1);
  else
    nanotube_packet_read(packet, &dat0, 17, 1);
  printf("Sel: %x Data: %x\n", (unsigned)sel0, (unsigned)dat0);


  /* A more diverse set, we hope we can merge the first, and the last
   * operation */
  uint8_t sel1;
  uint8_t dat1;
  /* Also a rearrange oportunity, here. Move this one to the front, and
   * combine with the first. */
  nanotube_packet_read(packet, &sel1, 1, 1);
  uint8_t val[8];
  if( sel1 > 127 ) {
    /* Read some data from a map */
    nanotube_packet_read(packet, &dat1, 18, 1);
    nanotube_map_read(nt_ctx, map_A, (u8p)"foo", 3, val, 0, 8);
    val[7]++;
  } else {
    /* Read some data from the packet */
    nanotube_packet_read(packet, &dat1, 19, 1);
    nanotube_packet_read(packet, val, 20, 8);
    val[5]++;
  }
  /* Merge oportunity: consecutive writes */
  nanotube_packet_write(packet, val, 32, 4);
  nanotube_packet_write(packet, val + 4, 36, 4);
  printf("Sel: %x Data: %x\n", (unsigned)sel1, (unsigned)dat1);

  /* Do something with the live values from the beginning */
  uint8_t buf[1];
  buf[0] = live_val;
  nanotube_map_write(nt_ctx, map_D, (u8p)"bax", 3, buf, 0, 1);
  nanotube_map_write(nt_ctx, map_D, (u8p)"bax", 3, live_mem, 1, 4);

  return NANOTUBE_PACKET_PASS;
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

  nanotube_add_plain_packet_kernel("simple", simple, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("simple_memory_phi", simple_memory_phi, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("partial_memory_phi", partial_memory_phi, -1, 0 /*false*/);

  nanotube_add_plain_packet_kernel("very_simple_separate", very_simple_separate, -1, 0 /*false*/);
#ifdef STEPHAN_ADDED_HANDLING_FOR_PHI_OF_ADDR_IN_CONVERGE
  nanotube_add_plain_packet_kernel("very_simple_phi_of_addr", very_simple_phi_of_addr, -1, 0 /*false*/);
#endif
  nanotube_add_plain_packet_kernel("very_simple_memoryphi", very_simple_memoryphi, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("very_simple_lmnmp", very_simple_lmnmp, -1, 0 /*false*/);
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
