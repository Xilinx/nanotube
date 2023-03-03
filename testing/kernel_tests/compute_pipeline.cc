/**************************************************************************\
*//*! \file compute_pipeline.cc
** \author  Stephan Diestelhorst <stephand@amd.com>
**  \brief  Simple packet manipulation logic for testing.
**   \date  2021-09-21
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

enum {
  ADD = 0,
  NAND,
  CMP,
  COPY,
  XCHG,
  ILLEGAL,
};
struct op_bundle_t {
  uint8_t op;
  uint8_t src0;
  uint8_t src1;
  uint8_t dst;
};

#define MANUAL_CONVERGE
nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  /* Read the OP bundle */
  op_bundle_t opb;
  nanotube_packet_read(packet, (uint8_t*)&opb, 0 + platform_offset, sizeof(opb));

  /* Sanity check */
#ifdef MANUAL_CONVERGE
  //XXX: Needs a converged version of this
#else
  if( opb.op >= ILLEGAL )
    goto drop;
#endif

  /* XXX: How does a length check work in the wordified model? */
#if 0
  if( opb.src0 + 4 + sizeof(opb) > packet_len ||
      opb.src1 + 4 + sizeof(opb) > packet_len ||
      opb.dst  + 4 + sizeof(opb) > packet_len )
    goto drop;
#endif

  {
    uint16_t loc0 = (uint16_t)opb.src0 + sizeof(opb);
    uint16_t loc1 = (uint16_t)opb.src1 + sizeof(opb);
    uint16_t dloc = (uint16_t)opb.dst  + sizeof(opb);

    uint32_t src0, src1, dst;
    /* Read the operands, their locations are relative to after the header! */
    nanotube_packet_read(packet, (uint8_t*)&src0, loc0 + platform_offset, 4);
#ifdef MANUAL_CONVERGE
    nanotube_packet_read(packet, (uint8_t*)&src1, loc1 + platform_offset,
        (opb.op != COPY) ? 4 : 0);
#else
    if( opb.op != COPY) {
      nanotube_packet_read(packet, (uint8_t*)&src1, loc1 + platform_offset, 4);
    }
#endif

    /* Perform the actual operation, nothing too exciting to see here, one should think :) */
#ifndef MANUAL_CONVERGE
    bool dst_write = true;
#endif
    switch( opb.op ) {
      case ADD:
        dst = src0 + src1;
        break;
      case NAND:
        dst = ~(src0 & src1);
        break;
      case CMP:
        dst = (src0 < src1) ? 255 : ((src0 == src1) ? 0 : 1 );
        break;
      case COPY:
        dst = src0;
        break;
      case XCHG: {
#ifdef MANUAL_CONVERGE
        break;
#else
        /* XCHG is special as it performs in place! */
        nanotube_packet_write(packet, (uint8_t*)&src0, loc1 + platform_offset, 4);
        nanotube_packet_write(packet, (uint8_t*)&src1, loc0 + platform_offset, 4);
        dst_write = false;
#endif
        break;
      }

      default:
#ifdef MANUAL_CONVERGE
        //XXX: this needs to get converged properly!
        dst = src0;
        break;
#else
        goto drop;
#endif
    }

#ifdef MANUAL_CONVERGE
    /* By default write dest to dloc; or src0 to loc1 */
    nanotube_packet_write(packet,
                          (opb.op != XCHG) ? (uint8_t*)&dst : (uint8_t*)&src0,
                          ((opb.op != XCHG) ? dloc : loc1) + platform_offset, 4);
    /* If needed write src1 to loc0, nothing otherwise */
    nanotube_packet_write(packet, (uint8_t*)&src1, loc0 + platform_offset,
                          (opb.op != XCHG) ? 0 : 4);
#else
    /* Write the result if needed */
    if( dst_write )
      nanotube_packet_write(packet, (uint8_t*)&dst, dloc + platform_offset, 4);
#endif
    return NANOTUBE_PACKET_PASS;
  }

#ifndef MANUAL_CONVERGE
drop:
#endif
  return NANOTUBE_PACKET_DROP;
}


void nanotube_setup() {
  // Note: name adjusted to match manual test output :)
  nanotube_add_plain_packet_kernel("packets_in", process_packet, NANOTUBE_BUS_ID_SB, true);
}
