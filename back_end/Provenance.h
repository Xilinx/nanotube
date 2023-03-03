/*******************************************************/
/*! \file Provenance.h
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Support for tracking Nanotube values.
**   \date 2020-07-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <map>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"

namespace nanotube {
 typedef uint8_t nt_prov;
 enum : nt_prov {
   NONE  =  0,
   STACK =  1,
   MAP   =  2,
   PKT   =  4,
   CTX   =  8,
   OTHER = 16,
   CONST = 32,
   INDIR = 64,
 };

  /**
  ** Finds out where a value comes from / lives.  Encoded in the enum
  ** returned, various options ORed together.
  ** @param v .. Value to be looked for
  **/
  extern nt_prov get_provenance(llvm::ArrayRef<llvm::Value*> vs);
  extern nt_prov get_provenance(llvm::Value* v);
}; // namespace nanotube

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
