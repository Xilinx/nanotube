/*******************************************************/
/*! \file  mem2req_mixed.manual.c
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Manual mem2req conversion for maps and packets.
**   \date 2020-08-24
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nanotube_api.h"

typedef struct {
  uint8_t b[6];
} __attribute__((packed)) mac_addr;

typedef struct {
  mac_addr dst;
  mac_addr src;
  uint32_t tag;
  uint16_t len;
  uint8_t  ip_version:4;
  uint8_t  ihl;
} __attribute__((packed)) eth_ip_hdr;

static const nanotube_map_id_t map_id = 0;

nanotube_kernel_rc_t one_pkt_one_map(nanotube_context_t *nt_ctx,
                                     nanotube_packet_t *packet) {
  size_t len = nanotube_packet_bounded_length(packet, 65535);
  if( len < sizeof(eth_ip_hdr) ) {
    return NANOTUBE_PACKET_DROP;
  }

  mac_addr src_mac;
  nanotube_packet_read(packet, src_mac.b, offsetof(eth_ip_hdr, src),
                       sizeof(src_mac));
  mac_addr new_dst;
  size_t nread = nanotube_map_read(nt_ctx, map_id, src_mac.b,
                                   sizeof(src_mac), new_dst.b, 0,
                                   sizeof(new_dst));
  if( nread == 0 ) {
    printf("Could not find source MAC\n");
    return NANOTUBE_PACKET_DROP;
  }

  nanotube_packet_write(packet, new_dst.b, offsetof(eth_ip_hdr, dst),
                        sizeof(new_dst.b));
  return NANOTUBE_PACKET_PASS;
}

void nanotube_setup() {
  nanotube_context_t* ctx = nanotube_context_create();
  nanotube_map_t* map     = nanotube_map_create(map_id,
                              NANOTUBE_MAP_TYPE_HASH, 6, 6);
  nanotube_context_add_map(ctx, map);
  nanotube_add_plain_packet_kernel("one_pkt_one_map", one_pkt_one_map, -1, 0 /*false*/);
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
