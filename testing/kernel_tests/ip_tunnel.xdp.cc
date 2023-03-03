/**************************************************************************\
*//*! \file ip_tunnel.xdp.cc
** \author  Neil Turton <neilt@amd.com>
**  \brief  A simple tunnel test.
**   \date  2021-09-16
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <cstdint>
#include <cstring>
#include <linux/bpf.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

/*
 * This test acts as a GRE tunnel endpoint on a stick.  In one
 * direction, it accepts packets from an IP router, encapsulates them
 * and sends them to the other GRE endpoint.  In the other direction,
 * it accepts encapsulated packets from the GRE endpoint, decapsulates
 * them and send them to the IP router.  It also needs to respond to
 * ARP requests.
 *
 * The packet transformations are:
 *   Encapsulate:
 *     Input: Eth(0:router->local), IP(1:routed_net->gre_net)
 *     Output: Eth(local->gre), IP(local->gre), GRE, IP(1)
 *
 *   Decapsulate:
 *     Input: Eth(0:gre->local), IP(1:gre->local), GRE, IP(2:gre_net->routed_net)
 *     Output: Eth(local->router), IP(2)
 *
 *   ICMP echo:
 *     Input: Eth(0:req_eth->local), IP(req_ip->local), ICMP(echo_req)
 *     Output: Eth(local->req_eth), IP(local->req_ip), ICMP(echo_resp)
 *
 *   ARP:
 *     Input: Eth(0:requester->bcast), ARP(who-has:local-ip)
 *     Output: Eth(local->requester), ARP(resp:local-ip has local-mac)
 */

static int (*bpf_xdp_adjust_head)(void *ctx, int delta) =
  (int (*)(void *ctx, int delta)) BPF_FUNC_xdp_adjust_head;

static const uint8_t bcast_mac[6] = { 0xff,0xff,0xff,0xff,0xff,0xff };
static const uint8_t local_mac[6] = { 2,0,0,0,1,3 };
static const uint8_t local_ip[4] = { 192,168,1,3 };
static const uint8_t router_mac[6] = { 2,0,0,0,1,2 };
static const uint8_t gre_ep_mac[6] = { 2,0,0,0,1,1 };
static const uint8_t gre_ep_ip[4] = { 192,168,1,1 };
static const uint8_t remote_ip[3] = { 192,168,2 };

typedef struct {
  uint8_t ether_daddr[6];
  uint8_t ether_saddr[6];
  uint8_t ether_type[2];
} __attribute__((packed)) eth_header;

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
} __attribute__((packed)) arp_header;

typedef struct {
  uint8_t ip_ver_ihl;
  uint8_t ip_tos;
  uint8_t ip_tot_length[2];
  uint16_t ip_id;
  uint16_t ip_frag_off;
  uint8_t ip_ttl;
  uint8_t ip_proto;
  uint16_t ip_csum;
  uint8_t ip_saddr[4];
  uint8_t ip_daddr[4];
} __attribute__((packed)) ip_header;

typedef struct {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
} __attribute__((packed)) icmp_header;

typedef struct {
  uint16_t gre_flags;
  uint8_t gre_proto[2];
} __attribute__((packed)) gre_header;

int ipt_memcmp(const void *p1, const void *p2, size_t n)
#if __clang__
  __attribute__((always_inline))
#endif
{
  const char *s1 = (const char*)p1;
  const char *s2 = (const char*)p2;
  while (n != 0) {
    if (*s1 != *s2)
      return (*s1 < *s2 ? -1 : +1);
    s1++;
    s2++;
    n--;
  }

  return 0;
}

static
bool handle_arp(struct xdp_md *ctx, char *l3_header, char *end)
#if __clang__
  __attribute__((always_inline))
#endif
{
  arp_header *arp = (arp_header*)l3_header;
  if (end - l3_header < sizeof(*arp)) {
    return false;
  }

  // Check the header fields.
  if (arp->arp_htype[0] != 0x00 || arp->arp_htype[1] != 0x01 ||
      arp->arp_ptype[0] != 0x08 || arp->arp_ptype[1] != 0x00 ||
      ipt_memcmp(arp->arp_tpa, local_ip, sizeof(local_ip)) != 0 ||
      arp->arp_oper[0] != 0x00 || arp->arp_oper[1] != 0x01) {
    return false;
  }

  // Set the target addresses.
  memcpy(arp->arp_tpa, arp->arp_spa, sizeof(arp->arp_tpa));
  memcpy(arp->arp_tha, arp->arp_sha, sizeof(arp->arp_tha));

  // Set the source addresses.
  memcpy(arp->arp_spa, local_ip, sizeof(arp->arp_spa));
  memcpy(arp->arp_sha, local_mac, sizeof(arp->arp_sha));

  // Send the packet back to where it came from.
  char *data = (char*)(uint64_t)(ctx->data);
  eth_header *eth = (eth_header*)data;
  memcpy(eth->ether_daddr, eth->ether_saddr, sizeof(eth->ether_daddr));
  memcpy(eth->ether_saddr, local_mac, sizeof(eth->ether_saddr));

  // Set the operation to REPLY.
  arp->arp_oper[0] = 0x00;
  arp->arp_oper[1] = 0x02;

  return true;
}

static
bool handle_encap(struct xdp_md *ctx, char *end)
#if __clang__
  __attribute__((always_inline))
#endif
{
  int extra_size = sizeof(ip_header) + sizeof(gre_header);

  int orig_length = end-(char*)(uint64_t)(ctx->data);

  // We need to replace the Ethernet header with Eth/IP/GRE.  IP
  // header. Note that the adjust_head call takes a delta to apply
  // to the head pointer.
  auto fail = bpf_xdp_adjust_head(ctx, -extra_size);
  if (fail) {
    return false;
  }

  // Write the Ethernet header.
  auto *eth = (eth_header*)(uint64_t)(ctx->data);
  memcpy(eth->ether_daddr, gre_ep_mac, sizeof(eth->ether_daddr));
  memcpy(eth->ether_saddr, local_mac, sizeof(eth->ether_saddr));
  eth->ether_type[0] = 0x08;
  eth->ether_type[1] = 0x00;

  // Write the IP header.
  auto *ip = (ip_header*)(eth+1);
  ip->ip_ver_ihl = 0x45;
  ip->ip_tos = 0;
  int ip_length = orig_length + (extra_size - sizeof(eth_header));
  ip->ip_tot_length[0] = ip_length >> 8;
  ip->ip_tot_length[1] = ip_length >> 0;
  ip->ip_id = 0;
  ip->ip_frag_off = ntohs(IP_DF); // Don't Fragment.
  ip->ip_ttl = 0xff;
  ip->ip_csum = 0;
  ip->ip_proto = IPPROTO_GRE;
  memcpy(ip->ip_saddr, local_ip, sizeof(ip->ip_saddr));
  memcpy(ip->ip_daddr, gre_ep_ip, sizeof(ip->ip_daddr));

  // Calculate the checksum.
  uint32_t csum = 0xffff;
  csum += ((uint16_t*)ip)[0];
  csum += ((uint16_t*)ip)[1];
  csum += ((uint16_t*)ip)[2];
  csum += ((uint16_t*)ip)[3];
  csum += ((uint16_t*)ip)[4];
  csum += ((uint16_t*)ip)[5];
  csum += ((uint16_t*)ip)[6];
  csum += ((uint16_t*)ip)[7];
  csum += ((uint16_t*)ip)[8];
  csum += ((uint16_t*)ip)[9];
  csum = (csum & 0xffff) + (csum >> 16);
  csum = (csum & 0xffff) + (csum >> 16);
  csum = 0xffff & ~csum;

  ip->ip_csum = csum;

  // Write the GRE header.
  auto *gre = (gre_header*)(ip+1);
  gre->gre_flags = 0;
  gre->gre_proto[0] = 0x08;
  gre->gre_proto[1] = 0x00;

  return true;
}

static
bool handle_decap(xdp_md *ctx, gre_header *gre, char *end)
#if __clang__
  __attribute__((always_inline))
#endif
{
  // Check the GRE header.
  if (gre->gre_flags != 0 || gre->gre_proto[0] != 0x08 ||
      gre->gre_proto[1] != 0x00) {
    return false;
  }

  // Replace the Eth/IP/GRE headers with an Ethernet header.
  int removed_size = (sizeof(ip_header) + sizeof(gre_header));
  auto fail = bpf_xdp_adjust_head(ctx, removed_size);
  if (fail) {
    return false;
  }

  auto *eth = (eth_header*)(uint64_t)(ctx->data);
  memcpy(eth->ether_daddr, router_mac, sizeof(eth->ether_daddr));
  memcpy(eth->ether_saddr, local_mac, sizeof(eth->ether_saddr));
  eth->ether_type[0] = 0x08;
  eth->ether_type[1] = 0x00;

  return true;
}

static
bool handle_icmp(struct xdp_md *ctx, ip_header *ip,
                 icmp_header *icmp, char *end)
{
  if ((end - (char*)icmp) < sizeof(*icmp)) {
    return false;
  }

  // Handle echo requests.
  if (icmp->type == ICMP_ECHO && icmp->code == 0) {
    // Set the type.
    icmp->type = ICMP_ECHOREPLY;

    // Adjust the checksum.  Maximum value 0xffff.
    uint32_t sum = icmp->checksum;

    // Add the old value since it was previously added and will no
    // longer be.  Maximum value 0x1fffe.
    sum += ntohs(ICMP_ECHO);

    // Subtract the new value since it was not being added and now
    // will be.  The offset of 0xffff will not change the checksum and
    // is used to avoid wrapping.  Maximum value 0x2fffd.
    sum += 0xffff - ntohs(ICMP_ECHOREPLY);

    // Perform a fold.  Possible maximum values:
    //   0x0001 + 0xffff = 0x10000
    //   0x0002 + 0xfffd = 0x0ffff
    // Maximum value 0x10000.
    sum = (sum >> 16) + (sum & 0xffff);

    // Perform a second fold.  Possible maximum values:
    //   0x0000 + 0xffff = 0xffff
    //   0x0001 + 0x0000 = 0x0000
    // Maximum value 0xffff.
    sum = (sum >> 16) + (sum & 0xffff);

    // Set the new checksum field.
    icmp->checksum = sum;

    // Swap the IP addresses.
    memcpy(ip->ip_daddr, ip->ip_saddr, sizeof(ip->ip_daddr));
    memcpy(ip->ip_saddr, local_ip, sizeof(ip->ip_saddr));

    // Swap the Ethernet addresses.
    char *data = (char*)(uint64_t)(ctx->data);
    eth_header *eth = (eth_header*)data;
    memcpy(eth->ether_daddr, eth->ether_saddr, sizeof(eth->ether_daddr));
    memcpy(eth->ether_saddr, local_mac, sizeof(eth->ether_saddr));
    return true;
  }

  // Unrecognised.
  return false;
}

static
bool handle_ip(xdp_md *ctx, char *l3_header, char *end)
#if __clang__
  __attribute__((always_inline))
#endif
{
  ip_header *ip = (ip_header*)l3_header;
  if ((end - l3_header) < sizeof(*ip)) {
    return false;
  }

  // Calculate the checksum.
  uint32_t csum = 0xffff;
  csum += ((uint16_t*)l3_header)[0];
  csum += ((uint16_t*)l3_header)[1];
  csum += ((uint16_t*)l3_header)[2];
  csum += ((uint16_t*)l3_header)[3];
  csum += ((uint16_t*)l3_header)[4];
  csum += ((uint16_t*)l3_header)[5];
  csum += ((uint16_t*)l3_header)[6];
  csum += ((uint16_t*)l3_header)[7];
  csum += ((uint16_t*)l3_header)[8];
  csum += ((uint16_t*)l3_header)[9];
  csum = (csum & 0xffff) + (csum >> 16);
  csum = (csum & 0xffff) + (csum >> 16);
  csum = 0xffff & ~csum;

  // Check the required header fields.
  if (ip->ip_ver_ihl != 0x45 || csum != 0) {
    return false;
  }

  // Match a packet which can be routed through the tunnel.
  int daddr_is_remote = ipt_memcmp(ip->ip_daddr, remote_ip, sizeof(remote_ip)) == 0;
  if (daddr_is_remote) {
    return handle_encap(ctx, end);
  }

  // Match an encapsulated packet.
  int daddr_is_local = ipt_memcmp(ip->ip_daddr, local_ip, sizeof(local_ip)) == 0;
  int saddr_is_gre_ep = ipt_memcmp(ip->ip_saddr, gre_ep_ip, sizeof(gre_ep_ip)) == 0;
  if (ip->ip_proto == IPPROTO_GRE && daddr_is_local && saddr_is_gre_ep &&
      (ip->ip_frag_off & ~ntohs(IP_DF)) == 0) {
    auto *gre = (gre_header*)(ip+1);
    return handle_decap(ctx, gre, end);
  }

  if (ip->ip_proto == IPPROTO_ICMP && daddr_is_local &&
      (ip->ip_frag_off & ~ntohs(IP_DF)) == 0) {
    auto *icmp = (icmp_header*)(ip+1);
    return handle_icmp(ctx, ip, icmp, end);
  }

  // Unrecognised.
  return false;
}

extern "C"
int ip_tunnel(struct xdp_md *ctx)
{
  char *data = (char*)(uint64_t)(ctx->data);
  char *end = (char*)(uint64_t)(ctx->data_end);

  // Check that the packet is long enough for an Ethernet header.
  eth_header *eth = (eth_header*)data;
  if (end - data < sizeof(*eth)) {
    return XDP_DROP;
  }

  // Check the MAC address.
  int not_local = ipt_memcmp(eth->ether_daddr, local_mac, sizeof(local_mac)) != 0;
  int not_bcast = ipt_memcmp(eth->ether_daddr, bcast_mac, sizeof(bcast_mac)) != 0;
  if (not_local && not_bcast) {
    return XDP_DROP;
  }

  char *l3_header = (char*)(eth+1);

  // Check the Ether-type.
  if (eth->ether_type[0] == 0x08 && eth->ether_type[1] == 0x06) {
    if (handle_arp(ctx, l3_header, end))
      return XDP_PASS;
    return XDP_DROP;

  }

  if (eth->ether_type[0] == 0x08 && eth->ether_type[1] == 0x00) {
    if (handle_ip(ctx, l3_header, end))
      return XDP_PASS;
    return XDP_DROP;
  }

  // Unknown.
  return XDP_DROP;
}
