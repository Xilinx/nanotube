/*******************************************************/
/*! \file packet_highlevel.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Implement high-level packets with the low-level interface.
**   \date 2020-10-01
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <alloca.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DONT_INLINE_NANOTUBE_FUNCTIONS
#include "nanotube_api.h"

#ifdef __clang__
__attribute__((always_inline))
#endif
size_t nanotube_packet_write(nanotube_packet_t* packet,
                             const uint8_t* data_in, size_t offset,
                             size_t length) {
  size_t mask_size = (length + 7) / 8;
  uint8_t* all_one = (uint8_t*)alloca(mask_size);
  memset(all_one, 0xff, mask_size);

  return nanotube_packet_write_masked(packet, data_in, all_one, offset, length);
}

#ifdef __clang__
__attribute__((always_inline))
#endif
void
nanotube_merge_data_mask(uint8_t* inout_data, uint8_t* inout_mask,
                         const uint8_t* in_data, const uint8_t* in_mask,
                         size_t offset, size_t data_length) {
  for( size_t i = 0; i < data_length; i++ ) {
    size_t maskidx = i / 8;
    uint8_t bitidx = i % 8;
    size_t out_pos = offset + i;
    bool update = in_mask[maskidx] & (1 << bitidx);

    if( update ) {
      /* Update the overall data and mask */
      inout_data[out_pos] = in_data[i];
      size_t out_maskidx = out_pos / 8;
      uint8_t out_bitidx = out_pos % 8;
      inout_mask[out_maskidx] |= (1 << out_bitidx);
    }
  }
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
