/*******************************************************/
/*! \file  flatten_phi.cc
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Test phi node flattening.
**   \date 2022-12-07
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nanotube_api.h"

#define phi_case(N, inp, outp) \
  if( inp & (1L << N) ) {      \
    outp = N;                  \
    goto done;                 \
  }

nanotube_kernel_rc_t phi2(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;
  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi3(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;
  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi4(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi5(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi6(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi7(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi8(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);
  phi_case(7, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi9(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);
  phi_case(7, one_hot, val);
  phi_case(8, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi10(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);
  phi_case(7, one_hot, val);
  phi_case(8, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi15(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);
  phi_case(7, one_hot, val);
  phi_case(8, one_hot, val);
  phi_case(9, one_hot, val);
  phi_case(10, one_hot, val);
  phi_case(11, one_hot, val);
  phi_case(12, one_hot, val);
  phi_case(13, one_hot, val);
  phi_case(14, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi16(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);
  phi_case(7, one_hot, val);
  phi_case(8, one_hot, val);
  phi_case(9, one_hot, val);
  phi_case(10, one_hot, val);
  phi_case(11, one_hot, val);
  phi_case(12, one_hot, val);
  phi_case(13, one_hot, val);
  phi_case(14, one_hot, val);
  phi_case(15, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi17(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);
  phi_case(7, one_hot, val);
  phi_case(8, one_hot, val);
  phi_case(9, one_hot, val);
  phi_case(10, one_hot, val);
  phi_case(11, one_hot, val);
  phi_case(12, one_hot, val);
  phi_case(13, one_hot, val);
  phi_case(14, one_hot, val);
  phi_case(15, one_hot, val);
  phi_case(16, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi23(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);
  phi_case(7, one_hot, val);
  phi_case(8, one_hot, val);
  phi_case(9, one_hot, val);
  phi_case(10, one_hot, val);
  phi_case(11, one_hot, val);
  phi_case(12, one_hot, val);
  phi_case(13, one_hot, val);
  phi_case(14, one_hot, val);
  phi_case(15, one_hot, val);
  phi_case(16, one_hot, val);
  phi_case(17, one_hot, val);
  phi_case(18, one_hot, val);
  phi_case(19, one_hot, val);
  phi_case(20, one_hot, val);
  phi_case(21, one_hot, val);
  phi_case(22, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi24(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);
  phi_case(7, one_hot, val);
  phi_case(8, one_hot, val);
  phi_case(9, one_hot, val);
  phi_case(10, one_hot, val);
  phi_case(11, one_hot, val);
  phi_case(12, one_hot, val);
  phi_case(13, one_hot, val);
  phi_case(14, one_hot, val);
  phi_case(15, one_hot, val);
  phi_case(16, one_hot, val);
  phi_case(17, one_hot, val);
  phi_case(18, one_hot, val);
  phi_case(19, one_hot, val);
  phi_case(20, one_hot, val);
  phi_case(21, one_hot, val);
  phi_case(22, one_hot, val);
  phi_case(23, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}
nanotube_kernel_rc_t phi25(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = -1;

  phi_case(0, one_hot, val);
  phi_case(1, one_hot, val);
  phi_case(2, one_hot, val);
  phi_case(3, one_hot, val);
  phi_case(4, one_hot, val);
  phi_case(5, one_hot, val);
  phi_case(6, one_hot, val);
  phi_case(7, one_hot, val);
  phi_case(8, one_hot, val);
  phi_case(9, one_hot, val);
  phi_case(10, one_hot, val);
  phi_case(11, one_hot, val);
  phi_case(12, one_hot, val);
  phi_case(13, one_hot, val);
  phi_case(14, one_hot, val);
  phi_case(15, one_hot, val);
  phi_case(16, one_hot, val);
  phi_case(17, one_hot, val);
  phi_case(18, one_hot, val);
  phi_case(19, one_hot, val);
  phi_case(20, one_hot, val);
  phi_case(21, one_hot, val);
  phi_case(22, one_hot, val);
  phi_case(23, one_hot, val);
  phi_case(24, one_hot, val);

done:
  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}

nanotube_kernel_rc_t value_merge(nanotube_context_t *nt_ctx, nanotube_packet_t *packet) {
  uint8_t* data = nanotube_packet_data(packet);
  uint8_t* end  = nanotube_packet_end(packet);

  if( end - data < 8 ) {
    return NANOTUBE_PACKET_DROP;
  }
  uint32_t one_hot = *(uint32_t*)data;

  uint32_t val = 0xdeadbeef;

  switch( one_hot ) {
    case 0x1:
      val = 42;
      break;
    case 0x2:
      val = 7;
      break;
    case 0x4:
      val = 9;
      break;
    case 0x8:
      val = 42;
      break;
    case 0x10:
      val = 7;
      break;
    case 0x20:
      val = 9;
      break;
    case 0x40:
      val = 42;
      break;
    case 0x80:
      val = 9;
      break;
    default:
      /* intentionally left blank */
      ;
  }

  ((uint32_t*)data)[1] = val;
  return NANOTUBE_PACKET_PASS;

}

void nanotube_setup() {
  nanotube_add_plain_packet_kernel("phi2", phi2, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi3", phi3, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi4", phi4, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi5", phi5, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi6", phi6, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi7", phi7, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi8", phi8, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi9", phi9, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi10", phi10, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi15", phi15, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi16", phi16, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi17", phi17, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi23", phi23, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi24", phi24, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("phi25", phi25, -1, 0 /*false*/);
  nanotube_add_plain_packet_kernel("value_merge", value_merge, -1, 0 /*false*/);
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
