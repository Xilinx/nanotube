/**************************************************************************\
*//*! \file move_alloca.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A pass to move allocas to the entry basic block.
**   \date  2020-10-05
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

// The move-alloca pass
// ====================
//
// This pass moves alloca instructions to the entry block of each
// function where possible.  An alloca can be moved if every CFG path
// from immediately after the alloca to immediately before the alloca
// involves a stackrestore which unwinds the alloca.
//
// Input conditions
// ----------------
//
// None
//
// Output conditions
// -----------------
//
// Where possible, each alloca is moved to the start of the entry
// basic block of its function.
//
// Theory of operation
// -------------------
//
// In order to find loops, a depth first traversal is performed over a
// graph which is computed from the CFG.  The idea is to traverse the
// CFG, not considering chronological successors, but by considering
// successors which inherit the stack state.  In this view, a
// stacksave can jump to a corresponding stackrestore.  It doesn't
// jump directly, but via a path from the stacksave to the
// stackrestore.  When execution continues after the stackrestore, it
// is as if the path from the stacksave to the stackrestore had not
// been executed since the stack is unwound to the state at the
// stacksave.
//
// The computed graph is the same as the CFG except basic blocks are
// split after calls to stacksave and stackrestore.  Edges are
// introduced between a stacksave and each stackrestore which
// potentially uses the result of the stacksave.  Nodes which are
// found to be free of loops are scanned for allocas which are moved
// to the entry block.
//
// Each node of the computed graph is a contiguous sequence of non-PHI
// instructions which ends with a basic block terminator, a call to
// stacksave or a call to stackrestore.  Each non-PHI instruction is
// in exactly one node.  There are three types of edge.  One type is
// from the end of a basic block to the first non-PHI of a successor
// basic block.  The second type is from a stacksave to the
// instruction following a stackrestore which uses the value produced
// by the stacksave.  The third type is from a stacksave to the
// instruction immediately following it within the same basic block.
//
// A node which ends in a stackrestore does not have an edge to the
// node which starts immediately after the stackrestore.  This is
// because the stack is unwound by the stackrestore so the edges to
// that node come from the nodes which end in the stacksaves rather
// than the node which ends in the stackrestore.  Since the stack is
// unwound, the instructions between the stacksave and the
// stackrestore are ignored for the purposes of this path.
//
// It is worth considering whether the LLVM loop infrastructure is
// suitable for this task.  Alas, it is not.  The first reason is that
// LLVM only considers natural loops and not all cyclic paths in the
// CFG.  A natural loop is one which has only a single entry basic
// block.  If there is a cyclic path which can be entered at two or
// more points then it is not considered to be a loop by LLVM.  The
// other reason for not using the LLVM loop infrastructure is that we
// want to find loops in the computed graph, not loops in the CFG.
// Having found a loop in the CFG we would need to determine whether
// it corresponds to a loop in the computed graph since the loop may
// be broken by calls to stacksave and stackrestore.  Doing that is no
// easier than finding loops in the computed graph directly.
//
// The depth first traversal is performed using a stack of nodes which
// have been entered but not exited and a map holding the state of
// each node which has been entered.  The map is used to make sure no
// node is entered twice.  Each node of the computed graph is
// represented by a pointer to its first instruction.
//
// The map's key is a pointer to the first instruction of the node and
// the map's value is a flag indicating whether the node is currently
// being traversed.  A map entry is present if the traversal has
// entered the node.  The flag is set iff the traversal has not exited
// the node.
//
// Each stack entry contains a pointer to the first instruction of a
// node which has been entered along with iterators over the
// successors of the node.  Successors are basic block successors, the
// uses of a stacksave or the instruction after a call to stacksave.
// A stack entry also holds a flag indicating whether the node is
// involved in a loop.
//
// When a node is entered it is pushed onto the stack and a map entry
// is created.  It is then scanned to find the last instruction, which
// will be a call to stacksave, a call to stackrestore or a terminator
// instruction.  Each successor is then examined to determine whether
// it has been entered.  If not then the successor is entered.  If a
// successor has been visited then the map flag is examined to
// determine whether the node is being traversed.  If it is then a
// loop has been found so the stack is examined to find the nodes
// which are included in the loop.  The current node is the node on
// the top of the stack and the successor node will be somewhere on
// the stack.  All of the stack entries between current node and the
// successor node inclusive are part of the loop so their loop flags
// are set.
//
// After all the successors of a node have been visited, the node is
// exited.  At this point, the loop flag is examined to determine
// whether the node is part of a loop.  If it is not part of a loop
// then the block is scanned for alloca instructions and any which are
// found are moved to the entry basic block.

#include "llvm_common.h"
#include "llvm_insns.h"
#include "llvm_pass.h"
#include "utils.h"

#include "llvm/IR/CFG.h"

#include <utility>

#define DEBUG_TYPE "move-alloca"

///////////////////////////////////////////////////////////////////////////

using namespace nanotube;

namespace
{
/*! A class holding working state for the move_alloca pass. */
class move_alloca
{
public:
  /*! The type of the map containing information about a node.
  **
  ** The key is a pointer to the first instruction of the node.  The
  ** value is a flag indicating whether the node is currently being
  ** traversed.
  */
  typedef DenseMap<Instruction *, bool> node_map_t;

  /*! Construct state for the move_alloca pass. */
  move_alloca(Function *f);

  /*! Return the instruction after any alloca instructions.
  **
  ** Examines the sequence of instructions starting with the specified
  ** instruction, stopping when a non-alloca instruction is found.
  **
  ** \param insn The first instruction to examine.
  **
  ** \returns the first non-alloca instruction encountered.
  **/
  BasicBlock::iterator skip_allocas(BasicBlock::iterator insn);

  /*! Mark a set of nodes as loop nodes.
  **
  ** Iterates from the top of the stack down until the node starting
  ** with the specified instruction is found.  All the nodes
  ** associated with these stack entries, including the top of stack
  ** and specfied node, are marked as loop nodes.
  **
  ** \param The successor node of the back-edge.
  **/
  void mark_loop(Instruction *first_insn);

  /*! Process a successor of the current node.
  **
  ** Try to enter a successor node of the current node.  If the node
  ** has already been entered then either process a loop or simply
  ** skip the node.  Otherwise, mark the node as entered and push it
  ** onto the stack.
  **/
  void try_enter_node(Instruction *first_insn);

  /*! Move the allocas in a region to the entry basic block.
  **
  ** Search the specified sequence of instructions for allocas which
  ** can be moved to the entry block.  The number of elements must be
  ** constant for an alloca to be eligible to be moved.
  **
  ** \param map_it An iterator to the node in m_node_info.
  ** \param last_insn The last instruction in the region to process.
  **/
  void do_node_moves(node_map_t::iterator map_it,
                     Instruction *last_insn);

  /*! Exit the top-most node on the stack.
  **
  ** Pop the top-most node from the stack.  If it is not a loop node,
  ** move all the eligible allocas out of it.
  **/
  void exit_node();

  /*! Process the stack until it is empty.
  **
  ** While the stack is not empty, process the node at the top of the
  ** stack.  If it has any more successors to examine, try to enter
  ** one.  Otherwise, exit the node.
  **/
  bool process();

private:
  /*! A stack entry, describing a node. */
  struct stack_entry {
    /*! The first instruction in the node. */
    Instruction *first_insn;

    /*! The last instruction in the node. */
    Instruction *last_insn;

    /*! The next user of a stacksave to enter. */
    SmallVector<User*, 4> users;

    /*! The next successor basic block to enter. */
    llvm::succ_iterator succ_iter;

    /*! Whether the successor instruction should be processed. */
    bool succ_insn_pending;

    /*! A flag indicating whether a loop through the node has been
    **  found. */
    bool is_loop;

    /*! Get the first instruction of a successor node by following an
     *  edge from the stacksave at the end of this node to a
     *  stackrestore immediately before the successor.
     *
     *  \returns nullptr if there are no more successor nodes. */
    Instruction *get_next_stackrestore();

    /*! Get the first instruction of a successor node by following an
     *  edge between basic blocks.
     *
     *  \returns nullptr if there are no more successor nodes. */
    Instruction *get_next_successor_bb();

    /*! Get the first instruction of a successor node by following the
     *  flow of control within a basic block.  This is needed for
     *  nodes which end in a stacksave in which case the successor
     *  node will start at the instruction following the stacksave.
     *
     *  \returns nullptr if there are no more successor nodes. */
    Instruction *get_next_tail();

    /*! Get the first instruction of a successor node.  Calling this
     *  multiple times will iterate over all the edges out of this
     *  node.
     *
     *  \returns nullptr if there are no more successor nodes. */
    Instruction *get_next();
  };

  /*! The function being processed. */
  Function *m_func;

  /*! Indicates whether any changes have been made. */
  bool m_changes_made;

  /*! The stack holding nodes which need to be processed. */
  SmallVector<stack_entry, 64> m_stack;

  /*! Information about each node which has been entered.
  **
  ** A map from the first instruction of a node to a flag which
  ** indicates whether the node is still being traversed.
  */
  node_map_t m_node_info;
};

/*! The move-alloca pass. */
class move_alloca_pass: public llvm::FunctionPass
{
public:
  /*! A variable at a unique address. */
  static char ID;

  /*! Construct the pass. */
  move_alloca_pass();

  /*! Run the pass on a function. */
  bool runOnFunction(Function &f) override;
};

} // namespace

///////////////////////////////////////////////////////////////////////////

move_alloca::move_alloca(Function *f):
  m_func(f),
  m_changes_made(false)
{
  LLVM_DEBUG(
    dbgs() << "Moving allocas in function " << f->getName() << "\n";);
}

BasicBlock::iterator
move_alloca::skip_allocas(BasicBlock::iterator it)
{
  BasicBlock *bb = it->getParent();
  while (it != bb->end()) {
    if (!isa<AllocaInst>(*it))
      return it;
    ++it;
  }

  report_fatal_errorv("{0}:{1}: No terminator for basic block {2}",
                      __FILE__, __LINE__, *bb);
}

void move_alloca::mark_loop(Instruction *first_insn)
{
  LLVM_DEBUG(dbgs() << "  Marking loop:\n");

  // Process stack entires, starting from the top of stack.
  for (auto it = m_stack.rbegin(); it != m_stack.rend(); it++) {
    LLVM_DEBUG(
      dbgs() << formatv("    {0}      {1}\n",
                        it->is_loop ? "Dup node: " : "Loop node:",
                        *(it->first_insn)));

    // Check to make sure the entry is in the m_node_info map.
    auto map_it = m_node_info.find(it->first_insn);
    if (map_it == m_node_info.end())
      report_fatal_errorv("{0}:{1}: No info for node {2}",
                          __FILE__, __LINE__, *first_insn);

    // Mark the node as part of the loop.
    it->is_loop = true;

    // Stop when the loop is complete.
    if (it->first_insn == first_insn) {
      LLVM_DEBUG(dbgs() << "  End of loop.\n");
      return;
    }
  }

  // The method should only be called with a node which is on the
  // stack.
  report_fatal_errorv("{0}:{1}: No stack entry for node {2}",
                      __FILE__, __LINE__, *first_insn);
}

static void walk_phitree_leaves(Value *root,
                                llvm::SmallVectorImpl<User*>* leaves)
{
  for (auto* user : root->users()) {
    if (!isa<PHINode>(user))
      leaves->push_back(user);
    else
      walk_phitree_leaves(user, leaves);
  }
}

void move_alloca::try_enter_node(Instruction *first_insn)
{
  // Try to insert the node into the node info map.  If this fails
  // then the node has already been entered.
  auto map_ins = m_node_info.insert(std::make_pair(first_insn, true));
  if (!map_ins.second) {
    // Check whether the node is being traversed.  If so, mark the
    // loop.  In either case, do not enter the node since it has
    // already been entered.
    if (map_ins.first->second) {
      LLVM_DEBUG(
        dbgs() << formatv("  Active node:      {0}\n",
                          *first_insn));
      mark_loop(first_insn);
    } else {
      LLVM_DEBUG(
        dbgs() << formatv("  Completed node:   {0}\n",
                          *first_insn));
    }
    return;
  }

  LLVM_DEBUG(dbgs() << formatv("Entering node:      {0}\n", *first_insn));

  // Find the last instruction of the node.
  BasicBlock *bb = first_insn->getParent();
  BasicBlock::iterator it{first_insn};
  bool is_terminator;
  llvm::Intrinsic::ID iid;
  while (true) {
    if (it == bb->end())
      report_fatal_errorv("{0}:{1}: No terminator for basic block {2}",
                          __FILE__, __LINE__, *bb);

    // Determine whether the instruction is the last in the node.
    Instruction *insn = &*it;
    const CallBase *call = dyn_cast<CallBase>(insn);
    iid = ( call == NULL
            ? llvm::Intrinsic::not_intrinsic
            : call->getIntrinsicID() );
    is_terminator = insn->isTerminator();

    if (iid == llvm::Intrinsic::stacksave ||
        iid == llvm::Intrinsic::stackrestore ||
        is_terminator)
      break;

    ++it;
  }
  Instruction *last_insn = &*it;

  LLVM_DEBUG(dbgs() << formatv("  Node ends at:     {0}\n", *last_insn));

  // A stackrestore has no successors.  Exit the node without even
  // pushing it onto the stack.
  if (iid == llvm::Intrinsic::stackrestore) {
    // The node is not being traversed.  This node cannot be involved
    // in a loop.
    LLVM_DEBUG(dbgs() << formatv("Quick exit node:    {0}\n", *first_insn));
    map_ins.first->second = false;
    do_node_moves(map_ins.first, last_insn);
    return;
  }

  // Collect users of the stacksave (if present)
  SmallVector<User*, 4> users;
  if (iid == llvm::Intrinsic::stacksave)
    walk_phitree_leaves(last_insn, &users);

    //users.assign(last_insn->user_begin(), last_insn->user_end());

  // Initialise the iterator for the successor BBs
  llvm::succ_iterator succ_iter(nullptr);
  if (is_terminator)
    succ_iter = llvm::succ_begin(last_insn);

  // Push a new entry onto the stack.
  m_stack.push_back(stack_entry{
      first_insn, last_insn,
      users, succ_iter,
      !is_terminator, false});
}

void move_alloca::do_node_moves(node_map_t::iterator map_it,
                                Instruction *last_insn)
{
  // The first instruction is the map entry key.
  Instruction *first_insn = map_it->first;

  // Get the insert point in the entry basic block.
  BasicBlock &entry_bb = m_func->getEntryBlock();
  Instruction *insert_point = entry_bb.getFirstNonPHI();
  bool moved_any = false;

  BasicBlock::iterator insn_it {first_insn};
  BasicBlock::iterator end {last_insn};

  LLVM_DEBUG(
    dbgs() << formatv("  Move start:       {0}\n", *first_insn);
    dbgs() << formatv("  Move end:         {0}\n", *last_insn););

  assert(map_it->second == false);

  // Process all the instructions in the node.
  while (insn_it != end) {
    // Find the next instruction so that it is possible to move the
    // alloca without breaking iteration.
    BasicBlock::iterator next = insn_it;
    ++next;

    // If it is an alloca and the size is constant then move it.
    AllocaInst *alloca = dyn_cast<AllocaInst>(&*insn_it);
    if (alloca != nullptr) {
      Value *num_elems = alloca->getArraySize();
      if (isa<ConstantInt>(num_elems)) {
        LLVM_DEBUG(
          dbgs() << formatv("  Moving alloca     {0}\n", *insn_it));

        // Move the alloca.
        moved_any = true;
        insn_it->moveBefore(insert_point);

        // Update the first instruction if the alloca was first.
        if (&*insn_it == first_insn)
          first_insn = &*next;

      } else {
        LLVM_DEBUG(
          dbgs() << formatv("  Non-const alloca  {0}\n", *insn_it));
      }
    }

    // Move on to the next instruction.
    insn_it = next;
  }

  if (moved_any) {
    // Indicate that changes have been made.
    m_changes_made = true;

    // Re-insert the map entry if the first instruction has changed.
    if (first_insn != map_it->first) {
      m_node_info.erase(map_it);
      auto map_ins = m_node_info.insert(
        std::make_pair(first_insn, false));
      assert(map_ins.second);
    }
  }
}

void move_alloca::exit_node()
{
  assert(!m_stack.empty());
  stack_entry &current = m_stack.back();

  LLVM_DEBUG(
    dbgs() << formatv("Exiting node        {0}\n", *current.first_insn));

  // Find the node info.
  auto map_it = m_node_info.find(current.first_insn);
  if (map_it == m_node_info.end())
      report_fatal_errorv("{0}:{1}: No info for node {2}",
                          __FILE__, __LINE__, *current.first_insn);

  // Mark the node as not being traversed.
  map_it->second = false;

  // Move any allocas in the node.
  if (!current.is_loop) {
    do_node_moves(map_it, current.last_insn);
  } else  {
    LLVM_DEBUG(dbgs() << "  Skipping loop node.\n");
  }

  // Pop the stack entry.
  m_stack.pop_back();
}

Instruction* move_alloca::stack_entry::get_next_stackrestore()
{
  // Process the next user if there is one.
  if (users.empty())
    return nullptr;

  User *user = users.pop_back_val();

  LLVM_DEBUG(dbgs() << formatv("  Next user is      {0}\n", *user));

  // Every user of a stacksave must be a call to
  // llvm.stackrestore.
  CallBase *call = dyn_cast<CallBase>(user);
  if (call == nullptr)
    report_fatal_errorv("{0}:{1}: User of {2} is not a call"
                        " instruction: {3}.",
                        __FILE__, __LINE__, *last_insn,
                        *user);

  auto iid = call->getIntrinsicID();
  if (iid != llvm::Intrinsic::stackrestore)
    report_fatal_errorv("{0}:{1}: User of {2} is not a"
                        " stackrestore: {3}.",
                        __FILE__, __LINE__, *last_insn,
                        *user);

  // Advance the iterator and enter the node.
  BasicBlock::iterator it{call};
  ++it;
  return &*it;
}

Instruction* move_alloca::stack_entry::get_next_successor_bb()
{
  // Process the next basic block if there is one.
  if (!last_insn->isTerminator() ||
      succ_iter == llvm::succ_end(last_insn))
    return nullptr;

  // Find the next basic block.
  BasicBlock *bb = *(succ_iter);
  LLVM_DEBUG(
    dbgs() << formatv("  Next block is     {0}\n",
                      *bb->getFirstNonPHI()));

  // Advance the iterator and enter the node.
  ++succ_iter;
  return bb->getFirstNonPHI();
}

Instruction* move_alloca::stack_entry::get_next_tail()
{
  // Process the successor instruction if necessary.
  if (!succ_insn_pending)
    return nullptr;
  // Mark the successor as done.
  succ_insn_pending = false;

  // Find the next instruction and enter the node.
  auto insn = last_insn->getNextNode();
  LLVM_DEBUG(
    dbgs() << formatv("  Next insn is      {0}\n", *insn));
  return insn;
}

Instruction* move_alloca::stack_entry::get_next()
{
  // Pull out one node from our sources of next nodes and follow that
  // for depth first traversal.
  Instruction *next = get_next_stackrestore();
  if (next != nullptr)
    return next;
  next = get_next_successor_bb();
  if (next != nullptr)
    return next;
  next = get_next_tail();
  if (next != nullptr)
    return next;
  return nullptr;
}

bool move_alloca::process()
{
  // Push the first node onto the stack.
  BasicBlock &entry_bb = m_func->getEntryBlock();
  Instruction *first_insn = entry_bb.getFirstNonPHI();
  LLVM_DEBUG(
    dbgs() << formatv("First non-PHI is    {0}\n", *first_insn));

  // Skip over allocas in the entry basic block since they don't need
  // to be moved.  They aren't considered part of the entry node.
  first_insn = &*skip_allocas(BasicBlock::iterator(first_insn));
  LLVM_DEBUG(
    dbgs() << formatv("First non-alloca is {0}\n", *first_insn));

  // Enter the entry node, pushing it onto the stack.
  assert(m_stack.empty());
  try_enter_node(first_insn);
  assert(!m_stack.empty());

  // Process the top of the stack while the stack is not empty.
  while (!m_stack.empty()) {
    stack_entry &current = m_stack.back();
    LLVM_DEBUG(
      dbgs() << formatv("Processing node     {0}\n",
                        *current.first_insn));

    // Find the first instruction of the next successor node to try to
    // enter.  Exit the current node if all of the successors have
    // been tried.
    auto* next = current.get_next();
    if (next != nullptr)
      try_enter_node(next);
    else
      exit_node();
  }

  return m_changes_made;
}

///////////////////////////////////////////////////////////////////////////

move_alloca_pass::move_alloca_pass():
  FunctionPass(ID)
{
}

bool move_alloca_pass::runOnFunction(Function &f)
{
  // Ignore function declarations.
  if (f.empty())
    return false;

  move_alloca state(&f);
  return state.process();
}

char move_alloca_pass::ID = 0;
static RegisterPass<move_alloca_pass>
  reg("move-alloca", "Move allocas to the function entry block",
    false,
    false
    );

///////////////////////////////////////////////////////////////////////////
