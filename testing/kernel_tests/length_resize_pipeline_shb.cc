/**************************************************************************\
*//*! \file length_resize_pipeline_shb.cc
** \author  Stephan Diestelhorst <stephand@amd.com>
**  \brief  Testing translation of the length and resize taps.
**   \date  2021-10-04
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_api.h"
#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_bus.h"
#include "softhub_bus.hpp"

static const uint16_t platform_offset = sizeof(softhub_bus::header);
//static const uint16_t platform_header = 0;

nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet) {
  //size_t len = 0xdeadbeef;
  auto len = (uint16_t)nanotube_packet_bounded_length(packet, 65535);
  nanotube_packet_resize(packet, 0 + platform_offset, sizeof(len));
  nanotube_packet_write(packet, (uint8_t*)&len, 0 + platform_offset,
                        sizeof(len));

  return NANOTUBE_PACKET_PASS;
}


void nanotube_setup() {
  // Note: name adjusted to match manual test output :)
  nanotube_add_plain_packet_kernel("packets_in", process_packet, NANOTUBE_BUS_ID_SHB, 1 /*true*/);
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
