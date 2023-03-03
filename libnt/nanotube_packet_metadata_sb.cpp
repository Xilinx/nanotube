/**************************************************************************\
*//*! \file nanotube_packet_metadata_sb.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube simple bus metadata.
**   \date  2022-03-22
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_capsule.h"
#include "nanotube_packet_metadata_sb.h"
#include "simple_bus.hpp"

#include <cstddef>

#ifdef __clang__
__attribute__((always_inline))
#endif
void
nanotube_packet_set_port_sb(nanotube_packet_t* packet,
                            nanotube_packet_port_t port)
{
  static_assert(sizeof(simple_bus::header::port) == 1,
                "port field size is incorrect");
  uint8_t buffer[1] = {port};
  static const uint8_t mask[1] = { 0xff };
  unsigned offset = offsetof(simple_bus::header, port);
  nanotube_packet_write_masked(packet, buffer, mask, offset, 1);
}

#ifdef __clang__
__attribute__((always_inline))
#endif
nanotube_capsule_class_t
nanotube_capsule_classify(nanotube_packet_t* packet)
{
  static_assert(sizeof(simple_bus::header::port) == 1,
                "port field size is incorrect");
  uint8_t buffer[1] = {0};
  unsigned offset = offsetof(simple_bus::header, port);
  nanotube_packet_read(packet, buffer, offset, 1);
  switch (buffer[0]) {
  case NANOTUBE_PORT_CONTROL: return NANOTUBE_CAPSULE_CLASS_CONTROL;
  default:                    return NANOTUBE_CAPSULE_CLASS_NETWORK;
  }
}
