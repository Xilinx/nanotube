/*******************************************************/
/*! \file Provenance.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Support for tracking Nanotube values.
**   \date 2020-07-16
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "Provenance.h"
#include "Intrinsics.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "provenance"

using namespace llvm;
using namespace nanotube;

static std::map<Value*, nt_prov>  provenance_cache;

/**
** Finds out where a value comes from / lives.  Encoded in the enum
** returned, various options ORed together.
** @param v .. Value to be looked for
**/
nt_prov nanotube::get_provenance(ArrayRef<Value*> vs) {
  nt_prov res = NONE;
  for( auto* v : vs)
    res |= get_provenance(v);
  return res;
}

nt_prov nanotube::get_provenance(Value* v) {
  /* Look up in the cache, first :) */
  auto it = provenance_cache.find(v);
  if( it != provenance_cache.end() )
    return it->second;

  nt_prov ret = NONE;
  User *user;

  /* Constants don't change the type but add the CONST field so we know
  ** we have an offset or so. */
  if( isa<ConstantInt>(v) ) {
    ret |= CONST; goto out;
  }

  /* Alloca's are the root of STACK variables */
  if( isa<AllocaInst>(v) ) {
    ret |= STACK; goto out;
  }

  /* We consider all arguments CTX for now */
  if( isa<Argument>(v) ) {
    auto* arg = cast<Argument>(v);
    auto* m = arg->getParent()->getParent();
    if( arg->getType() == get_nt_packet_type(*m)->getPointerTo() )
      ret |= PKT;
    if( arg->getType() == get_nt_context_type(*m)->getPointerTo() )
      ret |= CTX;
    goto out;
  }

  /* Bail on other values that are not users */
  user = dyn_cast<User>(v);
  if( user == nullptr ) {
    ret |= OTHER;
    goto out;
  }

  /* INDIR is added to values loaded from a memory location, and then
  ** that location is classified, as well.  For example, INDIR | STACK
  ** would be a value that lives in a location loaded from the stack.  In
  ** this case, we assume that the CTX points only to PKTs, so CTX |
  ** INDIR = PKT. */
  if( isa<LoadInst>(v) ) {
    ret |= INDIR;
    // Continue
  }

  /* Values coming from calls are their return values. In the general
  ** case, we do not know where a value comes from, so we OR in OTHER,
  ** and OR in the arguments for good measure.  For functions that we
  ** know abuot (this should really be all of them, eventually!) we
  ** encode the right thing, i.e.:
  ** * the resulting location is a MAP for map_lookup calls
  **/
  if( isa<CallInst>(v) ) {
    auto* call = cast<CallInst>(v);
    LLVM_DEBUG(dbgs() << "Call " << *call << " is NT intrinsic "
                      << get_intrinsic(call) << "\n");
    // XXX: Do we somehow only deal with pointer typed arguments!
    if( is_nt_map_lookup(call) ) {
      ret |= MAP;
      goto out;
    } else if( is_nt_packet_data(call) ) {
      ret |= PKT;
      goto out;
    }
    errs() << "Generic badly-handled call: " << *v << '\n';
    ret |= OTHER;
    //goto out; => actually OR together the arguments!
    // Continue tracing the operands of the call
  }

  /* PHI nodes simply mix the operands */
  if( isa<PHINode>(v) ) {
    ;
    // fall-through
  }

  /* .. and selects do the same :) */
  if( isa<SelectInst>(v) ) {
    ;
    /* Selects trace only the data arguments, and not the selection!
     * Obviously, there can still be hard cases....*/
    auto* sel = cast<SelectInst>(v);
    ret |= get_provenance(sel->getTrueValue());
    ret |= get_provenance(sel->getFalseValue());
    goto out;
  }

  if( isa<GetElementPtrInst>(v) ) {
    /* We determine the provenance only based on the base argument, not
     * the offset.  */
    auto* gep = cast<GetElementPtrInst>(v);
    ret |= get_provenance(gep->getPointerOperand());
    goto out;
  }

  /* Go through all operands and simply OR together their sources */
  for( unsigned op = 0; op < user->getNumOperands(); ++op )
    ret |= get_provenance(user->getOperand(op));

out:
  LLVM_DEBUG(dbgs() << "Value: " << *v << " Provenance: " << (unsigned)ret
                    << '\n');
  provenance_cache[v] = ret;
  return ret;
}

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
