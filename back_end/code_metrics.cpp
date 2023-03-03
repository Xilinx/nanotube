/*******************************************************/
/*! \file code_metrics.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Compute code metrics over packet kernel code.
**   \date 2022-03-02
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
 * The Code-Metrics pass computes interesting metrics over the provided
 * Nanotube packet kernel code.
 *
 * In more detail, it inspects instructions, operands, and calls to the
 * Nanotube API to extract data that can give a perspective on cost and
 * potential hardware implementation trade-offs.  The metrics will approximate
 * the real cost, depending on the specifics of the chosen back-end.
 * Therefore, they should really provide only a first-order approximation.
 *
 * Currently, the following metrics are extracted:
 * XXX
 *
 * _Theory of Operation_
 *
 * - go through all packet kernel pipeline functions
 * - trace instructions and control flow
 * - XXX
 *
 * _Prerequisites_
 *
 * The code presented to the pass has to be processed by all the compilation
 * steps including the Pipeline pass.  (It may be interesting to compute
 * similar metrics at earlier points in the compiler pipeline, too, but after
 * the Pipeline pass is the starting point, for now.)
 * XXX: What about after inlining the taps?
 *
 * _Expected Output_
 *
 * This pass does not change the code that is passed to it.  It will provide
 * the statistics output as XXX.
 */
#include "code_metrics.hpp"
#include <iomanip>
#include <iostream>
#include <unordered_map>

#include "llvm/Analysis/IteratedDominanceFrontier.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "common_cmd_opts.hpp"
#include "Dep_Aware_Converter.h"
#include "Intrinsics.h"
#include "printing_helpers.h"
#include "setup_func.hpp"

#define DEBUG_TYPE "code-metrics"
using namespace llvm;
using namespace nanotube;
cl::opt<bool> create_trace("create-trace", cl::desc("Creates kernel "\
                "copies that only contain a straight-line trace of "\
                "execution"));


void code_metrics::getAnalysisUsage(AnalysisUsage &info) const {
  info.addRequired<PostDominatorTreeWrapperPass>();
  info.addRequired<DominatorTreeWrapperPass>();
  info.addRequired<NanotubeAAPass>();
  info.addRequired<AAResultsWrapperPass>();
}

void code_metrics::get_all_analysis_results(Function* f) {
  pdt = &getAnalysis<PostDominatorTreeWrapperPass>(*f).getPostDomTree();
  dt  = &getAnalysis<DominatorTreeWrapperPass>(*f).getDomTree();
  aa  = &getAnalysis<AAResultsWrapperPass>(*f).getAAResults();
}

/**
 * Computes a (simple, one-dimensional) cost for the provided instruction.
 * This should be seen as a quick filter that for example outputs zero for
 * LLVM instructions which are "free", such as debug annotations, bitcasts,
 * etc.  For everything else, this likely depends also on an envisioned
 * backend implementation (are MULs more expensive than ADDs and are those
 * more expensive than AND?).
 *
 * Keep it very simple for now, and capture first-order effects.  There
 * will be other analysis code that looks at instruction mix (which could
 * tell us that the code is larlgely DIVs and ADDs and so we should have
 * different costs for these. */
static unsigned get_weight(Instruction* inst) {
  /* Handle special instructions */
  if( isa<AllocaInst>(inst) || isa<CastInst>(inst) )
    return 0;
  auto* gep = dyn_cast<GetElementPtrInst>(inst);
  if( (gep != nullptr) && gep->hasAllZeroIndices() )
    return 0;

  /* Check calls and intrinsics */
  auto* call = dyn_cast<CallBase>(inst);
  if( call != nullptr ) {
    auto i = get_intrinsic(call);

    /* Debug instructions are free */
    if( (i == Intrinsics::llvm_dbg_declare) ||
        (i == Intrinsics::llvm_dbg_value) )
      return 0;
    /* Lifetime annotations are free */
    if( (i == Intrinsics::llvm_lifetime_start) ||
        (i == Intrinsics::llvm_lifetime_end) )
      return 0;
  }

  /* Default: take one cycle / weight of one */
  return 1;
}
static unsigned get_weight(BasicBlock* bb) {
  unsigned sum = 0;
  for( auto& inst : *bb )
    sum += get_weight(&inst);
  return sum;
}

/***** Tracing the connection between conditions and PHI nodes *****/
struct cond_bb_t {
  Value*      cond;     /* A condition that affects a branch */
  BasicBlock* bb;       /* Basic block that contains the branch
                           (note: maybe != cond->getParent() !) */
};
typedef std::unordered_map<BasicBlock*, Value*> bb_to_cond_t;

/**
 * Get the value that drives the conditional exit of the provided block;
 * returns nullptr if the basic block does not terminate with a conditional
 * branch.
 *
 * @param bb    Basic block to get the conditional brnach value from.
 * @return      Value that drives the conditional exit of the provided
 *              block; nullptr if basic block has no conditional exit.
 */
static Value* get_condition(BasicBlock* bb) {
  /* Ignore basic blocks without multiple successors */
  if( bb->getSingleSuccessor() != nullptr )
    return nullptr;

  auto* term = bb->getTerminator();
  /* Record the condition value of conditional branchs and switches */
  auto* br = dyn_cast<BranchInst>(term);
  if( br != nullptr ) {
    assert(br->isConditional());
    return br->getCondition();
  }

  auto* sw = dyn_cast<SwitchInst>(term);
  if( sw != nullptr )
    return sw->getCondition();

  /* Returns are ignored */
  auto* ret = dyn_cast<ReturnInst>(term);
  if( ret != nullptr )
    return nullptr;

  /* Everything else we don't handle, so complain ;) */
  errs() << "FIXME: Unhandled basic block exit " << *term << " in "
         << __FILE__ << ":" << __LINE__ << " Aborting.\n";
  abort();
}

/**
 * Collect all basic blocks that have more than one successor and find out
 * their conditions that determine which successor is taken.
 */
static bb_to_cond_t collect_bb_and_conds(Function* f) {
  bb_to_cond_t bb2cond;
  for( auto& bb : *f ) {
    auto* cond = get_condition(&bb);
    if( cond != nullptr )
      bb2cond[&bb] = cond;
  }
  return bb2cond;
}

typedef std::unordered_map<BasicBlock*, std::vector<Value*>>
  bb_incoming_cond_t;

/**
 * Compute which conditions (values that affect conditional branches and
 * other control flow) are steering an execution path towards a particular
 * basic block.
 *
 * XXX: Examples (draw ASCII art!)
 * A -> B; B -> C; B -> D; C -> E; D -> E; E -> G; A -> F; F -> G
 *
 * A: nil
 * B: cond_A
 * C: cond_A, cond_B
 * D: cond_A, cond_B
 * E: cond_A (note: cond_B does not matter, because when we
 *                  execute B, we will always execute E!)
 * F: cond_A
 * G: nil
 */
#if 0 //NOTE: I found a simpler representation for this
static bb_incoming_cond_t
compute_incoming_conditions(Function* f, DominatorTree* dt,
                            PostDominatorTree* pdt) {

  /* Collect conditions and which outgoing BBs they affect.  Also create a
   * reverse map for faster access. */
  auto outgoing_conds = collect_bb_and_conds(f);
  std::unordered_map<Value*, BasicBlock*> cond2outgoing;
  for( auto& bb_cond : outgoing_conds )
    cond2outgoing[bb_cond.second] = bb_cond.first;

  bb_incoming_cond_t incoming_conds;

  typedef Dep_Aware_Converter<BasicBlock*> dac_t;
  dac_t dac;
  for( auto& bb : *f )
    dac.insert(&bb, pred_size(&bb));

  dac.execute(
    [&](dac_t* dac, BasicBlock* bb) {
      /* Check whether conditions at the tail could be removed because they
       * don't actually affect our execution anymore */
      // XXX: Check and explain why doing this from the tail is always
      // correct :)
      while( !incoming_conds[bb].empty() ) {
        auto* last_cond = incoming_conds[bb].back();
        if( pdt->dominates(bb, cond2outgoing[last_cond]) )
          incoming_conds[bb].pop_back();
        else
          break;
      }

      /* If we ourselves have a conditional exit, make our children
       * dependent on it */
      auto it = outgoing_conds->find(bb);
      if( it != outgoing_conds->end() ) {
        for( auto* succ : successors(bb) )
          incoming_conds[succ]->emplace_back(it->second);
      }


      for( auto* succ : successors(bb) )
        dac->mark_dep_ready(succ);
    });
  return incoming_conds;
}
#endif

/**
 * Computes the control-merge dependence for a basic block.
 *
 * The control-merge dependence is those basic blocks that have a
 * control-dependence on the predecessors of the queried basic block, but
 * not on the lowest common dominator of those predecessors.
 *
 * In short, it is the basic blocks which affect the control flow that
 * selects which one of the phi node entries is picked.
 */
static void
compute_control_merge_dependence(BasicBlock* bb_qry,
                                 SmallVectorImpl<BasicBlock*>* deps,
                                 DominatorTree* dt, PostDominatorTree* pdt)
{
  /* The Dominance frontier of BB bb_a is those basic blocks that are
   * just not dominated anymore by bb_a.  Meaning that a direct
   * predecessor of bb_a is, but bb_a is not anymore.  For the
   * post-dominante tree, this means that if a bb_b is part of the
   * reverse (post-) dominance frontier (RDF), it must be a conditional
   * block and at least one of the paths out from it does not go through
   * bb_a, while at least one other path will go through bb_a.
   *
   * Therefore, to understand which value a phi node should select, we
   * need to know which of the predecessors was executed and which
   * conditions were responsible for that.
   *
   * Instead of querying the RDF for every single predecessor and doing a
   * set union, we query them together.
   *
   * Further, we will remove the prefix of the frontier, namely we only
   * have to think about conditions up to bb_dom =
   * dominator(preds(bb_qry)), because when we are in bb_qry, we knew we
   * had to come through bb_dom, and thus those conditions are clear!
   *
   * The LLVM implementation solves that by using liveness, basically
   * marking only those basic blocks that we care about.  To use that
   * efficiently, we would have to mark live all the nodes between bb_dom
   * and bb_qry, but that seems a bit tedious, so instead let's simply
   * delete the prefix from bb_dom.
   *
   * Sadly, the whole algorithm is N^2 with N being the entire size of
   * the program, because it reconstructs everything from scratch and
   * each operation is linear.  It could do a much better job in one go
   * which should be linear in the program size.  Or at least have an
   * early out parameter in IDFCalculator::calculate.  That would mean
   * that the whole computation is only O(N * K) with K being the number
   * of basic blocks between bb_dom and bb_qry.
   *
   * Because we know that bb_dom is the dominator of bb_qry, we know that
   * we can just get all the predecessors of bb_qry and will eventually
   * get to bb_dom :)  And that will then be the live set.
   *
   * XXX: I am wondering whether this is actually correct and optimised.
   * How would this handle a case of:
   *
   * A -> B -> C -> D
   *      B -> Z
   *           C-> Z
   * A -> E -> F -> D
   *      E -> Z
   *           F -> Z
   *
   * and then querying for D?  Surely, the conditions in B, C, E, and F
   * will not influence the phi node in D, because they only can cause
   * execution to not hit D.  Once the phi is however executed (or looked
   * at), we know the outcomes of these conditions.  The interesting aspect
   * here is that we need to remove the edges from B, C, E, and F that do
   * not lead eventually to D.  Discounting them, we find that B, C, E, and
   * F are not conditional anymore.  The interesting nodes for that are the
   * nodes that are on the post-dominance frontier of D, i.e., those that
   * have edges bypasing D.  These nodes will also be on the PDF for C & F,
   * but we need to distinguish those edges that bypass C & F (but still
   * get to D) vs those that actually bypass D.
   */

  /* Find the nearest dominator that dominates(preds(bb)) */
  auto* bb_dom = bb_qry;
  for( auto* pred : predecessors(bb_qry) )
    bb_dom = dt->findNearestCommonDominator(bb_dom, pred);

  LLVM_DEBUG(
    dbgs() << "bb_qry: " << bb_qry->getName()
           << " bb_dom: " << bb_dom->getName() << '\n');

  /* Collect all the basic blocks between bb_qry and bb_dom */
  SmallPtrSet<BasicBlock*, 8> roi;
  SmallVector<BasicBlock*, 8> todo;
  todo.emplace_back(bb_qry);
  while( !todo.empty() ) {
    auto* t = todo.back();
    todo.pop_back();
    if( t == bb_dom )
      continue;
    for( auto* pred : predecessors(t) ) {
      if( roi.count(pred) != 0 )
        continue;
      roi.insert(pred);
      todo.emplace_back(pred);
    }
  }

  LLVM_DEBUG(
    dbgs() << "Interesting BBs between bb_qry and bb_dom: ";
    for( auto* i : roi )
      dbgs() << i->getName() << ' ';
    dbgs() << '\n');

  /* query all predecessors at once */
  SmallPtrSet<BasicBlock*, 8> preds(pred_begin(bb_qry), pred_end(bb_qry));

  ReverseIDFCalculator dom_front(*pdt);
  dom_front.setDefiningBlocks(preds);
  dom_front.setLiveInBlocks(roi);
  dom_front.calculate(*deps);

  LLVM_DEBUG(
    dbgs() << "Control Impact for " << bb_qry->getName()
           << ": size " << deps->size() << " ";
    for( auto* fbb : *deps )
      dbgs() << fbb->getName() << " ";
    dbgs() << '\n';
    dbgs() << "Conditions:\n";
    for( auto* fbb : *deps )
      dbgs() << "  " << *get_condition(fbb) << '\n';
    dbgs() << '\n');

#ifdef STEPWISE_COMPUTATION
  /* Attempt 2: query one by one */
  SmallPtrSet<BasicBlock*, 8> accum_front;
  for( auto* pred : predecessors(bb_qry) ) {
    def.clear();
    def.insert(pred);
    deps->clear();
    dom_front.calculate(*deps);
    accum_front.insert(deps->begin(), deps->end());
    dbgs() << "  Pred: " << pred->getName() << ": ";
    for( auto* fbb : *deps )
      dbgs() << fbb->getName() << ' ';
    dbgs() << '\n';
  }
  dbgs() << "Control Impact for preds(" << bb.getName()
         << "): size " << accum_front.size() << " ";
  for( auto* fbb : accum_front )
    dbgs() << fbb->getName() << " ";
  dbgs() << '\n';
#endif

#ifdef ATTEMPT_REDUCTION
  /* NOTE: This is wrong code; the correct version performs the range
   * between bb_qry and bb_dom */
  def.clear();
  def.insert(bb_qry);
  deps->clear();
  dom_front.calculate(*deps);
  dbgs() << "Control impact for PHI bb " << bb_qry->getName()
         << ": size " << deps->size() << " ";
  for( auto* fbb : *deps )
    dbgs() << fbb->getName() << " ";
  dbgs() << '\n';

  for( auto* fbb : *deps )
    accum_front.erase(fbb);

  dbgs() << "Reduced control impact for PHI bb " << bb_qry->getName()
         << ": size " << accum_front.size() << " ";
  for( auto* fbb : accum_front )
    dbgs() << fbb->getName() << " ";
  dbgs() << '\n';
#endif
}

/********** Types for tracking dependencies **********/

/**
 * A tracking structure which keeps directed graphs and allows quick
 * traversal of outgoing and incoming edges of a node.
 */
template <typename T>
class doubly_linked_t {
public:
  typedef SmallVector<T,8> list_t;
  typedef std::unordered_map<T, list_t> map_t;

  /* Adds an edge from src -> dst */
  void insert(T src, T dst) {
    outgoing[src].emplace_back(dst);
    incoming[dst].emplace_back(src);
  }

  /* Returns the number of incident edges for dst */
  size_t size_in(T dst) const { return size(incoming, dst); }
  bool empty_in(T dst) const { return size_in(dst) == 0; }

  /* Returns the number of outgoing edges for src */
  size_t size_out(T src) const { return size(outgoing, src); }
  bool empty_out(T src) const { return size_out(src) == 0; }

  list_t in(T dst) const  { return list(incoming, dst); }
  list_t out(T src) const { return list(outgoing, src); }

  /* Combine two graphs */
  doubly_linked_t<T>& operator+=(const doubly_linked_t<T>& other) {
    merge_in(incoming, other.incoming);
    merge_in(outgoing, other.outgoing);
    return *this;
  }
  doubly_linked_t<T> operator+(const doubly_linked_t<T>& rhs) {
    doubly_linked_t<T> res(*this);
    return res += rhs;
  }

private:
  static void merge_in(map_t& dst, const map_t& src);

  size_t size(const map_t& map, T node) const {
    auto it = map.find(node);
    if( it == map.end() )
      return 0;
    return it->second.size();
  }

  list_t list(const map_t& map, T node) const {
    auto it = map.find(node);
    if( it == map.end() )
      return list_t();
    return it->second;
  }

  map_t outgoing;
  map_t incoming;
};

template <typename T> void
doubly_linked_t<T>::merge_in(doubly_linked_t<T>::map_t& tgt,
                             const doubly_linked_t<T>::map_t& oth) {
  typedef std::unordered_set<T> set_t;

  for( auto src_dsts : oth ) {
    auto& src  = src_dsts.first;
    auto& dsts = src_dsts.second;

    /* Merge all entries (node, edge vector) to matching entry in tgt */
    auto it = tgt.find(src);
    if( it == tgt.end() ) {
      /* We don't have an entry so simply copy the entire oth */
      tgt.emplace(src, dsts);
      continue;
    }

    /* We already have entries, so merge the two. */
    auto& our_dsts = it->second;
    /* Use a set for faster lookup */
    set_t present(our_dsts.begin(), our_dsts.end());
    /* Add the other dsts that we do not yet have */
    for( auto& dst : dsts ) {
      if( present.count(dst) > 0 )
        continue;
      our_dsts.emplace_back(dst);
      present.emplace(dst);
    }
  }
}
typedef doubly_linked_t<Instruction*> dep_map_t;

static void
compute_memory_deps(Function& f, AliasAnalysis* aa, dep_map_t* mem_dep);
static void
compute_data_deps(Function& f, dep_map_t* data_dep);
static void
compute_phi_deps(Function& f, DominatorTree* dt, PostDominatorTree* pdt,
                 dep_map_t* phi_sel_dep);

/**
 * Compute the length and actual critical path through the provided
 * function observing the provided dependencies (between pairs of
 * instructions and between pairs of basic blocks).
 *
 * Depending on the provided dependencies, various hardware implementations
 * can be emulated.
 */
static unsigned
get_critical_path(Function* f, Instruction* start, Instruction* end,
                  const dep_map_t& all_deps,
                  std::vector<Instruction*>* path,
                  DominatorTree* dt, PostDominatorTree* pdt,
                  AliasAnalysis* aa);

typedef std::unordered_set<BasicBlock*> bb_set_t;
typedef std::unordered_set<Instruction*> inst_set_t;
static void
collect_app_insts(Function* f, Instruction* start, Instruction* end,
                  bb_set_t* app_bbs, inst_set_t* app_insts) {
  /* Mark instructions / basic blocks as application vs overhead */
  auto* start_bb = (start != nullptr) ? start->getParent()
                                      : &f->getEntryBlock();
  auto* end_bb   = (end != nullptr) ? end->getParent()
                                    : (BasicBlock*)nullptr;

  /* Record individual instructions in the start / end bb */
  if( start == nullptr ) {
    LLVM_DEBUG(dbgs() << "App BB: " << start_bb->getName() << '\n');
    app_bbs->emplace(start_bb);
  }
  while( start != nullptr ) {
    LLVM_DEBUG(dbgs() << "App Inst: " << *start << '\n');
    app_insts->emplace(start);
    start = start->getNextNode();
  }
  while( end != nullptr ) {
    app_insts->emplace(end);
    end = end->getPrevNode();
  }

  /* Traverse all basic blocks between start_bb and end_bb and record the
   * basic blocks between them.
   *
   * NOTE: This relies on end_bb actually post-dominating all the basic
   * blocks in between.  A more sophisticated traversal could start at both
   * the start_bb and the end_bb (going reverse) and only consider those
   * basic blocks as being application that are reachable from both. */
  std::vector<BasicBlock*> todo;
  for( auto* app_bb : successors(start_bb) )
    todo.emplace_back(app_bb);

  while( !todo.empty() ) {
    auto* app_bb = todo.back();
    todo.pop_back();
    if( (app_bb == end_bb) || (app_bbs->count(app_bb) > 0) )
      continue;
    LLVM_DEBUG(dbgs() << "App BB: " << app_bb->getName() << '\n');
    app_bbs->emplace(app_bb);

    for( auto* succs: successors(app_bb) )
      todo.emplace_back(succs);
  }
}

/**
 * Compute the length of the critical path through the provided function,
 * assuming only data dependencies, and assuming conditionals are
 * flattened.  This assumes that
 * cond = compute_cond(zap);
 * if( cond )
 *   foo = compute1(bar);
 * else
 *   foo = compute2(gib);
 *
 * will return a path length which includes:
 *   - the longer of compute1(bar) and compute2(gib); assuming they both
 *     need to be computed, and can be computed in parallel
 *   - the latency of computing cond is only ever accounted for when
 *     accounting for the phi node that selects the "real" foo, i.e.,
 *     compute_cond(zap) can be executed in parallel with compute1(bar) and
 *     compute2(gib).
 */
static unsigned
get_data_flow_critical_path(Function* f, Instruction* start,
                            Instruction* end,
                            std::vector<Instruction*>* path,
                            DominatorTree* dt, PostDominatorTree* pdt,
                            AliasAnalysis* aa) {
  LLVM_DEBUG(dbgs() << "Function: " << f->getName() << '\n');

  /* Data-flow means.. only track actual data dependencies */
  dep_map_t phi_sel_dep, mem_dep, data_dep, all_deps;

  compute_memory_deps(*f, aa, &mem_dep);
  compute_data_deps(*f, &data_dep);
  compute_phi_deps(*f, dt, pdt, &phi_sel_dep);

  all_deps = mem_dep + data_dep + phi_sel_dep;
  return get_critical_path(f, start, end, all_deps, path, dt, pdt, aa);
}

static unsigned
get_critical_path(Function* f, Instruction* start, Instruction* end,
                  const dep_map_t& all_deps,
                  std::vector<Instruction*>* path,
                  DominatorTree* dt, PostDominatorTree* pdt,
                  AliasAnalysis* aa) {
  /* Pull out app-only code */
  bb_set_t   app_bbs;
  inst_set_t app_insts;
  collect_app_insts(f, start, end, &app_bbs, &app_insts);

  /* Add instructions to the DAC */
  typedef dep_aware_converter<Instruction> dac_t;
  dac_t dac;
  for( auto& inst : instructions(f) ) {
    /* Ignore branch and switch instructions in the critical path */
    if( isa<BranchInst>(inst) || isa<SwitchInst>(inst) )
      continue;

    /* Add instruction to the scheduling structure */
    dac.insert(&inst, all_deps.size_in(&inst));
  }

  /* Schedule the instructions */
  std::unordered_map<Instruction*, unsigned> ready;
  std::unordered_map<Instruction*, Instruction*> slowest_input;
  Instruction* slowest = nullptr;

  unsigned total_length = 0;
  dac.execute([&](dac_t* dac, Instruction* inst) {
    bool is_app = (app_bbs.count(inst->getParent()) > 0) ||
                  (app_insts.count(inst) > 0);
    if( is_app ) {
      /* Collect all dependencies */
      auto input_deps = all_deps.in(inst);

      /* Go through all input dependencies and find latest ready time */
      unsigned latest = 0;
      for( auto* dep : input_deps ) {
        if( ready[dep] > latest ) {
          latest = ready[dep];
          slowest_input[inst] = dep;
        }
      }
      auto done = latest + get_weight(inst);
      ready[inst] = done;

      /* Update critical path estimates and links */
      if( done > total_length ) {
        total_length = done;
        slowest      = inst;
        LLVM_DEBUG(dbgs() << "Inst: " << *inst
                          << " new length: " << total_length << '\n');
      }

      LLVM_DEBUG(dbgs() << "Inst: " << *inst
                        << " Start: " << latest
                        << " End: " << done
                        << '\n');
    } else { /* !is_app */
      /* Short circuit non-app instructions */
      ready[inst] = 0;
    }

    /* Unblock all of those instructions waiting on us */
    for( auto* dep : all_deps.out(inst) )
      dac->mark_dep_ready(dep);
  });

  if( path != nullptr ) {
    /* Build critical path */
    std::vector<Instruction*> critical_path;
    while( true ) {
      if( slowest == nullptr )
        break;
      critical_path.emplace_back(slowest);
      auto it = slowest_input.find(slowest);
      if( it == slowest_input.end() )
        break;
      slowest = it->second;
    }
    path->insert(path->end(),
                 critical_path.rbegin(), critical_path.rend());
  }

  return total_length;
}


typedef std::unordered_set<Instruction*>               inst_set_t;
typedef std::unordered_map<MemoryLocation, inst_set_t> loc2insts_t;
typedef std::unordered_map<BasicBlock*, loc2insts_t>   bb2loc2insts_t;
typedef std::unordered_set<MemoryLocation>             mlocs_t;
typedef std::unordered_map<BasicBlock*, mlocs_t>       known_loc_t;

/**
 * An implementation of std::hash for llvm::MemoryLocation which allows us
 * to use MemoryLocations as the key in a hash set / map.
 */
namespace std {
template<>
struct hash<llvm::MemoryLocation> {
  std::size_t operator()(const llvm::MemoryLocation m) const {
    auto* v = const_cast<Value*>(m.Ptr);
    if( m.Size.hasValue() )
      return std::hash<Value*>{}(v) ^
             std::hash<uint64_t>{}(m.Size.getValue());
    else
      return std::hash<Value*>{}(v);
  }
};
}

/**
 * Parse an AllocaInst and get a memory location that covers the
 * allocation.
 */
static
MemoryLocation get_mloc(AllocaInst* alloca) {
  const auto& dl = alloca->getModule()->getDataLayout();
  auto size   = alloca->getAllocationSizeInBits(dl);
  auto tysize = LocationSize::unknown();
  if( size.hasValue() ) {
    tysize = LocationSize::precise(size.getValue() / 8);
  } else {
    errs() << "Unknown size for alloca: " << *alloca << '\n';
  }
  return MemoryLocation(alloca, tysize);
}

/**
 * Check whether an instruction is an alloca and if so, add its memory to
 * the set of known memory locations.
 */
static void
parse_alloca(Instruction& inst, mlocs_t* mlocs) {
  /* Record interesting memory locations; from allocas only (for now) */
  auto* alloca = dyn_cast<AllocaInst>(&inst);
  if( alloca != nullptr )
    mlocs->emplace(get_mloc(alloca));
}

/**
 * Ignore function calls to functions that don't really affect the
 * arguments in a meaningful way.  Things like LLVM helpers and printf.
 */
static
bool ignore_function(CallBase* call) {
  /* Skip printf, lifetime.start, and lifetime.end */
  auto intrin = get_intrinsic(call);
  /**
   * We ignore the following:
   * lifetime.start / lifetime.end .. because we will be computing our own
   * stacksave / -restore .. because they only change the
   *                         visibility of allocas and potentially
   *                         deallocate them
   * printf .. because AA suggests that printf modifies its arguments?
   */
  if( intrin == Intrinsics::llvm_lifetime_start ||
      intrin == Intrinsics::llvm_lifetime_end  ||
      intrin == Intrinsics::llvm_stacksave ||
      intrin == Intrinsics::llvm_stackrestore )
    return true;
  auto* callee = call->getCalledFunction();
  if( callee == nullptr ) {
    errs() << "ERROR: Unknown callee in call\n" << *call
           << "\nContaining BB: " << *call->getParent()
           << "\nAborting!\n";
    abort();
  }
  StringRef name = callee->getName();
  if( name.equals("printf") ||
      name.equals("__assert_fail") )
    return true;
  return false;
}

/**
 * Should the current instruction be skipped when looking at memory
 * dependencies?
 */
static bool
skip_boring_inst(Instruction& inst) {
  if( !inst.mayReadFromMemory() && !inst.mayWriteToMemory() )
    return true;;

  /* Skip calls to uninteresting / confusing functions */
  if( isa<CallInst>(&inst) && ignore_function(cast<CallInst>(&inst)) )
    return true;

  return false;
}

/**
 * Merge in a new set of entries of BB -> loc -> {insts}.
 *
 * Only creates non-empty keys, rather than adding BB -> loc -> <empty_set>
 * cases.
 */
static bool
merge_last_accesses(loc2insts_t* cur, const loc2insts_t& merge) {
  bool is_mem_phi = false;
  for( auto& mloc_lws : merge ) {
    MemoryLocation mloc  = mloc_lws.first;
    const inst_set_t& pred_lws = mloc_lws.second;

    auto it = cur->find(mloc);
    if( it != cur->end() ) {
      /* We already know mloc: add instructions one by one and check for
       * multiple last-writes */
      inst_set_t& cur_lws = it->second;
      for( Instruction* wr : pred_lws ) {
        auto res = cur_lws.insert(wr);
        is_mem_phi |= res.second;
      }
    } else {
      /* We have not seen any writes to mloc yet -> initialise */
      (*cur)[mloc].insert(pred_lws.begin(), pred_lws.end());
    }
  }
  return is_mem_phi;
}

/**
 * Print a warning for memory phi nodes.
 *
 * Memory phi-nodes are control-flow converge points where the same memory
 * location was modified by different writes in the predecessing basic
 * blocks.
 */
static void
warn_mem_phi(const loc2insts_t& cur_loc_lws) {
  errs() << "WARNING: Multiple merging definitions (memory-phi) for\n";
  for( auto& mloc_lws : cur_loc_lws ) {
    if( mloc_lws.second.size() == 1 )
      continue;
    errs() << "  " << mloc_lws.first << '\n';
    for( auto* wr : mloc_lws.second )
      errs() << "    " << *wr << '\n';
  }
}

/**
 * Initialise tracking structures of a basic block from the predecessors.
 */
static void
compute_bb_inputs(BasicBlock* bb, known_loc_t& known_locs,
                  bb2loc2insts_t& last_writes, bb2loc2insts_t& last_reads)
{
  auto&        cur_known   = known_locs[bb];
  loc2insts_t& cur_loc_lws = last_writes[bb];
  loc2insts_t& cur_loc_rds = last_reads[bb];
  bool is_mem_phi = false;
  for( auto* pred : predecessors(bb) ) {
    /* Compute in-memloc as the union over the previous out-memloc */
    auto& pred_known = known_locs[pred];
    cur_known.insert(pred_known.begin(), pred_known.end());

    is_mem_phi |= merge_last_accesses(&cur_loc_lws, last_writes[pred]);
    merge_last_accesses(&cur_loc_rds, last_reads[pred]);
  }
  if( is_mem_phi )
    warn_mem_phi(cur_loc_lws);

  LLVM_DEBUG(
    dbgs() << "Entry last-writes:\n";
    for( auto& mloc_lws : cur_loc_lws ) {
      dbgs() << "  " << mloc_lws.first << ": ";
      for( auto* lw : mloc_lws.second )
        dbgs() << nullsafe(lw) << " ";
      dbgs() << '\n';
    }
    dbgs() << "Entry last-reads:\n";
    for( auto& mloc_rds : cur_loc_rds ) {
      dbgs() << "  " << mloc_rds.first << ": ";
      for( auto* lrd : mloc_rds.second )
        dbgs() << nullsafe(lrd) << " ";
      dbgs() << '\n';
    }
  );
}

/**
 * Intersect an instruction with a provided memory location.
 *
 * Checking whether the provided instruction accesses (read / write /
 * read^write / nothing) the provided memory location and update the
 * tracking structures accordingly.  We keep track of the last write(s) of
 * the location and update that if the provided instruction is a write.
 */
static void
check_inst_mloc(Instruction& inst, const MemoryLocation& mloc,
                loc2insts_t* loc2lw, loc2insts_t* loc2lr,
                dep_map_t* deps, aa_helper& aah) {
  inst_set_t* last_writes = nullptr;
  auto it = loc2lw->find(mloc);
  if( it != loc2lw->end() ) {
    last_writes = &it->second;
    assert(!last_writes->empty());
  }

  inst_set_t* last_reads = nullptr;
  auto rd_it = loc2lr->find(mloc);
  if( rd_it != loc2lr->end() ) {
    last_reads = &rd_it->second;
    assert(!last_reads->empty());
  }

  auto res = aah.get_mri(&inst, mloc);
  if( isNoModRef(res) )
    return;
  bool rd = inst.mayReadFromMemory() && isRefSet(res);
  bool wr = inst.mayWriteToMemory()  && isModSet(res);

  LLVM_DEBUG(
    if( rd || wr )
      dbgs() << "Memory Location: " << mloc << '\n';

    if( rd ) {
      dbgs() << "Read: " << inst << " reads from ";
      if( last_writes != nullptr ) {
        for( auto* lw : *last_writes )
          dbgs() << *lw << " ";
        dbgs() << '\n';
      } else {
        dbgs() << "<empty>\n";
      }
    }
    if( wr ) {
      dbgs() << "Write: " << inst << " overwrites ";
      if( last_writes != nullptr ) {
        for( auto* lw : *last_writes )
          dbgs() << *lw << " ";
        dbgs() << '\n';
      } else {
        dbgs() << "<empty>\n";
      }
    }
  );

  if( rd ) {
    (*loc2lr)[mloc].insert(&inst);

    if( last_writes == nullptr ) {
      errs() << "ERROR: Uninitialised read " << inst
             << " from location " << mloc << '\n'
             << "Aborting!\n";
      abort();
    } else if( last_writes->size() > 1 ) {
      errs() << "WARNING: Read " << inst
             << "reads from multiple stores:\n";
      for( auto* wr : *last_writes )
        errs() << "  " << *wr << '\n';
    }
    for( auto* w : *last_writes ) {
      /* Capture read-after-write dependencies */
      deps->insert(w, &inst);
    }
  }
  if( wr ) {
    /* Capture write-after-write dependencies */
    if( last_writes != nullptr ) {
      for( auto* w : *last_writes )
        deps->insert(w, &inst);
    }
    /* Capture write-after-read dependencies */
    if( last_reads != nullptr ) {
      for( auto* r : *last_reads )
        deps->insert(r, &inst);
    }
    /* Clear last writes and last reads for this location (and basic block)
     * Note: overwrite potentially multiple previous writes! */
    (*loc2lw)[mloc].clear();
    (*loc2lw)[mloc].insert(&inst);
    loc2lr->erase(mloc);
  }
}

static void
compute_memory_deps(Function& f, AliasAnalysis* aa, dep_map_t* mem_dep) {
  aa_helper aah(aa);
  known_loc_t known_locs;
  /* Note: the last_writes could be more than one write, in case of join
   * BBs! */
  bb2loc2insts_t last_writes;
  bb2loc2insts_t last_reads;

  typedef dep_aware_converter<BasicBlock> dac_t;
  dac_t dac;
  dac.init_forward(f);


  /* Memory dependencies track order of memory instructions.  We record the
   * following orders (for now):
   * - reads following writes to the same location (true deps, RAW)
   * - writes following reads (anti-deps, WAR)
   * - writes following writes (WAW)
   *
   * we do not track dependencies to the transitive closure of writes;
   * instead, each write (and each read) will only depend on the immediate
   * preceeding write(s).  This can still be more than one in the case of
   * converging control flow and two writes directly preceeding a read /
   * write.  We could drop tracking WAW dependencies if there are any reads
   * between two writes (because we will get a RAW - WAR edge), but in case
   * the reads get squished / optimised away /.., it is safer to have the
   * write-to-write edges in, as well.
   *
   * Reads amongst themselve can be freel reordered and therefore do not
   * have any dependencies between them.
   *
   * We currently track accesses on the granularity of allocations, so
   * writes to distinct parts of these allocations will be ordered even
   * though they don't have to.  Similar to non-overlapping reads & writes.
   * There are two ways of making that more precise:
   *
   *   (1) we track allocations on a finer granularity (byte level / region
   *       level), potentially combining that with the selective forward
   *       flowing of allocations to memory instructions so that we do not
   *       have to quiz each instruction for all the allocation bytes, or
   *
   *   (2) we can just look at the memory instructions in isolation and do
   *       pairwise alias analysis queries going backwards; I am not sure
   *       how much work that tracing is, however.
   *
   * For now, work forwards and scan entire allocations exhaustively.
   */

  /* Walk through all basic blocks in CFG compatible order, and collect
   * information about allocas and how they are accessed */
  dac.execute([&](dac_t* dac, BasicBlock* bb) {
    LLVM_DEBUG(dbgs() << bb->getName() << '\n');
    compute_bb_inputs(bb, known_locs, last_writes, last_reads);

    auto& cur_known = known_locs[bb];
    loc2insts_t& cur_loc_lws = last_writes[bb];
    loc2insts_t& cur_loc_lrs = last_reads[bb];

    for( auto& inst : *bb ) {
      parse_alloca(inst, &cur_known);
      if( skip_boring_inst(inst) )
        continue;

      for( auto& mloc : cur_known ) {
        check_inst_mloc(inst, mloc, &cur_loc_lws, &cur_loc_lrs,
                        mem_dep, aah);
      }
    }
    dac->ready_forward(bb);
  });

  /* Print a DOT graph of the memory dependencies */
  if( opt_print_analysis_info ) {
    outs() << "digraph {\n"
           << "  label=\"" << f.getName() << "\"\n";
    unsigned id = 0;
    std::unordered_map<Instruction*, unsigned> inst_ids;

    for( auto& inst : instructions(f) ) {
      if( mem_dep->empty_in(&inst) && mem_dep->empty_out(&inst) )
        continue;
      outs() << "  inst" << id << " [label=\"" << inst << "\"]\n";
      inst_ids[&inst] = id++;
    }
    for( auto& inst : instructions(f) ) {
      if( mem_dep->empty_out(&inst) )
        continue;
      outs() << "  inst" << inst_ids[&inst] << " -> {";
      for( auto* succ : mem_dep->out(&inst) )
        outs() << "inst" << inst_ids[succ] << " ";
      outs() << "}\n";
    }
    outs() << "}\n";
  }
}

static void
compute_data_deps(Function& f, dep_map_t* data_dep) {
  /* Determine input and output deps for every instruction */
  for( auto& inst : instructions(f) ) {
    LLVM_DEBUG(dbgs() << "Instruction: " << inst << '\n');

    /* All instructions depend on their operands */
    for( auto* v : inst.operand_values() ) {
      /* Instructions: record dependency */
      auto* dep = dyn_cast<Instruction>(v);
      if( dep != nullptr ) {
        data_dep->insert(dep, &inst);
        LLVM_DEBUG(dbgs() << "  Dataflow input: " << *dep << '\n');
        continue; // inst.operand_values()
      }

      /* Ignore known ready operands */
      if( isa<Constant>(v) || isa<Argument>(v) ||
          isa<MetadataAsValue>(v) || isa<BasicBlock>(v))
        continue;

      errs() << "IMPLEMENT ME: Unknown operand " << *v
             << " in instruction " << inst
             << "\nAborting.\n";
      abort();
    }
  }
}

static void
compute_phi_deps(Function& f, DominatorTree* dt, PostDominatorTree* pdt,
                 dep_map_t* phi_sel_dep) {
  auto bb2cond = collect_bb_and_conds(&f);
  LLVM_DEBUG(
    for( auto bb_cond : bb2cond ) {
      dbgs() << "BB: " << bb_cond.first->getName()
             << " Cond: " << *bb_cond.second << '\n';
    });

  /* For every node with PHI nodes, check which conditions affect the
   * predecessors */
  std::unordered_map<BasicBlock*, SmallVector<BasicBlock*,8>> bb2deps;
  for( auto& bb : f ) {
    /* Skip nodes that do not converge control flow */
    if( bb.getSinglePredecessor() != nullptr )
      continue;
    /* Skip nodes that do not start with a phi node */
    if( !isa<PHINode>(bb.front()) )
      continue;

    compute_control_merge_dependence(&bb, &bb2deps[&bb], dt, pdt);
  }
  LLVM_DEBUG(
    for( auto& bb_dep : bb2deps ) {
      dbgs() << "BB: " << bb_dep.first->getName() << " Dep: ";
      for( auto* bb : bb_dep.second )
        dbgs() << bb->getName() << ' ';
      dbgs() << '\n';
    });

  /* Compute the actual dependencies for all PHI nodes */
  for( auto& inst : instructions(f) ) {
    auto* phi = dyn_cast<PHINode>(&inst);
    if( phi == nullptr )
      continue;

    LLVM_DEBUG(dbgs() << "PHI Node: " << *phi << '\n');
    for( auto* bb : bb2deps[phi->getParent()] ) {
      auto* dep = cast<Instruction>(bb2cond[bb]);
      phi_sel_dep->insert(dep, phi);
      LLVM_DEBUG(dbgs() << "  Condition input: " << *dep << '\n');
    }
  }
}

/**
 * Computes the critical path of all instructions inside a single basic
 * block.  This assumes that all values defined outside of the basic block
 * are ready for use, and that all instructions can execute the moment all
 * their dependencies are available.
 */
static unsigned
get_data_flow_critical_path(BasicBlock* bb) {
  std::unordered_map<Instruction*, unsigned> inst_to_ready;
  unsigned max_in_bb = 0;
  for( auto& inst : *bb ) {
    unsigned max = 0;
    //dbgs() << inst << '\n';
    for( auto* v : inst.operand_values() ) {

      /* Instructions: either schedule or already ready */
      auto* prod_inst = dyn_cast<Instruction>(v);
      if( prod_inst != nullptr ) {
        /* Instructions from elsewhere are ready now */
        if( prod_inst->getParent() != bb )
          continue;

        /* Instructions from this BB update the ready time */
        //dbgs() << "  " << *prod_inst << '\n';
        auto it = inst_to_ready.find(prod_inst);
        assert(it != inst_to_ready.end());
        max = (it->second > max) ? it->second : max;
        continue;
      }

      /* Other cases that are ready at the start of the BB */
      if( isa<Constant>(v) || isa<Argument>(v) ||
          isa<MetadataAsValue>(v) || isa<BasicBlock>(v))
        continue;

      errs() << "Unknown case " << *v << '\n';
      assert(false);
    }
    unsigned ready_time  = max + get_weight(&inst);
    //dbgs() << inst << " ready: " << ready_time << '\n';
    inst_to_ready[&inst] = ready_time;
    max_in_bb = (ready_time > max_in_bb) ? ready_time : max_in_bb;
  }
  return max_in_bb;
}

typedef unsigned bb_cost_func_t(BasicBlock* bb);

/**
 * Schedule basic blocks in a function (with optional start / end markers)
 * according to a cost function provided.  The schedule will compute the
 * critical path through the CFG according to the cost function and return
 * its length.  If a result vector is specified, it will collect the
 * selected critical path of basic blocks.
 */
static unsigned
bb_schedule(Function* f, Instruction* start, Instruction* end,
            bb_cost_func_t* bb_cost_func,
            std::vector<BasicBlock*>* path = nullptr) {
  std::unordered_map<BasicBlock*, unsigned> bb_to_cost;
  typedef dep_aware_converter<BasicBlock> bb_dac_t;
  bb_dac_t dac;

  /* Collect the critical path lengts through each block */
  for( auto& bb : *f) {
    auto cost = bb_cost_func(&bb);
    bb_to_cost[&bb] = cost;

    LLVM_DEBUG(
      auto cp = get_data_flow_critical_path(&bb);
      auto w  = get_weight(&bb);
      dbgs() << "  BB: " << bb.getName()
             << " Cost: " << cost << " ILP: "
             << formatv("{0:f}", (float)w / cp) << '\n');
    dac.insert(&bb, pred_size(&bb));
  }

  /* Schedule each block: start when all predecessors are ready */
  unsigned max = 0;
  std::unordered_map<BasicBlock*, unsigned> bb_complete;
  std::unordered_map<BasicBlock*, BasicBlock*> slowest_pred;

  /* Keep track of whether a basic block is part of application code or
   * "overhead" that should be discounted */
  bool  in_app   = (start == nullptr);
  auto* start_bb = (start != nullptr) ? start->getParent() : nullptr;
  auto* end_bb   = (end != nullptr)   ? end->getParent() : nullptr;

  BasicBlock* max_bb = nullptr;

  dac.execute(
    [&](bb_dac_t* dac, BasicBlock* bb) {
      if( bb == start_bb )
        in_app = true;

      if( in_app ) {
        /* This BB can start when all of its predecessors are done */
        unsigned start      = 0;
        BasicBlock* slowest = nullptr;
        for( auto* pred : predecessors(bb) ) {
          if( bb_complete[pred] < start )
            continue;
          start   = bb_complete[pred];
          slowest = pred;
        }
        slowest_pred[bb] = slowest;

        /* Update the overall completion time of this BB and check whehter
         * it is the longest running BB in the program. */
        unsigned finish = start + bb_to_cost[bb];
        bb_complete[bb] = finish;
        if( finish > max ) {
          max = finish;
          max_bb = bb;
        }

        LLVM_DEBUG(
          dbgs() << "  BB: " << bb->getName() << " Start: " << start
                 << " Finish: " << finish << '\n');
        in_app = (bb != end_bb);
      }

      for( auto* succ : successors(bb) )
        dac->mark_dep_ready(succ);
    });
  /* Construct the actual longest path through the program */
  if( path != nullptr ) {
    std::vector<BasicBlock*> rev_path;
    while( max_bb != nullptr ) {
      rev_path.emplace_back(max_bb);
      max_bb = slowest_pred[max_bb];
    }
    path->insert(path->end(), rev_path.rbegin(), rev_path.rend());
  }
  return max;
}

/**
 * Compute another longest path through the CFG of the provided function
 * this time taking the total weight of each BB into account, rather than
 * the critical path through it.
 *
 * This will tell us how many instructions we will have to execute worst
 * case per stage if we branch to basic blocks.
 */
static unsigned
get_cfg_longest_path(Function* f, Instruction* start, Instruction* end,
                     std::vector<BasicBlock*>* path = nullptr) {
  return bb_schedule(f, start, end, get_weight, path);
}

/**
 * Compute the CFG (control flow graph) critical path through the provided
 * function.
 *
 * This traversal operates in a hierarchical fashion and only extracts
 * parallelism within a basic block:
 *   - for each basic block in the function, find the length of the
 *     critical path through it, assuming that all values defined in other
 *     basic blocks are ready at the start of the basic block
 *   - this length includes the cost of computing a potentially present
 *     conditional that is used to branch to the next block
 *   - then find the critical path on the CFG of the function, where each
 *     basic block has cost of its own critical path
 *
 * This corresponds to an execution which does not "speculate" past
 * branches, but instead resolves them in time.  In this scenario, phi
 * nodes also should be cheap, as they do not really have to perform any
 * selection.  This approach is pessimistic for coordinated branches:
 *
 * if( cond )
 *   a = long_op(foo);
 * else
 *   a = short_op(foo);
 * ...
 * if( cond )
 *   b = short_op(bar);
 * else
 *   b = long_op(bar);
 *
 * will assume that the critical path >= 2x long_op, even though no actual
 * execution would execute both long operations, irrespective of the value
 * of cond.
 */
static unsigned
get_cfg_critical_path(Function* f, Instruction* start, Instruction* end,
                      std::vector<BasicBlock*>* path = nullptr) {
  return bb_schedule(f, start, end, get_data_flow_critical_path, path);
}


/**
 * Compute the accumulated weight of all instructions in a function;
 * optionally limited by a start and end instruction.
 */
static unsigned
get_weight(Function* f, Instruction* start, Instruction* end) {
  typedef dep_aware_converter<BasicBlock> bb_dac_t;
  bb_dac_t dac;
  for( auto& bb : *f)
    dac.insert(&bb, pred_size(&bb));

  bool  in_app   = (start == nullptr);
  auto* start_bb = (start != nullptr) ? start->getParent() : nullptr;
  auto* end_bb   = (end != nullptr)   ? end->getParent() : nullptr;

  unsigned total = 0;

  dac.execute(
    [&](bb_dac_t* dac, BasicBlock* bb) {
      if( bb == start_bb )
        in_app = true;

      if( in_app )
        total += get_weight(bb);

      if( bb == end_bb )
        in_app = false;

      for( auto* succ : successors(bb) )
        dac->mark_dep_ready(succ);
    });
  return total;
}

/**
 * Find the application start and end in the provided function.  This uses
 * special metadata nodes that are inserted by the pipeline pass.
 */
static void
find_app_code(Function* f, Instruction*& app_entry, Instruction*& app_exit) {
  auto& c = f->getParent()->getContext();
  auto* md_entry = MDNode::get(c, MDString::get(c, "app_entry"));
  auto* md_exit  = MDNode::get(c, MDString::get(c, "app_exit"));

  app_entry = app_exit = nullptr;
  for( auto& inst : instructions(f) ) {
    /* Find application code start and end */
    auto* md = inst.getMetadata("nanotube.pipeline");
    if( md == md_entry )
      app_entry = &inst;
    if( md == md_exit )
      app_exit = &inst;
  }

  LLVM_DEBUG(dbgs() << "Function " << f->getName()
                    << "\n  app start: " << nullsafe(app_entry)
                    << "\n  app end: " << nullsafe(app_exit) << '\n');
}

static void compute_stage_stats(Function* f, DominatorTree* dt,
                                PostDominatorTree* pdt, AliasAnalysis* aa) {
  LLVM_DEBUG(dbgs() << "Computing stats for " << f->getName() << '\n');

  Instruction *app_entry, *app_exit;
  find_app_code(f, app_entry, app_exit);

  unsigned total     = get_weight(f, app_entry, app_exit);
  std::vector<Instruction*> path;
  auto data_flow_len = get_data_flow_critical_path(f, app_entry, app_exit,
                                                   &path, dt, pdt, aa);
  LLVM_DEBUG(
    dbgs() << "Critical Path\n";
    for( auto it = path.begin(); it != path.end(); ++it )
      dbgs() << **it << '\n';
    dbgs() << "Length: " << data_flow_len << '\n');
  auto cfg_len       = get_cfg_critical_path(f, app_entry, app_exit);
  auto cfg_long_len  = get_cfg_longest_path(f, app_entry, app_exit);

  LLVM_DEBUG(
    dbgs() << "Function " << f->getName() << " Cost: " << total
           << " Data-flow Critial Path: " << data_flow_len
           << " CFG-based Critial Path: " << cfg_len
           << " CFG-based Longest Path: " << cfg_long_len
           << " ILP: " << formatv("{0:f}", (float)total/cfg_len)
           << '\n');
  dbgs() << f->getName() << ", " << total << ", " << data_flow_len << ", "
         << cfg_len << ", " << cfg_long_len << '\n';

}

/**
 * Record that the control flow changed at the provided instruction and
 * which value (condition / switch value) affected the control flow
 * decision.  This is recorded by adding a call to a dummy function called
 * control_flow_determined_by.<type> that lives next to the original
 * terminator.
 *
 * @param inst   Original control-flow changing instruction.
 * @param cond   Condition that drives the control flow change.
 */
static void
record_condition(Instruction* inst, Value* cond) {
  auto* m = inst->getModule();
  auto& c = m->getContext();
  auto* fty = FunctionType::get(Type::getVoidTy(c), {cond->getType()},
                                false);
  auto name = Twine("control_flow_determined_by.i") +
              Twine(cast<IntegerType>(cond->getType())->getBitWidth());

  auto* func = m->getOrInsertFunction(name.str(), fty);
  CallInst::Create(fty, func, {cond}, "", inst);
}

/**
 * Create a single path version of the provided function; extracting the
 * critical path based on a provided per-BB cost function.  All other basic
 * blocks will be removed, and control flow changing instructions will be
 * turned into function calls recording the condition used and location of
 * the branch.  The funtion will have one large basic block as a result.
 *
 * @param f     Function to manipulate and turn into one big basic block.
 * @param cost  Function that provides a cost for each basic block; which
 *              is then used to construct the critical path.
 */
static void
flatten_trace(Function* f, bb_cost_func_t* cost) {
  std::vector<BasicBlock*> path;
  bb_schedule(f, nullptr, nullptr, cost, &path);

  LLVM_DEBUG(
    dbgs() << "Critical Path through " << f->getName() << '\n';
    for( auto* bb : path )
      dbgs() << bb->getName() << " ";
    dbgs() << '\n');

  /* Rewrite the basic block terminators to only point to the trace */
  for( auto it = path.begin(); it != path.end(); ) {
    auto* bb = *it;
    it++;
    /* Do not rewrite the last entry */
    if( it == path.end() )
      break;
    auto* bb_nxt = *it;

    /* Update the phi nodes of those BBs that are off the path */
    for( auto* succ : successors(bb) ) {
      if( succ == bb_nxt )
        continue;
      succ->removePredecessor(bb);
    }

    /* Update the phi nodes of the next BB in the path */
    for( auto* pred : predecessors(bb_nxt) ) {
      if( pred == bb )
        continue;
      bb_nxt->removePredecessor(pred);
    }

    /* Record the value used for driving the condition / switch */
    auto* t = bb->getTerminator();
    auto* br = dyn_cast<BranchInst>(t);
    if( br != nullptr ) {
      if( br->isConditional() )
        record_condition(br, br->getCondition());
    } else {
      auto* sw = dyn_cast<SwitchInst>(t);
      if( sw == nullptr ) {
        errs() << "ERROR: Unrecognised terminator " << *t
               << " in basic block " << bb->getName() << " Aborting!\n";
        abort();
      }
      record_condition(sw, sw->getCondition());
    }
    /* Update the actual terminator */
    auto* br_new = BranchInst::Create(bb_nxt);
    ReplaceInstWithInst(t, br_new);
  }

  /* Remove all basic blocks not on the path */
  std::vector<BasicBlock*> to_delete;
  std::unordered_set<BasicBlock*> path_set(path.begin(), path.end());
  for( auto& bb : *f ) {
    if( path_set.count(&bb) != 0 )
      continue;
    to_delete.emplace_back(&bb);
    bb.dropAllReferences();
  }
  for( auto* bb : to_delete )
    bb->eraseFromParent();

  /* Merge remaining basic blocks */
  for( auto it = path.rbegin(); it != path.rend(); ++it )
    MergeBlockIntoPredecessor(*it);
}
/**
 * Create single-trace functions from the provided original application
 * function.  This scheduled basic blocks in the function and then extracts
 * the critical path and synthesises a new function out of it.
 */
static void
synthesise_trace(Function* f) {
  Instruction *app_entry, *app_exit;
  find_app_code(f, app_entry, app_exit);

  /* This works as follows: schedule the basic blocks and get a trace of
   * basic blocks for the function, create a clone, and manipulate it such
   * that:
   * - all basic blocks not on the path are removed
   * - all conditional branches / switch statements are removed
   * - the remaining basic blocks are concatenated forming one big block
   */
  LLVM_DEBUG(dbgs() << "CFG-based Critical Path:\n");
  ValueToValueMapTy vmap;
  auto f_crit = CloneFunction(f, vmap);
  f_crit->setName(f->getName() + ".trace_long");
  flatten_trace(f_crit, get_data_flow_critical_path);

  LLVM_DEBUG(dbgs() << "CFG-based Fattest Path:\n");
  vmap.clear();
  auto f_fat = CloneFunction(f, vmap);
  f_fat->setName(f->getName() + ".trace_fat");
  flatten_trace(f_fat, get_weight);
}

bool code_metrics::runOnModule(Module& m) {
  setup_func setup(m, nullptr, false);
  dbgs() << "Function Name, Total Cost, DF CP, CFG CP, CFG LL\n";
  for( auto& thread : setup.threads() ) {
    get_all_analysis_results(thread.args().func);
    compute_stage_stats(thread.args().func, dt, pdt, aa);
  }
  for( auto& kernel : setup.kernels() ) {
    get_all_analysis_results(kernel.args().kernel);
    compute_stage_stats(kernel.args().kernel, dt, pdt, aa);
  }

  if( !create_trace)
    return false;

  for( auto& thread : setup.threads() )
    synthesise_trace(thread.args().func);
  for( auto& kernel : setup.kernels() )
    synthesise_trace(kernel.args().kernel);
  return true;
}

char code_metrics::ID = 0;
static RegisterPass<code_metrics>
  X("code-metrics", "Compute statistics for Nanotube packet kernels.",
    true,
    true
  );

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
