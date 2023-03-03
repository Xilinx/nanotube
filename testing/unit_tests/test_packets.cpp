/*******************************************************/
/*! \file test_packets.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Unit tests for Nanotube packet implementations.
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

#include "nanotube_packet.hpp"
#include "nanotube_api.h"

static void print_bytes(uint8_t* data, size_t len) {
  for( size_t i = 0; i < len; ++i ) {
    printf("%02x ", data[i]);
    if( (i % 8) == 7 )
      printf("\n");
  }
}


/*!
** Allocate a new packet of specified size.
** \param len Size of the packet in bytes.
** \return Pointer to a freshly allocated packet.
**/
nanotube_packet_t *malloc_pkt(size_t len) {
  nanotube_packet_t *pkt = new nanotube_packet_t();
  auto sec = NANOTUBE_SECTION_WHOLE;
  pkt->resize(sec, len);
  return pkt;
}

/*!
** Free a Nanotube packet.
** \param pkt Pointer to the packet that will be freed.
**/
void free_pkt(nanotube_packet_t *pkt) {
  delete pkt;
}

/*!
** Test Nanotube packet access functions.
** \param len Packet size to perform the tests with
** \return Number of errors.
**/
int test_packets(size_t pkt_len) {
  int err = 0;
  const size_t buffer_size = 16384;
  printf("\n\n_Testing_Packets_\n");
  nanotube_packet_t *pkt = malloc_pkt(pkt_len);

  /* Basics, first :) */
  size_t len;
  len = nanotube_packet_bounded_length(pkt, 16);
  printf("Packet restricted length %zu\n", len);
  if( len != 16 )
    ++err;

  len = nanotube_packet_bounded_length(pkt, buffer_size);
  printf("Packet length %zu\n", len);
  if( len != pkt_len && pkt_len < buffer_size )
    ++err;

  auto sec = NANOTUBE_SECTION_WHOLE;
  assert(nanotube_packet_data(pkt) == pkt->begin(sec));

  uint8_t *data = (uint8_t*)malloc(buffer_size);

  /* Fill packet with test data */
  auto pkt_data = pkt->begin(sec);
  for( size_t i = 0; i < pkt_len; ++i )
    pkt_data[i] = i & 0xff;

  /* Read / write x offsets & lengths */
  size_t n;
  n = nanotube_packet_read(pkt, data, 0, len);
  printf("Read %zu bytes, asking for %zu\n", n, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  size_t offs = 16;
  n = nanotube_packet_read(pkt, data, offs, 8);
  printf("Read %zu bytes from offset %zu, asking for %i\n", n, offs, 8);
  print_bytes(data, len); printf("\n");
  if( n != 8 )
    ++err;
  if( data[0] != data[offs] )
    ++err;

  /* Small write + offset */
  uint8_t *all_enabled = (uint8_t *)malloc(buffer_size / 8);
  memset(all_enabled, 0xff, buffer_size / 8);
  uint8_t write_data[] = {0xde, 0xad, 0xbe, 0xef};

  offs = 40;
  n = nanotube_packet_write_masked(pkt, write_data, all_enabled, offs,
                                   sizeof(write_data));
  printf("Wrote %zu bytes to offset %zu, with req size %zu\n", n, offs,
         sizeof(write_data));
  if( n != sizeof(write_data) )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if ( memcmp(data + offs, write_data, 4) != 0 )
    ++err;

  /* Small unmasked write */
  offs = 44;
  n = nanotube_packet_write(pkt, write_data, offs, sizeof(write_data));
  printf("Wrote %zu bytes to offset %zu, with req size %zu\n", n, offs,
         sizeof(write_data));
  if( n != sizeof(write_data) )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if ( memcmp(data + offs, write_data, 4) != 0 )
    ++err;

  /* Full write */
  for( size_t i = 0; i < len; ++i )
    data[i] ^= 0x0f;
  n = nanotube_packet_write_masked(pkt, data, all_enabled, 0, len);
  printf("Wrote %zu bytes, with req size %zu\n", n, len);
  if( n != len )
    ++err;
  memset(data, 0, len);
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  /* Write w/ enables */
  uint8_t *fancy_enable = (uint8_t*)malloc(len / 8);
  memset(fancy_enable, 0, len / 8);
  {
    /* Craft a pattern of alternating on/off that are getting larger */
    size_t i = 0;
    size_t s = 1;
    while ( i < len ) {
      /* Skip S bits */
      i = i + s;
      /* Fill S bits */
      for( size_t j = 0; j < s && i < len; ++i, ++j )
        fancy_enable[i / 8] |= (1 << (i % 8));
      s = s * 2;
    }
  }
  memset(data, 0xff, len);
  offs = 16;
  n = nanotube_packet_write_masked(pkt, data, fancy_enable, offs,
                   len - offs);
  printf("Wrote %zu bytes, with fancy mask, offset %zu, req size %zu\n", n,
         offs, len - offs);
  if( n != len - offs )
    ++err;
  memset(data, 0, len);
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( data[offs + 1] != 0xff || data[offs + 4] != 0xff ||
      data[offs + 5] != 0xff )
    ++err;

  /* Corner cases: Reads */
  /* Outside packet */
  offs = len + 8;
  n = nanotube_packet_read(pkt, data, offs, len);
  printf("Read %zu bytes, asking for %zu from offset %zu\n", n, len, offs);
  print_bytes(data, len); printf("\n");
  if( n != 0 )
    ++err;

  /* Partial & across end of packet */
  offs = len - 8;
  n = nanotube_packet_read(pkt, data, offs, len);
  printf("Read %zu bytes, asking for %zu from offset %zu\n", n, len, offs);
  print_bytes(data, len); printf("\n");
  if( n != len - offs )
    ++err;
  if( data[0] != 0x37 || data[n] != 0 )
    ++err;

  /* Empty read */
  offs = 0;
  n = nanotube_packet_read(pkt, data, offs, 0);
  printf("Read %zu bytes, asking for %i\n", n, 0);
  if( n != 0 )
    ++err;

  /*print_bytes(data, len); printf("\n"); */
  /* Read from NULL packet */
  n = nanotube_packet_read(NULL, data, offs, len);
  printf("Read %zu bytes, asking for %zu from NULL packet\n", n, len);
  print_bytes(data, len); printf("\n");
  if( n != 0 )
    ++err;

  /* Read to NULL data */
  n = nanotube_packet_read(pkt, NULL, offs, len);
  printf("Read %zu bytes, asking for %zu to NULL data array\n", n, len);
  if( n != 0 )
    ++err;

  /* Corner cases: Writes */
  /* Outside packet */
  offs = len + 8;
  n = nanotube_packet_write_masked(pkt, write_data, all_enabled, offs,
                   sizeof(write_data));
  printf("Wrote %zu bytes to offset %zu, with req size %zu\n", n, offs,
      sizeof(write_data));
  if( n != 0 )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  n = nanotube_packet_write(pkt, write_data, offs, sizeof(write_data));
  printf("Wrote %zu bytes to offset %zu, with req size %zu\n", n, offs,
      sizeof(write_data));
  if( n != 0 )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  /* Partial & across end of packet */
  offs = len - sizeof(write_data) / 2;
  n = nanotube_packet_write_masked(pkt, write_data, all_enabled, offs,
                   sizeof(write_data));
  printf("Wrote %zu bytes to offset %zu, with req size %zu\n", n, offs,
         sizeof(write_data));
  if( n != len - offs )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;
  if( memcmp(data + offs, write_data, len - offs) != 0 )
    ++err;

  uint8_t write_data2[] = {0xbe, 0xda, 0x22, 0x1e};
  n = nanotube_packet_write(pkt, write_data2, offs, sizeof(write_data));
  printf("Wrote %zu bytes to offset %zu, with req size %zu\n", n, offs,
         sizeof(write_data2));
  if( n != len - offs )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;
  if( memcmp(data + offs, write_data2, len - offs) != 0 )
    ++err;

  /* Empty write */
  offs = 0;
  n = nanotube_packet_write_masked(pkt, write_data, all_enabled, offs, 0);
  printf("Wrote %zu bytes to offset %zu, with req size %i\n", n, offs, 0);
  if( n != 0 )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  n = nanotube_packet_write(pkt, write_data, offs, 0);
  printf("Wrote %zu bytes to offset %zu, with req size %i\n", n, offs, 0);
  if( n != 0 )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  /* Write to NULL packet */
  n = nanotube_packet_write_masked(NULL, write_data, all_enabled, offs,
                   sizeof(write_data));
  printf("Wrote %zu bytes to NULL packet\n", n);
  if( n != 0 )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  n = nanotube_packet_write(NULL, write_data, offs, sizeof(write_data));
  printf("Wrote %zu bytes to NULL packet\n", n);
  if( n != 0 )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  /* Write with NULL data */
  n = nanotube_packet_write_masked(pkt, NULL, all_enabled, offs,
                   sizeof(write_data));
  printf("Wrote %zu bytes with NULL data\n", n);
  if( n != 0 )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  n = nanotube_packet_write(pkt, NULL, offs, sizeof(write_data));
  printf("Wrote %zu bytes with NULL data\n", n);
  if( n != 0 )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;

  /* Write with NULL enables */
  n = nanotube_packet_write_masked(pkt, write_data, NULL, offs,
                   sizeof(write_data));
  printf("Wrote %zu bytes with NULL enables\n", n);
  if( n != 0 )
    ++err;
  n = nanotube_packet_read(pkt, data, 0, len);
  print_bytes(data, len); printf("\n");
  if( n != len )
    ++err;


  free(all_enabled);
  free(data);
  free(fancy_enable);
  free_pkt(pkt);

  return err;
}


int main(int argc, char *argv[]) {
  int num_errs = 0;

  num_errs += test_packets(64);
  /*test_packets(1500); */
  /*test_packets(9000); */

  printf("Found %i errors\n", num_errs);
  return num_errs;
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
