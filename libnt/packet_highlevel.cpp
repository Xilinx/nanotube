#include <iostream>
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
#include <endian.h>
#include <net/ethernet.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DONT_INLINE_NANOTUBE_FUNCTIONS
#include "nanotube_api.h"

#ifdef NANOTUBE_SYNTHESIS
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

#ifdef NANOTUBE_SYNTHESIS
__attribute__((always_inline))
#endif
void
nanotube_init_mask(uint8_t* mask_out,
                   size_t mask_byte_size,
                   size_t ones_bit_start,
                   size_t ones_bit_length)
{
  size_t ones_bit_end = ones_bit_start + ones_bit_length;

  /* Loop through the bytes of the bitmask. */
  for( size_t i = 0; i < mask_byte_size; i++ ) {
    size_t iter_bit_start = 8 * i;
    size_t iter_bit_end = iter_bit_start + 8;

    uint8_t after_start_mask;
    if( iter_bit_end <= ones_bit_start ) {
      /* The ones haven't started yet. */
      after_start_mask = 0x00;
    } else if( iter_bit_start <= ones_bit_start ) {
      /* The ones start within this byte.  The bit offset will be
       * between 0 and 7. */
      uint8_t bit_offset = ones_bit_start - iter_bit_start;
      after_start_mask = -(1 << bit_offset);
    } else {
      /* The ones bits have already started. */
      after_start_mask = 0xff;
    }

    uint8_t after_end_mask;
    if( iter_bit_end <= ones_bit_end ) {
      /* The ones haven't ended yet. */
      after_end_mask = 0x00;
    } else if( iter_bit_start <= ones_bit_end ) {
      /* The ones end within this byte.  The bit offset will be
       * between 0 and 7. */
      uint8_t bit_offset = ones_bit_end - iter_bit_start;
      after_end_mask = -(1 << bit_offset);
    } else {
      /* The ones bits have already ended. */
      after_end_mask = 0xff;
    }

    mask_out[i] = after_start_mask & ~after_end_mask;
  }
}

#ifdef NANOTUBE_SYNTHESIS
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

#ifdef NANOTUBE_SYNTHESIS
__attribute__((always_inline))
#endif
nanotube_capsule_class_t
nanotube_capsule_classify_eth_encap(nanotube_packet_t* packet,
                                    size_t md_size,
                                    size_t *offset_out)
{
  uint16_t ether_type;
  size_t offset = md_size + offsetof(struct ether_header, ether_type);

  (void)nanotube_packet_read(packet, (uint8_t*)&ether_type, offset,
                             sizeof(ether_type));

  *offset_out = md_size + sizeof(struct ether_header);

  if (ether_type == htobe16(ETHERTYPE_NANOTUBE_CONTROL))
    return NANOTUBE_CAPSULE_CLASS_CONTROL;

  return NANOTUBE_CAPSULE_CLASS_NETWORK;
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
