/**************************************************************************\
*//*! \file nanotube_packet_metadata_shb.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube soft hub bus metadata.
**   \date  2022-03-22
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_packet_metadata_shb.h"
#include "softhub_bus.hpp"

#include <cstddef>

#ifdef __clang__
__attribute__((always_inline))
#endif
void
nanotube_packet_set_port_shb(nanotube_packet_t* packet,
                             nanotube_packet_port_t port)
{
  uint8_t buffer[softhub_bus::CH_ROUTE_RAW_PKT_LENGTH] = {0x0};
  static const uint8_t mask[1] = { 0xff };
  unsigned offset = softhub_bus::CH_ROUTE_RAW_PKT_OFFSET;
  nanotube_packet_read(packet, buffer, offset,
                       softhub_bus::CH_ROUTE_RAW_PKT_LENGTH);
  softhub_bus::set_ch_route_raw(buffer, port);
  nanotube_packet_write_masked(packet, buffer, mask, offset, 
                               softhub_bus::CH_ROUTE_RAW_PKT_LENGTH);
}
