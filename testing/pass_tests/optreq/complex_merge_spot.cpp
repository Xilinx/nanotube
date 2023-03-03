/*******************************************************/
/*! \file  complex_merge_spot.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Testing more complex mergeable writes.
**   \date 2021-11-24
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <stdint.h>
#include <string.h>
#include "nanotube_api.h"

void complex_test(nanotube_context_t *nt_ctx,
                 nanotube_packet_t *packet)
{
  if( nanotube_packet_bounded_length(packet, 19) < 19 )
    return ;

  uint8_t pos;
  nanotube_packet_read(packet, &pos, 0, 1);
  bool cond0 = (pos & 1);
  bool cond1 = (pos & 2);
  bool cond2 = (pos & 4);
  bool cond3 = (pos & 8);
  pos = pos & 15;
  uint8_t d[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  if( cond0 ) {
    if( cond1 ) {
      nanotube_packet_write(packet, &d[1], 1, 1);
      nanotube_packet_write(packet, &d[2], 2, 1);
    } else {
      nanotube_packet_write(packet, &d[3], 2, 1);
      nanotube_packet_write(packet, &d[4], 1, 1);
    }
    nanotube_packet_write(packet, &d[5], 3, 1);
  } else {
    if( cond1 ) {
      nanotube_packet_write(packet, &d[6], pos + 1, 1);
      nanotube_packet_write(packet, &d[7], pos + 2, 1);
    } else {
      nanotube_packet_write(packet, &d[8], pos + 2, 1);
      nanotube_packet_write(packet, &d[9], pos + 1, 1);
    }
    if( cond2 )
      d[10] += 37;
    if( cond3 ) {
      uint8_t tmp;
      nanotube_packet_read(packet, &tmp, 1, 1);
      d[10] += tmp;
    }
    nanotube_packet_write(packet, &d[10], pos + 3, 1);
  }
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
