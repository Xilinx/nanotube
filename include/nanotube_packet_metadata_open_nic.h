/**************************************************************************\
*//*! \file nanotube_packet_metadata_open_nic.h
** \author  Simone Mannarino
**  \brief  Nanotube OpenNIC bus metadata.
**   \date  2025-06-18
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PACKET_METADATA_OPEN_NIC_H
#define NANOTUBE_PACKET_METADATA_OPEN_NIC_H

#include "nanotube_api.h"

extern "C" void
nanotube_packet_set_port_open_nic(nanotube_packet_t* packet,
                              nanotube_packet_port_t port);

#endif // NANOTUBE_PACKET_METADATA_OPEN_NIC_H
