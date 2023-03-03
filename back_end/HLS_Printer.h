/**************************************************************************\
*//*! \file HLS_Printer.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube HLS printer
**   \date  2019-02-27
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_HLS_PRINTER_H
#define NANOTUBE_HLS_PRINTER_H

#include "llvm_pass.h"

#include <string>

namespace nanotube {
Pass *create_hls_printer(const std::string &output_directory, bool overwrite);
}

#endif // NANOTUBE_HLS_PRINTER_H
