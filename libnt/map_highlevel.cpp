/*******************************************************/
/*! \file map_highlevel.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Implement the high-level maps interface with the low-level one.
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
#include "nanotube_capsule.h"

#ifdef __clang__
__attribute__((always_inline))
#endif
size_t nanotube_map_read(nanotube_context_t* ctx, nanotube_map_id_t id,
                         const uint8_t* key, size_t key_length,
                         uint8_t* data_out, size_t offset,
                         size_t data_length) {
  return nanotube_map_op(ctx, id, NANOTUBE_MAP_READ, key, key_length,
                         NULL /* data_in */, data_out,
                         NULL /* enables */, offset, data_length);
}

#ifdef __clang__
__attribute__((always_inline))
#endif
size_t nanotube_map_write(nanotube_context_t* ctx, nanotube_map_id_t id,
                          const uint8_t* key, size_t key_length,
                          const uint8_t* data_in, size_t offset,
                          size_t data_length) {
  size_t mask_size = (data_length + 7) / 8;
  uint8_t* all_one = (uint8_t*)alloca(mask_size);
  memset(all_one, 0xff, mask_size);
  return nanotube_map_op(ctx, id, NANOTUBE_MAP_WRITE, key, key_length,
                         data_in, NULL /* data_out */,
                         all_one,  offset, data_length);
}

#ifdef __clang__
__attribute__((always_inline))
#endif
void nanotube_map_process_capsule(nanotube_context_t *ctxt,
                                  nanotube_map_id_t map_id,
                                  uint8_t *capsule,
                                  size_t key_length,
                                  size_t value_length)
{
#ifndef NANOTUBE_CAPSULE_LITTLE_ENDIAN
#error "Expected little-endian format."
#endif

  // Get pointers to the key and value.
  uint8_t *key_ptr = 
    ( capsule +
      NANOTUBE_CAPSULE_MAP_KEY_OFFSET(key_length, value_length) );
  uint8_t *value_ptr =
    ( capsule +
      NANOTUBE_CAPSULE_MAP_VALUE_OFFSET(key_length, value_length) );

  // Generate a mask which is required for some operations.
  size_t mask_size = (value_length + 7) / 8;
  uint8_t* all_one = (uint8_t*)alloca(mask_size);
  memset(all_one, 0xff, mask_size);

  // Extract the opcode.
  static_assert(NANOTUBE_CAPSULE_MAP_OPCODE_SIZE == 2,
                "Opcode size mismatch.");
  auto opcode = ( (uint16_t(capsule[NANOTUBE_CAPSULE_MAP_OPCODE_OFFSET+0]) << 0) |
                  (uint16_t(capsule[NANOTUBE_CAPSULE_MAP_OPCODE_OFFSET+1]) << 8) );
  enum map_access_t access = NANOTUBE_MAP_NOP;

  // Perform the relevant operation.
  switch (opcode) {
  case NANOTUBE_CAPSULE_MAP_OPCODE_READ:
    access = NANOTUBE_MAP_READ;
    break;

  case NANOTUBE_CAPSULE_MAP_OPCODE_INSERT:
    access = NANOTUBE_MAP_INSERT;
    break;

  case NANOTUBE_CAPSULE_MAP_OPCODE_UPDATE:
    access = NANOTUBE_MAP_UPDATE;
    break;

  case NANOTUBE_CAPSULE_MAP_OPCODE_WRITE:
    access = NANOTUBE_MAP_WRITE;
    break;

  case NANOTUBE_CAPSULE_MAP_OPCODE_REMOVE:
    access = NANOTUBE_MAP_READ;
    break;

  case NANOTUBE_CAPSULE_MAP_OPCODE_NEXT_KEY:
  default:
    break;
  }

  nanotube_map_op(ctxt, map_id, access, key_ptr, key_length,
                  value_ptr, value_ptr, all_one, 0, value_length);
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
