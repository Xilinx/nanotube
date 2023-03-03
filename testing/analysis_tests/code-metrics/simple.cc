/**************************************************************************\
*//*! \file simple.cc
** \author  Stephan Diestelhorst <stephand@amd.com>
**  \brief  Testing the code metrics analysis, especially the memory
            dependencies.
**   \date  2022-08-23
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_api.h"
#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_bus.h"
#include "simple_bus.hpp"

static const uint16_t platform_offset = sizeof(simple_bus::header);
//static const uint16_t platform_header = 0;

/**
 * A simple memory dependency example; read some bytes from the packet and
 * write them out again; each time using a different mechanism.
 */
nanotube_kernel_rc_t simple_mem(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  /* Do a number of reads first */
  uint8_t direct[10];
  nanotube_packet_read(packet, direct, 0 + platform_offset,
                       sizeof(direct));

  uint8_t read_buf[10];
  uint8_t write_buf[10];
  nanotube_packet_read(packet, read_buf, 10 + platform_offset,
                       sizeof(direct));

  uint8_t pre_init[10];
  memset(pre_init, 42, sizeof(pre_init));

  uint8_t read_ldst[10];
  uint8_t write_ldst[10];
  nanotube_packet_read(packet, read_ldst, 20 + platform_offset,
                       sizeof(direct));

  /* Then write out in the same order so that the analysis has something to
   * skip over */
  nanotube_packet_write(packet, direct, 30 + platform_offset,
                        sizeof(direct));

  memcpy(write_buf, read_buf, sizeof(write_buf));
  nanotube_packet_write(packet, write_buf, 40 + platform_offset,
                        sizeof(write_buf));

  nanotube_packet_write(packet, pre_init, 50 + platform_offset,
                        sizeof(pre_init));


  /* Copy manually 8 + 2 bytes */
  ((uint64_t*)write_ldst)[0] = ((uint64_t*)read_ldst)[0];
  ((uint16_t*)write_ldst)[4] = ((uint16_t*)read_ldst)[4];
  nanotube_packet_write(packet, write_ldst, 60 + platform_offset,
                        sizeof(write_ldst));

  return NANOTUBE_PACKET_PASS;
}

/**
 * Test write-after-read anti-dependencies.
 */
nanotube_kernel_rc_t test_war(nanotube_context_t* nt_ctx, nanotube_packet_t* packet) {
  /* Testing same size write-after-read */
  {
    uint8_t buf[10];
    nanotube_packet_read(packet, buf, 0 + platform_offset, 10);
    uint32_t* ptr = ((uint32_t*)buf + 1);
    uint32_t rd = ptr[0];                   /* read value */
    ptr[0] = 42;                            /* and overwrite */
    nanotube_packet_write(packet, buf, 10 + platform_offset, 10);
    nanotube_packet_write(packet, (uint8_t*)&rd, 20 + platform_offset, 4);
  }

  /* Testing same size write-after-read but non-overlapping */
  {
    uint8_t buf[10];
    nanotube_packet_read(packet, buf, 30 + platform_offset, 10);
    uint32_t* ptr = ((uint32_t*)buf + 1);
    uint32_t rd = ptr[0];                   /* read value */
    ptr[1] = 42;                            /* and write next to it */
    nanotube_packet_write(packet, buf, 40 + platform_offset, 10);
    nanotube_packet_write(packet, (uint8_t*)&rd, 50 + platform_offset, 4);
  }

  /* Testing overlapping wide read, narrow write */
  {
    uint8_t buf[10];
    nanotube_packet_read(packet, buf, 60 + platform_offset, 10);
    uint32_t* ptr = ((uint32_t*)buf + 1);
    uint32_t rd = ptr[0];                   /* read value */
    buf[5] = 42;                            /* and write byte inside */
    nanotube_packet_write(packet, buf, 70 + platform_offset, 10);
    nanotube_packet_write(packet, (uint8_t*)&rd, 80 + platform_offset, 4);
  }

  /* Testing overlapping narrow read, wide write */
  {
    uint8_t buf[10];
    nanotube_packet_read(packet, buf, 90 + platform_offset, 10);
    uint32_t* ptr = ((uint32_t*)buf + 1);
    uint8_t rd = buf[5];                    /* read byte value */
    ptr[0] = 42;                            /* and overwrite with 32bit */
    nanotube_packet_write(packet, buf, 100 + platform_offset, 10);
    nanotube_packet_write(packet, (uint8_t*)&rd, 110 + platform_offset, 1);
  }

  return NANOTUBE_PACKET_PASS;
}

/**
 * Combine memory transfer and compute dependencies.
 */
nanotube_kernel_rc_t mem_compute(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  //size_t len = 0xdeadbeef;
  uint8_t pre_init[10];
  uint8_t packet_data[10];

  memset(pre_init, 42, sizeof(pre_init));

  nanotube_packet_read(packet, packet_data, 0 + platform_offset,
                       sizeof(packet_data));
  packet_data[0] += packet_data[1] - packet_data[2];
  nanotube_packet_write(packet, packet_data, 0 + platform_offset,
                        sizeof(packet_data));

  nanotube_packet_write(packet, pre_init, 10 + platform_offset,
                        sizeof(pre_init));

  uint8_t swizzled[10];
  memcpy(swizzled, packet_data, sizeof(packet_data));
  swizzled[4] = 1;
  swizzled[5] = 2;
  memcpy(swizzled + 6, pre_init, 2);

  nanotube_packet_write(packet, swizzled, 20 + platform_offset,
                        sizeof(swizzled));

  return NANOTUBE_PACKET_PASS;
}

void nanotube_setup() {
  nanotube_add_plain_packet_kernel("simple_mem", simple_mem,
                                   NANOTUBE_BUS_ID_SB, 1);
  nanotube_add_plain_packet_kernel("test_war", test_war,
                                   NANOTUBE_BUS_ID_SB, 1);
  nanotube_add_plain_packet_kernel("mem_compute", mem_compute,
                                   NANOTUBE_BUS_ID_SB, 1);
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
