/*******************************************************/
/*! \file nanotube_map.hpp
** \author Neil Turton <neilt@amd.com>,
**         Stephan Diestelhorst <stephand@amd.com>
**  \brief Implementation of Nanotube maps.
**   \date 2020-08-26
*//*****************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_MAP_HPP
#define NANOTUBE_MAP_HPP

#include "nanotube_api.h"

#include <cstdint>
#include <ostream>

/*!
** A Nanotube map as an abstract class with a few common concrete fields.
**
** This class is the base for various map implementations that applications
** can use.  Applications will never see the content of this data structure,
** but instead only interact through a C-style API with it.
**/
struct nanotube_map {
public:
  virtual uint8_t* lookup(const uint8_t* key)                          = 0;
  virtual uint8_t* insert_empty(const uint8_t* key)                    = 0;
  virtual bool remove(const uint8_t* key)                              = 0;
  virtual std::ostream& print_entries(std::ostream &os) const          = 0;
  virtual ~nanotube_map() {};
  enum map_type_t get_type() const { return type; }

  std::ostream& print(std::ostream &os);

  nanotube_map(nanotube_map_id_t id, size_t key_sz, size_t value_sz,
               uint32_t max_entries, map_type_t type, uint32_t flags) :
               id(id), key_sz(key_sz), value_sz(value_sz),
               max_entries(max_entries), type(type), flags(flags) {}

  nanotube_map_id_t id;
  size_t            key_sz;
  size_t            value_sz;
  uint32_t          max_entries;
  map_type_t        type;
  uint32_t          flags;
};

/*!
** Print out the specified map C++ stle.
**/
std::ostream& operator<<(std::ostream &os, nanotube_map *map);
std::istream& operator>>(std::istream &is, nanotube_map *&map);
nanotube_map* nanotube_map_read_from(std::istream& is);

#endif // NANOTUBE_MAP_HPP
