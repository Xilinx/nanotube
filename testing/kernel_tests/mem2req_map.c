/*******************************************************/
/*! \file  mem2req_map.c
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Test mem2req conversion for packet accesses.
**   \date 2020-08-04
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanotube_api.h"

typedef struct __attribute__((packed)) {
  uint8_t off8;
  uint8_t dat8;

  uint16_t off16;
  uint16_t dat16;

  uint32_t off32;
  uint32_t dat32;

  uint64_t off64;
  uint64_t dat64;

  uint8_t  array[8];
} value_t;

static const nanotube_map_id_t map_A = 0;

nanotube_kernel_rc_t map_read_test(nanotube_context_t *nt_ctx,
                                   nanotube_packet_t *packet)
{
  // uint8_t* data = nanotube_packet_data(packet);
  // uint8_t* end  = nanotube_packet_end(packet);

  // if( end - data < 64 )
  //   return -1;

  /* Create a writable key containing "foo" */
  char key[3];
  memcpy(key, "foo", 3);

  value_t* value =
    (value_t*)nanotube_map_lookup(nt_ctx, map_A, (uint8_t*)key, 3, sizeof(value_t));
  if( value == NULL ) {
    /* Introduce a phi node on the pointer */
    value = (value_t*)nanotube_map_lookup(nt_ctx, map_A, (uint8_t*)"bak",
                                          3, sizeof(value_t));
    if( value == NULL ) {
      printf("Did not find key foo or bak\n");
      return NANOTUBE_PACKET_DROP;
    }
  }

  /* Manipulate the key after lookup; copy must be taken at lookup time! */
  key[0] = 'b';

  /* Perform reads of the value */
  printf("8 bit off: %u data: %x\n", (unsigned)value->off8, (unsigned)value->dat8);
  printf("16 bit off: %hu data: %hx\n", value->off16, value->dat16);
  printf("32 bit off: %u data: %x\n",   value->off32, value->dat32);
  printf("64 bit off: %lu data: %lx\n", value->off64, value->dat64);

  /* Introduce a PHI node on the offset. */
  uint8_t  idx = 3;
  if( value->dat8 > 42)
    idx = (value->dat16 / 53 *  value->dat8) % 8;
  printf("8 bit idx %u data: %x\n", (unsigned)idx, (unsigned)value->array[idx]);

  value =
    (value_t*)nanotube_map_lookup(nt_ctx, map_A, (uint8_t*)"zzz", 3, sizeof(value_t));
  if( value == NULL ) {
    printf("Did not find key zzz\n");
    return NANOTUBE_PACKET_DROP;
  }

  printf("8 bit off: %u data: %x\n", (unsigned)value->off8, (unsigned)value->dat8);
  printf("16 bit off: %hu data: %hx\n", value->off16, value->dat16);
  printf("32 bit off: %u data: %x\n",   value->off32, value->dat32);
  printf("64 bit off: %lu data: %lx\n", value->off64, value->dat64);

  /* Try getting a select on the offset */
  idx = 5;
  if( value->dat8 > 42)
    idx = 7;
  printf("8 bit idx %u data: %x\n", (unsigned)idx, (unsigned)value->array[idx]);

  return NANOTUBE_PACKET_PASS;
}

nanotube_kernel_rc_t map_write_test(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet) {
  /* Create a writable key containing "foo" */
  char key[3];
  memcpy(key, "foo", 3);

  value_t* value =
    (value_t*)nanotube_map_lookup(nt_ctx, map_A, (uint8_t*)key, 3, sizeof(value_t));
  if( value == NULL ) {
    return NANOTUBE_PACKET_DROP;
  }

  /* Manipulate the key after lookup; copy must be taken at lookup time! */
  key[0] = 't';

  value->dat8  = 0xbb;
  value->dat16 = 0xf00f;
  value->dat32 = 0xcafeb00c;
  value->dat64 = 0x123456789abcdef;

  value =
    (value_t*)nanotube_map_lookup(nt_ctx, map_A, (uint8_t*)"zzz", 3, sizeof(value_t));
  if( value == NULL ) {
    printf("Did not find key zzz\n");
    return NANOTUBE_PACKET_DROP;
  }

  value->dat8  = 0xcc;
  value->dat16 = 0xc00c;
  value->dat32 = 0xaffeb00c;
  value->dat64 = 0xfedcba987654321;

  return NANOTUBE_PACKET_PASS;
}

nanotube_kernel_rc_t map_memset_memcpy_test(nanotube_context_t *nt_ctx,
                                            nanotube_packet_t *packet) {
  value_t* value =
    (value_t*)nanotube_map_lookup(nt_ctx, map_A, (uint8_t*)"foo", 3, sizeof(value_t));

  size_t len = 17;
  size_t off = 3;
  uint8_t* tmp = alloca(len);

  /* Create a backup copy of the zapped region */
  memcpy(tmp, (uint8_t*)value + off, len);
  /* Zap the region */
  memset((uint8_t*)value + off, 0x77, len);
  /* And do some intra-region copies */
  memcpy(&value->dat16, &value->dat64, 6);

  /* Investigate */
  printf("8 bit off: %u data: %x\n", (unsigned)value->off8, (unsigned)value->dat8);
  printf("16 bit off: %hu data: %hx\n", value->off16, value->dat16);
  printf("32 bit off: %u data: %x\n", value->off32, value->dat32);
  printf("64 bit off: %lu data: %lx\n", value->off64, value->dat64);

  /* And restore */
  memcpy((uint8_t*)value + off, tmp, len);
  printf("8 bit off: %u data: %x\n", (unsigned)value->off8, (unsigned)value->dat8);
  printf("16 bit off: %hu data: %hx\n", value->off16, value->dat16);
  printf("32 bit off: %u data: %x\n", value->off32, value->dat32);
  printf("64 bit off: %lu data: %lx\n", value->off64, value->dat64);

  return NANOTUBE_PACKET_PASS;
}

void nanotube_setup() {
  nanotube_context_t* ctx = nanotube_context_create();
  nanotube_map_t* map_A_  = nanotube_map_create(map_A,
                              NANOTUBE_MAP_TYPE_HASH, 3, sizeof(value_t));
  nanotube_context_add_map(ctx, map_A_);

  nanotube_add_plain_packet_kernel("map_read_test", map_read_test, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("map_write_test", map_write_test, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("map_read_test2", map_read_test, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("map_memset_memcpy_test", map_memset_memcpy_test, -1, 0 /*false*/);
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
