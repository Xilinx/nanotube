/*******************************************************/
/*! \file unify_function_returns.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert functions to have a single exit.
**   \date 2023-02-07
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/*!
** Simple utility function that scans the provided function and converts
** multiple returns into a single one.  The algorithm works by scanning and
** collecting all returns and if there are multiple, replacing them with a
** branch to a single basic block that returns from the function.  A phi node
** at the beginning of that block will select the right return value if it is a
** return with a value.
**/

#include "unify_function_returns.hpp"

#include <vector>

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "unify-function-returns"
using namespace llvm;
using namespace nanotube;

/** Converge the function exit blocks
*
* - collect all basic blocks that can leave the function
* - link them all to a single basic block with a ret inside
* - profit
*/
bool nanotube::unify_function_returns(Function& f, DominatorTree* dt,
                                      PostDominatorTree* pdt) {
  std::vector<BasicBlock*> exit_bbs;
  std::vector<Value*>      exit_vals;
  std::vector<BasicBlock*> unreach_bbs;

  for( auto& bb : f ) {
    auto* term = bb.getTerminator();
    bool is_intra = isa<BranchInst>(term) ||
                    isa<SwitchInst>(term);
    bool is_unrch = isa<UnreachableInst>(term);
    bool is_ret   = isa<ReturnInst>(term);

    /* No fancy exceptions, please */
    if( !(is_intra || is_ret || is_unrch) ) {
      errs() << "Unhandled block terminator " << *term << '\n';
    }
    assert(is_intra || is_ret || is_unrch);

    if( is_ret ) {
      auto* ret = cast<ReturnInst>(term);
      auto* rv = ret->getReturnValue();
      exit_bbs.emplace_back(&bb);
      exit_vals.emplace_back(rv);
    } else if( is_unrch ) {
      unreach_bbs.emplace_back(&bb);
    }
  }
  LLVM_DEBUG(
    dbgs() << "Function " << f.getName() << " exit blocks: ";
    for( auto* bb : exit_bbs )
      dbgs() << bb->getName() << " ";
    dbgs() << '\n';
    dbgs() << "Unreachable blocks: ";
    for( auto* bb : unreach_bbs )
      dbgs() << bb->getName() << " ";
    dbgs() << '\n';
  );

  assert(exit_bbs.size() >= 1);
  assert(exit_bbs.size() == exit_vals.size());

  /* Create single exit basic block, link the original exits to it, and use
   * a phi node to select the right return value. */
  unsigned nexits = exit_bbs.size() + unreach_bbs.size();
  if( nexits == 1 )
    return false;

  auto& ctx = f.getParent()->getContext();
  auto* conv_exit = BasicBlock::Create(ctx, "converged_exit", &f);
  auto* type = exit_vals[0]->getType();

  /* Create a PHI node for non-void returns */
  PHINode* phi = nullptr;
  if( !type->isVoidTy() )
    phi = PHINode::Create(type, nexits, "ret_val", conv_exit);
  ReturnInst::Create(ctx, phi, conv_exit);

  for( unsigned i = 0; i < exit_bbs.size(); i++ ) {
    auto* bb = exit_bbs[i];
    auto* rv = exit_vals[i];
    LLVM_DEBUG(dbgs() << "Changing BB: " << bb->getName() << '\n');
    auto* t = cast<ReturnInst>(bb->getTerminator());
    t->eraseFromParent();
    BranchInst::Create(conv_exit, bb);
    LLVM_DEBUG(dbgs() << *bb << '\n');

    if( phi != nullptr )
      phi->addIncoming(rv, bb);
    if( dt != nullptr )
      dt->insertEdge(bb, conv_exit);
    if( pdt != nullptr )
      pdt->insertEdge(bb, conv_exit);
  }
  for( auto* bb : unreach_bbs ) {
    LLVM_DEBUG(dbgs() << "Changing BB: " << bb->getName() << '\n');
    auto* t = cast<UnreachableInst>(bb->getTerminator());
    t->eraseFromParent();
    BranchInst::Create(conv_exit, bb);
    LLVM_DEBUG(dbgs() << *bb << '\n');

    if( phi != nullptr )
      phi->addIncoming(UndefValue::get(type), bb);
    if( dt != nullptr )
      dt->insertEdge(bb, conv_exit);
    if( pdt != nullptr )
      pdt->insertEdge(bb, conv_exit);
  }
  LLVM_DEBUG(dbgs() << *conv_exit << '\n');
  return true;
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
