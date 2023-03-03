/*******************************************************/
/*! \file test_hash_maps.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Unit tests for Nanotube hash map implementation.
**   \date 2020-03-04
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nanotube_api.h"
#include "nanotube_context.hpp"
#include "test.hpp"

static void print_bytes(uint8_t* data, size_t len) {
  for( size_t i = 0; i < len; ++i ) {
    printf("%02x ", data[i]);
    if( (i % 8) == 7 )
      printf("\n");
  }
}

/*!
** Sentinel / canary entries and separate map.
**
** The sentinel map and sentinel map entries serve as canaries to detect
** modifications of other maps / map entries than those explicitly manipulated
** in the test.  These modifications could occur due to bad pointer arithmetic
** in the map implementation, stray writes, and use-after-free.
**
** We create a set of sentinel keys + value and put those into the "work" map
** (under keys that are different than the "work" key from the test), and also
** use the same keys and values (in a different mapping) in a separate
** sentinel map.
**/
#define SENTINELS (10)            /*< Number of sentinel entries to use */
#define SENTINEL_MAP (0xfee7)     /*< ID of the sentinel map */
static nanotube_map_t* sentinel_map;

/* Golden keys and data for canary / sentinel checking */
static uint8_t sentinel_key[SENTINELS][8];
static uint8_t sentinel_data[SENTINELS][64];


/*!
** Generate sentinel / canary data.
**
** Generate sentinel constant data (keys & value) and insert into the work map
** specified by map, and also into a separate map.  Neither keys in the work
** map, nor the content of the separate map should be modified by the test
** application.
**
** \param map ID of the work map that is being tested
**/
static void add_sentinel_data(nanotube_context_t* ctx,
                              nanotube_map_id_t work_map) {
  int len;
  sentinel_map = nanotube_map_create(SENTINEL_MAP, NANOTUBE_MAP_TYPE_HASH,
                                     8, 64);
  nanotube_context_add_map(ctx, sentinel_map);

  for( int i = 0; i < SENTINELS; ++i ) {
    /* Generate "random" keys */
    for ( size_t j = 0; j < sizeof(sentinel_key[0]); ++j )
      sentinel_key[i][j] = 0x12 + i + j;
    /* Generate "random" values */
    for ( size_t j = 0; j < sizeof(sentinel_data[0]); ++j )
      sentinel_data[i][j] = i ^ j;
  }

  /* Add key->value to the work map and to the separate sentinel map */
  for ( int i = 0; i < SENTINELS; ++i ) {
    /* In work map: key[i] -> value[i] */
    len = nanotube_map_write(ctx, work_map,
              sentinel_key[i], sizeof(sentinel_key[0]),
              sentinel_data[i], 0, sizeof(sentinel_data[0]));
    assert_eq(len, sizeof(sentinel_data[0]));
    /* Reverse mapping for sentinel map */
    unsigned i_rev = SENTINELS - 1 - i;
    len = nanotube_map_write(ctx, SENTINEL_MAP,
              sentinel_key[i_rev], sizeof(sentinel_key[0]),
              sentinel_data[i], 0, sizeof(sentinel_data[0]));
    assert_eq(len, sizeof(sentinel_data[0]));
  }
}

/*!
** Check that all sentinel data is still intact.
**
** \param map ID of work map that contains sentinel entries.
**/
static void check_sentinel_data(nanotube_context_t* ctx,
                                nanotube_map_id_t work_map) {
  int len;
  uint8_t data[64];

  for( int i = 0; i < SENTINELS; ++i ) {
    /* Read and check data */
    len = nanotube_map_read(ctx, work_map,
              sentinel_key[i], sizeof(sentinel_key[0]),
              data, 0, sizeof(data));
    assert_eq(len, sizeof(data));
    assert_eq(memcmp(data, sentinel_data[i], sizeof(data)), 0);
    /* Reverse mapping for sentinel map */
    len = nanotube_map_read(ctx, SENTINEL_MAP,
              sentinel_key[i], sizeof(sentinel_key[0]),
              data, 0, sizeof(data));
    assert_eq(len, sizeof(data));
    unsigned i_rev = SENTINELS - 1 - i;
    assert_eq(memcmp(data, sentinel_data[i_rev], sizeof(data)), 0);
  }
}

/*!
** Remove sentinel data from supplied map.
**/
void remove_sentinel_data(nanotube_context_t* ctx, nanotube_map_id_t map) {
  size_t len;
  for( int i = 0; i < SENTINELS; ++i) {
    len = nanotube_map_remove(ctx, map, sentinel_key[i],
                              sizeof(sentinel_key[0]));
    assert_eq(len==0, 0);
  }
}

/*!
** Test code for testing maps.
**
** \return Number of errors found.
**/
static void test_maps(void) {
  printf("\n\n_Testing_Maps_\n");
  auto* ctx = new nanotube_context();

  const nanotube_map_id_t A_id = 42;
  nanotube_map_t* map_A;
  map_A = nanotube_map_create(A_id, NANOTUBE_MAP_TYPE_HASH, 8, 64);
  nanotube_context_add_map(ctx, map_A);
  add_sentinel_data(ctx, A_id);
  check_sentinel_data(ctx, A_id);

  uint8_t all_enabled[8];
  memset(all_enabled, 0xff, 8);

  uint8_t key[8]  = {0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed, 0xba, 0xcc};
  uint8_t data[64];
  for( size_t i = 0; i < sizeof(data); ++i )
    data[i] = i;
  size_t len;

  /* Map read / write testing */
  nanotube_map_t* maps[3];

  printf("Map input\n");
  for( int i = 0; i < 3; ++i ) {
    maps[i] = nanotube_map_create_from_stdin();
    printf("  Read map %hi from stdin\n", nanotube_map_get_id(maps[i]));
  }
  assert_eq(nanotube_map_get_id(maps[0]), 123);
  assert_eq(nanotube_map_get_id(maps[1]), 999);
  assert_eq(nanotube_map_get_id(maps[2]), NANOTUBE_MAP_ILLEGAL);

  printf("Map output\n");
  for( int i = 0; i < 3; ++i ) {
    if( maps[i] != NULL )
      nanotube_map_print(maps[i]);
  }

  /* Simple map write */
  len = nanotube_map_write_masked(ctx, A_id, key, sizeof(key), data,
                                  all_enabled, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes:\n", len);
  print_bytes(data, 64); printf("\n");
  assert_eq(len, 64);

  /* Testing map read of existing value */
  uint8_t data2[64];
  len = nanotube_map_read(ctx, A_id, key, sizeof(key), data2, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes:\n", len);
  print_bytes(data2, 64); printf("\n");
  assert_eq(memcmp(data, data2, 64), 0);

  /* Writing & reading from an offset */
  size_t pos = 16;
  data[pos]  = 0xab;
  len = nanotube_map_write(ctx, A_id, key, sizeof(key), data + pos, pos, 1);
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes at pos %zu value %02x.\n", len, pos, data[pos]);
  assert_eq(len, 1);

  pos = 10;
  len = nanotube_map_read(ctx, A_id, key, sizeof(key), data2 + pos, pos, 10);
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes with offset %zu:\n", len, pos);
  print_bytes(data2, 64); printf("\n");
  assert_eq(len, 10);
  assert_eq(data2[16], 0xab);

  /* Writing with enables & offset */
  uint8_t data_splat[32];
  memset(data_splat, 0xff, 32);
  /* Alternating, growing on/off pattern */
  uint8_t fancy_enable[4] = {/*00110010b*/  50, /*00111100b*/ 60,
                             /*11000000b*/ 192, /*00111111b*/ 63};
  pos = 32;
  len = nanotube_map_write_masked(ctx, A_id, key, sizeof(key), data_splat,
                                  fancy_enable, pos, sizeof(data_splat));
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes at pos %zu with fancy enable mask.\n", len, pos);
  assert_eq(len, sizeof(data_splat));

  pos = 0;
  len = nanotube_map_read(ctx, A_id, key, sizeof(key), data2 + pos, pos,
                          sizeof(data2));
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes with offset %zu:\n", len, pos);
  print_bytes(data2, 64); printf("\n");
  assert_eq(len, sizeof(data2));

  /* Illegal reads */
  /* Wrong map */
  nanotube_map_id_t illegal_id = 66;
  assert((illegal_id != A_id) && (illegal_id != SENTINEL_MAP));
  len = nanotube_map_read(ctx, illegal_id, key, sizeof(key), data2, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes from map %i\n", len, illegal_id);
  assert_eq(len, 0);

  /* No key */
  len = nanotube_map_read(ctx, A_id, NULL, sizeof(key), data2, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes from NULL key\n", len);
  assert_eq(len, 0);

  /* No data */
  len = nanotube_map_read(ctx, A_id, key, sizeof(key), NULL, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes to NULL data\n", len);
  assert_eq(len, 0);

  /* Missing key */
  uint8_t missing_key[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  assert_eq(memcmp(missing_key, key, sizeof(key)) != 0, 1);
  len = nanotube_map_read(ctx, A_id, missing_key, sizeof(missing_key), data2,
                          0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes from missing key\n", len);
  assert_eq(len, 0);

  /* Too big key */
  len = nanotube_map_read(ctx, A_id, key, sizeof(key) + 3, data2, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes from too big key\n", len);
  assert_eq(len, 0);

  /* Too small key */
  len = nanotube_map_read(ctx, A_id, key, sizeof(key) - 3, data2, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes from too small key\n", len);
  assert_eq(len, 0);

  /* Too much data */
  uint8_t huge_data[80] = {0};;
  len = nanotube_map_read(ctx, A_id, key, sizeof(key), huge_data, 0,
              sizeof(huge_data));
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes with request of %zu bytes\n", len,
         sizeof(huge_data));
  print_bytes(huge_data, sizeof(huge_data)); printf("\n");
  /* We expect to read one value if the return space is too big */
  assert_eq(len, 64);

  /* Offset too far */
  pos = 32;
  memset(data2, 0, sizeof(data2));
  len = nanotube_map_read(ctx, A_id, key, sizeof(key), data2, pos,
              sizeof(data2));
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes from offset %zu and requesting %zu bytes\n", len,
         pos, sizeof(data2));
  print_bytes(data2, sizeof(data2)); printf("\n");
  /* We (for now) expect to read as much as we can. */
  assert_eq(len, 32);
  /* Check that the overwritten 0xff's are still there */
  assert_eq(data2[1], 0xff);
  assert_eq(data2[4], 0xff);
  assert_eq(data2[5], 0xff);

  /* Illegal writes */
  /* Wrong map */
  len = nanotube_map_write_masked(ctx, illegal_id, key, sizeof(key), data,
                                  all_enabled, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes to map %i\n", len, illegal_id);
  assert_eq(len, 0);

  /* No key */
  len = nanotube_map_write_masked(ctx, A_id, NULL, sizeof(key), data,
                                  all_enabled, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes to NULL key\n", len);
  assert_eq(len, 0);

  /* No data */
  len = nanotube_map_write_masked(ctx, A_id, key, sizeof(key), NULL,
                                  all_enabled, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes of NULL data\n", len);
  assert_eq(len, 0);

  /* No enables */
  len = nanotube_map_write_masked(ctx, A_id, key, sizeof(key), data, NULL, 0,
                                  64);
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes with NULL enables\n", len);
  assert_eq(len, 0);

  /* Too big key */
  len = nanotube_map_write_masked(ctx, A_id, key, sizeof(key) + 3, data,
                                  all_enabled, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes to large key\n", len);
  assert_eq(len, 0);

  /* Too small key */
  len = nanotube_map_write_masked(ctx, A_id, key, sizeof(key) - 3, data,
                                  all_enabled, 0, 64);
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes to small key\n", len);
  assert_eq(len, 0);

  /* Too much data (due to offset) */
  pos = 5;
  len = nanotube_map_write_masked(ctx, A_id, key, sizeof(key), data,
                                  all_enabled, pos, 64);
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes to offset %zu with req size %u \n", len, pos, 64);
  assert_eq(len, 0);

  /* Too much data (due to length) */
  pos = 0;
  len = nanotube_map_write_masked(ctx, A_id, key, sizeof(key), huge_data,
                                  all_enabled, pos, sizeof(huge_data));
  check_sentinel_data(ctx, A_id);
  printf("Wrote %zu bytes with length %zu\n", len, sizeof(huge_data));
  assert_eq(len, 0);

  /* Test the different "special" writes: UPDATE and INSERT */
  /* UPDATE with a present key */
  pos = 0;
  memset(data, 0xf0, sizeof(data) / 2);
  len = nanotube_map_update_masked(ctx, A_id, key, sizeof(key), data + pos,
                                   all_enabled, pos, sizeof(data) / 2);
  check_sentinel_data(ctx, A_id);
  printf("Updated %zu bytes at pos %zu.\n", len, pos);
  assert_eq(len, sizeof(data) / 2);
  len = nanotube_map_read(ctx, A_id, key, sizeof(key), data2 + pos, pos,
                          sizeof(data2));
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes with offset %zu:\n", len, pos);
  print_bytes(data2, 64); printf("\n");
  assert_eq(len, sizeof(data2));
  assert_eq(data2[0], 0xf0);

  /* UPDATE with a non-existing key */
  ++key[5];
  pos = 0;
  len = nanotube_map_update_masked(ctx, A_id, key, sizeof(key), data + pos,
                                   all_enabled, pos, sizeof(data) / 2);
  check_sentinel_data(ctx, A_id);
  printf("Updated %zu bytes at pos %zu with missing key.\n", len, pos);
  assert_eq(len, 0);

  len = nanotube_map_update_masked(ctx, A_id, key, sizeof(key), data + pos,
                                   all_enabled, pos, sizeof(data) / 2);
  check_sentinel_data(ctx, A_id);
  printf("Re-Updated %zu bytes at pos %zu with missing key.\n", len, pos);
  assert_eq(len, 0);
  --key[5];

  /* INSERT with a non-existing key */
  ++key[5];
  pos = 0;
  len = nanotube_map_insert_masked(ctx, A_id, key, sizeof(key), data + pos,
                                   all_enabled, pos, sizeof(data) / 2);
  check_sentinel_data(ctx, A_id);
  printf("Inserted %zu bytes at pos %zu with new key.\n", len, pos);
  assert_eq(len, sizeof(data) / 2);
  len = nanotube_map_read(ctx, A_id, key, sizeof(key), data2 + pos, pos,
                          sizeof(data2));
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes with offset %zu:\n", len, pos);
  print_bytes(data2, 64); printf("\n");
  assert_eq(len, sizeof(data2));
  assert_eq(data2[0], 0xf0);

  /* INSERT with the same, now-existing key */
  len = nanotube_map_insert_masked(ctx, A_id, key, sizeof(key), data + pos,
                                   all_enabled, pos, sizeof(data) / 2);
  check_sentinel_data(ctx, A_id);
  printf("Re-inserted %zu bytes at pos %zu with (not so) new key.\n", len, pos);
  assert_eq(len, 0);

  /* Remove entry from the map */
  len = nanotube_map_remove(ctx, A_id, key, sizeof(key));
  printf("Removed newly inserted key, result %zu.\n", len);
  check_sentinel_data(ctx, A_id);
  assert_eq(len!=0, 1);

  /* And again .. remove entry from the map */
  len = nanotube_map_remove(ctx, A_id, key, sizeof(key));
  printf("Re-removed newly inserted key, result %zu.\n", len);
  check_sentinel_data(ctx, A_id);
  assert_eq(len, 0);

  /* Read from the removed key */
  len = nanotube_map_read(ctx, A_id, key, sizeof(key), data2 + pos, pos,
                          sizeof(data2));
  check_sentinel_data(ctx, A_id);
  printf("Read %zu bytes with offset %zu from removed key\n", len, pos);
  print_bytes(data2, 64); printf("\n");
  assert_eq(len, 0);

  /* And print out the final map */
  remove_sentinel_data(ctx, A_id);
  printf("Final map state:\n");
  nanotube_map_print(map_A);

  nanotube_map_destroy(map_A);
  nanotube_map_destroy(sentinel_map);
  delete ctx;
}

int main(int argc, char *argv[]) {
  test_init(argc, argv);
  test_maps();
  return test_fini();
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
