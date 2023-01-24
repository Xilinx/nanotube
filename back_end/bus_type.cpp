/**************************************************************************\
*//*! \file bus_type.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube bus type functions.
**   \date  2022-03-14
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "bus_type.hpp"
#include "simple_bus.hpp"
#include "softhub_bus.hpp"
#include "utils.h"
#include "x3rx_bus.hpp"

#include "llvm/Support/CommandLine.h"

#include <unordered_map>

using namespace nanotube;

///////////////////////////////////////////////////////////////////////////

static llvm::cl::opt<std::string>
opt_bus("bus", llvm::cl::desc("Which bus format to use"),
        llvm::cl::init("sb"));

std::unordered_map<std::string, enum nanotube_bus_id_t> bus_types = {
  // Recommended names.
  { "sb", NANOTUBE_BUS_ID_SB },
  { "shb", NANOTUBE_BUS_ID_SHB },
  { "x3rx", NANOTUBE_BUS_ID_X3RX },
};

///////////////////////////////////////////////////////////////////////////

enum nanotube_bus_id_t nanotube::get_bus_type()
{
  auto it = bus_types.find(opt_bus.getValue());
  if (it == bus_types.end()) {
    report_fatal_errorv("Unsupported bus type '{0}'.",
                        opt_bus.getValue());
  }
  return it->second;
}

///////////////////////////////////////////////////////////////////////////

const char* nanotube::get_bus_suffix(enum nanotube_bus_id_t type) {
  const char* bus_suffixes[] = {
    "_sb",
    "_shb",
    "_eth",
    "_x3rx",
  };

  /* Make sure that the array here matches what is defined in the enum in
   * nanotube_packet_taps_bus.h */
  assert(type < NANOTUBE_BUS_ID_TOTAL);
  static_assert(NANOTUBE_BUS_ID_TOTAL ==
                sizeof(bus_suffixes) / sizeof(bus_suffixes[0]),
                "Wrong number of members in bus_suffixes!");

  return bus_suffixes[type];
}

///////////////////////////////////////////////////////////////////////////

nanotube_tap_offset_t nanotube::get_bus_word_size() {
  /* For now assume that each bus has only a single bus word size.  Of course,
   * this can be made more complex in the future and the user might want to
   * select simple-bus-32 vs simple-bus-64 etc. */
  nanotube_tap_offset_t word_sizes[] = {
    sizeof(simple_bus::word),
    sizeof(softhub_bus::word),
    0,
    sizeof(x3rx_bus::word),
  };

  auto type = get_bus_type();
  /* Make sure that the array here matches what is defined in the enum in
   * nanotube_packet_taps_bus.h */
  assert(type < NANOTUBE_BUS_ID_TOTAL);
  static_assert(NANOTUBE_BUS_ID_TOTAL ==
                sizeof(word_sizes) / sizeof(word_sizes[0]),
                "Wrong number of members in word_sizes!");
  return word_sizes[type];
}

///////////////////////////////////////////////////////////////////////////

nanotube_packet_size_t nanotube::get_bus_md_size()
{
  nanotube_packet_size_t packet_sizes[] = {
    sizeof(simple_bus::header),
    sizeof(softhub_bus::header),
    0,
    sizeof(x3rx_bus::header),
  };

  auto type = get_bus_type();
  /* Make sure that the array here matches what is defined in the enum in
   * nanotube_packet_taps_bus.h */
  assert(type < NANOTUBE_BUS_ID_TOTAL);
  static_assert(NANOTUBE_BUS_ID_TOTAL ==
                sizeof(packet_sizes) / sizeof(packet_sizes[0]),
                "Wrong number of members in packet_sizes!");
  return packet_sizes[type];
}

///////////////////////////////////////////////////////////////////////////

nanotube_packet_size_t nanotube::get_bus_sb_size()
{
  nanotube_packet_size_t sideband_sizes[] = {
    simple_bus::sideband_bytes,
    softhub_bus::sideband_bytes,
    0,
    x3rx_bus::sideband_bytes,
  };

  auto type = get_bus_type();
  /* Make sure that the array here matches what is defined in the enum in
   * nanotube_packet_taps_bus.h */
  assert(type < NANOTUBE_BUS_ID_TOTAL);
  static_assert(NANOTUBE_BUS_ID_TOTAL ==
                sizeof(sideband_sizes) / sizeof(sideband_sizes[0]),
                "Wrong number of members in sideband_sizes!");
  return sideband_sizes[type];
}

///////////////////////////////////////////////////////////////////////////

nanotube_packet_size_t nanotube::get_bus_sb_signals_size()
{
  nanotube_packet_size_t sideband_signals_sizes[] = {
    simple_bus::sideband_signals_bytes,
    softhub_bus::sideband_signals_bytes,
    0,
    x3rx_bus::sideband_signals_bytes,
  };

  auto type = get_bus_type();
  /* Make sure that the array here matches what is defined in the enum in
   * nanotube_packet_taps_bus.h */
  assert(type < NANOTUBE_BUS_ID_TOTAL);
  static_assert(NANOTUBE_BUS_ID_TOTAL ==
                sizeof(sideband_signals_sizes) / sizeof(sideband_signals_sizes[0]),
                "Wrong number of members in sideband_signals_sizes!");
  return sideband_signals_sizes[type];
}
