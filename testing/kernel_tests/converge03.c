/**
 * A simple example that tests packet access conversion with a non-trivial
 * control flow.
 * Author: Stephan Diestelhorst <stephand@amd.com>
 * Date:   2020-02-10
 */

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>
#include "nanotube_api.h"

nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet)
{
    /**
     * Test a simple CFG like this:
     *                (A)
     *               /   \
     *             [X]   (B)
     *              |   /   \
     *              |  /     \
     *              (C)     [Y]
     *                \     /
     *                 \   /
     *                  (D)
     *
     * where [X] and [Y] are packet accesses.
     */
    // Dummy data
    uint8_t decision;
    uint8_t data;
    nanotube_packet_read(packet, &decision, 0, 1);
    nanotube_packet_read(packet, &data,     1, 1);

    // Node A
    if (decision == 0x7f) {
        // Node [X]
        nanotube_packet_read(packet, &data, 2, 1);
        //goto C;
    } else {
        // Node (B)
        if (data + decision != 0x7f) {
            data = 0xFF;
            //goto C;
        } else {
            nanotube_packet_read(packet, &data, 3, 1);
            goto D;
        }
    }
    // Node (C):
    data++;
    // Node (D):
D:  nanotube_packet_write(packet, &data, 4, 1);

    return NANOTUBE_PACKET_PASS;
}

/* NOTE: Even though the code has multiple returns, Clang / LLVM tends to turn
 * these into a single exit.  Manual editing of the compiled result into the
 * converge03.multi_exit.ll file is needed so that we actually have code with
 * multiple exits. */
nanotube_kernel_rc_t multi_exit(nanotube_context_t *nt_ctx,
                                nanotube_packet_t *packet)
{
    // Dummy data
    uint8_t decision;
    uint8_t data;
    nanotube_packet_read(packet, &decision, 0, 1);
    nanotube_packet_read(packet, &data,     1, 1);

    if (decision == 0x7f) {
        nanotube_packet_read(packet,  &data, 2, 1);
        data++;
        nanotube_packet_write(packet, &data, 4, 1);
        return NANOTUBE_PACKET_DROP;
    }

    if (data + decision != 0x7f) {
        data = 0xFF;
    } else {
        nanotube_packet_read(packet, &data, 3, 1);
        goto D;
    }
    data++;
D:
    nanotube_packet_write(packet, &data, 4, 1);
    return NANOTUBE_PACKET_PASS;
}

void nanotube_setup() {
  nanotube_add_plain_packet_kernel("process_packet", process_packet, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("multi_exit",     multi_exit,     -1, 0 /*false*/);
}
