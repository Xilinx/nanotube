/*******************************************************/
/*! \file nt_attributes.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Simple pass to add the correct attributes to Nanotube functions.
**   \date 2021-11-25
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
 * _Goal_
 * The goal of this pass is to add per-function and per-argument annotations to
 * Nanotube API function definitions / declarations so that the LLVM optimiser
 * can make better decisions.
 *
 * _Theory of Operation_
 * Go through all function declarations / defintions in the current module,
 * check if they are Nanotube API functions, and if so, determine which
 * annotations they should get.  Change the function declaration / definition
 * accordingly.
 */
#include "llvm/Pass.h"

#include "Intrinsics.h"

#define DEBUG_TYPE "nt-attributes"
using namespace llvm;
using namespace nanotube;

struct nt_attributes : public ModulePass {
  nt_attributes() : ModulePass(ID) {
  }

  void getAnalysisUsage(AnalysisUsage &info) const override {
  }

  /* LLVM-specific */
  static char ID;
  bool runOnModule(Module& m) override;
};

bool nt_attributes::runOnModule(Module& m) {
  bool changes = false;
  for( auto& f : m ) {
    auto id = get_intrinsic(&f);
    if( id == Intrinsics::none )
      continue;
    if( (id < Intrinsics::packet_read) ||
        (id > Intrinsics::get_time_ns) )
      continue;

    auto fmrb = get_nt_fmrb(id);
    LLVM_DEBUG( dbgs() << "Func: " << f.getName()
                       << " Intr: " << intrinsic_to_string(id)
                       << " FMRB: " << fmrb << '\n');

    switch( fmrb ) {
      case FMRB_OnlyReadsArgumentPointees:
      case FMRB_OnlyAccessesArgumentPointees:
        changes |= true;
        f.addFnAttr(Attribute::ArgMemOnly);
        break;

      case FMRB_OnlyAccessesInaccessibleOrArgMem:
        changes |= true;
        f.addFnAttr(Attribute::InaccessibleMemOrArgMemOnly);
        break;

      case FMRB_OnlyAccessesInaccessibleMem:
        changes |= true;
        f.addFnAttr(Attribute::InaccessibleMemOnly);
        break;

      default:
        /* Do nothing */
        ;
    }
  }
  return changes;
}

char nt_attributes::ID = 0;
static RegisterPass<nt_attributes>
  X("nt-attributes", "Add attributes to Nanotube API functions.",
    true,
    false
  );
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
