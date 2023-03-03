/**************************************************************************\
*//*! \file bus_id.h
** \author  Kieran Mansley <kieranm@amd.com>
**  \brief  Nanotube bus id type
**   \date  2022-07-15
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_BUS_ID_H
#define NANOTUBE_BUS_ID_H

enum nanotube_bus_id_t {
  NANOTUBE_BUS_ID_SB = 0,
  NANOTUBE_BUS_ID_SHB = 1,
  NANOTUBE_BUS_ID_ETH = 2,
  NANOTUBE_BUS_ID_TOTAL
};

#endif /* NANOTUBE_BUS_ID_H */
