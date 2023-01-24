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
  //TODO somehow set the relevant sideband bytes to store the port
  //e.g. by calling x3rx_bus::set_sideband()
}
