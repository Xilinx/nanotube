/**
 * Simple test file to test LLVM passes that convert wide loads / stores to per
 * byte accesses.
 * Author: Stephan Diestelhorst <stephand@amd.com>
 * Date:   2020-01-10
 */

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>
#include <stdio.h>

#include "nanotube_api.h"

uint8_t   ug8 = 0x42;
uint16_t ug16 = 0xcafe;
uint32_t ug32 = 0xfeedbacc;
uint64_t ug64 = 0x600dface2badfee7;

struct Umix {
    uint8_t   u8_1;
    uint16_t u16;
    uint8_t   u8_2;
    uint32_t u32;
    uint8_t   u8_3;
    uint64_t u64;
};

struct Umix_packed {
    uint8_t   u8_1;
    uint16_t u16;
    uint8_t   u8_2;
    uint32_t u32;
    uint8_t   u8_3;
    uint64_t u64;
} __attribute__ ((__packed__));

union Uunion {
    uint8_t   u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
};


nanotube_kernel_rc_t wide_to_byte(nanotube_context_t *nt_ctx,
                                  nanotube_packet_t *packet)
{
    printf("8: %x 16: %hx 32: %x 64: %lx\n", ug8, ug16, ug32, ug64);

    // Local variables
    uint8_t   lu8 =  ug8;
    uint16_t lu16 = ug16;
    uint32_t lu32 = ug32;
    uint64_t lu64 = ug64;
    printf("8: %x 16: %hx 32: %x 64: %lx\n", lu8, lu16, lu32, lu64);

    // Unpacked structs
    struct Umix umix;
    umix.u8_1 =  ug8;
    umix.u16  = ug16;
    umix.u32  = ug32;
    umix.u64  = ug64;
    umix.u8_2 =  ug8 + 1;
    umix.u8_3 =  ug8 + 2;
    printf("8: %x 16: %hx 8: %x 32: %x 8: %x 64: %lx\n",
           umix.u8_1, umix.u16,umix.u8_2, umix.u32, umix.u8_3, umix.u64);

    // Packed structs
    struct Umix_packed upack;
    upack.u8_1 =  ug8;
    upack.u16  = ug16;
    upack.u32  = ug32;
    upack.u64  = ug64;
    upack.u8_2 =  ug8 + 1;
    upack.u8_3 =  ug8 + 2;
    printf("8: %x 16: %hx 8: %x 32: %x 8: %x 64: %lx\n",
           upack.u8_1, upack.u16,upack.u8_2, upack.u32, upack.u8_3, upack.u64);

    // Union fun
    union Uunion uu;
    uu.u64 = ug64;
    printf("8: %x 16: %hx 32: %x 64: %lx\n", uu.u8, uu.u16, uu.u32, uu.u64);

    uu.u32 = ug32;
    printf("8: %x 16: %hx 32: %x 64: %lx\n", uu.u8, uu.u16, uu.u32, uu.u64);

    uu.u16 = ug16;
    printf("8: %x 16: %hx 32: %x 64: %lx\n", uu.u8, uu.u16, uu.u32, uu.u64);

    uu.u8 = ug8;
    printf("8: %x 16: %hx 32: %x 64: %lx\n", uu.u8, uu.u16, uu.u32, uu.u64);


    return NANOTUBE_PACKET_PASS;
}

void nanotube_setup(void) {
    nanotube_add_plain_packet_kernel("wide_to_byte", wide_to_byte, -1, 0 /*false*/);
}
