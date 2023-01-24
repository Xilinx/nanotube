/**************************************************************************\
*//*! \file nanotube_packet_metadata_x3rx.h
** \author  Kieran Mansley <kieran.mansley@amd.com>
**  \brief  Nanotube x3rx bus metadata.
**   \date  2022-03-22
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PACKET_METADATA_X3RX_H
#define NANOTUBE_PACKET_METADATA_X3RX_H

#include "nanotube_api.h"

extern "C" void
nanotube_packet_set_port_x3rx(nanotube_packet_t* packet,
                              nanotube_packet_port_t port);

#endif // NANOTUBE_PACKET_METADATA_X3RX_H
