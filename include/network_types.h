#ifndef __INCLUDE_NETWORK_TYPES_H__
#define __INCLUDE_NETWORK_TYPES_H__
/*************************************************************************/
/*! \file network_types.c
** \author  Neil Turton <neilt@amd.com>
**  \brief  Frequently used types for network packets.
**   \date  2021-09-16
*//***********************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>

typedef struct {
  uint8_t   ether_daddr[6];
  uint8_t   ether_saddr[6];
  uint16_t  ether_type_be16;
} __attribute__((packed)) eth_header_t;

typedef struct {
  uint8_t   ip_ver_ihl;
  uint8_t   ip_tos;
  uint8_t   ip_tot_length[2];
  uint16_t  ip_id;
  uint16_t  ip_frag_off;
  uint8_t   ip_ttl;
  uint8_t   ip_proto;
  uint16_t  ip_csum;
  uint8_t   ip_saddr[4];
  uint8_t   ip_daddr[4];
} __attribute__((packed)) ipv4_header_t;

typedef struct {
  uint32_t  ip_ver_tc_label;
  uint16_t  ip_len;
  uint8_t   ip_next_hdr;
  uint8_t   ip_hop_lim;
  uint8_t   ip_saddr[16];
  uint8_t   ip_daddr[16];
} __attribute__((packed)) ipv6_header_t;

typedef struct {
  uint16_t  tcp_source;
  uint16_t  tcp_dest;
  uint32_t  tcp_seq;
  uint32_t  tcp_ack;
  uint16_t  tcp_flags;
  uint16_t  tcp_window;
  uint16_t  tcp_csum;
  uint16_t  tcp_urg;
} __attribute__((packed)) tcp_header_t;

typedef struct {
  uint16_t  udp_source;
  uint16_t  udp_dest;
  uint16_t  udp_length;
  uint16_t  udp_csum;
} __attribute__((packed)) udp_header_t;

typedef struct {
  uint8_t arp_htype[2];
  uint8_t arp_ptype[2];
  uint8_t arp_hlen;
  uint8_t arp_plen;
  uint8_t arp_oper[2];
  uint8_t arp_sha[6];
  uint8_t arp_spa[4];
  uint8_t arp_tha[6];
  uint8_t arp_tpa[4];
} __attribute__((packed)) arp_header_t;

typedef struct {
  uint8_t  type;
  uint8_t  code;
  uint16_t checksum;
} __attribute__((packed)) icmp_header_t;

typedef struct {
  uint16_t gre_flags;
  uint8_t  gre_proto[2];
} __attribute__((packed)) gre_header_t;
#endif // __INCLUDE_NETWORK_TYPES_H__

