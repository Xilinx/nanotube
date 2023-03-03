/*******************************************************/
/*! \file PointerTrace.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Tracing pointers in Nanotube programs and generate alias /
**         noalias information.
**   \date 2020-04-07
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "Intrinsics.h"
#include "Provenance.h"

#include "../include/nanotube_api.h"

#define DEBUG_TYPE "nt-alias-annotate"

using namespace llvm;
using namespace nanotube;

namespace {
struct nanotube_alias_annotator : public ModulePass {
  static char ID;

  nanotube_alias_annotator() : ModulePass(ID) { }

  Value *getPointerOperand(Instruction *inst) {
    switch( inst->getOpcode() ) {
      case Instruction::Load: {
        //errs() << "|" << *inst << '\n';
        auto *ld = cast<LoadInst>(inst);
        return ld->getPointerOperand();
      }
      case Instruction::Store: {
        //errs() << "|" << *inst << '\n';
        auto *st = cast<StoreInst>(inst);
        return st->getPointerOperand();
      }
      default:
        return nullptr;
    }
  }
  bool is_nt_map_lookup(CallInst* call) {
    if( call == nullptr )
      return false;

    Value* v = call->getCalledOperand();
    if( !v->hasName() )
      return false;

    return v->getName().equals("nanotube_map_lookup");
  }

  std::string spaces(unsigned depth) {
    std::string s;
    s.append(depth, ' ');
    return s;
  }

  std::map<unsigned, MDNode *> tag2mdnode;

  /**
   * Convert a provenance bit vector into a list of alias.scope / noalias
   * metadata arrays.
   */
  bool prov2alias(nt_prov prov,
                  SmallVectorImpl<Metadata*>& alias,
                  SmallVectorImpl<Metadata*>& noalias) {
    nt_prov orig = prov;
    for( auto& i : tag2mdnode ) {
      if( prov & i.first)
        alias.push_back(i.second);
      else
        noalias.push_back(i.second);
      /* Switch off provenance bit */
      prov = prov & ~i.first;
    }
    /* Ignore constant offsets signalled through CONST */
    prov = prov & ~CONST;
    if( prov == 0 )
      return true;

    /* We have some flags that we ignored.  Complain! */
    errs() << "Unknown remaining provenenance code " << (unsigned)prov
           << " original: " << (unsigned)orig << '\n';
    return false;
  }

  /**
   * Compute alias.scope and noalias sets for an instruction.
   *
   * Review the instruction and its operands to understand which memory
   * locations it may / cannot access.  Encaode that information in alias /
   * noalias arrays.  Note that this is *different* from the get_provenance,
   * above.  This function computes which memory this instruction
   * potentially accesses, not where its results live.
   *
   * @param inst .. Input instruction to compute alias information for
   * @param alias .. Output parameter to store the alias.scope info
   * @param noalias .. Output parameter to store noalias info
   */
  void computeAliasNoalias(Instruction* inst,
                           SmallVectorImpl<Metadata*>& alias,
                           SmallVectorImpl<Metadata*>& noalias) {
    switch( inst->getOpcode() ) {
    case Instruction::Load:
      computeAliasNoalias(cast<LoadInst>(inst), alias, noalias);
      break;
    case Instruction::Store:
      computeAliasNoalias(cast<StoreInst>(inst), alias, noalias);
      break;
    case Instruction::Call:
      computeAliasNoalias(cast<CallInst>(inst), alias, noalias);
      break;
    default:
      //errs() << "No alias information for " << *inst << '\n';
      ;
    }
  }


  /**
   * CallInst alias handling.
   *
   * Calls reference memory inputs, and possibly also the memory location
   * that they return (manufactured through black magic).
   * NOTE: This may need some revising!
   */
  void computeAliasNoalias(CallInst* call,
                           SmallVectorImpl<Metadata*>& alias,
                           SmallVectorImpl<Metadata*>& noalias) {
    unsigned prov = 0;
    bool have_ptrs = false;
    for( unsigned i = 0, m = call->getNumArgOperands(); i < m; ++i ) {
      auto* arg = call->getArgOperand(i);
      /* Skip arguments that are not pointers */
      if( !arg->getType()->isPointerTy() )
        continue;
      have_ptrs = true;
      prov |= get_provenance(arg);
    }
    // Do not generate alias info for calls that do not have pointer args
    // XXX: What about those that return pointer data?
    if( !have_ptrs )
      return;

    bool success = prov2alias(prov, alias, noalias);
    if( !success ) {
      errs() << "Error converting " << *call << '\n';
      alias.clear();
      noalias.clear();
    }
  }

  void computeAliasNoalias(LoadInst* ld,
                           SmallVectorImpl<Metadata*>& alias,
                           SmallVectorImpl<Metadata*>& noalias) {
    Value* ptr    = getPointerOperand(ld);
    unsigned prov = get_provenance(ptr);
    bool success  = prov2alias(prov, alias, noalias);
    if( !success ) {
      errs() << "Error converting " << *ld << '\n';
      alias.clear();
      noalias.clear();
    }
  }

  void computeAliasNoalias(StoreInst* st,
                           SmallVectorImpl<Metadata*>& alias,
                           SmallVectorImpl<Metadata*>& noalias) {
    Value* ptr = getPointerOperand(st);
    nt_prov prov = get_provenance(ptr);
    bool success  = prov2alias(prov, alias, noalias);
    if( !success ) {
      errs() << "Error converting " << *st << '\n';
      alias.clear();
      noalias.clear();
    }
  }

  Value* stripCast(Value* v) {
    auto* c = dyn_cast<Constant>(v);
    if( c != nullptr ) {
      errs() << "SC: " << *v << " " << *c->getOperand(0) << '\n';
      return c->getOperand(0);
    }

    auto* i = dyn_cast<CastInst>(v);
    if( i != nullptr )
      return i->getOperand(0);
    return v;
  }

  /**
   * Converts a map_lookup call into properly typed calls.
   */

  bool runOnFunction(Function &F) {
    std::set<Instruction *> mem_insts;
    std::set<Instruction *> map_calls;

    MDBuilder mdb(F.getContext());
    MDNode* aa_domain = mdb.createAliasScopeDomain("Nanotube");
    tag2mdnode[CTX]   = mdb.createAliasScope("ctx", aa_domain);
    tag2mdnode[PKT]   = mdb.createAliasScope("pkt", aa_domain);
    tag2mdnode[STACK] = mdb.createAliasScope("stack", aa_domain);
    tag2mdnode[MAP]   = mdb.createAliasScope("map", aa_domain);

    SmallVector<Metadata*, 4> alias;
    SmallVector<Metadata*, 4> noalias;
    /* Now go through all instructions and annotate them with the pointer
       info they dereference. */
    for (auto &bbl : F) {
      for (auto ii = bbl.begin(); ii != bbl.end(); ++ii) {
        Instruction* ip = &(*ii);

        alias.clear();
        noalias.clear();

        computeAliasNoalias(ip, alias, noalias);

        if( alias.empty() && noalias.empty() )
          continue;

        if( !alias.empty() )
          ip->setMetadata("alias.scope", MDTuple::get(F.getContext(),
                          alias));
        if( !noalias.empty() )
          ip->setMetadata("noalias", MDTuple::get(F.getContext(),
                          noalias));

        LLVM_DEBUG(
          dbgs() << "Computing alias/noalias for " << *ip
                 << '\n';

          if( !alias.empty() ) {
            dbgs() << "Alias\n";
            for (auto* a : alias)
              dbgs() << *a << '\n';
          }
          if( !noalias.empty() ) {
            dbgs() << "Noalias\n";
            for (auto* a : noalias)
              dbgs() << *a << '\n';
          }
        );
      }
    }
    return true;
  }

  bool runOnModule(Module& m) override {

    /* Run function passes on all functions */
    for( auto& f : m )
      runOnFunction(f);

    return true;
  }
};
} // anonymous namespace

char nanotube_alias_annotator::ID = 0;
static RegisterPass<nanotube_alias_annotator>
    X("nt-alias-annotate", "Annotate memory operations with alias.scope / "
      "noalias info",
      false,
      false
      );

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
