/**************************************************************************\
*//*! \file hls_validate.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Validate a thread function for HLS output.
**   \date  2020-08-03
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

// Make sure the function contains no loops.  Do this by visiting all
// the basic blocks starting at the entry block.  Any other block can
// be visited if all of its predecessors have been visited.  This will
// only visit all basic blocks if the function is in the correct form.
//
// When a basic block is visited add it to pending_bbs so that its
// successors can be examined.
//
// At the same time, we make sure each call of the function obeys
// the following rules:
//   R1. There must be channel activity or a call to thread_wait.
//   R2. Every call to thread_wait must follow a failed read.
//   R3. There must be no thread_wait after a blocking call.
//   R4. Only nop instructions and branches can follow a thread_wait.
//
// Given these rules, the function can be converted into HLS using
// one-to-one translation of constructs.  The caller will invoke
// multiple polling functions and call thread_wait if none of them
// make progress.
//
// The rules are tracked through the following flags:
//   can_return - It is valid to return from the function.
//   read_fail - A read failed.
//   no_blocking - There has been no blocking call.
//   no_wait - There has not been a call to thread_wait.
//
// Each flag is set at a point in the code if the condition is true
// on all paths to that point.  When basic blocks join, each flag
// are combined using logical-and operation.  The bb_flags map
// contains the flags before the terminator instruction for each
// visited basic block.
//
// The checking is implemented as follow:
//
//   On a channel write:
//     Check no_wait since this has a side-effect (R4).
//     Set can_return since there is channel activity (R1).
//     Clear no_blocking since this is a blocking call (R3).
//
//   On a call to thread wait:
//     Check read_fail is set (R2).
//     Check no_blocking is set (R3).
//     Check no_wait is set since this has a side-effect (R4).
//     Set can_return (R1).
//     Clear no_blocking since this is a blocking call (R3).
//     Clear no_wait since thread_wait has been called (R4).
//
//   On any other instruction which is not a NOP:
//     Check no_wait is set (R4).
//
//   On a successful read edge:
//     Set can_return since there is channel activity (R1).
//
//   On a failed read edge:
//     Set read_fail since thread_wait can be called (R2).
//
//   On a return:
//     Check can_return is set (R1).

#include "hls_validate.hpp"

#include "Intrinsics.h"
#include "llvm_common.h"
#include "llvm_insns.h"
#include "llvm_pass.h"
#include "utils.h"

#include <cstdint>

#include "llvm/IR/CFG.h"

using namespace nanotube;

#define DEBUG_TYPE "validate-hls"

namespace
{
  typedef uint8_t thread_flags_t;
  static const auto thread_flags_none       = thread_flags_t(0);
  static const auto thread_flags_all        = thread_flags_t(0xf);
  static const auto thread_flag_can_return  = thread_flags_t(1 << 0);
  static const auto thread_flag_read_fail   = thread_flags_t(1 << 1);
  static const auto thread_flag_no_blocking = thread_flags_t(1 << 2);
  static const auto thread_flag_no_wait     = thread_flags_t(1 << 3);
  static const auto thread_flags_default    = ( thread_flag_no_blocking |
                                                thread_flag_no_wait );

  class validate_hls
  {
  public:
    validate_hls(const Function &thread_func);

    void validate_cfg();
    void try_visit(const BasicBlock *bb);
    void update_flags_for_block(thread_flags_t &flags_in_out,
                                const BasicBlock *bb);
    void check_flags_for_return(thread_flags_t &flags_in_out,
                                const ReturnInst &insn);
    void update_flags_for_call(thread_flags_t &flags_in_out,
                               const Instruction &insn);
    thread_flags_t adjust_edge_flags(const BasicBlock *pred_bb,
                                     const BasicBlock *succ_bb,
                                     thread_flags_t flags_in);

  private:
    const Function &m_thread_func;
    llvm::DenseMap<const BasicBlock *, thread_flags_t> m_bb_flags;
    SmallVector<const BasicBlock *, 8> m_pending_bbs;
  };
};

validate_hls::validate_hls(const Function &thread_func):
  m_thread_func(thread_func)
{
}

void validate_hls::validate_cfg()
{
  const BasicBlock *entry_bb = &(m_thread_func.getEntryBlock());

  thread_flags_t bb_flags = thread_flags_default;
  update_flags_for_block(bb_flags, entry_bb);
  m_bb_flags[entry_bb] = bb_flags;
  m_pending_bbs.push_back(entry_bb);

  LLVM_DEBUG(dbgs() << "Checking CFG for HLS output\n";);

  // Try to visit all the basic blocks.
  while (!m_pending_bbs.empty()) {
    const BasicBlock *current_bb = m_pending_bbs.back();
    m_pending_bbs.pop_back();

    LLVM_DEBUG(
      dbgv("Examining successors of ");
      current_bb->printAsOperand(dbgs(), false);
      dbgs() << "\n";
      );

    // Examine the successor basic blocks in case any can be visited.
    for (const BasicBlock *succ_bb: successors(current_bb)) {
      try_visit(succ_bb);
    }
  }

  // Make sure all the basic blocks have been visited.
  bool have_unvisited_blocks = false;
  for (const BasicBlock &current_bb: m_thread_func) {
    if (m_bb_flags.find(&current_bb) != m_bb_flags.end())
      continue;

    errs() << "ERROR: Unvisited basic block ";
    current_bb.printAsOperand(errs(), false);
    errs() << "\n";
    have_unvisited_blocks = true;
  }
  if (have_unvisited_blocks)
    report_fatal_errorv("Function {0} contains a loop or loops.",
                        m_thread_func.getName());
}

void validate_hls::try_visit(const BasicBlock *bb)
{
  // Ignore any successor which has already been visited.
  if (m_bb_flags.find(bb) != m_bb_flags.end()) {
    LLVM_DEBUG(
      dbgv("  Successor has been visited: ");
      bb->printAsOperand(dbgs(), false);
      dbgs() << "\n";
      );
    return;
  }

  LLVM_DEBUG(
    dbgv("  Examining successor: ");
    bb->printAsOperand(dbgs(), false);
    dbgs() << "\n";
    );

  // Check whether there is an unvisited predecessor.  Collect the
  // thread flags in case all the predecessors have been visited.
  bool unvisited_pred = false;
  thread_flags_t bb_flags = thread_flags_all;
  for (const BasicBlock *pred_bb: predecessors(bb)) {
    auto it = m_bb_flags.find(pred_bb);
    if (it == m_bb_flags.end()) {
      LLVM_DEBUG(
        dbgv("    Has unvisited predecessor: ");
        pred_bb->printAsOperand(dbgs(), false);
        dbgs() << "\n";
        );
      unvisited_pred = true;
      break;
    }

    // Adjust the thread flags due to the branch condition if
    // necessary.
    thread_flags_t edge_flags =
      adjust_edge_flags(pred_bb, bb, it->second);

    // Combine the edge flags with the flags for the basic block.
    bb_flags &= edge_flags;
  }

  // If there is an unvisited predecessor then this basic block
  // cannot be visited.
  if (unvisited_pred)
    return;

  // Visit this basic block.
  LLVM_DEBUG(dbgv("    Start of block has flags {0:x}.\n", bb_flags););
  update_flags_for_block(bb_flags, bb);
  m_bb_flags[bb] = bb_flags;
  m_pending_bbs.push_back(bb);
}

void validate_hls::update_flags_for_block(thread_flags_t &flags_in_out,
                                          const BasicBlock *bb)
{
  // Update the flags from the instructions.
  for (const Instruction &insn: *bb) {
    auto opcode = insn.getOpcode();
    switch (opcode) {
    case Instruction::Br:
    case Instruction::GetElementPtr:
      // Ignore the no_wait check since these instructions have no
      // side-effects.
      break;

    case Instruction::Ret:
      // Only check the flags.
      check_flags_for_return(flags_in_out, cast<ReturnInst>(insn));
      break;

    case Instruction::Call:
      // Check and update the flags.
      update_flags_for_call(flags_in_out, insn);
      break;

    default:
      // Ignore no-op instructions.
      if (isa<CastInst>(insn))
        break;

      // Make sure no_wait is set.
      if ( (flags_in_out & thread_flag_no_wait) == 0)
        report_fatal_errorv("Invalid instruction after call to"
                            " nanotube_thread_wait: {0}", insn);
      break;
    }
  }
}

void validate_hls::check_flags_for_return(thread_flags_t &flags_in_out,
                                          const ReturnInst &insn)
{
#if 0 // Disabled for NANO-178.
  // Make sure the can_return flag is set for returns from the
  // function.
  if ( (flags_in_out & thread_flag_can_return) == 0) {
    errs() << "ERROR: Basic block ";
    insn.getParent()->printAsOperand(errs(), false);
    errs() << " returns without activity.\n";
    report_fatal_errorv("Function {0} can return without activity.",
                        m_thread_func.getName());
  }
#endif
}

void validate_hls::update_flags_for_call(thread_flags_t &flags_in_out,
                                         const Instruction &insn)
{
  auto iid = get_intrinsic(&insn);
  switch (iid) {
  case Intrinsics::channel_write:
    // Check no_wait is set.
    if ((flags_in_out & thread_flag_no_wait) == 0)
      report_fatal_errorv("Invalid write to channel after call to"
                          " nanotube_thread_wait: {0}", insn);

    // Set can_return and clear no_blocking.
    flags_in_out |= thread_flag_can_return;
    flags_in_out &= ~thread_flag_no_blocking;
    break;

  case Intrinsics::thread_wait:
    // Check read_fail, no_blocking and no_wait are set.
    if ( (flags_in_out & thread_flag_no_wait) == 0 )
      report_fatal_errorv("Multiple calls nanotube_thread_wait: {0}",
                          insn);
#if 0 // See NANO-282
    if ( (flags_in_out & thread_flag_read_fail) == 0 )
      report_fatal_errorv("Call to nanotube_thread_wait does not"
                          " follow read failure: {0}", insn);
#endif
    if ( (flags_in_out & thread_flag_no_blocking) == 0 )
      report_fatal_errorv("Call to nanotube_thread_wait follows a"
                          " blocking call: {0}", insn);

    // Set can_return and clear no_blocking and no_wait.
    flags_in_out |= thread_flag_can_return;
    flags_in_out &= ~(thread_flag_no_blocking | thread_flag_no_wait);
    break;

  default:
    if ( (!intrinsic_is_nop(iid)) &&
         (flags_in_out & thread_flag_no_wait) == 0 ) {
      report_fatal_errorv("Invalid instruction after call to"
                          " nanotube_thread_wait: {0}", insn);
    }
    break;
  }
}

thread_flags_t validate_hls::adjust_edge_flags(const BasicBlock *pred_bb,
                                               const BasicBlock *succ_bb,
                                               thread_flags_t flags_in)
{
  const Instruction *insn = pred_bb->getTerminator();
  if (insn == nullptr)
    report_fatal_errorv("HLS-validate: Basic block is mal-formed:\n"
                        "{0}\n", *pred_bb);

  const BranchInst *br_insn = dyn_cast<BranchInst>(insn);
  if (br_insn == nullptr)
    return flags_in;

  // The flags on the edge are the same as the flags before the
  // branch if it is unconditional.
  if (br_insn->isUnconditional()) {
    LLVM_DEBUG(
      dbgv("    Adding unconditional edge from: ");
      pred_bb->printAsOperand(dbgs(), false);
      dbgv(" with flags {0:x}\n", flags_in);
      );
    return flags_in;
  }

  // Check whether this is the true edge or the false edge.
  assert(br_insn->isConditional());
  bool is_true  = (succ_bb == br_insn->getSuccessor(0));

  // Sanity check the false flag.  Limit the scope of is_false to
  // avoid updating it when is_true is modified.
  {
    bool is_false = (succ_bb == br_insn->getSuccessor(1));
    assert(is_true || is_false);
    if (is_true == is_false) {
      LLVM_DEBUG(
        dbgv("    Adding double conditional edge from: ");
        pred_bb->printAsOperand(dbgs(), false);
        dbgv(" with flags {0:x}\n", flags_in);
        );
      return flags_in;
    }
  }

  // Examine the condition.  Strip any supported modifiers.
  const Value *br_cond = br_insn->getCondition();
  while (true) {
    const auto *icmp_insn = dyn_cast<ICmpInst>(br_cond);
    if (icmp_insn != nullptr) {
      const Value *op1 = icmp_insn->getOperand(1);
      const auto *op1_const = dyn_cast<ConstantInt>(op1);
      if ( icmp_insn->isEquality() &&
           op1_const != nullptr && op1_const->isZero() ) {
        bool invert = ( icmp_insn->getPredicate() ==
                        CmpInst::ICMP_EQ );
        is_true = ( invert ? !is_true : is_true );
        br_cond = icmp_insn->getOperand(0);
        continue;
      }
    }

    // No match.
    break;
  }

  // No modifiers left.  Check whether the result is the result of a
  // call to nanotube_channel_try_read.
  const auto *cond_insn = dyn_cast<Instruction>(br_cond);
  if ( cond_insn != nullptr &&
       get_intrinsic(cond_insn) == Intrinsics::channel_try_read ) {
    if (is_true) {
      LLVM_DEBUG(
        dbgv("    Adding read-succ edge from: ");
        pred_bb->printAsOperand(dbgs(), false);
        dbgv(" with flags {0:x}\n", flags_in);
        );
      return ( flags_in | thread_flag_can_return );
    }

    LLVM_DEBUG(
      dbgv("    Adding read-fail edge from: ");
      pred_bb->printAsOperand(dbgs(), false);
      dbgv(" with flags {0:x}\n", flags_in);
      );
    return ( flags_in | thread_flag_read_fail );
  }

  // For an unrecognised condition, just return the flags in.  This
  // could be a check of the result of nanotube_channel_try_read which
  // could cause us to raise an error for well behaved but complicated
  // code.  However, we won't fail to raise an error for badly behaved
  // code since a check of the result of nanotube_channel_try_read
  // never makes code invalid.
  LLVM_DEBUG(
    dbgv("    Adding unrecognised edge from: ");
    pred_bb->printAsOperand(dbgs(), false);
    dbgv(" with flags {0:x}\n", flags_in);
    );
  return flags_in;
}

void nanotube::validate_hls_thread_function(const Function &thread_func)
{
  auto v = validate_hls(thread_func);
  v.validate_cfg();
}
