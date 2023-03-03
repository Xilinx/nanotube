/**************************************************************************\
*//*! \file nanotube_private.hpp
** \author  Stephan Diestelhorst <stephand@amd.com>
**          Neil Turton <neilt@amd.com>
**  \brief  Nanotube internal definitions.
**   \date  2020-08-27
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef NANOTUBE_PRIVATE_HPP
#define NANOTUBE_PRIVATE_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>

/*! Read a map descriptor from a file.
 *
 * This only reads the descriptor, not the whole map.
 */
extern std::istream &operator>>(std::istream& is,
                                struct nanotube_map& desc);

#endif // NANOTUBE_PRIVATE_HPP
