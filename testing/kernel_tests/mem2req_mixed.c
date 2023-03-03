/*******************************************************/
/*! \file  mem2req_mixed.c
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Test mem2req conversion for maps and packets.
**   \date 2020-08-18
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
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
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < sizeof(eth_ip_hdr) ) {
    return NANOTUBE_PACKET_DROP;
  }

  eth_ip_hdr* hdr = (eth_ip_hdr*)data;
  mac_addr* new_dst = (mac_addr*)nanotube_map_lookup(nt_ctx, map_id,
                      hdr->src.b, sizeof(mac_addr), sizeof(mac_addr));
  if( new_dst == NULL ) {
    printf("Could not find source MAC\n");
    return NANOTUBE_PACKET_DROP;
  }

  hdr->dst = *new_dst;
  return NANOTUBE_PACKET_PASS;
}

void nanotube_setup() {
  nanotube_context_t* ctx = nanotube_context_create();
  nanotube_map_t* map     = nanotube_map_create(map_id,
                              NANOTUBE_MAP_TYPE_HASH, sizeof(mac_addr),
                              sizeof(mac_addr));
  nanotube_context_add_map(ctx, map);

  nanotube_add_plain_packet_kernel("one_pkt_one_map", one_pkt_one_map, -1, 0 /*false*/);
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
