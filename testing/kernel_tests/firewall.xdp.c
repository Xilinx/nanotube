/*************************************************************************/
/*! \file firewall.xdp.c
** \author  Neil Turton <neilt@amd.com>
**  \brief  A simple IP firewall test.
**   \date  2023-05-31
*//***********************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <endian.h>
#include <linux/bpf.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <string.h>

#include "bpf/bpf_helpers.h"
#include "network_types.h"

/*
 * This test acts as a simple IP firewall.  It has two rule tables,
 * one for blocked source IP addresses and one for allowed destination
 * services.  Both rule tables return a stats ID which indexes a stats
 * table.  An entry in the stats table counts packets and bytes.
 *
 * The behaviour in different cases is:
 *   Malformed:
 *     Use stats ID STATS_ID_MALFORMED
 *     Drop packet
 *   Non-IP:
 *     Use stats ID STATS_ID_NON_IP
 *     Pass packet
 *   Match in source table:
 *     Use stats ID from source table
 *     Drop packet
 *   Miss in source table, match in service table:
 *     Use stats ID from service table
 *     Pass packet
 *   Miss in both tables
 *     Use stats ID STATS_ID_UNKNOWN
 *     Drop packet
 */

///////////////////////////////////////////////////////////////////////////

typedef uint32_t stats_id_t;

typedef struct {
  union {
    uint8_t v4_addr[4];
    uint8_t v6_addr[16];
  };
  uint8_t discr;
} __attribute__((packed)) source_key_t;

typedef struct {
  stats_id_t stats_id;
} __attribute__((packed)) source_value_t;

struct bpf_map_def SEC("maps") source_map = {
  .type = BPF_MAP_TYPE_HASH,
  .key_size = sizeof(source_key_t),
  .value_size = sizeof(source_value_t),
  .max_entries = 1024,
  .map_flags = 0,
};

///////////////////////////////////////////////////////////////////////////

typedef struct {
  union {
    uint8_t v4_addr[4];
    uint8_t v6_addr[16];
  };
  uint8_t discr;
  uint8_t ip_proto;
  uint16_t port_be16;
} __attribute__((packed)) service_key_t;

typedef struct {
  stats_id_t stats_id;
} __attribute__((packed)) service_value_t;

struct bpf_map_def SEC("maps") service_map = {
  .type = BPF_MAP_TYPE_HASH,
  .key_size = sizeof(service_key_t),
  .value_size = sizeof(service_value_t),
  .max_entries = 1024,
  .map_flags = 0,
};

///////////////////////////////////////////////////////////////////////////

static const stats_id_t STATS_ID_MALFORMED = 0;
static const stats_id_t STATS_ID_NON_IP = 1;
static const stats_id_t STATS_ID_UNKNOWN = 2;
typedef struct {
  stats_id_t stats_id;
} __attribute__((packed)) stats_key_t;

typedef struct {
  uint64_t packets;
  uint64_t bytes;
} __attribute__((packed)) stats_value_t;

struct bpf_map_def SEC("maps") stats_map = {
  .type = BPF_MAP_TYPE_ARRAY,
  .key_size = sizeof(stats_key_t),
  .value_size = sizeof(stats_value_t),
  .max_entries = 1024,
  .map_flags = 0,
};

///////////////////////////////////////////////////////////////////////////

static
stats_id_t parse(struct xdp_md *ctx, source_key_t *source_key_out,
                 service_key_t *service_key_out)
{
  char *data = (char*)(uint64_t)(ctx->data);
  char *end = (char*)(uint64_t)(ctx->data_end);

  // Check that the packet is long enough for an Ethernet header.
  eth_header_t *eth = (eth_header_t*)data;
  if (end - data < sizeof(*eth)) {
    return STATS_ID_MALFORMED;
  }

  // Parse the Ethernet header.
  uint16_t ether_type_be16 = eth->ether_type_be16;
  source_key_out->discr = 0;

  // Parse the L3 header.
  char *l4_header = NULL;
  uint8_t discr = 0;
  uint8_t ip_proto = 0;
  if (ether_type_be16 == htobe16(ETHERTYPE_IP)) {
    ipv4_header_t *ipv4 = (ipv4_header_t*)(eth+1);
    if (end - (char*)ipv4 < sizeof(*ipv4)) {
      return STATS_ID_MALFORMED;
    }

    memcpy(source_key_out->v4_addr, ipv4->ip_saddr, sizeof(source_key_out->v4_addr));
    memcpy(service_key_out->v4_addr, ipv4->ip_daddr, sizeof(service_key_out->v4_addr));

    discr = 4;
    ip_proto = ipv4->ip_proto;
    l4_header = (char*)(ipv4+1);

   } else if (ether_type_be16 == htobe16(ETHERTYPE_IPV6)) {
    ipv6_header_t *ipv6 = (ipv6_header_t*)(eth+1);
    if (end - (char*)ipv6 < sizeof(*ipv6)) {
      return STATS_ID_MALFORMED;
    }

    memcpy(source_key_out->v6_addr, ipv6->ip_saddr, sizeof(source_key_out->v6_addr));
    memcpy(service_key_out->v6_addr, ipv6->ip_daddr, sizeof(service_key_out->v6_addr));

    discr = 6;
    ip_proto = ipv6->ip_next_hdr;
    l4_header = (char*)(ipv6+1);

  } else {
    return STATS_ID_NON_IP;
  }

  source_key_out->discr = discr;
  service_key_out->discr = discr;

  // Parse the L4 header.
  service_key_out->ip_proto = ip_proto;
  service_key_out->port_be16 = 0;
  if (ip_proto == IPPROTO_TCP) {
    tcp_header_t *tcp = (tcp_header_t*)l4_header;
    if (end - (char*)tcp < sizeof(*tcp)) {
      return STATS_ID_MALFORMED;
    }

    service_key_out->port_be16 = tcp->tcp_dest;

  } else if (ip_proto == IPPROTO_UDP) {
    udp_header_t *udp = (udp_header_t*)l4_header;
    if (end - (char*)udp < sizeof(*udp)) {
      return STATS_ID_MALFORMED;
    }

    service_key_out->port_be16 = udp->udp_dest;
  }

  return STATS_ID_UNKNOWN;
}

static
int classify(stats_key_t *stats_key_out, const source_key_t *source_key,
             const service_key_t *service_key)
{
  // Perform the rule lookups.
  source_value_t *source_value = (source_value_t*)
    bpf_map_lookup_elem(&source_map, (void*)source_key);
  service_value_t *service_value = (service_value_t*)
    bpf_map_lookup_elem(&service_map, (void*)service_key);

  if (source_value != NULL) {
    stats_key_out->stats_id = source_value->stats_id;
    return XDP_DROP;

  } else if (service_value != NULL) {
    stats_key_out->stats_id = service_value->stats_id;
    return XDP_PASS;
  }

  stats_key_out->stats_id = STATS_ID_UNKNOWN;
  return XDP_DROP;
}

int ip_firewall(struct xdp_md *ctx)
{
  source_key_t source_key = {};
  service_key_t service_key = {};
  stats_key_t stats_key = {};
  int rc;

  // Parse the packet.
  stats_key.stats_id = parse(ctx, &source_key, &service_key);

  // Classify the packet.
  switch (stats_key.stats_id) {
  case STATS_ID_MALFORMED:
    rc = XDP_DROP;
    break;

  case STATS_ID_NON_IP:
    rc = XDP_PASS;
    break;

  default:
    rc = classify(&stats_key, &source_key, &service_key);
    break;
  }

  // Update the stats.
  stats_value_t *stats_value = (stats_value_t*)bpf_map_lookup_elem(&stats_map, &stats_key);
  if (stats_value != NULL) {
    char *data = (char*)(uint64_t)(ctx->data);
    char *end = (char*)(uint64_t)(ctx->data_end);
    uint64_t length = end - data;

    stats_value->packets += 1;
    stats_value->bytes += length;
  }

  // Return the classification result.
  return rc;
}
