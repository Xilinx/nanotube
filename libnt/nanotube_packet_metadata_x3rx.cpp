/**************************************************************************\
*//*! \file nanotube_packet_metadata_x3rx.cpp
** \author  Kieran Mansley <kieran.mansley@amd.com>
**  \brief  Nanotube x3rx bus metadata.
**   \date  2023-01-17
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_packet_metadata_x3rx.h"
#include "x3rx_bus.hpp"

#include <cstddef>
#ifdef __clang__
__attribute__((always_inline))
#endif
void
nanotube_packet_set_port_x3rx(nanotube_packet_t* packet,
                              nanotube_packet_port_t port)
{
  // Not yet supported.  No metadata on X3RX to store the port in
}

#ifdef __clang__
__attribute__((always_inline))
#endif
nanotube_capsule_class_t
nanotube_capsule_classify_x3rx(nanotube_packet_t* packet,
                               size_t *offset_out)
{
  // No metadata on X3RX.
  *offset_out = 0;
  return NANOTUBE_CAPSULE_CLASS_NETWORK;
}
