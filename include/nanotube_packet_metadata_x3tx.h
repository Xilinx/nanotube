/**************************************************************************\
*//*! \file nanotube_packet_metadata_x3tx.h
** \author  Kieran Mansley <kieran.mansley@amd.com>
**  \brief  Nanotube x3tx bus metadata.
**   \date  2022-03-22
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PACKET_METADATA_X3TX_H
#define NANOTUBE_PACKET_METADATA_X3TX_H

#include "nanotube_api.h"

extern "C" void
nanotube_packet_set_port_x3tx(nanotube_packet_t* packet,
                              nanotube_packet_port_t port);

extern "C" nanotube_capsule_class_t
nanotube_capsule_classify_x3tx(nanotube_packet_t* packet,
                               size_t *offset_out);

#endif // NANOTUBE_PACKET_METADATA_X3TX_H
