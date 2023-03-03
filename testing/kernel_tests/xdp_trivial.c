/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/udp.h>

#include <stdint.h>

#define SEC(NAME) __attribute__((section(NAME), used))

SEC("prog")
int xdp_trivial_benchmark_traffic(struct xdp_md *ctx)
{
	return XDP_PASS;
}

