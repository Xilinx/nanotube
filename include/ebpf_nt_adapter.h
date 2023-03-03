/*******************************************************/
/*! \file ebpf_nt_adapter.h
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Adapters to make Nanotube functions look more like EBPF
**   \date 2020-10-02
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <stdint.h>

#include "nanotube_api.h"

/*!
** Perform an operation (map_access_t) on the specified map.
** \param map_id     Map-ID of the map to work on
** \param type       Type of the access
** \param key        Key of the entry that will be accessed
** \param key_length Size of the key
** \param data_in    Data to be written (including data that is not written
**                   due to mask!)
** \param data_out   Buffer for data read by reads (caller allocated!)
** \param mask       Byte enables: each set bit in mask enables the
**                   corresponding byte to be written.  The offsets are
**                   relative to data__in.
** \param offset     Offset in bytes in the map value to access
** \param length     Total size of access (including data that will
**                   not be written due to mask.)
** \return           Linux-style error message.
**/
//XXX: Specialise that to bpf_map_update
#ifdef __cplusplus
extern "C" {
#endif
extern
int32_t map_op_adapter(nanotube_context_t* ctx, nanotube_map_id_t map_id,
                       enum map_access_t type, const uint8_t* key,
                       size_t key_length, const uint8_t* data_in,
                       uint8_t* data_out, const uint8_t* mask,
                       size_t offset, size_t data_length);

/*!
** Move the head of a packet either increasing (negative values) or
** decreasing (positive offsets) the packet size.  Applications should not
** use old packet pointers, nor rely on the packet being copied.  The
** implementation is free to do either.
**
** \param packet   The packet to adjust.
** \param offset   The offset by which to move the head.
** \return         Linux-style error message.
**/
extern
int32_t packet_adjust_head_adapter(nanotube_packet_t* packet,
                                   int32_t offset);

/*!
** Set the packet port so that it matches XDP semantics.
** \param packet   The packet to adjust the port.
** \param xdp_ret  The XDP return code.
**/
extern
nanotube_kernel_rc_t packet_handle_xdp_result(nanotube_packet_t* packet,
                                              int xdp_ret);
#ifdef __cplusplus
}
#endif


/* vim: set ts=8 et sw=2 sts=2 tw=75: */
