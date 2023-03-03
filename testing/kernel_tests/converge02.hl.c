/**
 * Test converging / coalescing of a variety of constructs, and also provide
 * some meat for the alias analysis -- highlevel API version.
 * Author: Stephan Diestelhorst <stephand@amd.com>
 * Date:   2021-01-13
 */

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nanotube_api.h"

typedef const uint8_t *u8p;

static const nanotube_map_id_t map_A = 0;
static const nanotube_map_id_t map_B = 1;
static const nanotube_map_id_t map_C = 2;
static const nanotube_map_id_t map_D = 3;

/**
 * There is a limitation in the Nanotube compiler that does not support
 * mixed-origin pointers (coming from potentially either packet or map),
 * and the optimiser might introduce them through CSE in the example,
 * below.  Use this *VERY UGLY HACK* to make the pain go away for a bit :)
 */
void force_memory_present_and_scare_the_optimiser(void *mem) {
  asm ("":"=m"(mem):"m"(mem));
}

nanotube_kernel_rc_t very_simple_separate(nanotube_context_t *nt_ctx,
                                          nanotube_packet_t *packet) {
  uint8_t* pdat = nanotube_packet_data(packet);
  uint8_t* pend = nanotube_packet_end(packet);
  if( pend - pdat < 40 )
    goto out;

  uint8_t sel1;
  sel1 = pdat[1];
  if( sel1 > 127 ) {
    /* Read some data from a map */
    uint64_t val;
    uint8_t* mapval = nanotube_map_lookup(nt_ctx, map_A, (u8p)"foo", 3, 8);
    val = *(uint64_t*)mapval;
    force_memory_present_and_scare_the_optimiser(&val);
    *(uint64_t*)(&pdat[32]) = val;
  } else {
    /* Read some data from the packet */
    uint64_t val;
    val = *(uint64_t*)(&pdat[20]);
    force_memory_present_and_scare_the_optimiser(&val);
    *(uint64_t*)(&pdat[32]) = val;
  }

out:
  return NANOTUBE_PACKET_PASS;
}

#ifdef STEPHAN_ADDED_HANDLING_FOR_PHI_OF_ADDR_IN_CONVERGE
nanotube_kernel_rc_t very_simple_phi_of_addr(nanotube_context_t *nt_ctx,
                                             nanotube_packet_t *packet) {
  uint8_t* pdat = nanotube_packet_data(packet);
  uint8_t* pend = nanotube_packet_end(packet);
  if( pend - pdat < 40 )
    goto out;

  uint8_t sel1;
  sel1 = pdat[1];
  uint64_t* p;
  if( sel1 > 127 ) {
    /* Read some data from a map */
    uint64_t val;
    uint8_t* mapval = nanotube_map_lookup(nt_ctx, map_A, (u8p)"foo", 3, 8);
    val = *(uint64_t*)mapval;
    p = &val;
    force_memory_present_and_scare_the_optimiser(&val);
  } else {
    /* Read some data from the packet */
    uint64_t val;
    val = *(uint64_t*)(&pdat[20]);
    force_memory_present_and_scare_the_optimiser(&val);
    p = &val;
  }
  *(uint64_t*)(&pdat[32]) = *p;

out:
  return NANOTUBE_PACKET_PASS;
}
#endif

nanotube_kernel_rc_t very_simple_memoryphi(nanotube_context_t *nt_ctx,
                                           nanotube_packet_t *packet) {
  uint8_t* pdat = nanotube_packet_data(packet);
  uint8_t* pend = nanotube_packet_end(packet);
  if( pend - pdat < 40 )
    goto out;

  uint8_t sel1;
  sel1 = pdat[1];
  uint64_t val;
  if( sel1 > 127 ) {
    /* Read some data from a map */
    uint8_t* mapval = nanotube_map_lookup(nt_ctx, map_A, (u8p)"foo", 3, 8);
    val = *(uint64_t*)mapval;
    force_memory_present_and_scare_the_optimiser(&val);
  } else {
    /* Read some data from the packet */
    val = *(uint64_t*)(&pdat[20]);
    force_memory_present_and_scare_the_optimiser(&val);
  }
  *(uint64_t*)(&pdat[32]) = val;

out:
  return NANOTUBE_PACKET_PASS;
}

nanotube_kernel_rc_t very_simple_lmnmp(nanotube_context_t *nt_ctx,
                                       nanotube_packet_t *packet) {
  uint8_t* pdat = nanotube_packet_data(packet);
  uint8_t* pend = nanotube_packet_end(packet);
  if( pend - pdat < 40 )
    goto out;

  uint8_t sel1;
  sel1 = pdat[1];
  uint64_t* p;

  /* Read some data from a map */
  uint64_t val_map;
  uint8_t* mapval = nanotube_map_lookup(nt_ctx, map_A, (u8p)"foo", 3, 8);
  val_map = *(uint64_t*)mapval;
  force_memory_present_and_scare_the_optimiser(&val_map);
  if( sel1 > 127 )
    p = &val_map;

  /* Read some data from the packet */
  uint64_t val_packet;
  val_packet = *(uint64_t*)(&pdat[20]);
  force_memory_present_and_scare_the_optimiser(&val_packet);
  if( sel1 <= 127 )
    p = &val_packet;

  *(uint64_t*)(&pdat[32]) = *p;

out:
  return NANOTUBE_PACKET_PASS;
}
/**
 * Test a very straightforward combination of converging accesses on separate,
 * parallel basic blocks.
 */
nanotube_kernel_rc_t simple(nanotube_context_t *nt_ctx, 
                            nanotube_packet_t *packet)
{
  uint8_t* pdat = nanotube_packet_data(packet);
  uint8_t* pend = nanotube_packet_end(packet);
  if( pend - pdat < 40 )
    goto out;

  /* Prepare some values with a long lifetime to ensure they get passed on
   * properly */
  uint8_t live_val = 0;
  uint8_t live_mem[4];
  memcpy(live_mem, pdat + 24, 4);
  live_val = live_mem[0] - 17;
  printf("Live val: %x live mem[3]: %x\n", (unsigned)live_val, (unsigned)live_mem[3]);

  /* Two parallel identical operationa, also have a merge oportunity, read
   * both 16 & 17, and just pull out one of the two */
  uint8_t sel0;
  uint8_t dat0;
  sel0 = pdat[0];
  if( sel0 > 127)
    dat0 = pdat[16];
  else
    dat0 = pdat[17];
  printf("Sel: %x Data: %x\n", (unsigned)sel0, (unsigned)dat0);


  /* A more diverse set, we hope we can merge the first, and the last
   * operation */
  uint8_t sel1;
  uint8_t dat1;
  /* Also a rearrange oportunity, here. Move this one to the front, and
   * combine with the first. */
  sel1 = pdat[1];
  if( sel1 > 127 ) {
    /* Read some data from a map */
    uint8_t val[8];
    dat1 = pdat[18];
    uint8_t* mapval = nanotube_map_lookup(nt_ctx, map_A, (u8p)"foo", 3, 8);
    memcpy(val, mapval, 8);
    force_memory_present_and_scare_the_optimiser(val);
    /* Merge oportunity: consecutive writes */
    memcpy(pdat + 32, val, 4);
    memcpy(pdat + 36, val + 4, 4);
  } else {
    /* Read some data from the packet */
    uint8_t val[8];
    dat1 = pdat[19];
    memcpy(val, pdat + 20, 8);
    force_memory_present_and_scare_the_optimiser(val);
    /* Merge oportunity: consecutive writes */
    memcpy(pdat + 32, val, 4);
    memcpy(pdat + 36, val + 4, 4);
  }
  printf("Sel: %x Data: %x\n", (unsigned)sel1, (unsigned)dat1);

  /* Do something with the live values from the beginning */
  uint8_t* mapval = nanotube_map_lookup(nt_ctx, map_D, (u8p)"baz", 3, 8);
  mapval[0] = live_val;
  memcpy(mapval + 1, live_mem, 4);

out:
  return NANOTUBE_PACKET_PASS;
}

/**
 * Similar to simple, but this time around, use a single memory location
 * that has different stores in the different paths.
 */
nanotube_kernel_rc_t simple_memory_phi(nanotube_context_t *nt_ctx,
                                       nanotube_packet_t *packet)
{
  uint8_t* pdat = nanotube_packet_data(packet);
  uint8_t* pend = nanotube_packet_end(packet);
  if( pend - pdat < 40 )
    goto out;

  /* Prepare some values with a long lifetime to ensure they get passed on
   * properly */
  uint8_t live_val = 0;
  uint8_t live_mem[4];
  memcpy(live_mem, pdat + 24, 4);
  live_val = live_mem[0] - 17;
  printf("Live val: %x live mem[3]: %x\n", (unsigned)live_val, (unsigned)live_mem[3]);

  /* Two parallel identical operationa, also have a merge oportunity, read
   * both 16 & 17, and just pull out one of the two */
  uint8_t sel0;
  uint8_t dat0;
  sel0 = pdat[0];
  if( sel0 > 127)
    dat0 = pdat[16];
  else
    dat0 = pdat[17];
  printf("Sel: %x Data: %x\n", (unsigned)sel0, (unsigned)dat0);


  /* A more diverse set, we hope we can merge the first, and the last
   * operation */
  uint8_t sel1;
  uint8_t dat1;
  /* Also a rearrange oportunity, here. Move this one to the front, and
   * combine with the first. */
  sel1 = pdat[1];
  uint8_t val[8];
  if( sel1 > 127 ) {
    /* Read some data from a map */
    dat1 = pdat[18];
    uint8_t* mapval = nanotube_map_lookup(nt_ctx, map_A, (u8p)"foo", 3, 8);
    memcpy(val, mapval, 8);
    force_memory_present_and_scare_the_optimiser(val);
  } else {
    /* Read some data from the packet */
    dat1 = pdat[19];
    memcpy(val, pdat + 20, 8);
    force_memory_present_and_scare_the_optimiser(val);
  }
  /* Merge oportunity: consecutive writes */
  memcpy(pdat + 32, val, 4);
  memcpy(pdat + 36, val + 4, 4);
  printf("Sel: %x Data: %x\n", (unsigned)sel1, (unsigned)dat1);

  /* Do something with the live values from the beginning */
  // NOTE: Use the write API, as the key is not present
  nanotube_map_write(nt_ctx, map_D, (u8p)"bay", 3, &live_val, 0, 1);
  nanotube_map_write(nt_ctx, map_D, (u8p)"bay", 3, live_mem, 1, 4);

out:
  return NANOTUBE_PACKET_PASS;
}

nanotube_kernel_rc_t partial_memory_phi(nanotube_context_t *nt_ctx,
                                        nanotube_packet_t *packet)
{
  uint8_t* pdat = nanotube_packet_data(packet);
  uint8_t* pend = nanotube_packet_end(packet);
  if( pend - pdat < 40 )
    goto out;

  /* Prepare some values with a long lifetime to ensure they get passed on
   * properly */
  uint8_t live_val = 0;
  uint8_t live_mem[4];
  memcpy(live_mem, pdat + 24, 4);
  live_val = live_mem[0] - 17;
  printf("Live val: %x live mem[3]: %x\n", (unsigned)live_val, (unsigned)live_mem[3]);

  /* Two parallel identical operationa, also have a merge oportunity, read
   * both 16 & 17, and just pull out one of the two */
  uint8_t sel0;
  uint8_t dat0;
  sel0 = pdat[0];
  if( sel0 > 127)
    dat0 = pdat[16];
  else
    dat0 = pdat[17];
  printf("Sel: %x Data: %x\n", (unsigned)sel0, (unsigned)dat0);


  /* A more diverse set, we hope we can merge the first, and the last
   * operation */
  uint8_t sel1;
  uint8_t dat1;
  /* Also a rearrange oportunity, here. Move this one to the front, and
   * combine with the first. */
  sel1 = pdat[1];
  uint8_t val[8];
  if( sel1 > 127 ) {
    /* Read some data from a map */
    dat1 = pdat[18];
    uint8_t* mapval = nanotube_map_lookup(nt_ctx, map_A, (u8p)"foo", 3, 8);
    memcpy(val, mapval, 8);
    val[7]++;
    force_memory_present_and_scare_the_optimiser(val);
  } else {
    /* Read some data from the packet */
    dat1 = pdat[19];
    memcpy(val, pdat + 20, 8);
    val[5]++;
    force_memory_present_and_scare_the_optimiser(val);
  }
  /* Merge oportunity: consecutive writes */
  memcpy(pdat + 32, val, 4);
  memcpy(pdat + 36, val + 4, 4);
  printf("Sel: %x Data: %x\n", (unsigned)sel1, (unsigned)dat1);

  /* Do something with the live values from the beginning */
  // NOTE: Use the write API, as the key is not present
  nanotube_map_write(nt_ctx, map_D, (u8p)"bax", 3, &live_val, 0, 1);
  nanotube_map_write(nt_ctx, map_D, (u8p)"bax", 3, live_mem, 1, 4);

out:
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
