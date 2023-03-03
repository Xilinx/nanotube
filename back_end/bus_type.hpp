/**************************************************************************\
*//*! \file bus_type.hpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube bus type functions.
**   \date  2022-03-14
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_BUS_TYPE_HPP
#define NANOTUBE_BUS_TYPE_HPP

#include "nanotube_api.h"
#include "nanotube_packet_taps.h"

#include "bus_id.h"

///////////////////////////////////////////////////////////////////////////

namespace nanotube
{
  // Return the currently selected bus type.
  enum nanotube_bus_id_t get_bus_type();

  // Return the size of the metadata.  This is a bus-defined header
  // that typically precedes each packet. E.g. for the keystone
  // streaming sub-system this is the capsule header.
  nanotube_packet_size_t get_bus_md_size();
  
  // Return the size of the sideband.
  // In AXIS these bytes are TUSER and can represent an additional
  // channel of data that flows along with the main data path 
  nanotube_packet_size_t get_bus_sb_size();
  
  // Return the size of the sideband signals.
  // In AXIS these are TKEEP/TSTRB/TLAST and should be a function of 
  // the data width.  They are not normally of interest to the kernel
  // but must be correctly maintained
  nanotube_packet_size_t get_bus_sb_signals_size();

  // Returns a consistent suffix for specialised bus functions
  const char* get_bus_suffix(enum nanotube_bus_id_t type);

  // Return the width of a bus word of the currently selected bus
  nanotube_tap_offset_t get_bus_word_size();
} // namespace nanotube

#endif // NANOTUBE_BUS_TYPE_HPP
