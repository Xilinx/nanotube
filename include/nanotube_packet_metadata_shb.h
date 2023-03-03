/**************************************************************************\
*//*! \file nanotube_packet_metadata_shb.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube soft hub bus metadata.
**   \date  2022-03-22
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PACKET_METADATA_SHB_H
#define NANOTUBE_PACKET_METADATA_SHB_H

#include "nanotube_api.h"

extern "C" void
nanotube_packet_set_port_shb(nanotube_packet_t* packet,
                             nanotube_packet_port_t port);

#endif // NANOTUBE_PACKET_METADATA_SHB_H
