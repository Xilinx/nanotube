/**************************************************************************\
*//*! \file enable_loop_unroll.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Modify metadata to enable loop unrolling.
**   \date  2021-01-22
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "llvm_common.h"
#include "llvm_insns.h"
#include "llvm_pass.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"

#define DEBUG_TYPE "enable_loop_unroll"

// The enable loop unroll pass
// ===========================
//
// When clang compiles functions to LLVM-IR, it can remove
// optimisation requests which the programmer added using pragmas.  In
// particular, it can disable loop unrolling on some loops.  This
// pass modifies the metadata to re-enable loop unrolling.
//
// Input conditions
// ----------------
//
// None
//
// Output conditions
// -----------------
//
// All loops are marked with the metadata:
//   llvm.loop.unroll.enable
//
// The following metadata is removed:
//   llvm.loop.unroll.disable
//
// Theory of operation
// -------------------
//
// This pass is straight-forward.  LLVM runs it on each loop.  It then
// checks whether the loop metadata node exists.  If it does, it
// determines whether any changes are needed.  If the metadata node
// exists and is correct, the pass exits indicating that no changes
// were made.  Otherwise, it creates a new metadata node, copies any
// entries which are not being added or removed and sets it as the
// loop metadata.

using namespace nanotube;
using llvm::LLVMContext;
using llvm::Loop;
using llvm::MDNode;
using llvm::MDOperand;
using llvm::MDString;
using llvm::Metadata;

namespace
{
  class enable_loop_unroll: public llvm::LoopPass
  {
  public:
    static char ID;
    enable_loop_unroll();
    bool runOnLoop(Loop* L, llvm::LPPassManager& LPM) override;
  };
} // anonymous namespace

char enable_loop_unroll::ID;

enable_loop_unroll::enable_loop_unroll():
  LoopPass(ID)
{
}

bool enable_loop_unroll::runOnLoop(Loop *L, llvm::LPPassManager &LPM)
{
  // The LLVM context.
  LLVMContext &context = L->getHeader()->getContext();

  // Check for loop meta-data.
  MDNode* loop_md = L->getLoopID();
  bool need_update = false;
  bool found_enable = false;

  LLVM_DEBUG(
    dbgs() << formatv("Processing loop at {0}\n", L->getHeader()->front());
  );

  if (loop_md != nullptr) {
    LLVM_DEBUG(
      dbgs() << formatv("  Scanning metadata: {0}\n", *loop_md);
    );

    // Scan for a loop unroll directive.
    bool is_first = true;
    for (const MDOperand &op: loop_md->operands()) {
      if (is_first) {
        is_first = false;
        continue;
      }

      Metadata *op_md = op.get();

      // Only examine MDNodes with at least one entry.
      MDNode *op_node = cast<MDNode>(op_md);
      Metadata *op_name_md = op_node->getOperand(0).get();
      MDString *op_name_str = dyn_cast<MDString>(op_name_md);
      if (op_name_str != nullptr) {
        StringRef str = op_name_str->getString();

        LLVM_DEBUG(
          dbgs() << formatv("  Found string '{0}'\n", str);
        );

        if (str.equals("llvm.loop.unroll.enable")) {
          LLVM_DEBUG(
            dbgs() << formatv("    Is loop enable.\n");
          );
          found_enable = true;
          continue;
        }

        if (str.equals("llvm.loop.unroll.disable")) {
          LLVM_DEBUG(
            dbgs() << formatv("    Need update.\n");
          );
          need_update = true;
          continue;
        }

      } else {
        LLVM_DEBUG(
          dbgs() << formatv("  Found non-string '{0}'\n", *op_name_md);
        );
      }
    }
  }

  // Do nothing if no change is required.
  if (found_enable && !need_update) {
    LLVM_DEBUG(
      dbgs() << "  No updates needed.\n";
    );
    return false;
  }

  // Build a new MD node which is an edited copy of the old node.
  MDNode *dummy = MDNode::get(context, {});
  SmallVector<Metadata *, 16> props { dummy };

  if (loop_md != nullptr) {
    bool is_first = true;
    for (const MDOperand &op: loop_md->operands()) {
      if (is_first) {
        LLVM_DEBUG(
          dbgs() << formatv("  Ignoring first operand: {0}\n", op);
        );
        is_first = false;
        continue;
      }

      Metadata *op_md = op.get();

      // Only examine MDNodes with at least one entry.
      MDNode *op_node = cast<MDNode>(op_md);
      Metadata *op_name_md = op_node->getOperand(0).get();
      MDString *op_name_str = dyn_cast<MDString>(op_name_md);
      if (op_name_str != nullptr) {
        StringRef str = op_name_str->getString();

        LLVM_DEBUG(
          dbgs() << formatv("  Checking string '{0}'\n", str);
        );

        // Strip any properties which are not required.
        if (str.equals("llvm.loop.unroll.disable")) {
          LLVM_DEBUG(
            dbgs() << formatv("    Ignoring.\n");
          );
          continue;
        }
      }

      LLVM_DEBUG(
        dbgs() << formatv("  Adding metadata '{0}'\n", *op_md);
      );
      props.push_back(op_md);
    }
  }

  if (!found_enable) {
    LLVM_DEBUG(
      dbgs() << formatv("  Adding unroll enable.\n");
    );
    Metadata *md[] = {
        MDString::get(context, "llvm.loop.unroll.enable")
    };
    MDNode *enable = MDNode::get(context, md);
    props.push_back(enable);
  }

  LLVM_DEBUG(
    dbgs() << formatv("  Setting loop metadata.\n");
  );
  loop_md = MDNode::get(context, props);
  loop_md->replaceOperandWith(0, loop_md);
  L->setLoopID(loop_md);

  return true;
}

static RegisterPass<enable_loop_unroll>
    X("enable-loop-unroll", "Modify metadata to enable loop unrolling.",
      true, // IsCFGOnlyPass
      false // IsAnalysis
      );
