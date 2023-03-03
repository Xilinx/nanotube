/*******************************************************/
/*! \file base_map.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**         Neil Turton <neilt@amd.com>
**  \brief Implementation of Nanotube maps.
**   \date 2020-08-26
*//*****************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

//XXX: This file needs merging with nanotube_map.cpp
#include "nanotube_map.hpp"

#include "nanotube_private.hpp"

std::ostream& nanotube_map::print(std::ostream &os) {
  os << "nanotube_map: " << id << " " << get_type() << " "
     << key_sz << " " << value_sz <<'\n';
  print_entries(os);
  os << "end\n\n";
  return os;
}

