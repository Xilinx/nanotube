/**************************************************************************\
*//*! \file nanotube_packet_metadata_x3tx.cpp
** \author  Kieran Mansley <kieran.mansley@amd.com>
**  \brief  Nanotube x3tx bus metadata.
**   \date  2023-01-17
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_packet_metadata_x3tx.h"
#include "x3tx_bus.hpp"

#include <cstddef>
#ifdef __clang__
__attribute__((always_inline))
#endif
void
nanotube_packet_set_port_x3tx(nanotube_packet_t* packet,
                              nanotube_packet_port_t port)
{
  static_assert(sizeof(x3tx_bus::header::port) == 1,
                "port field size is incorrect");
  uint8_t buffer[1] = {port};
  static const uint8_t mask[1] = { 0xff };
  unsigned offset = offsetof(x3tx_bus::header, port);
  nanotube_packet_write_masked(packet, buffer, mask, offset, 1);
}

#ifdef __clang__
__attribute__((always_inline))
#endif
nanotube_capsule_class_t
nanotube_capsule_classify_x3tx(nanotube_packet_t* packet)
{
  return NANOTUBE_CAPSULE_CLASS_NETWORK;
}
