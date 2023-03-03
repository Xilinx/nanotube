/*******************************************************/
/*! \file ebpf_nt_adapter.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Adapters to make Nanotube functions look more like EBPF
**   \date 2020-10-02
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "ebpf_nt_adapter.h"

#include <asm/errno.h>
#include <assert.h>
#include <linux/bpf.h>

#include "nanotube_api.h"

/**
 * NOTE: The EBPF native functions report errors via a normal int32, and
 * use the typical Linux kernel errno values.  The user-space API needs the
 * user to read errno, while the functions themselves return that error
 * code directly.  The best description for the actual error values, check
 * the map syscall documentation ("man bpf").
 */
extern "C" {
#ifdef __clang__
__attribute__((always_inline))
#endif
int32_t map_op_adapter(nanotube_context_t* ctx, nanotube_map_id_t map_id,
                       enum map_access_t type, const uint8_t* key,
                       size_t key_length, const uint8_t* data_in,
                       uint8_t* data_out, const uint8_t* mask,
                       size_t offset, size_t data_length) {
  /* The call and arguments are the same .. */
  size_t ret = nanotube_map_op(ctx, map_id, type, key, key_length, data_in,
                               data_out, mask, offset, data_length);
  /* ... but we need to convert the return code. */
  if( ret == data_length )
    return 0; /* No error */

  //XXX: The interface has a more nuanced error code, but let's just be
  //     cheap for the moment
  return -EINVAL;
}

#ifdef __clang__
__attribute__((always_inline))
#endif
int32_t packet_adjust_head_adapter(nanotube_packet_t* p, int32_t offset) {
  /* bpf_xdp_adjust_head takes a delta argument, where a negative value
   * grows the packet (moves the head backwards). However
   * nanotube_packet_resize takes an adjust argument, where a negative
   * value shrinks the packet!
   */
  auto success = nanotube_packet_resize(p, 0, -offset);

  return success ? 0 : -1;
}

#ifdef __clang__
__attribute__((always_inline))
#endif
int32_t packet_adjust_meta_adapter(nanotube_packet_t* p, int32_t offset) {
  /* bpf_xdp_adjust_meta takes a delta argument, where a negative value
   * grows the packet (moves the head backwards). However
   * nanotube_packet_meta_resize takes an adjust argument, where a negative
   * value shrinks the packet!
   */
  auto success = nanotube_packet_meta_resize(p, 0, -offset);

  return success ? 0 : -1;
}

#ifdef __clang__
__attribute__((always_inline))
#endif
nanotube_kernel_rc_t packet_handle_xdp_result(nanotube_packet_t* packet, int xdp_ret) {
  switch( xdp_ret ) {
    case XDP_PASS:
      return NANOTUBE_PACKET_PASS;
      break;
    case XDP_DROP:
      /* Fall-through*/
    default:
      return NANOTUBE_PACKET_DROP;
  }
}

} // extern "C"


/* vim: set ts=8 et sw=2 sts=2 tw=75: */
