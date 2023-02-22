/*******************************************************/
/*! \file Pipeline.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert a properly structured Nanotube fragement into pipeline stages.
**   \date 2021-01-11
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
 * The pipeline pass converts a structured and pre-processed packet kernel
 * function into separate pipeline stages.  This pass is also known as the
 * wordify pass, because it also converts the unit of data from an entire
 * packet to a packet word.
 *
 * The pipeline stages are new functions that will be chained together by
 * nanotube channels.  The split points are packet accesses which get
 * translated from the simple XXX API (with simple functions such as XXX
 * and XXX) to a lower-level interface that processes packets on a per-word
 * basis.  For that, additional control flow and variables have to be
 * introduced.
 *
 * ALGORITHM STEPS
 *
 * The algorithm performs the following steps:
 * - preprocess the packet kernel
 *   - parse all Nanotube calls and find out maximum buffer sizes
 *   - split calls that will need to occupy two pipeline stages: packet
 *     resize and map_op calls
 *   - check whether applications use the return code of Nanotube API
 *     calls; there are subtle API differences between the request- and the
 *     tap-level of Nanotube API calls around signalling errors
 *   - remove calls to stacksave & stackrestore
 *   - convert phi-of-pointer instructions (currently just into new
 *     allocations + memcpy the right data if needed)
 *   - recompute the live state
 *
 * Do the actual pipelining:
 * - identify pipeline split points
 * - create new functions per identified pipeline stage
 * - create the stages preamble:
 *   - if needed: read live-in state from the state channel and unmarshal
 *                it (once per packet)
 *   - if needed: read map responses from the map channel (once per packet)
 *   - read the packet word, check for eop and reset per-packet trackers
 * - copy over the application between the pipeline split points and
 *   replace live-in values with the unmarshalled versions from the live
 *   state; this may cause splitting of basic blocks at the beginning and
 *   end of the graph. Because split points must align with convergence
 *   points, no control flow edges will sneak out of the CFG web of the
 *   single pipeline stage
 * - convert the calls to Nanotube API from the higher-level to the lower
 *   level API
 * - if needed: write out live state to the live app state channel
 *   connecting to the next pipeline stage
 * - send the output packet word to the next stage
 *
 * Update the nanotube_setup_function:
 * - create contexts & threads per pipeline stage
 * - create and connect channels
 * - create and connect map stages
 *
 * PIPELINE SPLITS
 *
 * Currently, the computation is split into pipeline stages at every
 * Nanotube API call (packet / map operation), even though several external
 * operations may be executed per stage because they don't conflict, for
 * example:
 * - independent packet reads (optreq will aim to merge them into a single
 *   access if they are in close proximity)
 * - packet read followed by a write to a position at or after the read
 *   location
 * - map operations live between stages (stage N sends a request and stage
 *   N+1 receives the response) and can be merged with all packet
 *   operations in those stages
 *
 * Another question is the placement of compute relative to the API access;
 * is the Nanotube call the first or last part of the pipeline, or can it
 * be either / both?  For now, the code is biased towards executing the
 * Nanotube API call as the first thing in the pipeline stage.  It is
 * conceivable that more careful scheduling of logic could equalise
 * pipeline sizes; note that for that the control flow needs to be
 * converged at the cut point if the logic is being spread across multiple
 * stages.
 *
 * MAPS
 *
 * Maps are implemented with a request / response interfaces via channels
 * and thus the actual map access happens between two pipeline stages as
 * follows:
 *
 * +---------+ -------- packet words -------> +-----------+
 * |         |            +-----+             |           |
 * | stage N | -map req-> | map | -map resp-> | stage N+1 |
 * |         |            +-----+             |           |
 * +---------+ --------- app state ---------> +-----------+
 *
 * In this example, stage N is the map request stage, and stage N+1 the map
 * response stage, and sending the map request will be the last thing
 * (logically) that happens in stage N, and likewise, receiving the
 * response is the first thing in stage N+1.
 *
 * We therefore need to make both stage N and N+1 "map aware".  The
 * simplest would be to perform both the map request and the map response
 * in separate pipeline stages in isolation (without any other packet / map
 * accesses in these stages).  That would, however, turn every map
 * operation into two (three if also counting the map implementation stage)
 * pipeline stages, where especially the request stage will not contain a
 * lot of logic because the request is the last thing that happens in that
 * stage, but we currently bias so that application logic follows the
 * Nanotube API call which lives at the beginning of the stage.
 *
 * I think it therefore makes sense to add the map request part as part of
 * the previous existing pipeline stage.  For the response, when splitting
 * the pipeline at the original nanotube_map_op and then say at a
 * subsequent nanotube_packet_read, stage N+1 will contain just the map
 * response receive and application logic that computes with it.  It should
 * be possible to also merge the next pipeline stage in, but that is left
 * for future work.
 *
 * EXAMPLES
 *
 * A good example of a manual translation can be found in
 *   - testing/kernel_tests/wordify.manual.cc .. manual translation
 *   - testing/kernel_tests/wordify.cc .. high-level version for source code
 * And an example that uses maps here:
 *   - testing/kernel_tests/pipeline_maps*.cc
 *
 *
 * NOTES / LIMITATIONS / TODO ITEMS
 *
 * - allow multiple packet operations in the same pipeline stage (e.g.,
 *   read and write to a later part of the packet; resize operations)
 *
 */
//#define DEBUG_DOMINATOR_TREE
#include "Pipeline.hpp"
#include "bus_type.hpp"
#include "nanotube_map_taps.h"
#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_bus.h"

#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/OrderedInstructions.h"
#include "llvm/Analysis/ValueTracking.h"

#define DEBUG_TYPE "pipeline"
using namespace llvm;
using namespace nanotube;

void Pipeline::get_all_analysis_results(Function& f) {
  dt   = &getAnalysis<DominatorTreeWrapperPass>(f).getDomTree();
  pdt  = &getAnalysis<PostDominatorTreeWrapperPass>(f).getPostDomTree();
  mssa = &getAnalysis<MemorySSAWrapperPass>(f).getMSSA();
  aa   = &getAnalysis<AAResultsWrapperPass>(f).getAAResults();
  tli  = &getAnalysis<TargetLibraryInfoWrapperPass>(f).getTLI();
  livi = &getAnalysis<liveness_analysis>(f).get_liveness_info();
}
void Pipeline::getAnalysisUsage(AnalysisUsage &info) const {
  info.addRequired<DominatorTreeWrapperPass>();
  info.addRequired<PostDominatorTreeWrapperPass>();
  info.addRequired<MemorySSAWrapperPass>();
  info.addRequired<AAResultsWrapperPass>();
  info.addRequired<TargetLibraryInfoWrapperPass>();
  info.addRequired<liveness_analysis>();
  info.addRequired<ExternalAAWrapperPass>();
  info.addRequired<NanotubeAAPass>();
  //info.addRequired<NanotubeAAWrapper>();
  // TODO: Add things we preserve
}

static llvm::cl::opt<bool> pipeline_stats("pipeline-stats",
    llvm::cl::desc("Print statistics for the pipeline pass"),
    llvm::cl::Hidden, llvm::cl::init(false));

static
StructType* get_live_state_type(LLVMContext& c,
                                ArrayRef<Value*> live_values,
                                ArrayRef<MemoryLocation> live_mem);
static const Function* get_function(const Value* v);

/**
 * Checks whether an instruction in the packet kernel is a split point
 *
 * Split points are instructions at which the application logic needs to be
 * split into two pipeline stages).  This implicitly assumes that the code
 * has been properly converged; i.e., there are no control flow edges going
 * past this instruction!
 */
static
bool is_split_point(Instruction* inst) {
  auto i = get_intrinsic(inst);
  return (i == Intrinsics::packet_read) ||
         (i == Intrinsics::packet_write) ||
         (i == Intrinsics::packet_write_masked) ||
         (i == Intrinsics::packet_resize_ingress) ||
         (i == Intrinsics::packet_resize_egress) ||
         (i == Intrinsics::packet_bounded_length) ||
         (i == Intrinsics::map_op_receive) ||
         (i == Intrinsics::packet_drop) ||
         isa<ReturnInst>(inst);
}

/**
 * Trace through a value / instruction to determine if it is a complex
 * pointer.  Complex pointers are those that are non-trivial phi nodes.
 * If the value is not a complex pointer, return nullptr.
 * peel_one_phi .. trace through a single outermost phi
 */
Value* get_complex_pointer(Value* inst, bool peel_one_phi) {
  assert(inst->getType()->isPointerTy());
  if( isa<AllocaInst>(inst) )
    return nullptr;
  if( isa<UndefValue>(inst) )
    return nullptr;
  if( isa<Argument>(inst) )
    return nullptr;

  auto* gep = dyn_cast<GetElementPtrInst>(inst);
  if( gep != nullptr ) {
    return gep->hasAllConstantIndices() ?
             get_complex_pointer(gep->getPointerOperand(), false) : inst;
  }

  auto* sel = dyn_cast<SelectInst>(inst);
  if( sel != nullptr )
    return inst;

  auto* phi = dyn_cast<PHINode>(inst);
  if( phi != nullptr ) {
    if( phi->hasConstantOrUndefValue() ) {
      for( auto& v : phi->incoming_values() ) {
        if( !isa<UndefValue>(v) )
          return get_complex_pointer(v, false);
      }
    } else {
      if( peel_one_phi ) {
        for( auto& v : phi->incoming_values() ) {
          Value* complex_val = get_complex_pointer(v, false);
          if( complex_val != nullptr )
            return complex_val;
        }
        return nullptr;
      } else {
        return inst;
      }
    }
  }

  auto* bc = dyn_cast<BitCastInst>(inst);
  if( bc != nullptr )
    return get_complex_pointer(bc->getOperand(0) , false);

  auto* call = dyn_cast<CallInst>(inst);
  if( call != nullptr ) {
    auto intrin = get_intrinsic(call);
    if( intrin == Intrinsics::llvm_stacksave ||
        intrin == Intrinsics::llvm_stackrestore ) {
      return nullptr;
    } else {
      return inst;
    }
  }

  errs() << "Unknown pointer inst: " << *inst << '\n';
  return inst;
}

bool is_complex_pointer(Value* val, bool peel_one_phi) {
  return get_complex_pointer(val, peel_one_phi) != nullptr;
}

void Pipeline::print_pipeline_stats(Function& f) {
  //XXX: Better stats here and parse the actual stages!
  const auto& dl = f.getParent()->getDataLayout();

  typedef dep_aware_converter<BasicBlock> dac_t;
  dac_t dac;
  for( auto& bb : f )
    dac.insert(&bb, pred_size(&bb));

  dac.execute([&](dac_t* dac, BasicBlock* bb) {
    LLVM_DEBUG(dbgs() << '\n' << bb->getName() << ":\n");
    for ( auto& inst : *bb ) {
      if( !is_split_point(&inst) )
        continue;

      LLVM_DEBUG(
        dbgs() << "Split\n  Inst:" << inst << '\n';
        dbgs() << "  Live In: ";
      );

      unsigned in_size = 0;
      std::vector<Value*> live_in_val, live_out_val;
      std::vector<MemoryLocation> live_in_mem, live_out_mem;
      livi->get_live_in(&inst, &live_in_val, &live_in_mem);
      livi->get_live_out(&inst, &live_out_val, &live_out_mem);

      for( auto* live_in : live_in_val ) {
        auto* ty = live_in->getType();
        bool isp = ty->isPointerTy();

        LLVM_DEBUG(
          if( isp )
            dbgs() << "(";
          live_in->printAsOperand(dbgs());
          if( isp )
            dbgs() << ")";
          dbgs() << " ";
        );
        /* Don't accumulate pointers, we will convert those into values
         * eventually and will accumulate their size in the memory liveness
         * pass. */
        if( !isp )
          in_size += dl.getTypeSizeInBits(ty);

        /* Review live pointers and show those that are complex, i.e.,
         * are a PHI node / Select or a non-constant GEP. */
        if( isp ) {
          bool is_arg = cast<CallInst>(inst).hasArgument(live_in);
          if( is_complex_pointer(live_in, is_arg) )
            errs() << "Complex live in PTR: " << live_in->getName()
                   << " component: " <<*get_complex_pointer(live_in, is_arg)
                   << '\n';
        }
      }
      LLVM_DEBUG( dbgs() <<"\n  Live Out: ");

      unsigned out_size = 0;
      for( auto* live_out : live_out_val ) {
        auto* ty = live_out->getType();
        bool isp = ty->isPointerTy();

        LLVM_DEBUG(
          if( isp )
            dbgs() << "(";
          live_out->printAsOperand(dbgs());
          if( isp )
            dbgs() << ")";
          dbgs() << " ";
        );

        /* Don't accumulate pointers, we will convert those into values
         * eventually and will accumulate their size in the memory liveness
         * pass. */
        if( !isp )
          out_size += dl.getTypeSizeInBits(ty);

        /* Review live pointers and show those that are complex, i.e.,
         * are a PHI node / Select or a non-constant GEP. */
        if( isp ) {
          if( is_complex_pointer(live_out, false) )
            errs() << "Complex live out PTR: " << live_out->getName()
                   << " component: " <<*get_complex_pointer(live_out, false)
                   << '\n';
        }
      }
      LLVM_DEBUG(dbgs() <<'\n');

      unsigned in_mem_size = 0;
      for( auto& mloc : live_in_mem ) {
        if( mloc.Size.hasValue() )
          in_mem_size += mloc.Size.getValue();
      }

      unsigned out_mem_size = 0;
      for( auto& mloc : live_out_mem ) {
        if( mloc.Size.hasValue() )
          out_mem_size += mloc.Size.getValue();
      }

      if( pipeline_stats) {
        errs() << "Split\n  Inst:" << inst << '\n';
        errs() << "  Size (bits): " << in_size << " / " << out_size
               << " / " << in_mem_size * 8 << " / " << out_mem_size * 8
               << '\n';
      }
    }
    for( auto* succ : successors(bb) )
      dac->mark_dep_ready(succ);
  });
}

/********** Creating Pipeline Stages **********/
FunctionType* stage_function_t::get_function_type(Module* m, LLVMContext& c) {
#if 0
  Type* params[] = {get_nt_context_type(*m)->getPointerTo(),  /* context */
                    get_nt_packet_type(*m)->getPointerTo(),   /* packet */
                    Type::getInt8Ty(c)->getPointerTo(),       /* in_state */
                    Type::getInt8Ty(c)->getPointerTo()};      /* out_state */
  return FunctionType::get(Type::getVoidTy(c), params, false);
#endif
  return get_nt_thread_func_ty(*m);
}
Value* stage_function_t::get_context_arg()   { return func->arg_begin() + 0; }
Value* stage_function_t::get_packet_arg()    { return func->arg_begin() + 1; }
Value* stage_function_t::get_user_arg()      { return func->arg_begin() + 1; }

stage_function_t::stage_function_t() : func(nullptr),
                                       live_in_ty(nullptr),
                                       live_out_ty(nullptr),
                                       packet_word(nullptr),
                                       unmarshal_bb(nullptr),
                                       app_logic_entry(nullptr),
                                       app_logic_exit(nullptr),
                                       thread_wait_exit(nullptr),
                                       stage_exit(nullptr),
                                       nt_call(nullptr),
                                       map_op_req(nullptr),
                                       map_req_buf_size(nullptr),
                                       map_op_rcv(nullptr),
                                       map_rcv_buf_size(nullptr),
                                       drop_val(nullptr),
                                       nt_id(Intrinsics::none) {
}

stage_function_t* stage_function_t::create(const Twine& base_name,
                                           const Twine& stage_name,
                                           unsigned idx,
                                           Module* m, LLVMContext& c) {
  auto *stf = new stage_function_t();
  stf->func = Function::Create(get_function_type(m, c),
                               Function::ExternalLinkage,
                               base_name + "_" + stage_name,
                               m);
  stf->stage_name = stage_name.str();
  stf->idx = idx;
  return stf;
}

static
BasicBlock* clone_basic_block(BasicBlock* src, Instruction* start,
                              Instruction* end, ValueToValueMapTy& vmap,
                              Function* f, const Twine& suffix = "");
/**
 * Tiny helper to remove elements of packet types from the list.
 * NOTE: This can reorder the list of values
 */
static unsigned
remove_type(Module* m, std::vector<Value*>* vals, const Type* to_remove) {
  unsigned count = 0;
  for( auto it = vals->begin(); it != vals->end();) {
    /* Skip over all non-matching items */
    if( (*it)->getType() != to_remove ) {
      it++;
      continue;
    }

    /* Replace matching items with the tail */
    count++;
    *it = vals->back();
    vals->pop_back();
  }
  return count;
}
static unsigned
remove_type(Module* m, std::vector<MemoryLocation>* mems, const Type* to_remove) {
  unsigned count = 0;
  for( auto it = mems->begin(); it != mems->end();) {
    /* Skip over all non-matching items */
    if( it->Ptr->getType() != to_remove ) {
      it++;
      continue;
    }

    /* Replace matching items with the tail */
    count++;
    *it = mems->back();
    mems->pop_back();
  }
  return count;
}

static bool skip_inst(Instruction* inst) {
  auto* intrin = dyn_cast<IntrinsicInst>(inst);
  return (intrin != nullptr &&
          intrin->getIntrinsicID() == Intrinsic::lifetime_start) ||
         (isa<DbgInfoIntrinsic>(inst));
}
/**
 * Scan instruction stream backwards in the same BB, skipping DebugInfo and
 * lifetime intrinsics.
 */
static Instruction*
backward_skip_dbg_lifetime_start(Instruction* start) {
  while( true ) {
    auto* prev = start->getPrevNode();
    if( prev == nullptr )
      return start;
    if( skip_inst(prev) )
      start = prev;
    else
      return start;
  }
}

/**
 * Ensure that the control flow between start and end is fully converged,
 * i.e., there are no stray control-flow edges into / out of the region
 * between start and end.
 */
bool Pipeline::is_control_flow_converged(Instruction* start, Instruction* end) {
  auto* start_bb = start->getParent();
  auto* end_bb   = end->getParent();
  if( start_bb == end_bb )
        return true;

  /* Make sure that all paths from start go through end, and vice versa. */
  bool fwd_dom = dt->dominates(start, end);
  bool rev_dom = (start_bb == end_bb) || pdt->dominates(end_bb, start_bb);
  LLVM_DEBUG(
    dbgs() << "Checking control flow convergence between:"
           << "\n  Start: " << *start << " in " << start_bb->getName()
           << "\n  End: " << *end << " in " << end_bb->getName()
           << "\n  Fwd dom: " << fwd_dom
           << "\n  Rev dom: " << rev_dom << '\n';
#ifdef DEBUG_DOMINATOR_TREE
    dbgs() << "Dominator Tree:\n";
    dt->print(dbgs());
    dbgs() << "\nPost-Dominator Tree:\n";
    pdt->print(dbgs());
#endif
  );
  return (fwd_dom && rev_dom);
}

/**
 * Extract the relevant live in / out data of the code fragment between
 * start and end.
 */
void Pipeline::extract_live_data(Instruction* inst,
                                 std::vector<Value*>* liv,
                                 std::vector<MemoryLocation>* lim,
                                 bool is_out) {
//#define EXTRA_LIVE_DATA_DEBUG
  auto* m = inst->getModule();
  const char* str;
  str = "Live";

  std::vector<Value*>         live_val_v;
  if( !is_out )
    livi->get_live_in(inst,  &live_val_v, lim);
  else
    livi->get_live_out(inst, &live_val_v, lim);

#ifdef EXTRA_LIVE_DATA_DEBUG
  LLVM_DEBUG(
    dbgs() << str << ":\n";
    for( auto* v : live_val_v )
      dbgs() << "  " << *v << '\n';
    for( auto& m : *lim )
      dbgs() << "  " << m << '\n';
  );
#endif

  /* Remove the packet from live data, because it gets send along always */
  unsigned count = 0;
  count = remove_type(m, &live_val_v, get_nt_packet_type(*m)->getPointerTo());
  assert(count <= 1);
  /* Also remove the resize cword from live state because it will get a
   * separate channel to be sent through! */
  count = remove_type(m, &live_val_v,
                     get_nt_tap_packet_resize_cword_ty(*m)->getPointerTo());
  count = remove_type(m, lim,
                     get_nt_tap_packet_resize_cword_ty(*m)->getPointerTo());
  assert(count <= 1);

#ifdef EXTRA_LIVE_DATA_DEBUG
  LLVM_DEBUG(
    dbgs() << str << " (no packet, no cword):\n";
    for( auto* v : live_val_v )
      dbgs() << "  " << *v << '\n';
    for( auto& m : *lim )
      dbgs() << "  " << m << '\n';
  );
#endif

  /* Check that the live memory locations are not complex. */
  for( auto& m : *lim ) {
    auto* p = get_complex_pointer(const_cast<Value*>(m.Ptr), false);
    if( p != nullptr ) {
      errs() << "Inst: " << *inst << " has a complex live pointer " << *p
             << " in memory location: " << m << '\n';
    }
    assert( p == nullptr );
  }

  /* Put into sets for faster access. */
  std::unordered_set<Value*> live_val(live_val_v.begin(),
                                      live_val_v.end());
  std::unordered_map<const Value*, MemoryLocation> live_mem;
  for( auto& m : *lim)
    live_mem[m.Ptr] = m;

  /**
   * Remove all live pointers, because:
   * - pointers that point to dead memory (no corresponding live_in_mem
   *   location) are just there because they were alloca-ed early =>
   *   removing them as they do not contain any interesting data
   * - pointers that point to live memory (has entry in live_in_mem) have
   *   no benefit of having the pointer value live, as we will copy the
   *   data over at the split point into a new memory location.
   *
   * Also check that no depth-2+ pointer (pointers to pointers) are live --
   * these will break the handling of memory locations and patching up
   * single-depth pointers.
   */
  for( auto* v : live_val ) {
    if( !v->getType()->isPointerTy() ) {
      liv->emplace_back(v);
      continue;
    }
    auto* ty = cast<PointerType>(v->getType());
    if( ty->getPointerElementType()->isPointerTy() ) {
      errs() << "ERROR: Found a double-indirect live pointer value which "
             << "is not supported by the pointer handling in the pipeline "
             << "pass.  Please check your code for errors and change it. "
             << "Offending value:" << *v << " used by:\n";
      for( auto* u : v->users() )
        errs() << "  " << *u << '\n';
      errs() << "Aborting!\n";
      exit(1);
    }
    // NOTE: For the opaque pointers transition, one would have to go
    // through all the users of the pointer value and check that they are
    // not loading / storing other pointers to this pointer!
  }

#ifdef EXTRA_LIVE_DATA_DEBUG
  LLVM_DEBUG(dbgs() << "Checking live pointer memory liveness: \n");
  for( auto* v : live_val ) {
    if( !v->getType()->isPointerTy() )
      continue;
    bool ptr2live = (live_mem.find(v) != live_mem.end() );
    LLVM_DEBUG(dbgs() << "  " << *v << ": " << (ptr2live ? "live" : "dead")
                      << " mem\n");
  }
#endif

  LLVM_DEBUG(
    dbgs() << str << " (no pointers):\n";
    for( auto* v : *liv )
      dbgs() << "  " << *v << '\n';
    dbgs() << str << " (memory locations):\n";
    for( auto& m : live_mem)
      dbgs() << "  " << *m.first << " Loc: " << m.second << '\n';
  );
}

static void
remap_live_values(std::vector<Value*>* live_vals,
                  std::vector<MemoryLocation>* live_mls,
                  ValueToValueMapTy& vmap) {
  /* Map the live-out values through the remapping table */
  for( auto& val : *live_vals ) {
    auto it = vmap.find(val);
    if( it != vmap.end() )
      val = it->second;
  }
  for( auto& ml : *live_mls) {
    auto it = vmap.find(ml.Ptr);
    if( it != vmap.end() )
      ml.Ptr = it->second;
  }
}

void
Pipeline::collect_liveness_data(stage_function_t* stage,
                                Type* in_state_ty, LLVMContext& c) {
  LLVM_DEBUG(dbgs() << "Stage Live In:\n");
  extract_live_data(stage->start, &stage->live_in_val, &stage->live_in_mem,
                    /* is_out = */ false);
  /* The stage is [start, end) so stage_live_out = live_in(end) */
  LLVM_DEBUG(dbgs() << "Stage Live Out:\n");
  extract_live_data(stage->end,  &stage->live_out_val, &stage->live_out_mem,
                    /* is_out = */ false);

  stage->live_in_ty  = get_live_state_type(c, stage->live_in_val,
                                           stage->live_in_mem);
  stage->live_out_ty = get_live_state_type(c, stage->live_out_val,
                                           stage->live_out_mem);

  if( stage->live_in_ty != in_state_ty ) {
    errs() << "Computed live-in type: " << nullsafe(stage->live_in_ty)
           << " does not match expected (previous live-out): "
           << nullsafe(in_state_ty) << '\n';
    assert(stage->live_in_ty == in_state_ty);
  }
}

static stage_function_t*
new_stage(Pipeline* pip,
          std::vector<stage_function_t*>* stages,
          Instruction* start, Instruction** end,
          std::vector<BasicBlock*>&& content,
          stage_function_t* prev_stage, unsigned& stage_id) {
  Type* prev_out_ty = (prev_stage != nullptr) ?
                        prev_stage->get_live_out_ty() :
                        nullptr;
  //static unsigned count = 0;
  //WriteGraph((const Function*)&f, f.getName() + std::to_string(count++));
  auto* stage = pip->get_stage(start, end, stage_id++, prev_out_ty);
  assert(prev_out_ty == stage->get_live_in_ty());
  stage->bbs = content;
  stages->emplace_back(stage);

  LLVM_DEBUG(
    auto* packet = stage->get_packet_arg();
    auto* ctxt   = stage->get_context_arg();

    dbgs() << "New pipeline stage: " << *stage->func
           << "Start: " << *start << "\nEnd: " << **end
           << "\nCtxt: " << *ctxt
           << "\nPacket: " << *packet;

    if( stage->get_live_in_ty() != nullptr )
      dbgs() << "\nIn state type: " << *stage->get_live_in_ty();
    else
      dbgs() << "\nIn state type: <empty>";

    if( stage->get_live_out_ty() != nullptr )
      dbgs() << "\nOut state type: " << *stage->get_live_out_ty();
    else
      dbgs() << "\nOut state type: <empty>";
    dbgs() << '\n';
  );
  return stage;
}

void
Pipeline::determine_stages(std::vector<stage_function_t*>* stages,
                           Function& f) {
  auto* start_bb   = &f.getEntryBlock();

  typedef dep_aware_converter<BasicBlock> dac_t;
  dac_t dac;
  dac.insert_ready(start_bb);

  unsigned stage_id            = 0;
  stage_function_t* prev_stage = nullptr;

  Instruction* start = &*start_bb->begin();
  Instruction* end   = nullptr;
  std::vector<BasicBlock*> content;

  content.emplace_back(start_bb);

  /* Go through the basic blocks in DAG-friendly order and perform split
   * operations.  We rely on the split points being converge points, so it
   * is very clear, which basic blocks have to be copied over.
   */
  dac.execute([&](dac_t* dac, BasicBlock* bb) {
    //LLVM_DEBUG(dbgs() << '\n' << bb->getName() << ":\n");
    /* All reachable basic blocks will need to get copied over */
    content.emplace_back(bb);

    /* Find (if any) the split point in this basic block */
    for ( auto& inst : *bb ) {
      if( !is_split_point(&inst) )
        continue;
      /* Don't create empty stages */
      if( &inst == start )
        continue;

      /* Found a split point -> split off what we have so far */
      end = &inst;
      auto* stage = new_stage(this, stages, start, &end,
                              std::move(content), prev_stage, stage_id);

      /* Remember end of this split as start for next split */
      start_bb   = bb;
      prev_stage = stage;
      start      = end;

      /* Reset set of basic blocks that need to be moved around */
      content.clear();
      content.emplace_back(start_bb);
    }

    /* Ready our successor basic blocks */
    for( auto* succ : successors(bb) )
      if( dac->contains(succ) )
        dac->mark_dep_ready(succ);
      else
        dac->insert(succ, pred_size(succ) - 1);
  });
}

stage_function_t*
Pipeline::get_stage(Instruction* start, Instruction** end, unsigned id, Type* in_state_ty) {
  /* Back off from the end so that debug info etc stays with the split
   * point. */
  *end   = backward_skip_dbg_lifetime_start(*end);

  auto* start_bb = start->getParent();
  auto* end_bb   = (*end)->getParent();
  auto* func     = start_bb->getParent();
  auto* m        = func->getParent();
  auto& c        = m->getContext();

  LLVM_DEBUG(
    dbgs() << "Split\n"
           << "  From BB: " << start_bb->getName() <<'\n'
           << "  Inst:" << *start << '\n'
           << "  To BB: " << end_bb->getName() << '\n'
           << "  Inst:" << **end  << '\n';
  );

  if( !is_control_flow_converged(start, *end) ) {
    errs() << "Split points " << *start << " in " << start_bb->getName()
           << " => " << **end << " in " << end_bb->getName()
           << " don't dominate each other; did you converge"
           << " (-converge_mapa)?\n";
    errs() << "ERROR: Control flow is not converged. Aborting!\n";
    exit(1);
  }

  /**
   * Create a new function for the pipeline stage.  The new pipeline stage
   * starts by unmarshalling live data and then copies of the basic blocks
   * reserved for that stage.
   */
  auto idtxt = std::to_string(id);
  auto* stage = stage_function_t::create(func->getName(), "stage_" +
                                         idtxt, id, m, c);
  stage->start = start;
  stage->end   = *end;

  collect_liveness_data(stage, in_state_ty, c);
  return stage;
}

static BasicBlock*
generate_prologue(stage_function_t* stage, ValueToValueMapTy* vmap_) {
  auto& vmap = *vmap_;
  auto* start_bb = stage->start->getParent();
  auto* func     = start_bb->getParent();
  auto* m        = func->getParent();
  auto& c        = m->getContext();

  /* Function Prologue: reading packet data and application state from the
   * right channels */
  auto* bb  = BasicBlock::Create(c, "entry", stage->func);
  auto* ret = ReturnInst::Create(c, bb);
  IRBuilder<> ir(ret);

  /* Prologue: read application live state for stages that have it
   * state is read into a static so it lives across multiple packet words
   */
  if( stage->has_live_in() )
    bb = stage->read_app_state(stage->get_static_app_state(), bb);

  /* Capture map response if this stage needs one */
  if( stage->is_map_receive() ) {
    map_op_receive_args moa(stage->map_op_rcv);
    /* This is the quickest way: read the size off from the map_op call! */
    if( !isa<ConstantInt>(moa.data_length) ) {
      errs() << "FIXME: map_op data_length is not a constant but instead: "
             << *moa.data_length << " Handle this in " << __FILE__ << ":"
             << __LINE__ << "\nAborting!\n";
      exit(1);
    }
    auto len = cast<ConstantInt>(moa.data_length)->getZExtValue();
    stage->map_resp_data_ty = ArrayType::get(Type::getInt8Ty(c), len);
    auto resp_data = (len == 0
                      ? (Value*)Constant::getNullValue(Type::getInt8PtrTy(c))
                      : (Value*)stage->get_static_map_resp_data());
    bb = stage->read_map_response(resp_data,
                                  stage->get_static_map_result(), bb);
  }

  /* Prologue: read the packet word; it is processed every invocation, so
   * can be local and non-static */
  ir.SetInsertPoint(ret);
  if( !stage->needs_static_checked_packet_word() ) {
    stage->packet_word = ir.CreateAlloca(ir.getInt8Ty(),
                              ir.getInt64(get_bus_word_size()),
                              "packet_word");
    bb = stage->read_packet_word(stage->packet_word, ret);
  } else {
    stage->packet_word = stage->get_static_packet_word();
    bb = stage->read_packet_word(stage->packet_word, ret, true);
  }


  /* Prologue: Unmarshal live-in state if needed */
  BasicBlock* unmarshal_bb = nullptr;
  if( stage->has_live_in() ) {
    unmarshal_bb = stage->unmarshal_live_state(stage->live_in_val, stage->live_in_mem,
                                               &vmap, stage->get_live_in_ty(),
                                               stage->get_static_app_state());
    /* Connect unmarshalling block to the end of the prologue */
    ret->eraseFromParent();
    ret = nullptr;
    BranchInst::Create(unmarshal_bb, bb);
    bb = unmarshal_bb;
  } else {
    LLVM_DEBUG(dbgs() << "No live in state, not creating unmarshal basic block\n");
    ret->eraseFromParent();
    ret = nullptr;
  }

  /* Prologue: Read map response and make available to the application */
  if( stage->is_map_receive() ) {
    /* Do nothing here, and instead replace the map_op with a memcpy later
     * when we translate the Nanotube API functions. */
    ;
  }

  /* Function Prologue Done */
  return bb;
}

template <class T>
static
void remap(T** v, const ValueToValueMapTy& vmap) {
  if( *v != nullptr ) {
    auto it = vmap.find(*v);
    assert(it != vmap.end());
    *v = cast<T>(it->second);
  }
}

std::unordered_map<Value*, int> size_arg_to_max;
/**
 * Copy out the instructions between start and end and move them into a
 * separate function; more precisely, it is the interval [start, end) that
 * is copied; with some adjustments being made such that debug / lifetime
 * info instructions relating to end are not being split off.
 *
 * This is expected to be called as follows:
 *
 *   copy_app_code(entry inst, split point 1)
 *   copy_app_code(split point 1, split point 2)
 *   ...
 *   copy_app_code(split point N, ret)
 *
 * and will create separate functions for each stage.
 */
void
Pipeline::copy_app_code(stage_function_t* stage, BasicBlock* last_prologue,
                        ValueToValueMapTy& vmap) {
  auto* start_bb = stage->start->getParent();
  auto* end_bb   = stage->end->getParent();
  auto* func     = start_bb->getParent();
  auto* m        = func->getParent();
  auto& c        = m->getContext();

  /* Application Code: copy over and replace values */
  /* Create mapping entries for the function arguments */
  vmap[func->arg_begin()]     = stage->get_context_arg();
  vmap[func->arg_begin() + 1] = stage->get_packet_arg(); //XXX!

  /* Check basic blocks that we should copy */
  assert(start_bb == stage->bbs.front() && end_bb == stage->bbs.back());
  LLVM_DEBUG(
    dbgs() << "BasicBlocks to copy:\n";
    for( auto* bb : stage->bbs)
      dbgs() << "  " << bb->getName() << '\n';
  );

  /* Copy over the partial basic blocks */
  stage->copy_partial_bbs(stage->start, stage->end, &vmap);
  stage->copy_entire_bbs(stage->bbs,                &vmap);

  stage->recreate_allocas(&vmap);
  stage->remap_values(&vmap);
  stage->clean_up_dangling_debug();


  /* Connect the last prologue block to the first copied over block */
  BranchInst::Create(stage->app_logic_entry, last_prologue);

  auto* end_new_bb = stage->app_logic_exit;
  /* Application Code done */

  /* Terminate this function */
  auto* exit_bb = BasicBlock::Create(c, "exit", stage->func);

  BranchInst::Create(exit_bb, end_new_bb);
  ReturnInst::Create(c, nullptr, exit_bb);
  stage->stage_exit = exit_bb;

  /* Remove all undefs in PHI nodes */
  for( auto& inst : instructions(stage->func) ) {
    auto* phi = dyn_cast<PHINode>(&inst);
    if( phi == nullptr )
      continue;
    /* Replace undef with NULL */
    for( auto& iv : phi->incoming_values() ) {
      if( !isa<UndefValue>(*iv) )
        continue;
      iv = Constant::getNullValue(phi->getType());
    }
  }

  /* Remap meta-data captured in this stage */
  /* Remap the max size arguments */
  std::vector<std::pair<Value*,int>> update;
  for( auto& sz_max : size_arg_to_max ) {
    auto it = vmap.find(sz_max.first);
    if( it != vmap.end() ) {
      LLVM_DEBUG(dbgs() << "Updating max size: " << *sz_max.first << " => "
                        << *it->second << " max size: " << sz_max.second
                        << '\n');
      update.push_back(std::make_pair(it->second, sz_max.second));
    }
  }
  for( auto& sz_max : update )
    size_arg_to_max.insert(sz_max);

  /* Remap early detected calls */
  remap(&stage->nt_call, vmap);
  remap(&stage->map_op_req, vmap);
  remap(&stage->map_op_rcv, vmap);
  remap(&stage->drop_val, vmap);

  /* Remap live-out state to the newly created instructions */
  if( stage->has_live_out() )
    remap_live_values(&stage->live_out_val, &stage->live_out_mem, vmap);
}

/**
 * Copy over a basic block allowing for custom start / end instructions.
 *
 * @param src The source basic block
 * @oaram start The starting instruction to copy (inclusive); null -> copy
 *              from start
 * @param end  The instruction at which copying stops (exclusive); null ->
 *             copy to end
 */
BasicBlock* clone_basic_block(BasicBlock* src, Instruction* start,
                              Instruction* end, ValueToValueMapTy& vmap,
                              Function* f, const Twine& suffix) {
  /* Simple case: full block copy -> just use LLVM logic :) */
  if( start == nullptr && end == nullptr ) {
    LLVM_DEBUG(dbgs() << "Full block copy of " << src->getName() << '\n');
    return CloneBasicBlock(src, vmap, suffix, f);
  }

  /* Skip the instructions before start in the src basic block */
  auto start_it = (start != nullptr) ? start->getIterator() : src->begin();
  auto end_it   = (end != nullptr) ? end->getIterator() : src->end();

  /* Now copy until end */
  LLVM_DEBUG(dbgs() << "Partial copy of " << src->getName() << '\n');
  auto* bb = BasicBlock::Create(src->getContext(), src->getName(), f);
  IRBuilder<> ir(bb);
  while( start_it != end_it ) {
    LLVM_DEBUG(dbgs() << "  " << *start_it << '\n');
    auto* inst_new = ir.Insert(start_it->clone());
    if( start_it->hasName() )
      inst_new->setName(start_it->getName() + suffix);
    vmap[&*start_it] = inst_new;
    start_it++;
  }
  LLVM_DEBUG(dbgs() << "New BB:" << *bb << '\n');
  return bb;
}

/**
 * Constructs the structure containing all the live state that corresponds
 * to the split point at Instruction split.
 */
static
StructType* get_live_state_type(LLVMContext& c,
                                ArrayRef<Value*> live_values,
                                ArrayRef<MemoryLocation> live_mem) {
  bool have_live = (live_values.size() + live_mem.size()) > 0;
  if( !have_live )
    return nullptr;

  SmallVector<Type*, 8> live_types;
  /* Live values get a field in the struct that matches their type in the
   * order they are in the in array / vector */
  for( auto* v : live_values )
    live_types.emplace_back(v->getType());

  /* Live memory locations are placed into a suitably large array of bytes
   * for simplicity.  That way, we can be sure that we always copy enough
   * data over in case there are multiple overlapping sizes. */
  auto* int8_ty = Type::getInt8Ty(c);
  for( auto& m : live_mem )
    live_types.emplace_back(ArrayType::get(int8_ty, m.Size.getValue()));

  bool is_packed = true;
  return StructType::get(c, live_types, is_packed);
}

/**
 * Generates code that marshall the live state into the provided memory
 * array.
 */
BasicBlock*
stage_function_t::marshal_live_state(std::vector<Value*> live_out_val,
                                     std::vector<MemoryLocation> live_out_mem,
                                     Type* live_out_ty, Value* buf,
                                     BasicBlock* marshal_bb) {
  auto& c = func->getParent()->getContext();
  if( marshal_bb == nullptr )
    marshal_bb = BasicBlock::Create(c, "marshal_" + stage_name, func);

  IRBuilder<> ir(marshal_bb);

  auto* ty = live_out_ty->getPointerTo();
  auto* buffer = ir.CreateBitCast(buf, ty, "out_state_" + stage_name);
  unsigned idx = 0;
  SmallVector<Value*, 2> idxv(2);
  idxv[0] = ir.getInt32(0);

  /* Live values are simply written to the right field in the struct */
  for( auto* v : live_out_val ) {
    LLVM_DEBUG(dbgs() << "Marshal checking " << *v << '\n');
    if( !is_defined_here(v) ) {
      errs() << "When constructing " << func->getName()
             << "\nValue " << *v << " not defined here but in "
             << get_parent_name(v) << '\n';
      assert(is_defined_here(v));
    }

    idxv[1] = ir.getInt32(idx++);
    auto* gep = ir.CreateGEP(buffer, idxv, v->getName() + "_ptr");
    auto* st  = ir.CreateStore(v, gep);
    LLVM_DEBUG(dbgs() << *gep << '\n' << *st << '\n');
  }

  /* Live memory locations are copied out with memcpy into the allocated
   * fields. */
  for( auto& m : live_out_mem ) {
    if( !is_defined_here(m.Ptr) ) {
      errs() << "When constructing " << func->getName()
             << "\nMloc " << m << " not defined here but in "
             << get_parent_name(m.Ptr) << '\n';
      assert(is_defined_here(m.Ptr));
    }

    idxv[1] = ir.getInt32(idx++);
    auto* gep = ir.CreateGEP(buffer, idxv, m.Ptr->getName() + "_ptr");
    auto* cpy = ir.CreateMemCpy(gep, 0, const_cast<Value*>(m.Ptr), 0, m.Size.getValue());
    LLVM_DEBUG(dbgs() << *gep << '\n' << *cpy << '\n');
  }
  return marshal_bb;
}


/**
 * Generates code that restores the live state from the provided buffer,
 * and updates a map that maps the original live value to the unmarshalled
 * version of itself.
 */
BasicBlock*
stage_function_t::unmarshal_live_state(ArrayRef<Value*> live_val,
                                       ArrayRef<MemoryLocation> live_mem,
                                       ValueToValueMapTy* vmap,
                                       Type* live_state_ty,
                                       Value* buf) {
  auto& c = func->getParent()->getContext();
  /* Unmarshal live state in this pipeline stage from the buffer arg */
  unmarshal_bb = BasicBlock::Create(c, "unmarshal_" + stage_name,
                                    func);
  IRBuilder<> ir(unmarshal_bb);

  auto* buffer     = ir.CreateBitCast(buf, live_state_ty->getPointerTo(),
                                      "buffer_" + stage_name);
  auto* ty        = live_state_ty->getPointerTo();

  assert(buffer->getType()->isPointerTy());
  assert(buffer->getType() == ty);

  unsigned idx = 0;
  SmallVector<Value*, 2> idxv(2);
  idxv[0] = ir.getInt32(0);

  /* Simply load live values from the copy in memory */
  for( auto* v : live_val ) {
    idxv[1] = ir.getInt32(idx++);
    auto* gep = ir.CreateGEP(buffer, idxv, v->getName() +
                                            "_ptr_" + stage_name);
    auto* ld  = ir.CreateLoad(v->getType(), gep, v->getName() + "_" + stage_name);
    (*vmap)[v] = ld;
    LLVM_DEBUG(dbgs() << *gep << '\n' << *ld << '\n');
  }

  /* Live memory locations live in the static live-in state; while it is
   * possible to leave them there and to simply rewrite the pointer
   * references, we will create a non-static copy on the stack.  That way,
   * the static live-in state remains read-only which improves synthesis.
   * Do three things:
   *   - alloca
   *   - memcpy: static live-in state -> new buffer
   *   - type cast
   *   - update mapping
   */
  IRBuilder<> entry(func->getEntryBlock().getTerminator());
  for( auto& m : live_mem ) {
    idxv[1] = ir.getInt32(idx++);
    auto  name = m.Ptr->getName();
    auto  sz   = m.Size.getValue();
    auto* ty   = m.Ptr->getType();

    /* Local memory copies live on the stack; maintain pointer type opacity
     * and handle their allocations as byte arrays */
    auto* buf_stack = entry.CreateAlloca(ir.getInt8Ty(), ir.getInt32(sz),
                                         name + "_stack_" + stage_name);
    auto* bc = entry.CreateBitCast(buf_stack, ty, name + "_" + stage_name);

    /* Now copy from the static live-in state to the local stack copy */
    auto* gep = ir.CreateGEP(buffer, idxv, name + "_state_" + stage_name);
    auto* memcpy = ir.CreateMemCpy(buf_stack, 0, gep, 0, sz);

    /* Remember the mapping */
    (*vmap)[m.Ptr] = bc;
    LLVM_DEBUG(dbgs() << *buf_stack << '\n' << *bc << '\n'
                      << *gep << '\n' << *memcpy << '\n');
  }

  LLVM_DEBUG(dbgs() << "Unmarshal BB:" << *unmarshal_bb << '\n');
  return unmarshal_bb;
}

void
stage_function_t::copy_partial_bbs(Instruction* start, Instruction* end,
                                   ValueToValueMapTy* vmap)  {
  auto* start_bb = start->getParent();
  auto* end_bb   = end->getParent();

  if( start_bb == end_bb ) {
    /* A single basic block -> just copy what is needed */
    auto* bb = clone_basic_block(start_bb, start, end,
                                 *vmap, func, "_" + stage_name);
    (*vmap)[start_bb]    = bb;

    app_logic_entry      = bb;
    app_logic_exit       = bb;
    app_logic.push_back(bb);
  } else {
    auto* start_new = clone_basic_block(start_bb, start, nullptr, *vmap, func,
                                        "_" + stage_name);
    auto* end_new   = clone_basic_block(end_bb, nullptr,
                                        end, *vmap, func,
                                        "_" + stage_name);
    (*vmap)[start_bb]    = start_new;
    (*vmap)[end_bb]      = end_new;

    app_logic_entry = start_new;
    app_logic_exit  = end_new;
    app_logic.push_back(start_new);
    app_logic.push_back(end_new);
  }
}

void
stage_function_t::copy_entire_bbs(ArrayRef<BasicBlock*> bbs,
                                  ValueToValueMapTy* vmap)  {
  /* Copy over all remaining whole blocks */
  for( auto* bb : bbs ) {
    if( vmap->find(bb) != vmap->end() )
      continue;

    auto* new_bb = clone_basic_block(bb, nullptr, nullptr, *vmap, func,
                                     "_" + stage_name);
    (*vmap)[bb]  = new_bb;
    app_logic.push_back(new_bb);
  }
}

/* Check clone-safe instructions and clone them */
static void
clone_instruction(Function* func, std::string& stage_name,
                  Instruction* prod, ValueToValueMapTy* vmap,
                  IRBuilder<>& ir)  {
  /* new_prod will hold the clone instruction for simple clone cases */
  Instruction* new_prod = nullptr;
  switch( prod->getOpcode() ) {
    case Instruction::Alloca:
    case Instruction::IntToPtr:
    case Instruction::BitCast:
    case Instruction::GetElementPtr:
      /* these are all fine, simply copy */
      new_prod = prod->clone();
      break;
    case Instruction::PHI: {
#ifdef SAVESTACK_SORTED
      errs() << "ERROR: Unexpected pointer-phi " << *prod
             << " in BB " << *prod->getParent() << " missing in "
             << *func << "\nAborting!\n";
      exit(1);
#endif
#ifndef SAVESTACK_SORTED
      /* handle obvious PHIs of dead memory */
      auto* phi = cast<PHINode>(prod);
      if( !phi->hasConstantOrUndefValue() ) {
        errs() << "ERROR: Unable to handle PHI node " << *phi
               << "which is not constant-undef.  This needs "
               << "handling elsewhere.  Aborting!\n";
        exit(1);
      }
      Instruction* phi_val = nullptr;
      for( auto& v : phi->incoming_values() ) {
        if( isa<UndefValue>(v) )
          continue;
        auto* inst = cast<Instruction>(v);
        assert( (phi_val == nullptr) || (phi_val == inst) );
        phi_val = inst;
      }
      assert(phi_val != nullptr);
      assert(vmap->find(phi_val) != vmap->end());
      (*vmap)[prod] = (*vmap)[phi_val];
      errs() << "WARNING: Replacing trivial PHI "
             << *phi << " with " << *(*vmap)[phi_val] << '\n';
      return;
#endif
      }
    case Instruction::Call: {
      auto id = get_intrinsic(prod);
      if( id == Intrinsics::llvm_stacksave ) {
        /* Simply cloning a stacksave is probably very bad, but hey
         * ho. */
        new_prod = prod->clone();
        break;
      }
      /* Intentional fall-through! */
      }
    default:
      errs() << "ERROR: Unrecognised instruction in chain for "
             << "non-live memory location: " << *prod
             << " Aborting!\n";
      exit(1);
  }

  /* We have a pointer instruction that is used in this function, but
   * does not live here -> clone it. */
  ir.Insert(new_prod, prod->getName() + "_" + stage_name);
  (*vmap)[prod] = new_prod;
  LLVM_DEBUG(dbgs() << "Cloning " << *prod << " into " << *new_prod <<'\n');
}

/**
 * This function recreates those instructions that were
 * - not copied as part of the pipeline stage (they were not between start
 *   & end)
 * - not handled as live state (either live values or live memmory)
 * - but are used in this pipeline stage.
 *
 * The only instructions for which this is the case is pointer handling of
 * allocations which are dead (the value is live but there is nothing
 * stored in the allocation, yet).  We can thus replicate those
 * instructions in the pipeline stage where they are used.
 */
void
stage_function_t::recreate_allocas(ValueToValueMapTy* vmap) {
  /* Mirror allocas and pointer arithmetic in those function stages where
   * they are needed */

  /* Get instructions which need to be cloned from the previous stage */
  std::unordered_set<Instruction*> need_clone;
  for( auto& bb : *func ) {
    std::vector<Instruction*> worklist;
    for( auto it = bb.rbegin(); it != bb.rend(); it++ )
      worklist.push_back(&*it);

    while( !worklist.empty() ) {
      /* Review all instructions in this function and check where they are
       * coming from, adding input dependencies to the worklist as we find
       * them */
      auto* inst = worklist.back();
      worklist.pop_back();
      /* Ignore lifetime markers and debug for this analysis */
      if( inst->isLifetimeStartOrEnd() || isa<DbgInfoIntrinsic>(inst) )
        continue;

      /* Check the instruction itself: if it is used in this function, but
       * not defined here, nor being remapped... -> clone it */
      if( !is_defined_here(inst) && (vmap->find(inst) == vmap->end()) )
        need_clone.insert(inst);

      /* Check the instruction input operands */
      /*XXX: Strictly speaking this is not enough; one would really have to
        analyis the live-in / live-out values of the instruction. With
        broken AA, it can well happen that allocations are live, but they
        do not show up as an operand. Not handling these here will cause
        problems with cross function references! */
      for( unsigned idx = 0; idx < inst->getNumOperands(); idx++) {
        auto* v  = inst->getOperand(idx);
        /* Values defined here or being remapped are fine */
        if( is_defined_here(v) || (vmap->find(v) != vmap->end()) )
          continue;
        if( !v->getType()->isPointerTy() ) {
          LLVM_DEBUG(
            dbgs() << "Non-pointer value when cloning memory: "
                   << *v << " used in instruction: " << *inst
                   << " in function " << *func << '\n');
        }
#if 0
        //NOTE: It is possible that non-pointer values are needed that are
        //not live anymore... this is a little dangerous.. somehow this
        //needs to be done in a better way.
        if( !v->getType()->isPointerTy() ) {
          errs() << "Non-pointer value when cloning memory: "
                 << *v << " used in instruction: " << *inst
                 << " in function " << *func << '\n';
          assert(v->getType()->isPointerTy());
        }
#endif
        auto* prod = dyn_cast<Instruction>(v);
        if( prod == nullptr ) {
          auto* deff = get_function(v);
          Value* mapped_v = (*vmap)[v];
          errs() << "Non-local, non-instruction value when cloning memory:\n"
                 << *v << "\nused in instruction:\n" << *inst
                 << "\nin function\n  " << func->getName()
                 << "\ndefined in\n  " << (deff ? deff->getName() : "<nullptr>")
                 << '\n'
                 << "Value is mapped to: " << nullsafe(mapped_v) << '\n'
                 << "\n\nStage function:\n" << *func;
          if( deff != nullptr )
            errs() << "\n\nSource function:\n" << *deff << '\n';
          assert(prod != nullptr);
        }
        worklist.push_back(prod);
      }
    }
  }

  /* We now have a list of instructions that need cloning */
  unsigned n_clones = need_clone.size();
  if( n_clones == 0 )
    return;

  LLVM_DEBUG(
    dbgs() << "Function " << func->getName() << " needs clones of "
           << "these instructions:\n";
    for(auto *inst : need_clone)
      dbgs() << "  " << *inst << '\n';
  );

  /* Actually create the clones by walking the original function in CFG
   * compatible manner, and cloning those instructions that need cloning.
   */
  typedef dep_aware_converter<BasicBlock> bb_dac_t;
  bb_dac_t dac;
  auto* orig_f = (*need_clone.begin())->getFunction();
  for( auto& bb : *orig_f )
    dac.insert(&bb, pred_size(&bb));

  IRBuilder<> ir(func->getEntryBlock().getTerminator());

  dac.execute([&](bb_dac_t* dac, BasicBlock* bb) {
    LLVM_DEBUG(dbgs() << "Func: " << bb->getParent()->getName() << " BB: "
                      << bb->getName() << '\n');
    for( auto& inst : *bb ) {
      if( n_clones == 0 )
        return;
      if( need_clone.count(&inst) == 0 )
        continue;
      clone_instruction(func, stage_name, &inst, vmap, ir);
      n_clones--;
    }
    for(auto succ : successors(bb) )
      dac->mark_dep_ready(succ);
  });
}

void stage_function_t::clean_up_dangling_debug() {
  /* Clean up debug and lifetime statements that don't match anymore */
  SmallVector<Instruction*,8> to_erase;
  for( auto& bb : *func ) {
    for( auto& inst : bb) {
      Value* v = nullptr;
      /* Drop untranslated debug statements and lifetime markers */
      if( isa<DbgValueInst>(inst) ) {
        v = cast<DbgValueInst>(inst).getValue();
      } else if( inst.isLifetimeStartOrEnd() ) {
        v = inst.getOperand(1);
      }
      if( v == nullptr || is_defined_here(v) )
        continue;

      LLVM_DEBUG(dbgs() << "Dropping inst: " << inst
                        << " with val: " << *v << '\n');
      to_erase.push_back(&inst);
    }
  }

  for( auto* inst : to_erase )
    inst->eraseFromParent();
}

static
const Function* get_function(const Value* v) {
  if( isa<Instruction>(v) )
    return cast<Instruction>(v)->getFunction();
  if( isa<Argument>(v) )
    return cast<Argument>(v)->getParent();
  if( isa<BasicBlock>(v) )
      return cast<BasicBlock>(v)->getParent();
  return nullptr;
}
/**
 * Returns whether the value v is defined in the pipeline stage function.
 */
bool stage_function_t::is_defined_here(const Value* v) {
  if( get_function(v) == func )
    return true;
  if( isa<Constant>(v) )
    return true;
  return false;
}
std::string stage_function_t::get_parent_name(const Value* v) {
  if( isa<Instruction>(v) )
    return cast<Instruction>(v)->getFunction()->getName();
  if( isa<Argument>(v) )
    return cast<Argument>(v)->getParent()->getName();
  if( isa<Constant>(v) )
    return "<constant>";
  return "<unclear>";
}

void stage_function_t::remap_values(ValueToValueMapTy* vmap) {
  /* Patch up references in the created blocks */
  /* Remap the actual things; note, this does the handling of the ugly meta
   * data pieces, too; so better than writing your own... */
  SmallVector<BasicBlock*,8> new_bbs;
  for( auto& bb : *func )
    new_bbs.push_back(&bb);
#if 0
  /* Compute final resting value / transitive end */
  for( auto& in_out : *vmap ) {
    auto it = vmap->find(in_out.second);
    if( it == vmap->end() )
      continue;
    Value* end;
    do {
      end = it->second;
      it  = vmap->find(it->second);
    } while( it != vmap->end() );
    in_out.second = end;
  }
#endif
  remapInstructionsInBlocks(new_bbs, *vmap);
  remapInstructionsInBlocks(new_bbs, *vmap);
}

BasicBlock*
stage_function_t::try_call(Instruction* insert_before,
                           std::function<Value*(IRBuilder<>&)> gen) {
  /* Split the basic block such that we have bb_pre -(br)-> { insert_before;
   * bb } and we can insert our logic "into" the branch */
  auto* bb      = insert_before->getParent();
  auto* bb_post = split_app_bb(bb, insert_before,
                               bb->getName() + "_post");
  bb->getTerminator()->eraseFromParent();

  /* generate the code that produces the success value */
  IRBuilder<> ir(bb);
  auto* res = gen(ir);

  /* .. and check whether the read was successful. */
  auto* icmp = ir.CreateICmpEQ(res,
                 ConstantInt::get(res->getType(), 0),
                 "try_fail");
  auto* bb_exit = get_thread_wait_exit(bb_post);
  ir.CreateCondBr(icmp, bb_exit, bb_post);

  return bb_post;
}

BasicBlock*
stage_function_t::read_from_channel(Constant* channel_id, Value* data,
                                    Constant* data_size,
                                    Instruction* insert_before) {
  auto rd = [&](IRBuilder<>& ir) {
    Value* args[4];
    args[0] = get_context_arg();
    args[1] = channel_id;
    args[2] = ir.CreateBitCast(data, ir.getInt8PtrTy(), data->getName() + "_voidp");
    args[3] = data_size;
    auto* try_read_f = create_nt_channel_try_read(*func->getParent());
    auto* ty = cast<FunctionType>(cast<PointerType>(try_read_f->getType())->getElementType());
    auto* try_read = ir.CreateCall(ty , try_read_f, args, "read_channel");

    return try_read;
  };

  return try_call(insert_before, rd);
}

GlobalVariable*
stage_function_t::get_static(StringRef var_name, Type* type) {
  auto it = static_vars.find(var_name);
  if( it != static_vars.end() )
    return it->second;

  auto* m  = func->getParent();
  auto* gv = new GlobalVariable(*m, type, false,
                                GlobalValue::PrivateLinkage,
                                Constant::getNullValue(type),
                                var_name + "_" + stage_name);
  static_vars[var_name] = gv;
  return gv;
}

#define create_get_static(name, type)                          \
GlobalVariable*                                                \
stage_function_t::get_static_##name() {                        \
  auto* m  = func->getParent();                                \
  auto& c __attribute__((unused)) = m->getContext();           \
  return get_static(#name, type);                              \
}
create_get_static(have_app_state,   Type::getInt1Ty(c));
create_get_static(sent_app_state,   Type::getInt1Ty(c));
create_get_static(app_state,        get_live_in_ty());
create_get_static(have_packet_word, Type::getInt1Ty(c));
create_get_static(packet_word,      ArrayType::get(Type::getInt8Ty(c),
                                                   get_bus_word_size()));
create_get_static(have_map_resp,    Type::getInt1Ty(c));
create_get_static(map_resp_data,    get_map_resp_data_ty());
create_get_static(map_result,       Type::getInt32Ty(c));
create_get_static(word_is_eop,      Type::getInt1Ty(c));

create_get_static(packet_read_tap_state,
                  get_nt_tap_packet_read_state_ty(*m));
create_get_static(packet_write_tap_state,
                  get_nt_tap_packet_write_state_ty(*m));
create_get_static(packet_length_tap_state,
                  get_nt_tap_packet_length_state_ty(*m));
create_get_static(packet_resize_ingress_tap_state,
                  get_nt_tap_packet_resize_ingress_state_ty(*m));
create_get_static(packet_resize_egress_tap_state,
                  get_nt_tap_packet_resize_egress_state_ty(*m));
create_get_static(packet_resize_egress_tap_state_packet_word,
                  ArrayType::get(Type::getInt8Ty(c),
                    get_bus_word_size()));
create_get_static(packet_eop_tap_state,
                  get_nt_tap_packet_eop_state_ty(*m));

create_get_static(packet_resize_egress_cword,
                  get_nt_tap_packet_resize_cword_ty(*m));
create_get_static(packet_resize_egress_have_cword, Type::getInt1Ty(c));
#undef create_get_static

GlobalVariable*
stage_function_t::get_static_packet_read_data(unsigned size) {
  auto* m  = func->getParent();
  auto& c  = m->getContext();
  return get_static("packet_read_data",
                    ArrayType::get(Type::getInt8Ty(c), size));
}

/**
 * This function creates wrapping code to that checks a flag and if that
 * flag is false, branches to a new basic block that the caller can fill.
 * Further, it creates logic that sets the flag when the conditional block
 * has been executed.
 *
 * @param static_check       Global bool variable chosing whether the new
 *                           block should be executed.  If false, the block
 *                           will get executed and then static_cehck will
 *                           be set to true.
 * @param insert_before      Pointer to the instruction before which the
 *                           conditional diversion to the new basic block
 *                           will be inserted.
 * @param call_insert_place  Out value telling the caller before which
 *                           instruction they can insert their logic into
 *                           the new basic block.
 * @return Pointer to the split-off part of the original basic block (new
 *         location of insert_before).
 *
 * If this is called with (notice the use of parameter names!):
 *
 * bb:
 *   inst_1
 *   inst_2
 *   insert_before
 *   inst_4
 *   inst_5
 *
 * the generated code will look as follows:
 *
 * bb:
 *   inst_1
 *   inst_2
 *   %1 = load @static_check
 *   br %1, %bb_post, %call_bb
 *
 * call_bb:
 *   <app can fill this later>
 *   store 1, @static_check    ; <= *call_insert_place points here!
 *   br bb_post
 *
 * bb_post:
 *   insert_before
 *   inst_4
 */
BasicBlock*
stage_function_t::checked_call(Value* static_check,
                               Instruction* insert_before,
                               Instruction** call_insert_place) {
  auto* m  = func->getParent();
  auto& c  = m->getContext();

  /* Basic block surgery: BB -> BB + BB_post and create call_bb as the new
   * basic block */
  auto* bb      = insert_before->getParent();
  auto* bb_post = split_app_bb(bb, insert_before, bb->getName() + "_post");
  bb->getTerminator()->eraseFromParent();
  auto* call_bb = BasicBlock::Create(c, "call_bb", func, bb_post);

  /* Check the variable and branch to call_bb if needed */
  IRBuilder<> ir(bb);
  auto* ld = ir.CreateLoad(ir.getInt1Ty(), static_check);
  ir.CreateCondBr(ld, bb_post, call_bb);

  /* Fill call_bb: remember a successful execution and link to bb_post */
  ir.SetInsertPoint(call_bb);
  auto* st = ir.CreateStore(ir.getInt1(1), static_check);
  ir.CreateBr(bb_post);

  /* the caller will slot their code just before the store */
  *call_insert_place = st;

  return bb_post;
}

/**
 * Create a read from a channel that executes whenever a provided flag
 * variable is false.
 *
 * @param dest           Pointer to the location where the read data
 *                       should be stored.
 * @param ty             Type of the data that should be read; determines
 *                       the size of the read.
 * @param static_check   Memory location determining whether the read will
 *                       execute (when the stored value is false); if the
 *                       read executed successfully, the variable will be
 *                       set to true.
 * @param channel        Channel ID from which to read.
 * @param name           Name suffix associated with names generated.
 * @param insert_before  Instruction before which to insert the newly
 *                       generated code.
 * @param good_read_ip   Out value: Pointer to an instruction that tells
 *                       the caller where they can insert code that needs
 *                       to execute if the call executed and was
 *                       successful.  If nullptr, no info will be give.
 *                       See sketch below.
 * @return Pointer to the basic block where insert_before lives now.
 *
 * If this is called with (notice the use of parameter names!):
 *
 * bb:
 *   inst_1
 *   inst_2
 *   insert_before
 *   inst_4
 *   inst_5
 *
 * the generated code will look as follows:
 *
 * bb:
 *   inst_1
 *   inst_2
 *   %1 = load @static_check
 *   br %1, %bb_post, %read_<name>
 *
 * read_<name>:
 *   read_channel = nanotube_channel_try_read(%nt_ctx, channel, dest,
 *                                            sizeof(ty))
 *   try_fail = icmp eq %read_channel, 0
 *   br %try_fail, %thread_exit, %read_<name>_post
 *
 * read_<name>_post:
 *   store 1, @static_check  ; <= *good_read_ip points here
 *   br bb_post
 *
 * bb_post:
 *   insert_before
 *   inst_4
 */
BasicBlock*
stage_function_t::checked_read(Value* dest, Type* ty, Value* static_check,
                               unsigned channel, StringRef name,
                               Instruction* insert_before,
                               Instruction** good_read_ip) {
  Instruction* call_ip = nullptr;
  auto* tail_bb = checked_call(static_check, insert_before, &call_ip);
  assert(call_ip != nullptr);
  call_ip->getParent()->setName("read_" + name);
  if( good_read_ip != nullptr )
    *good_read_ip = call_ip;

  auto* m  = func->getParent();
  auto& c  = m->getContext();
  auto& dl = m->getDataLayout();
  auto size = dl.getTypeStoreSize(ty);

  auto* channel_id = ConstantInt::get(Type::getInt32Ty(c), channel);
  read_from_channel(channel_id,
                    dest,
                    ConstantInt::get(Type::getInt64Ty(c), size),
                    call_ip);
  return tail_bb;
}

BasicBlock*
stage_function_t::read_app_state(Value* dest_state, BasicBlock* insert_bb) {
  assert(has_live_in());

  return checked_read(dest_state, get_live_in_ty(),
                      get_static_have_app_state(), STATE_IN, "app_state",
                      &insert_bb->back());
}

Value*
stage_function_t::get_map_ptr(nanotube_map_id_t id) {
  auto* m = func->getParent();

  auto it = map_id_to_val.find(id);
  if( it != map_id_to_val.end() )
    return it->second;

  /* Read the map pointer from the user supplied info */
  auto* bb = &func->getEntryBlock();
  IRBuilder<> ir(bb, bb->getFirstInsertionPt());
  auto* mapty = get_nt_tap_map_ty(*m)->getPointerTo();
  auto  nmaps = setup->maps().size();
  auto* ty = ArrayType::get(mapty, nmaps);

  /* Create the instructions to read this from the user supplied arg */
  //XXX: We should translate this through the provided context here
  auto idx = setup->contexts()[0].map_user_id_to_idx(id);
  auto* bc = ir.CreateBitCast(get_user_arg(), ty->getPointerTo());
  auto* elem = ir.CreateConstInBoundsGEP2_32(ty, bc, 0, (unsigned)idx);
  auto* ld = ir.CreateLoad(mapty, elem, "map" + Twine((unsigned)id));

  map_id_to_val[id] = ld;
  return ld;
}

/**
 * Get the client ID for accessing a particular map.  Client IDs
 * distinguish different map_ops (send + receive pairs) for the same map.
 * The sender and the receiver resulting from the same map_op will have the
 * same client_id.
 */
unsigned
stage_function_t::get_client_id(nanotube_map_id_t id, bool rcv) {
  auto& m2i = rcv ? map_to_rcvs : map_to_reqs;

  auto& insts = m2i[id];

  unsigned sz = insts.size();
  unsigned idx = 0;
  for( ; idx < sz; idx++ )
    if( insts[idx] == this )
      break;

  if( idx == sz ) {
    insts.push_back(this);
    LLVM_DEBUG(dbgs() << "Adding a new " << (rcv ? "receive" : "request")
                       << " stage for map " << id << '\n');
  }
  LLVM_DEBUG(dbgs() << "Stage " << stage_name << " has client ID  " << idx
                    << " for map " << id << '\n');
  return idx;
}

BasicBlock*
stage_function_t::read_map_response(Value* dest_state, Value* result,
                                    BasicBlock* insert_bb) {
  assert(is_map_receive());

  auto& m = *func->getParent();
  auto& c = m.getContext();

  /* Parse out the information for the map receive */
  assert(map_op_rcv != nullptr);
  map_op_receive_args moa(map_op_rcv);

  Value* map_ptr   = get_map_ptr(moa.map);
  Value* client_id = ConstantInt::get(Type::getInt32Ty(c),
                                      get_client_id(moa.map, true));

  LLVM_DEBUG(
    dbgs() << "Reading map response into " << *dest_state
           << " size:" << *moa.data_length
           << " map ptr: " << *map_ptr << " client ID: " << *client_id
           << '\n');

  auto buf_size = cast<ConstantInt>(moa.data_length);
  if (isa<ConstantPointerNull>(moa.data_out))
    assert(buf_size->isZero());
  map_rcv_buf_size = buf_size;

  /**
   * This code below is a little convoluted, and I can only apologise to
   * you, the reader, for that!  The idea is that checked_call() provides the
   * surrounding code for a particular call to a Nanotube function that
   * should only happen once per packet.  That call is provided by
   * try_call().  For that, try_call() takes a call-back that creates the
   * actual function call and wraps that into logic that leaves the thread
   * when the call failed. */

  /* Create the framework for the checked call */
  Instruction* call_ip = nullptr;
  auto* tail_bb = checked_call(get_static_have_map_resp(),
                               &insert_bb->back(), &call_ip);
  /* Set the name of the basic block */
  call_ip->getParent()->setName("read_map_resp");

  /* Call-back will create the actual call and support instructions (casts,
   * ...) needed */
  auto map_resp = [&](IRBuilder<>& ir)->Value* {
    Value* args[5];
    args[0] = get_context_arg();
    args[1] = map_ptr;
    args[2] = client_id;
    args[3] = ir.CreateBitCast(dest_state, ir.getInt8PtrTy(),
                               dest_state->getName() + "_voidp");
    args[4] = result;

    auto* try_read_f = create_nt_tap_map_recv_resp(m);
    auto* ty = get_nt_tap_map_recv_resp_ty(m);

    auto* try_read = ir.CreateCall(ty , try_read_f, args, "map_read");
    return try_read;
  };

  /* Create the call using the try_call wrapper which handles thread abort
   * logic in case the call fails. */
  try_call(call_ip, map_resp);

  return tail_bb;
}

/**
 * Generate a sequence of instructions that figures out whether a packet
 * word is the last one of a packet.
 */

Value* stage_function_t::get_eop(IRBuilder<>& ir, Value* packet_word, Module* m) {
  auto* tap_state = get_static_packet_eop_tap_state();
  auto* eop_ty = get_tap_packet_is_eop_ty(*m);
  auto* eop_f  = create_tap_packet_is_eop(*m, get_bus_type());
  Value* args[] = { ir.CreateBitCast(packet_word, ir.getInt8PtrTy()),
                    tap_state };
  auto* eop    = ir.CreateCall(eop_ty, eop_f, args, "eop");
  return eop;
}

BasicBlock*
stage_function_t::read_packet_word(Value* dest_state,
                                   Instruction* insert_before,
                                   bool checked) {
  auto* m  = func->getParent();
  auto& c  = m->getContext();

  /**
   * Our aim is to get this layout:
   * bb:
   *   <old stuff>
   *   %res = nt_channel_try_read(.. %packet_word ...);
   *   %b   = icmp %res ...
   *   br %b, thread_wait_exit, bb_post
   *
   * thread_wait_exit:
   *   ...
   *
   * bb_post:
   *   %eop      = nt_tap_packet_is_eop(..)
   *   %in_packet = ... not eop ...
   *   store %in_packet, @have_app_state
   *   store %in_packet, @have_map_resp
   *   <insert_before>
   *   ...
   *
   * The fact that we are reading a packet means that we must have an
   * app_state / map_response already, so it is safe to overwrite the value
   * of those with one when being inside of the packet (and not at the
   * end!).  (Instead of only resetting the have_*_state variables whenever
   * %eop == 1)
   */

  auto* bb = insert_before->getParent();
  /* Read from the channel and handle not ready condition */
  auto  size   = get_bus_word_size();
  auto* ty     = ArrayType::get(Type::getInt8Ty(c), size);
  auto* channel_id = ConstantInt::get(Type::getInt32Ty(c), PACKETS_IN);

  BasicBlock* bb_post;
  if( !checked ) {
    bb_post = read_from_channel(channel_id, dest_state,
                                ConstantInt::get(Type::getInt64Ty(c), size),
                                insert_before);
  } else {
    Instruction* after_read_insert_place;

    bb_post = checked_read(dest_state, ty, get_static_have_packet_word(),
                           PACKETS_IN, "packet_word", insert_before,
                           &after_read_insert_place);
    /*
    What we have:
      check_packet_word:
      br have_packet_word, got_packet_word, read_packet_word

      read_packet_word:
      channel_read( ... static_packet_word ... )
      br got_packet_word <- we need a pointer to this

      got_packet_word:
      ...

    We want to turn this into :
      check_packet_word:
      br have_packet_word, got_packet_word, read_packet_word

      read_packet_word:
      channel_read( ... static_packet_word ... )
      INSERT ME: is_eop = get_eop (static_packet_word)
      INSERT ME: store is_eop, static_is_eop
      br got_packet_word

      got_packet_word:
      word_is_eop = load static_is_eop
      ...

    */
    IRBuilder<> ir(after_read_insert_place);
    auto* eop = get_eop(ir, dest_state, m);
    auto* static_word_is_eop = get_static_word_is_eop();
    ir.CreateStore(eop, static_word_is_eop);
  }

  /* Rename the basic block where the packet read was added */
  bb->setName("read_packet_word");

  IRBuilder<> ir(insert_before);

  /* Call the eop tap and store the result as we need to guarantee it's called
   * exactly once per word and may be needed again later.
   * In the "checked" case it's stored in a static above when the word is read,
   * so we pull that out here instead.
   */
  if( !checked ) {
    auto* eop = get_eop(ir, dest_state, m);
    word_is_eop = eop;
  } else {
    auto* static_word_is_eop = get_static_word_is_eop();
    word_is_eop = ir.CreateLoad(static_word_is_eop);
  }

  if( unset_app_state_eop() || unset_map_resp_eop() ) {
    /* Check for EOP and unset have_app_state if it is set
     * NOTE: Chosing to always write to have_app_state so that we have less
     * control transfer, and hopefully easier synthesis :) */

    /* *have_app_state = ((control & eop) == 0); */
    auto* in_packet = ir.CreateNot(word_is_eop, "in_packet");
    if( unset_app_state_eop() )
      ir.CreateStore(in_packet, get_static_have_app_state());
    if( unset_map_resp_eop() )
      ir.CreateStore(in_packet, get_static_have_map_resp());
  }
  return bb_post;
}

void
stage_function_t::write_to_channel(Constant* channel_id, Value* data,
                                   Constant* data_size,
                                   Instruction* insert_before) {
  IRBuilder<> ir(insert_before);
  auto* data_voidp = ir.CreateBitCast(data, ir.getInt8PtrTy());
  Value* args[] = {
    get_context_arg(),
    channel_id,
    data_voidp,
    data_size
  };
  auto* write_f = create_nt_channel_write(*func->getParent());
  auto* ty = cast<FunctionType>(cast<PointerType>(write_f->getType())->getElementType());
  ir.CreateCall(ty , write_f, args);
}

void
stage_function_t::write_app_state(Value* src_state, Instruction* insert_before) {
  auto* m = func->getParent();
  auto& c = m->getContext();
  const auto& dl = m->getDataLayout();

  auto* ty         = live_out_ty;
  auto szi         = dl.getTypeStoreSize(ty);
  auto* size       = ConstantInt::get(Type::getInt64Ty(c), szi);
  auto* channel_id = ConstantInt::get(Type::getInt32Ty(c), STATE_OUT);
  write_to_channel(channel_id, src_state, size, insert_before);
}

void
stage_function_t::write_packet_word(Value* src_word, Instruction* insert_before) {
  auto* m = func->getParent();
  auto& c = m->getContext();

  auto szi         = get_bus_word_size();
  auto* size       = ConstantInt::get(Type::getInt64Ty(c), szi);
  auto* channel_id = ConstantInt::get(Type::getInt32Ty(c), PACKETS_OUT);
  write_to_channel(channel_id, src_word, size, insert_before);
}

void
stage_function_t::write_cword(Value* cword, Instruction* insert_before) {
  auto* m = func->getParent();
  auto& c = m->getContext();
  const auto& dl = m->getDataLayout();

  auto* ty         = get_nt_tap_packet_resize_cword_ty(*m);
  auto szi         = dl.getTypeStoreSize(ty);
  auto* size       = ConstantInt::get(Type::getInt64Ty(c), szi);
  auto* channel_id = ConstantInt::get(Type::getInt32Ty(c), CWORD_OUT);
  write_to_channel(channel_id, cword, size, insert_before);
}

BasicBlock*
stage_function_t::get_thread_wait_exit(BasicBlock* insert_before) {
  auto* m = func->getParent();
  auto& c = m->getContext();
  if( thread_wait_exit != nullptr )
    return thread_wait_exit;
  thread_wait_exit = BasicBlock::Create(c, "thread_wait_exit", func,
                                        insert_before);
  IRBuilder<> ir(thread_wait_exit);
  auto* thread_wait_f = create_nt_thread_wait(*m);
  auto* ty = cast<FunctionType>(cast<PointerType>(thread_wait_f->getType())->getElementType());
  ir.CreateCall(ty, thread_wait_f);
  ir.CreateRetVoid(); //XXX: Better to branch to the single exit here?

  return thread_wait_exit;
}

void stage_function_t::set_nt_call(Instruction* call, Intrinsics::ID intr) {
  if( nt_call != nullptr ) {
    errs() << "ERROR: Multiple Nanotube calls per pipeline stage are "
           << "not yet supported.  First call: " << *nt_call
           << " Second call: " << *call << " Aborting!\n";
    exit(1);
  }
  nt_call = call;
  set_nt_id(intr);
}

void stage_function_t::scan_nanotube_accesses(setup_func* setup) {
  for( auto* bb : bbs ) {
    bool near_start = (bb == start->getParent());
    bool near_end   = (bb == end->getParent());
    auto s = near_start ? start->getIterator() : bb->begin();
    auto e = near_end   ? end->getIterator() : bb->end();

    for( auto it = s; it != e; ++it) {
      auto* cur = &*it;
      if( !isa<CallInst>(cur) )
        continue;

      auto* call = cast<CallInst>(cur);
      auto intr = get_intrinsic(call);
      if( (intr >= Intrinsics::none) && (intr <= Intrinsics::llvm_unknown) )
        continue;

      switch( intr ) {
        /* Intrinsics that this pass knows how to convert into taps */
        case Intrinsics::map_op_send:
          /* map_op_send will get converted in the epilogue and co-exist
           * with another nt-call */
          map_op_req = call;
          LLVM_DEBUG(dbgs() << "Stage " << stage_name
                            << " sends a map request " << *call << '\n');
          break;
        case Intrinsics::map_op_receive: {
          map_op_rcv = call;

          LLVM_DEBUG(
            map_op_receive_args args(call);
            //XXX: Make sure that there is only a single context :)
            const map_info& info = setup->get_map_info(0, args.map);

            dbgs() << "Found map_op to map " << args.map << " which has "
                   << "key_sz " << *info.args().key_sz
                   << " value_sz " << *info.args().value_sz << '\n';
          );
          set_nt_call(call, intr);
          break;
        }
        case Intrinsics::packet_drop: {
          packet_drop_args pda(call);
          drop_val = pda.drop;
          set_nt_call(call, intr);
          break;
        }
        case Intrinsics::packet_read:
        case Intrinsics::packet_write:
        case Intrinsics::packet_write_masked:
        case Intrinsics::packet_resize_ingress:
        case Intrinsics::packet_bounded_length:
        case Intrinsics::packet_resize_egress:
          set_nt_call(call, intr);
          break;

        /* we have generated these and do not need to do anything here */
        case Intrinsics::channel_try_read:
        case Intrinsics::channel_write:
        case Intrinsics::thread_wait:
          break;

        default:
          errs() << "ERROR: Unknown Nanotube call " << *call
                 << " in function " << func->getName() << ".  Aborting.\n";
          exit(1);
      };
    } // instructions
  } // basic blocks
}

/**
 * Check whether a value is used inside this stage, or part of the live-out
 * value.  Returns true if so, and will also update live_out_it to point to
 * the live-out location of this value if present, live_out_val.end() if
 * not there.
 */
bool stage_function_t::value_used(Value* v, std::vector<Value*>::iterator* live_out_it) {
  *live_out_it = live_out_val.end();
  for( auto it = live_out_val.begin(); it != live_out_val.end(); ++it ) {
    if( *it == v )
      *live_out_it = it;
  }
  return (!v->use_empty() || *live_out_it != live_out_val.end());
}

void stage_function_t::convert_packet_read(Value* packet_word, BasicBlock* bypass_bb) {
  auto* m = func->getParent();

  assert(nt_call != nullptr);
  assert(nt_id   == Intrinsics::packet_read);
  packet_read_args pra(nt_call);

  /**
   * we have code that looks as follows:
   *
   * (pre-call app logic)
   * res = nanotube_packet_read(.. buf ...);
   * (post-call app logic)
   *
   * and we will translate it to the following:
   * (allocate static read logic buffer global)
   * (allocate static read data buffer global)
   * ..
   * (pre-call app logic)
   * (prepare read req structure)
   * nanotube_tap_packet_read(#buffer_bytes, #buffer_index_bits, &resp,
   *                          @static_read_buffer, &state, packet_word, req)
   * valid = load resp.valid
   * br valid, app_code, bypass_bb
   *
   * app_code:
   *   memcpy(%buf, @static_read_buffer)
   *   (post-call app logic)
   *   ...
   * bypass_bb:
   *   ...
   *   ret
   */

  /* Split the basic block just after the packet_read to split off the
   * post-call app logic */
  auto* pre_call_bb  = nt_call->getParent();
  auto* post_call_bb = split_app_bb(pre_call_bb, nt_call,
                         pre_call_bb->getName() + ".post");
  pre_call_bb->setName(post_call_bb->getName() + ".pre");
  pre_call_bb->getTerminator()->eraseFromParent();

  IRBuilder<> ir(pre_call_bb);
  /* put together the required state */
  unsigned n = 0;
  if( isa<ConstantInt>(pra.length) ) {
    n = cast<ConstantInt>(pra.length)->getZExtValue();
  } else {
    auto it = size_arg_to_max.find(pra.length);
    if( it != size_arg_to_max.end() && (it->second >= 0)) {
      n = (unsigned)it->second;
    } else {
      errs() << "ERROR: Expecting packet_read length to be a constant or "
             << "parsed successfully. " << *pra.length <<" Handle it!\n";
      exit(1);
    }
  }
  assert(n < 65536);
  uint16_t buf_len      = (uint16_t)n;
  uint8_t  buf_idx_bits =  1;
  while( (1 << buf_idx_bits) < buf_len )
    buf_idx_bits++;
  auto* res_buf_len      = ir.getInt16(buf_len);
  auto* res_buf_idx_bits = ir.getInt8(buf_idx_bits);
  auto* resp_ty          = get_nt_tap_packet_read_resp_ty(*m);
  auto* resp             = ir.CreateAlloca(resp_ty , 0, "resp");
  auto* res_buf          = ir.CreateBitCast(get_static_packet_read_data(n),
                                            ir.getInt8PtrTy());
  auto* tap_state        = get_static_packet_read_tap_state();
  auto* req_ty           = get_nt_tap_packet_read_req_ty(*m);
  auto* req              = ir.CreateAlloca(req_ty, 0, "req");

  /* fill the request structure with arguments from the original call */
  auto* req_valid       = ir.CreateStructGEP(req_ty, req, 0, "req.valid.p");
  auto* req_read_offset = ir.CreateStructGEP(req_ty, req, 1, "req.read_offset.p");
  auto* req_read_length = ir.CreateStructGEP(req_ty, req, 2, "req.read_length.p");
  ir.CreateStore(ir.getInt8(1), req_valid);
  ir.CreateStore(ir.CreateTrunc(pra.offset, ir.getInt16Ty()),
                 req_read_offset);
  ir.CreateStore(ir.CreateTrunc(pra.length, ir.getInt16Ty()),
                 req_read_length);

  /* Actually call the tap */
  auto* tap     = create_tap_packet_read(*m, get_bus_type());
  auto* tap_ty  = get_tap_packet_read_ty(*m);
  Value* args[] = { res_buf_len, res_buf_idx_bits, resp, res_buf,
                    tap_state, packet_word, req };
  ir.CreateCall(tap_ty, tap, args);

  /* Parse the response */
  auto* resp_valid = ir.CreateStructGEP(resp_ty, resp, 0, "resp.valid.p");
  auto* valid_i8   = ir.CreateLoad(ir.getInt8Ty(), resp_valid, "resp.valid.i8");
  auto* valid      = ir.CreateTrunc(valid_i8, ir.getInt1Ty(), "resp.valid");
  ir.CreateCondBr(valid, post_call_bb, bypass_bb);

  /* Copy out the data to the application buffer at the beginning of the
   * post_call_bb */
  ir.SetInsertPoint(post_call_bb, post_call_bb->getFirstInsertionPt());
  ir.CreateMemCpy(pra.data_out, 0, res_buf, 0, n);

  /* Patch up the post-call logic */
  ir.SetInsertPoint(nt_call);
  auto lov_it = live_out_val.end();
  if( value_used(nt_call, &lov_it) ) {
    /* If anyone was interested in the length, read that value from the
     * response */
    auto* resp_res_len = ir.CreateStructGEP(resp_ty, resp, 1, "resp.result_length.p");
    Value* res_len     = ir.CreateLoad(resp_ty->getElementType(1), resp_res_len,
                                       "resp.result_length");
    if( res_len->getType() != nt_call->getType() )
      res_len = ir.CreateZExt(res_len, nt_call->getType());
    nt_call->replaceAllUsesWith(res_len);

    /* And also adjust this in the live-out state */
    if( lov_it != live_out_val.end() )
      *lov_it = res_len;
    // NOTE: Cannot be part of live_out_mem
  }
  nt_call->eraseFromParent();
}

Value* stage_function_t::convert_packet_write(Value* packet_word) {
  auto* m = func->getParent();

  assert(nt_call != nullptr);
  assert((nt_id == Intrinsics::packet_write) ||
         (nt_id == Intrinsics::packet_write_masked));
  packet_write_args pwa(nt_call);

  /**
   * we have code that looks as follows:
   *
   * (pre-call app logic)
   * res = nanotube_packet_write(...);
   * (post-call app logic)
   *
   * and we will translate it to the following:
   * (allocate static write logic buffer global)
   * ..
   * (pre-call app logic)
   * (prepare write req structure)
   * nanotube_tap_packet_write(#buffer_bytes, #buffer_index_bits,
   *                           &packet_word_out, &state,
   *                           &packet_word_in, &req,
   *                           &data_in, &mask_in)
   * (post-call app logic)
   */

  IRBuilder<> ir(nt_call);
  /* put together the required state */
  unsigned n = 0;
  if( isa<ConstantInt>(pwa.length) ) {
    n = cast<ConstantInt>(pwa.length)->getZExtValue();
  } else {
    auto it = size_arg_to_max.find(pwa.length);
    if( it != size_arg_to_max.end() && (it->second >= 0)) {
      n = (unsigned)it->second;
    } else {
      errs() << "ERROR: Expecting packet_write length to be a constant or "
             << "parsed successfully. " << *pwa.length <<" Handle it!\n";
      exit(1);
    }
  }

  /* Get the maximum length of the buffer and its log2 to get the number of
   * bits needed for addressing in the tap. This needs to be a compile time
   * constant. In cases where multiple sizes are possible, we would have to
   * find the maximum and put that in here. */
  assert(n < 65536);
  uint16_t buf_len      = (uint16_t)n;
  uint8_t  buf_idx_bits =  1;
  while( (1 << buf_idx_bits) < buf_len )
    buf_idx_bits++;

  auto* req_buf_len      = ir.getInt16(buf_len);
  auto* req_buf_idx_bits = ir.getInt8(buf_idx_bits);
  auto* packet_word_out  = ir.CreateAlloca(ir.getInt8Ty(),
                                           ir.getInt64(get_bus_word_size()),
                                           "packet_word.out");
  auto* tap_state        = get_static_packet_write_tap_state();
  auto* req_data_in      = pwa.data_in;
  unsigned mask_size     = (buf_idx_bits + 7) / 8;
  auto* req_mask_in      = pwa.mask;
  if( req_mask_in == nullptr ) {
    req_mask_in = ir.CreateAlloca(ir.getInt8Ty(),
                                  ir.getInt32(mask_size), "mask");
    ir.CreateMemSet(req_mask_in, ir.getInt8(0xff), mask_size, 1);
  }

  /* fill the request structure with arguments from the original call */
  auto* req_ty           = get_nt_tap_packet_write_req_ty(*m);
  auto* req              = ir.CreateAlloca(req_ty, 0, "req");
  auto* req_valid        = ir.CreateStructGEP(req_ty, req, 0, "req.valid.p");
  auto* req_write_offset = ir.CreateStructGEP(req_ty, req, 1, "req.write_offset.p");
  auto* req_write_length = ir.CreateStructGEP(req_ty, req, 2, "req.write_length.p");
  ir.CreateStore(ir.getInt8(1), req_valid);
  /* NOTE: The below two do not have to be compile time constants! */
  ir.CreateStore(ir.CreateTrunc(pwa.offset, req_ty->getElementType(1)),
                 req_write_offset);
  ir.CreateStore(ir.CreateTrunc(pwa.length, req_ty->getElementType(2)),
                 req_write_length);

  /* Actually call the tap */
  auto* tap     = create_tap_packet_write(*m, get_bus_type());
  auto* tap_ty  = get_tap_packet_write_ty(*m);
  Value* args[] = { req_buf_len, req_buf_idx_bits,
                    packet_word_out, tap_state,
                    packet_word, req, req_data_in,
                    req_mask_in };
  ir.CreateCall(tap_ty, tap, args);

  auto lov_it = live_out_val.end();
  if( value_used(nt_call, &lov_it) ) {
    errs() << "FIXME: packet_write has a user which is not supported by the tap. Users:\n";
    for( auto* u : nt_call->users() )
      errs() << "  " << *u << '\n';
    abort();
  }
  nt_call->eraseFromParent();
  return packet_word_out;
}

void stage_function_t::convert_packet_length(Value* packet_word, BasicBlock* bypass_bb) {
  auto* m = func->getParent();

  assert(nt_call != nullptr);
  assert(nt_id   == Intrinsics::packet_bounded_length);
  packet_bounded_length_args pbla(nt_call);

  /**
   * we have code that looks as follows:
   *
   * (pre-call app logic)
   * res = nanotube_packet_bounded_length(...);
   * (post-call app logic)
   *
   * and we will translate it to the following:
   * (allocate static length logic buffer global)
   * ..
   * (pre-call app logic)
   * (prepare length req structure)
   * nanotube_tap_packet_length(&resp, &state, packet_word, req)
   * valid = load resp.valid
   * br valid, app_code, bypass_bb
   * app_code:
   *   res = load resp.result_length;
   *   (post-call app logic)
   *   ...
   * bypass_bb:
   *   ...
   *   ret
   */

  /* Split the basic block just after the packet_bounded_length to split
   * off the post-call app logic */
  auto* pre_call_bb  = nt_call->getParent();
  auto* post_call_bb = split_app_bb(pre_call_bb, nt_call,
                         pre_call_bb->getName() + ".post");
  pre_call_bb->setName(post_call_bb->getName() + ".pre");
  pre_call_bb->getTerminator()->eraseFromParent();

  IRBuilder<> ir(pre_call_bb);

  /* put together the required state */
  auto* resp_ty          = get_nt_tap_packet_length_resp_ty(*m);
  auto* resp             = ir.CreateAlloca(resp_ty , 0, "resp");
  auto* tap_state        = get_static_packet_length_tap_state();
  auto* req_ty           = get_nt_tap_packet_length_req_ty(*m);
  auto* req              = ir.CreateAlloca(req_ty, 0, "req");

  /* fill the request structure with arguments from the original call */
  auto* req_valid      = ir.CreateStructGEP(req_ty, req, 0, "req.valid.p");
  auto* req_max_length = ir.CreateStructGEP(req_ty, req, 1, "req.max_length.p");
  ir.CreateStore(ir.getInt8(1), req_valid);
  auto* length_ty = req_ty->getElementType(1);
  /* Check the easy cases for overflow before truncating! */
  if( isa<ConstantInt>(pbla.max_length) ) {
    auto* ci  = cast<ConstantInt>(pbla.max_length);
    auto  len = ci->getZExtValue();
    auto  cix = cast<ConstantInt>(ci->getAllOnesValue(length_ty));
    auto  max = cix->getZExtValue();
    if( len > max) {
      errs() << "ERROR: Constant " << *ci << " does not fit " << *length_ty
             << " when converting " << *nt_call << "\nin stage "
             << func->getName() << " Aborting!\n";
      abort();
    }
  }
  ir.CreateStore(ir.CreateTrunc(pbla.max_length, length_ty),
                 req_max_length);

  /* Actually call the tap */
  auto* tap     = create_tap_packet_length(*m, get_bus_type());
  auto* tap_ty  = get_tap_packet_length_ty(*m);
  Value* args[] = { resp, tap_state, packet_word, req };
  ir.CreateCall(tap_ty, tap, args);

  /* Parse the response */
  auto* resp_valid = ir.CreateStructGEP(resp_ty, resp, 0, "resp.valid.p");
  auto* valid_i8   = ir.CreateLoad(ir.getInt8Ty(), resp_valid, "resp.valid.i8");
  auto* valid      = ir.CreateTrunc(valid_i8, ir.getInt1Ty(), "resp.valid");
  ir.CreateCondBr(valid, post_call_bb, bypass_bb);

  LLVM_DEBUG(dbgs() << "Pre-call BB: " << *pre_call_bb << '\n');

  /* Patch up the post-call logic */
  ir.SetInsertPoint(nt_call);
  auto live_out_it = live_out_val.end();
  for( auto it = live_out_val.begin(); it != live_out_val.end(); ++it ) {
    if( *it == nt_call )
      live_out_it = it;
  }
  auto lov_it = live_out_val.end();
  if( value_used(nt_call, &lov_it) ) {
    /* If the length is actually being used, read it off the response */
    auto* resp_res_len = ir.CreateStructGEP(resp_ty, resp, 1, "resp.result_length.p");
    Value* res_len     = ir.CreateLoad(resp_ty->getElementType(1), resp_res_len,
                                       "resp.result_length");
    if( res_len->getType() != nt_call->getType() )
      res_len = ir.CreateZExt(res_len, nt_call->getType());
    nt_call->replaceAllUsesWith(res_len);

    /* And also adjust this in the live-out state */
    if( lov_it != live_out_val.end() )
      *lov_it = res_len;
    // NOTE: Cannot be part of live_out_mem
  }
  LLVM_DEBUG(dbgs() << "Call BB: " << *nt_call->getParent() << '\n');
  nt_call->eraseFromParent();
}

void stage_function_t::convert_packet_resize_ingress(Value* packet_word) {
  auto* m = func->getParent();

  assert(nt_call != nullptr);
  assert(nt_id   == Intrinsics::packet_resize_ingress);
  packet_resize_ingress_args pria(nt_call);

  /**
   * A few more thoughts on this: the request must not change throughout
   * the packet.  That requires two things: the request must be made with
   * the first packet word present, and it must not change with subsequent
   * words.  The current code does that by having the application live-in
   * state static and available with the first packet word.  That means
   * the application code will run with every packet word and produce
   * exactly the same request throughout the packet.
   *
   * Another option would be to have the resize request in a static
   * variable and simply reuse that across packet words.  Given that the
   * current design has the application in-state as static, it should be
   * okay to do it the first way.
   *
   * This may need revisiting when there is actual logic (other than
   * unmarshalling) and potentially other taps in the same stage as and
   * before the resize_ingress.
   *
   * Further, the code that reads a packet already keeps track of the
   * packet end and when new app state needs to be read; therefore, we can
   * ignore the packet_done boolean return value!
   */

  /**
   * we have code that looks as follows:
   *
   * (pre-call app logic)
   * res = nanotube_packet_resize_ingress(...);
   * <nothing>
   *
   * and we will translate it to the following:
   * (allocate static resize ingress state global)
   * ..
   * (pre-call app logic)
   * resize_req req;
   * req.offset = offset;
   * req.insert = (adjust > 0) ? adjust : 0;
   * req.delete = (adjust < 0) ? -adjust : 0;
   * bool packet_done;
   * resize_cword cword;
   * nanotube_tap_packet_resize_ingress(&packet_done, &cword, &state, &req, packet_word)
   * (ignore packet_done)
   * write_channel(PACKET_OUT, packet_word)
   * write_channel(CWORD_OUT, cword
   * ...
   * (send app state with guard)
   *   ...
   *   ret
   */
  IRBuilder<> ir(nt_call);

  /* Prepare the request
   * NOTE: truncation required due to type mismatches! */
  auto* ty = get_nt_tap_packet_resize_req_ty(*m);
  auto* offs_ty       = ty->getElementType(0);
  auto* zero          = ConstantInt::get(offs_ty, 0);
  auto* resize_req    = ir.CreateAlloca(ty, nullptr, "req");
  auto* write_offset  = ir.CreateStructGEP(ty, resize_req, 0, "req.write_offset");
  auto* delete_length = ir.CreateStructGEP(ty, resize_req, 1, "req.delete_length");
  auto* insert_length = ir.CreateStructGEP(ty, resize_req, 2, "req.insert_length");

  auto* off_trunc = ir.CreateTrunc(pria.offset, offs_ty);
  ir.CreateStore(off_trunc, write_offset);

  auto* adj_trunc = ir.CreateTrunc(pria.adjust, offs_ty);
  auto* is_extend = ir.CreateICmpSGT(adj_trunc, zero, "is_extend");
  auto* del_len   = ir.CreateSelect(is_extend, zero,
                                    ir.CreateNeg(adj_trunc), "del_len");
  ir.CreateStore(del_len, delete_length);
  auto* ins_len   = ir.CreateSelect(is_extend, adj_trunc, zero, "ins_len");
  ir.CreateStore(ins_len, insert_length);

  /* Create the remaining parameters: cword, state, out_bool */
  auto* pdo   = ir.CreateAlloca(ir.getInt8Ty(), nullptr, "packet_done_out");
  auto* cword = ir.CreateAlloca(get_nt_tap_packet_resize_cword_ty(*m),
                                nullptr, "cword");
  auto* state = get_static_packet_resize_ingress_tap_state();

  /* Prepare and do the call */
  auto* ing_ty = get_tap_packet_resize_ingress_ty(*m);
  auto* ing_f  = create_tap_packet_resize_ingress(*m, get_bus_type());
  Value* args[] = { pdo, cword, pria.packet_length_out, state, resize_req, packet_word };
  ir.CreateCall(ing_ty, ing_f, args);

  /* Write out the cword */
  write_cword(cword, nt_call);

  nt_call->eraseFromParent();

  LLVM_DEBUG(dbgs() << "Updated resize-ingress BB: "
                    << *ir.GetInsertBlock());
}

Value*
stage_function_t::convert_packet_resize_egress(Value* packet_word, BasicBlock* bypass_bb) {
  auto* m = func->getParent();
  auto& c = m->getContext();

  assert(nt_call != nullptr);
  assert(nt_id   == Intrinsics::packet_resize_egress);
  packet_resize_egress_args prea(nt_call);

  /**
   * We get the following code:
   *
   * pre-call logic
   *   app-state read (exits if unsuccessful, reads once per packet)
   *   packet word read (exits if unsuccessful, reads every packed word)
   *   unmarshalling code of live-in state (does NOT contain cword)
   *   hardly any additional pre-call logic (back-to-back ingress; egress)
   * %val = nanotube_packet_resize_egress(packet_word, cword, new_length)
   * post-call app-code
   *
   * this will be translated into:
   * (static resize-egress state)
   * (static packet_word)
   * (static have_packet_word)
   * (static cword)
   * (static have_cword)
   * ...
   * changed packet_word read: static packet_word, static
   *                           have_packet_word
   * pre-call logic
   * checked_read( cword ...)
   *   br have_cword, egress_contrd, read_cword
   *
   *   read_cword:
   *   try_channel_read(CWORD_IN, &cword)
   *   br success, read_cword_success, thread_exit
   *
   *   read_cword_success:
   *   store 1, have_cword
   *   br egress_contd
   *
   * egress_contd:
   * input_done      = CreateAlloca(..)
   * packet_valid    = CreateAlloca(..)
   * packet_word_out = CreateAlloca(..)
   * nanotube_tap_packet_resize_egress(input_done, packet_done,
   *                                   packet_valid,
   *                                   packet_word_out, state, cword,
   *                                   packet_word_in, new_packet_len)
   * %eop = load packet_done
   * %not_eop = neg %eop
   * store %not_eop, @have_app_state
   * %id = load input_done
   * br %id, reset_have_cw_pw, packet_valid_check
   *
   * reset_have_cw_pw:
   * store 0, have_packet
   * store 0, have_cword
   * br packet_valid_check
   *
   * packet_valid_check:
   * %pv = load packet_valid
   * br %pv, app_logic, stage_exit (-> do not even send the packet word!!)
   *
   * app_logic:
   * replace uses of packet_word_in with packet_word_out
   */
  auto* have_pw    = get_static_have_packet_word();
  auto* cword_ty   = get_nt_tap_packet_resize_cword_ty(*m);
  auto* cword      = get_static_packet_resize_egress_cword();
  auto* have_cword = get_static_packet_resize_egress_have_cword();

  /* Get the cword, if we don't have it already */
  checked_read(cword, cword_ty, have_cword, CWORD_IN, "cword", nt_call);

  /* Prepare the data structures for the tap call */
  IRBuilder<> ir(nt_call);
  auto* input_done      = ir.CreateAlloca(ir.getInt8Ty(), nullptr, "input_done");
  auto* packet_done     = ir.CreateAlloca(ir.getInt8Ty(), nullptr, "packet_done");
  auto* packet_valid    = ir.CreateAlloca(ir.getInt8Ty(), nullptr, "packet_valid");
  auto* packet_word_out = ir.CreateAlloca(ir.getInt8Ty(),
                                          ir.getInt64(get_bus_word_size()),
                                          "packet_word_out");
  auto* state           = get_static_packet_resize_egress_tap_state();
  auto* packet_word_in  = get_static_packet_word();
  auto* state_pw = get_static_packet_resize_egress_tap_state_packet_word();
  //assert(packet_word == packet_word_in);

  /* Prepare and to the call to nanotube_tap_packet_resize_egress */
  auto* resize_egress_ty = get_tap_packet_resize_egress_ty(*m);
  auto* resize_egress_f  = create_tap_packet_resize_egress(*m, get_bus_type());
  Value* args[] = { input_done, packet_done, packet_valid, packet_word_out,
                    state, ir.CreateBitCast(state_pw, ir.getInt8PtrTy()),
                    cword,
                    ir.CreateBitCast(packet_word_in, ir.getInt8PtrTy()),
                    prea.new_length };

  ir.CreateCall(resize_egress_ty, resize_egress_f, args);

  /* Update state for whether it's the eop or not after resizing */
  auto* eop = ir.CreateLoad(packet_done, "eop");
  word_is_eop = ir.CreateTrunc(eop, ir.getInt1Ty());
  //? ir.CreateStore(word_is_eop, get_static_word_is_eop());

  /* Once we have processed the entire input, it is ready to read a new app
   * state */
  if(has_live_in()) {
    auto* zero = ConstantInt::get(eop->getType(), 0);
    auto* not_eop = ir.CreateICmpEQ(eop, zero, "not_eop");
    ir.CreateStore(not_eop, get_static_have_app_state());
  }

  /* If the tap is done with the input word, reset the guards for packet
   * and cword */
  auto* id_i8 = ir.CreateLoad(input_done, "input_done.v.i8");
  auto* id    = ir.CreateTrunc(id_i8, ir.getInt1Ty(), "input_done.v");

  /* Split and conjure up new basic blocks */
  auto* bb_egress = nt_call->getParent();
  bb_egress->setName("resize_egress_" + stage_name);
  auto* bb_packet_valid_check = split_app_bb(bb_egress, nt_call);
  bb_packet_valid_check->setName("packet_valid_check_" + stage_name);
  auto* bb_reset_have_cw_pw =
    BasicBlock::Create(c, "reset_have_cw_pw_" + stage_name, func,
                       bb_packet_valid_check);

  /* Replace the bb_egress -> bb_packet_valid_check branch with a
   * conditional which can branch off to the code that resets have_cword
   * and have_packet_word */
  bb_egress->getTerminator()->eraseFromParent();
  BranchInst::Create(bb_reset_have_cw_pw, bb_packet_valid_check, id,
                     bb_egress);

  /* Fill the reset_have_cw_pw basic block: reset the have_cword &
   * have_packet_word statics */
  ir.SetInsertPoint(bb_reset_have_cw_pw);
  ir.CreateStore(ir.getFalse(), have_cword);
  ir.CreateStore(ir.getFalse(), have_pw);
  ir.CreateBr(bb_packet_valid_check);

  /* Check that the packet word produced by the tap is actually valid */
  ir.SetInsertPoint(nt_call);
  auto* pvo_i8 = ir.CreateLoad(packet_valid, "packet_valid.v.i8");
  auto* pvo    = ir.CreateTrunc(pvo_i8, ir.getInt1Ty(), "packet_valid.v");
  auto* bb_app_logic = split_app_bb(bb_packet_valid_check, nt_call);
  bb_app_logic->setName("app_logic_" + stage_name);
  bb_packet_valid_check->getTerminator()->eraseFromParent();
  BranchInst::Create(bb_app_logic, stage_exit, pvo, bb_packet_valid_check);

  /* Clean-up: Now remove the old call and check that no one inspected the
   * return value; also remove the unused Cword allocation on the stack */
  if( !nt_call->user_empty() ) {
    errs() << "\nWARNING: The Pipeline pass and Packet Resize Tap do not "
           << "return a meaningful error code for: " << *nt_call << '\n'
           << "The following " << "instructions are using that value:\n";
    std::vector<User*> users;
    for( auto* u : nt_call->users() ) {
      errs() << "  " << *u << '\n';
      users.push_back(u);
    }
    errs() << "Working around this using a \"fake\" success code:\n";
    nt_call->replaceAllUsesWith(ir.getInt32(1));
    errs() << "  " << users << '\n';
    /* Also check the live-out state, in case the packet_resize_egress was
     * live */
    for( auto& v : live_out_val ) {
      if( v == nt_call ) {
        errs() << "Also replacing live-out value: " << *v << " with constant.\n";
        v = ir.getInt32(1);
      }
    }
    errs() << "Tracked as NANO-205\n";

  }
  nt_call->eraseFromParent();
  /* Also remove the useless Alloca for the cword content */
  if( !prea.cword->user_empty() ) {
    errs() << "ERROR: Unexpected uses of original Cword allocation "
           << *prea.cword << " found:\n";
    for( auto* u : prea.cword->users() )
      errs() << "  " << *u << '\n';
    errs() << "Aborting!\n";
    abort();
  }
  cast<Instruction>(prea.cword)->eraseFromParent();

  /* The new packet word in the application code is packet_word_out! */
  return packet_word_out;
}

void
stage_function_t::convert_packet_drop() {
  /* All the magic happens in the epilogue code for this. Just do some
   * sanity checks here. */
  assert(nt_call != nullptr);
  assert(nt_id   == Intrinsics::packet_drop);

  /* If this stage had live out, we would have to gate sending the live
   * state based on the drop value, too. */
  assert(!has_live_out());

  assert(drop_val != nullptr);
  packet_drop_args pda(nt_call);
  assert(drop_val == pda.drop);

  nt_call->eraseFromParent();
}

static void
handle_map_op_users(Instruction* map_op,
                    std::vector<llvm::Value*>::iterator lov_it,
                    stage_function_t* stage) {
  /* We have a user that checks the map_op / map_op_recv; this can happen
   * when translating EBPF's map_lookup + NULL check logic where the code
   * will do a one byte read to see if the entry was there.  Taps, on the
   * other hand, provide a more semantic return code.  For now, recompute a
   * "length" value by relying on the fact that the semantic return codes
   * are non-zero on success and when a non-zero length would have been
   * read.  We know from earlier checks that the application logic does not
   * do anything fancy here! */

  IRBuilder<> ir(map_op);
  auto* res = ir.CreateLoad(ir.getInt32Ty(),
                            stage->get_static_map_result(),
                            "map_op_res");
  static_assert(NANOTUBE_MAP_RESULT_ABSENT  == 0);
  static_assert(NANOTUBE_MAP_RESULT_PRESENT != 0);
  auto* res_cast = ir.CreateZExtOrTrunc(res, map_op->getType());
  map_op->replaceAllUsesWith(res_cast);

  /* Also replace in the live-out values */
  if( lov_it != stage->live_out_val.end() )
    *lov_it = res_cast;
}

void
stage_function_t::convert_map_op_receive() {
  /* Most of the work was done in the prologue already, so just replace the
   * map_op with a memcpy from the static buffer that contains the map
   * response into the application provided buffer. */
  assert(nt_call != nullptr);
  assert(nt_id == Intrinsics::map_op_receive);

  auto* map_op = nt_call;
  map_op_receive_args moa(map_op);

  IRBuilder<> ir(map_op);
  auto idx = setup->contexts()[0].map_user_id_to_idx(moa.map);

  auto& map_info = setup->get_map_info(idx);
  auto* value_sz    = map_info.args().value_sz;
  auto* data_length = moa.data_length;

  auto* m = func->getParent();
  const auto& dl = m->getDataLayout();

  Instruction* memcpy = nullptr;
  /* Only copy out if caller expects something and provides a buffer */
  if( !isa<ConstantPointerNull>(moa.data_out) ) {
    auto* dst_ty = cast<PointerType>(moa.data_out->getType());
    auto* src_ty = cast<PointerType>(get_static_map_resp_data()->getType());

    LLVM_DEBUG(
      dbgs() << "Map value_sz: " << *value_sz
             << " map_op data_length: " << *data_length
             << " Type size (dst): "
             << dl.getTypeStoreSize(dst_ty->getElementType())
             << " Type size (src): "
             << dl.getTypeStoreSize(src_ty->getElementType())
             << '\n');
    memcpy = ir.CreateMemCpy(moa.data_out, 0,
                             get_static_map_resp_data(), 0,
                             data_length);
  }

  /* Delete the original call */
  auto lov_it = live_out_val.end();
  if( value_used(map_op, &lov_it) )
    handle_map_op_users(map_op, lov_it, this);
  assert(map_op->user_empty());

  LLVM_DEBUG(
    if( memcpy != nullptr )
      dbgs() << "Replacing " << *nt_call << "\nwith " << *memcpy << '\n';
    else
      dbgs() << "Dropping " << *nt_call << "\ndata_out was not needed.\n";
  );
  nt_call->eraseFromParent();
  nt_call = memcpy;
}

static void
replace_func_with_call_to_stage(Function& f,
                                ArrayRef<stage_function_t*> stages)
                                __attribute__((unused));
void
replace_func_with_call_to_stage(Function& f,
                                ArrayRef<stage_function_t*> stages) {
  Module* m      = f.getParent();
  LLVMContext& c = m->getContext();

  /* Delete and rebuild original function to call the 1st pipeline stage */
  std::vector<BasicBlock*> to_erase;
  for( auto& bb : f )
    to_erase.push_back(&bb);

  for( auto* bb : to_erase ) {
    /* Do a sanity check that values are not used in other funcitons */
    for( auto& inst : *bb ) {
      bool out_of_func_users = false;
      for( auto* u : inst.users() ) {
        auto& ui = cast<Instruction>(*u);
        out_of_func_users |= (ui.getFunction() != &f);
      }
      if( out_of_func_users ) {
        errs() << "ERROR: Inst: " << inst << " has users in other functions:\n";
        for( auto* u : inst.users() ) {
          auto& ui = cast<Instruction>(*u);
          errs() << "  Use: " << *u << " In: " << ui.getFunction()->getName() << '\n';
        }
        errs() <<"\nAborting!\n";
        assert(!out_of_func_users);
      }
    }

    bb->eraseFromParent();
  }

  IRBuilder<> ir(BasicBlock::Create(c, "entry", &f));

  /* Create live-in / out state containers before calling all stages */
  std::vector<Value*> live_out;
  for( auto it = stages.begin(); it != stages.end(); it++ ) {
    auto* stage = *it;
    auto* next  = *(it + 1);

    if( !stage->has_live_out() ) {
      /* No live out, make sure the next stage has no live in. */
      assert( it + 1 == stages.end() || !next->has_live_in() );
      live_out.emplace_back(ConstantPointerNull::get(ir.getInt8PtrTy()));
    } else {
      /* Have a live out, make sure it matches the next live-in */
      /* .. and allocate the state for it. */
      assert( it + 1  != stages.end() && next->has_live_in());
      assert(stage->get_live_out_ty() == next->get_live_in_ty());

      auto* alloca = ir.CreateAlloca(stage->get_live_out_ty(), 0,
                                     stage->stage_name + "_to_" +
                                     next->stage_name + "_state");
      live_out.emplace_back(ir.CreateBitCast(alloca, ir.getInt8PtrTy(),
                              stage->stage_name + "_to_" +
                              next->stage_name + "_voidp"));
    }
  }
  ir.CreateRetVoid();

  LLVM_DEBUG(dbgs() << "Rebuilt function: " << f);
}

/**
 * Create the pipeline stage epilogue code.
 *
 * The pipeline state epilogue code consists of these components:
 * - send application live state (optional)
 *   - check whether application state needs sending (only 1x per overall
 *     packet) (optional, packet read tap may gate)
 *   - marshal the live-out state
 *   - send the marshalled live-out state
 * - send the packet word
 *
 * The function returns multiple basic blocks via out arguments:
 * - out_app_entry is the basic block that should be jumped to when the
 *   application code ran (e.g. packet read tap completed the request)
 * - out_bypass_entry should be branched to in case the application code is
 *   skipped
 */
void
Pipeline::create_stage_epilogue(stage_function_t* stage, Value* packet_word,
                      std::vector<Value*>& live_out_val,
                      std::vector<MemoryLocation>& live_out_mem,
                      BasicBlock** out_app_entry,
                      BasicBlock** out_bypass_entry) {
  auto* m = stage->func->getParent();
  auto& c = m->getContext();

  auto* exit_bb = BasicBlock::Create(c, stage->stage_name + "_epilogue",
                                    stage->func, stage->stage_exit);
  *out_app_entry    = exit_bb;
  *out_bypass_entry = exit_bb;
  BranchInst::Create(stage->stage_exit, exit_bb);

  bool per_packet_send = stage->has_live_out() || stage->is_map_request();

  if( per_packet_send ) {
    /* Generate code to marshal and send the live-out state */
    auto* send_bb = BasicBlock::Create(c, stage->stage_name + "_app_epilogue",
                                       stage->func, exit_bb);
    *out_app_entry = send_bb;

    /* Additional guard to make sure we only send the app state once per
     * packet */
    if( stage->needs_app_send_guard() ) {
      auto* guard_bb = BasicBlock::Create(c, stage->stage_name +
                                          "_app_send_guard", stage->func,
                                          send_bb);
      *out_app_entry = guard_bb;
      IRBuilder<> guard(guard_bb);
      auto* sent_g  = stage->get_static_sent_app_state();
      auto* sent_app_state = guard.CreateLoad(guard.getInt1Ty(),
                               sent_g,
                               stage->stage_name + "sent_app_state");

      /* Reset the sent_app_state field whenever we see an EOP as follows:
       *
       * The naive version:
       * if(!sent_app_state) {
       *   channel_write(.. state ..)
       *   sent_app_state := 1
       * }
       * if(eop)
       *   sent_app_state := 0;
       *
       * Can be simplified as follows (because there is no chance for
       * channel_write to fail), but that needs some squinting to realise:
       *
       * cur_sent_app_state = sent_app_state;
       * sent_app_state = !eop
       * if( !cur_sent_app_state ) {
       *   channel_write(.. state ..)
       * }
       */

      auto* not_eop = guard.CreateNot(stage->word_is_eop, "not_eop");
      guard.CreateStore(not_eop, sent_g);

      /* If we have already sent, skip to exit_bb, otherwise send_bb */
      guard.CreateCondBr(sent_app_state, exit_bb, send_bb);
    }

    IRBuilder<> ir(send_bb);
    /* Send live state if needed */
    if( stage->has_live_out() ) {
      ir.SetInsertPoint(send_bb);
      auto* live_state_ty = get_live_state_type(c, live_out_val,
                                                live_out_mem);
      assert(live_state_ty == stage->live_out_ty);
      auto* out_state = ir.CreateAlloca(live_state_ty, 0, ir.getInt32(1),
                                        "live_out_state");
      stage->live_out_ty  = live_state_ty;
      stage->marshal_live_state(live_out_val, live_out_mem, live_state_ty,
                                out_state, send_bb);
      ir.SetInsertPoint(send_bb);
      /* Create a temporary end of BB branch before which write_app_state
       * can generate its code. */
      auto* nop = ir.CreateBr(send_bb);
      stage->write_app_state(out_state, nop);
      /* Delete it afterwards because we're potentially generating more
       * code in this BB. */
      nop->eraseFromParent();
    }

    /* Send the map request if needed */
    if( stage->is_map_request() ) {
      ir.SetInsertPoint(send_bb);
      /* Parse out the information for the map request */
      assert(stage->map_op_req != nullptr);
      map_op_send_args moa(stage->map_op_req);

      Value* map_ptr   = stage->get_map_ptr(moa.map);
      Value* client_id =
        ConstantInt::get(Type::getInt32Ty(c),
                         stage->get_client_id(moa.map, false));

      LLVM_DEBUG(
        dbgs() << "Sending map request with key " << *moa.key
               << " data " << *moa.data_in
               << " size " << *moa.data_length
               << " map ptr: " << *map_ptr
               << " client ID: " << *client_id << '\n');

      auto buf_size = cast<ConstantInt>(moa.data_length);
      if (isa<ConstantPointerNull>(moa.data_in))
        assert(buf_size->isZero());
      stage->map_req_buf_size = buf_size;

      Value* args[6];
      args[0] = stage->get_context_arg();
      args[1] = map_ptr;
      args[2] = client_id;
      args[3] = moa.type;
      args[4] = ir.CreateBitCast(moa.key, ir.getInt8PtrTy(),
                                 moa.key->getName() + "_voidp");
      args[5] = ir.CreateBitCast(moa.data_in, ir.getInt8PtrTy(),
                                 moa.data_in->getName() + "_voidp");

      auto* send_f = create_nt_tap_map_send_req(*m);
      auto* ty = get_nt_tap_map_send_req_ty(*m);

      auto* send = ir.CreateCall(ty , send_f, args);
      LLVM_DEBUG(dbgs() << "Call: " << *send << '\n');

      /* Delete the map request instruction */
      stage->map_op_req->eraseFromParent();
      stage->map_op_req = nullptr;
    }
    ir.CreateBr(exit_bb);
  } else {
    LLVM_DEBUG(dbgs() << "No live out state, not creating marshal basic block\n");
  }

  if( !stage->is_packet_drop() ) {
    /* Normal stages will always write the packet word */
    stage->write_packet_word(packet_word, exit_bb->getTerminator());
  } else {
    /* Drop stages must check whether the packet should be dropped */
    /**
     * exit_bb:
     *   something
     *   something_B
     *   exit_term
     */
    auto* exit_bb_post = exit_bb->splitBasicBlock(exit_bb->getTerminator(),
                           exit_bb->getName() + "_post");
    auto* pw_write_bb  = BasicBlock::Create(c, "cond_packet_word_write",
                                            stage->func, exit_bb_post);
    /**
     * exit_bb:
     *   something
     *   something_B
     *   br exit_bb_post
     *
     * pw_write_bb:
     *
     * exit_bb_post:
     *   exit_term
     */
    exit_bb->getTerminator()->eraseFromParent();
    IRBuilder<> ir(exit_bb);
    auto* zero = Constant::getNullValue(stage->drop_val->getType());
    auto* drop = ir.CreateICmpNE(stage->drop_val, zero, "drop");
    ir.CreateCondBr(drop, exit_bb_post, pw_write_bb);
    ir.SetInsertPoint(pw_write_bb);
    ir.CreateBr(exit_bb_post);
    /**
     * exit_bb:
     *   something
     *   something_B
     *   %drop = cmp_ne stage_drop, 0
     *   br %drop, exit_bb_post, pw_write_bb
     *
     * pw_write_bb:
     *   br exit_bb_post
     *
     * exit_bb_post:
     *   exit_term
     */

    stage->write_packet_word(packet_word, pw_write_bb->getTerminator());
  }
}

void
Pipeline::convert_to_taps(stage_function_t* stage) {
  /**
   * The flow for these is a little peculiar, because:
   * - dataflow needs to be constructed front to back; later instructions
   *   need to know about the values of earlier instructions
   * - control flow is constructed back to front: branches need to know
   *   about future basic blocks as targets
   *
   * That causes a slightly weird situation here where packet writes have
   * to be converted first, because they affect the data flow of the
   * packet word.  Then, the stage epilogue has to be created, because it
   * consumes the packet word, and also creates the basic blocks that the
   * packet read tap needs to branch to while reading from the right
   * (pre-write) packet_word.
   *
   * NOTE: This (implicitly) assumes that a stage is either a packet
   * read, or a packet write, or at least the packet write comes after
   * the packet read.
   */
  auto* packet_word_in  = stage->packet_word;
  auto* packet_word_out = packet_word_in;
  bool converted = false;

  auto& c = stage->stage_exit->getContext();
  BasicBlock* app_bypass = BasicBlock::Create(c, "app_bypass",
                                              stage->func);
  LLVM_DEBUG(dbgs() << "Converting stage " << stage->func->getName()
                    << " to the tap interface."
                    << "\n  Call: " << nullsafe(stage->nt_call)
                    << "\n  Function: "
                    << (stage->nt_call ?
                          stage->nt_call->getFunction()->getName() :
                          "<none>")
                    << '\n');
  if( stage->is_packet_read() ) {
    LLVM_DEBUG(dbgs() << "Packet read: " << *stage->nt_call << '\n');
    stage->convert_packet_read(packet_word_in, app_bypass);
    converted = true;
  } else if( stage->is_packet_write() ) {
    LLVM_DEBUG(dbgs() << "Packet write: " << *stage->nt_call << '\n');
    packet_word_out = stage->convert_packet_write(packet_word_in);
    converted = true;
  } else if( stage->is_packet_length() ) {
    LLVM_DEBUG(dbgs() << "Packet length: " << *stage->nt_call << '\n');
    stage->convert_packet_length(packet_word_in, app_bypass);
    converted = true;
  } else if( stage->is_resize_ingress() ) {
    LLVM_DEBUG(dbgs() << "Packet resize (ingress): " << *stage->nt_call << '\n');
    stage->convert_packet_resize_ingress(packet_word_in);
    converted = true;
  } else if( stage->is_resize_egress() ) {
    //XXX: This should produce a new packet_word_in (for when we have
    //     more taps in this stage eventually!
    LLVM_DEBUG(dbgs() << "Packet resize (egress): " << *stage->nt_call << '\n');
    packet_word_out = stage->convert_packet_resize_egress(packet_word_in,
                                                          app_bypass);
    converted = true;
  } else if( stage->is_map_receive() ) {
    LLVM_DEBUG(dbgs() << "Map Op (receive): " << *stage->nt_call << '\n');
    stage->convert_map_op_receive();
    converted = true;
  } else if( stage->is_map_request() ) {
    /* Do nothing here, work is done in create epilogue */
  } else if( stage->is_packet_drop() ) {
    stage->convert_packet_drop();
    converted = true;
  }

  BasicBlock *app_epi_bb, *bypass_epi_bb;
  create_stage_epilogue(stage, packet_word_out, stage->live_out_val,
                        stage->live_out_mem, &app_epi_bb,
                        &bypass_epi_bb);

  LLVM_DEBUG(
    dbgs() << "app_bypass: " << app_bypass->getName()
           << "\napp_epi_bb: " << app_epi_bb->getName()
           << "\nbypass_epi_bb: " << bypass_epi_bb->getName()
           << "\nstage->app_logic_exit: "
           << stage->app_logic_exit->getName() << '\n'
  );
  app_bypass->replaceAllUsesWith(bypass_epi_bb);
  app_bypass->eraseFromParent();

  auto* app_term = stage->app_logic_exit->getTerminator();
  cast<BranchInst>(app_term)->setSuccessor(0, app_epi_bb);

  /* Record the application logic portion as metadata on the pipeline
   * function; that way, later stages can distinguish between application
   * and overheads / added pipelining code */
  auto* md_entry = MDString::get(c, "app_entry");
  auto* md_exit  = MDString::get(c, "app_exit");

  stage->app_logic_entry->front().setMetadata("nanotube.pipeline",
                                              MDNode::get(c, md_entry));
  stage->app_logic_exit->back().setMetadata("nanotube.pipeline",
                                            MDNode::get(c, md_exit));

  if( !converted && !stage->is_none() ) {
    errs() << "ERROR: Unhandled / unconverted stage "
           << stage->func->getName() << "\n  Call: "
           << *stage->nt_call << "\nAborting!\n";
    abort();
  }
}

static Value*
channel_create(Value* ch_name, size_t elem_size, size_t num_elem,
               IRBuilder<>& ir, Module& m, StringRef call_name = "") {
  Value* args[3];
  args[0] = ch_name;
  args[1] = ir.getInt64(elem_size);
  args[2] = ir.getInt64(num_elem);

  auto* f  = create_nt_channel_create(m);
  auto* ty = get_nt_channel_create_ty(m);
  auto* call = ir.CreateCall(ty , f, args, call_name);
  return call;
}
static Value*
channel_create(StringRef name, size_t elem_size, size_t num_elem,
               IRBuilder<>& ir, Module& m) {
  return channel_create(ir.CreateGlobalStringPtr(name), elem_size,
                        num_elem, ir, m, name);
}
static Value*
channel_set_attr(Value* channel, nanotube_channel_attr_id_t attr_id,
                 int32_t attr_val, IRBuilder<>& ir, Module& m)
{
  Value* args[3];
  args[0] = channel;
  args[1] = ConstantInt::get(get_nt_channel_attr_id_ty(m), attr_id);
  args[2] = ConstantInt::get(get_nt_channel_attr_val_ty(m), attr_val);

  auto* f  = create_nt_channel_set_attr(m);
  auto* ty = get_nt_channel_set_attr_ty(m);
  auto rc = ir.CreateCall(ty , f, args);
  return rc;
}
static void
channel_export(Value* channel, nanotube_channel_type_t type,
               nanotube_channel_flags_t flags, IRBuilder<>& ir, Module& m)
{
  Value* args[3];
  args[0] = channel;
  args[1] = ConstantInt::get(get_nt_channel_type_ty(m), type);
  args[2] = ConstantInt::get(get_nt_channel_flags_ty(m), flags);

  auto* f  = create_nt_channel_export(m);
  auto* ty = get_nt_channel_export_ty(m);
  ir.CreateCall(ty , f, args);
}
static void
context_add_channel(Value* ctx, nanotube_channel_id_t channel_id,
                    Value* channel, nanotube_channel_flags_t flags,
                    IRBuilder<>& ir, Module& m) {
  Value* args[4];
  args[0] = ctx;
  args[1] = ConstantInt::get(get_nt_channel_id_ty(m), channel_id);
  args[2] = channel;
  args[3] = ConstantInt::get(get_nt_channel_flags_ty(m), flags);

  auto* f  = create_nt_context_add_channel(m);
  auto* ty = get_nt_context_add_channel_ty(m);
  ir.CreateCall(ty , f, args);
}
static void
thread_create(Value* ctx, StringRef name, Function* func, Value* data,
              size_t data_size, IRBuilder<>& ir, Module& m) {
  Value* args[5];
  args[0] = ctx;
  args[1] = ir.CreateGlobalStringPtr(name);
  args[2] = func;
  if( data != nullptr )
    args[3] = ir.CreateBitCast(data, ir.getInt8PtrTy());
  else
    args[3] = Constant::getNullValue(ir.getInt8PtrTy());
  args[4] = ir.getInt64(data_size);

  static auto* f  = create_nt_thread_create(m);
  static auto* ty = get_nt_thread_create_ty(m);

  ir.CreateCall(ty , f, args);
}

static Value*
context_create(unsigned id, IRBuilder<>& ir, Module& m) {
  auto* f  = create_nt_context_create(m);
  auto* ty = get_nt_context_create_ty(m);
  return ir.CreateCall(ty , f, None, "context" + Twine(id));
}

static Value*
tap_map_create(enum map_type_t map_type, Value* key_length,
               Value* data_length, unsigned capacity,
               unsigned num_clients, nanotube_map_id_t id,
               IRBuilder<>& ir, Module& m) {
  auto& c = m.getContext();
  static auto* map_create_ty = get_nt_tap_map_create_ty(m);
  static auto* map_create_f  = create_nt_tap_map_create(m);

  std::array<Value*, 5> args = {
    ConstantInt::get(Type::getInt32Ty(c), map_type),
    key_length,
    data_length,
    ConstantInt::get(get_nt_map_depth_ty(m), capacity),
    ConstantInt::get(Type::getInt32Ty(c), num_clients),
  };
  auto* map = ir.CreateCall(map_create_ty, map_create_f, args,
                            "map_" + Twine(id));
  LLVM_DEBUG(dbgs() << "Creating map " << id
                    << " with type " << map_type
                    << " key size " << *key_length
                    << " value size " << *data_length
                    << " and " << num_clients << " clients:\n"
                    << *map << '\n');
  return map;
}

static void
tap_map_build(Value* map, unsigned id, IRBuilder<>& ir, Module& m) {
  static auto* ty = get_nt_tap_map_build_ty(m);
  static auto* f  = create_nt_tap_map_build(m);
  auto* call = ir.CreateCall(ty, f, {map});
  LLVM_DEBUG(dbgs() << "Building map " << id << " with " << *call << '\n');
}

static Value*
tap_map_add_client(Value* map, Value* key_sz, Value* data_in_sz,
                   bool needs_resp, Value* data_out_sz,
                   Value* req, nanotube_channel_id_t req_ch,
                   Value* rcv, nanotube_channel_id_t rcv_ch,
                   unsigned cid, nanotube_map_id_t id,
                   IRBuilder<>& ir, Module& m) {

  LLVM_DEBUG(dbgs() << "Map " << id
                    << ": Adding req " << *req << " ch: " << req_ch
                    << " and rcv " << *rcv << " ch: " << rcv_ch
                    << " with client id " << cid << '\n');

  auto& c = m.getContext();
  static auto* add_client_ty = get_nt_tap_map_add_client_ty(m);
  static auto* add_client_f  = create_nt_tap_map_add_client(m);
  std::array<Value*,9> ac_args = {
    map, key_sz, data_in_sz,
    ConstantInt::get(Type::getInt1Ty(c), needs_resp),
    data_out_sz,
    req, ConstantInt::get(get_nt_channel_id_ty(m), req_ch),
    rcv, ConstantInt::get(get_nt_channel_id_ty(m), rcv_ch)
  };

  auto* call = ir.CreateCall(add_client_ty, add_client_f, ac_args);
  LLVM_DEBUG(dbgs() << "  " << *call << '\n');
  return call;
}

/**
 * Update the nanotube_setup function and create threads for every pipeline
 * stage and fifos to connect them.
 */
void Pipeline::extend_nanotube_setup(const Pipeline::stages_t& stages,
                                     const kernel_info& ki) {
  auto* m = stages[0]->func->getParent();
  const auto& dl = m->getDataLayout();

  /* Make sure the kernel is of the right type */
  auto& kia = ki.args();
  assert(is_nt_packet_kernel(*kia.kernel) && kia.is_capsule);

  /* Modify the setup function where the original kernel was registered. */
  auto* add_plain_kernel = ki.creator();
  IRBuilder<> ir(add_plain_kernel);

  /**
   * Channel for packet words:
   * packets_in -> stage_0 -> packets_0_to_1 -> ...
   *                                  ...  -> stage N -> packets_out
   */
  auto  bus_word_size = get_bus_word_size();
  auto  sideband_size = get_bus_sb_size();
  auto  sideband_signals = get_bus_sb_signals_size();
  static const size_t packet_fifo_depth = 140; // 9000 / 64

  std::vector<Value*> packet_chs(stages.size() + 1);
  for( unsigned n = 0; n <= stages.size(); n++ ) {
    std::string ch_name;
    Value* ch;
    if( n == 0 ) {
      /* The test framework uses the name of the packet input channel as
       * the test name, so copy that off the old registration */
      auto* kernel_name = add_plain_kernel->getArgOperand(0);
      ch = channel_create(kernel_name, bus_word_size, packet_fifo_depth,
                          ir, *m, "packet_in");
    } else {
      if( n == stages.size() )
        ch_name = "packets_out";
      else
        ch_name = "packets_" + std::to_string(n - 1) + "_to_" +
                   std::to_string(n);
      ch = channel_create(ch_name, bus_word_size, packet_fifo_depth,
                          ir, *m);
    }
    if (sideband_size != 0)
      channel_set_attr(ch, NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES, sideband_size, ir, *m);
    if (sideband_signals != 0)
      channel_set_attr(ch, NANOTUBE_CHANNEL_ATTR_SIDEBAND_SIGNALS, sideband_signals, ir, *m);
    packet_chs[n] = ch;
  }

  nanotube_channel_type_t channel_type;
  switch( get_bus_type() ) {
    case NANOTUBE_BUS_ID_SHB:  channel_type = NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET; break;
    case NANOTUBE_BUS_ID_X3RX: channel_type = NANOTUBE_CHANNEL_TYPE_X3RX_PACKET; break;
    default:                   channel_type = NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET;
  }

  /* Export the overall packet in / out channels */
  channel_export(packet_chs[0], channel_type,
                 NANOTUBE_CHANNEL_WRITE, ir, *m);
  channel_export(packet_chs[stages.size()], channel_type,
                 NANOTUBE_CHANNEL_READ, ir, *m);

  /**
   * Channels for application state (where needed and of the right type):
   * stage_0 -> state_0_to_1 -> stage1
   * stage_K -> state_K_to_K+1 -> stage_K+1
   * ...
   */
  std::vector<Value*> app_state_chs(stages.size() - 1);
  for( unsigned n = 0; n < stages.size() - 1; n++ ) {
    auto* stout = stages[n];
    if( !stout->has_live_out() )
      continue;
    auto* stin = stages[n + 1];
    assert(stin->has_live_in());

    std::string ch_name;
    ch_name = "state_" + std::to_string(n) + "_to_" +
               std::to_string(n + 1);
    assert(stout->get_live_out_ty() == stin->get_live_in_ty());
    size_t live_size = dl.getTypeStoreSize(stout->get_live_out_ty());
    static const size_t live_fifo_depth = 10;
    auto* ch = channel_create(ch_name, live_size, live_fifo_depth, ir, *m);
    app_state_chs[n] = ch;
  }

  /**
   * Channels for resize tap cwords:
   * stage_0 -> cword_0_to_1 -> stage1
   * stage_K -> cword_K_to_K+1 -> stage_K+1
   * ...
   */
  std::vector<Value*> cword_chs(stages.size() - 1);
  for( unsigned n = 0; n < stages.size() - 1; n++ ) {
    auto* stout = stages[n];
    if( !stout->is_resize_ingress() )
      continue;
    auto* stin = stages[n + 1];
    assert(stin->is_resize_egress());

    std::string ch_name;
    ch_name = "cword" + std::to_string(n) + "_to_" +
               std::to_string(n + 1);
    size_t live_size = dl.getTypeStoreSize(get_nt_tap_packet_resize_cword_ty(*m));
    static const size_t cword_fifo_depth = 140;
    auto* ch = channel_create(ch_name, live_size, cword_fifo_depth, ir, *m);
    cword_chs[n] = ch;
  }

  /* Create contexts, and connect them to the right channels.  Also
   * initialise any static data that needs initialising. */
  static const int PACKETS_IN  = stage_function_t::PACKETS_IN;
  static const int PACKETS_OUT = stage_function_t::PACKETS_OUT;
  static const int STATE_IN    = stage_function_t::STATE_IN;
  static const int STATE_OUT   = stage_function_t::STATE_OUT;
  static const int CWORD_IN    = stage_function_t::CWORD_IN;
  static const int CWORD_OUT   = stage_function_t::CWORD_OUT;
  static const int MAP_REQ     = stage_function_t::MAP_REQ;
  static const int MAP_RESP    = stage_function_t::MAP_RESP;
  std::vector<Value*> contexts;
  for( unsigned i = 0; i < stages.size(); i++ ) {
    auto* ctx = context_create(i, ir, *m);
    contexts.push_back(ctx);

    /* Packet daisy chain */
    auto* pin = packet_chs[i];
    auto* pout = packet_chs[i + 1];
    context_add_channel(ctx, PACKETS_IN,  pin,  NANOTUBE_CHANNEL_READ, ir, *m);
    context_add_channel(ctx, PACKETS_OUT, pout, NANOTUBE_CHANNEL_WRITE, ir, *m);

    /* Live State: has_live_out (producer) => has_live_in (consumer) */
    if( stages[i]->has_live_out() ) {
      assert(i < app_state_chs.size());
      context_add_channel(ctx, STATE_OUT, app_state_chs[i],
                          NANOTUBE_CHANNEL_WRITE, ir, *m);
    }
    if( stages[i]->has_live_in() ) {
      assert(i > 0);
      context_add_channel(ctx, STATE_IN, app_state_chs[i - 1],
                          NANOTUBE_CHANNEL_READ, ir, *m);
    }

    /* Resize Cword: resize_ingress (producer) => resize_egress (consumer) */
    if( stages[i]->is_resize_ingress() ) {
      assert(i < app_state_chs.size());
      context_add_channel(ctx, CWORD_OUT, cword_chs[i],
                          NANOTUBE_CHANNEL_WRITE, ir, *m);
    }
    if( stages[i]->is_resize_egress() ) {
      assert(i > 0);
      context_add_channel(ctx, CWORD_IN, cword_chs[i - 1],
                          NANOTUBE_CHANNEL_READ, ir, *m);
    }

    /* Initialise per-stage static state */
    if( stages[i]->is_resize_ingress() ) {
      auto* init_ty = get_tap_packet_resize_ingress_state_init_ty(*m);
      auto* init_f  = create_tap_packet_resize_ingress_state_init(*m);
      Value* args[] = {
        stages[i]->get_static_packet_resize_ingress_tap_state() };
      ir.CreateCall(init_ty, init_f, args);
    } else if( stages[i]->is_resize_egress() ) {
      auto* init_ty = get_tap_packet_resize_egress_state_init_ty(*m);
      auto* init_f  = create_tap_packet_resize_egress_state_init(*m);
      Value* args[] = {
        stages[i]->get_static_packet_resize_egress_tap_state() };
      ir.CreateCall(init_ty, init_f, args);
    }
  }

  /* Create maps with nanotube_tap_map_create and store them in a map array
   * that can then be given to threads so that they can access the right
   * maps. */
  auto* mapty   = get_nt_tap_map_ty(*m)->getPointerTo();
  auto  nmaps   = setup->maps().size();
  Value* maparr        = nullptr;
  unsigned maparr_size = 0;
  if( nmaps > 0 ) {
    auto* maparr_ty = ArrayType::get(mapty, nmaps);
    maparr_size = dl.getTypeStoreSize(maparr_ty);
    maparr = ir.CreateAlloca(maparr_ty, nullptr, "map_arr");

    for( auto& mi : setup->maps() ) {
      auto idx = mi.index();
      const auto& mca = mi.args();
      auto id  = mca.id;

      /* Create map with nanotube_tap_map_create */
      const unsigned map_size = 10; //XXX: Fixme

      auto  num_clients   = stage_function_t::map_to_rcvs[id].size();
      if( num_clients == 0 ) {
        errs() << "WARNING: Defined but unused map found!\n"
               << "ID: " << id
               << " Type: " << mca.type
               << " Key SZ: " << *mca.key_sz
               << " Value SZ: " << *mca.value_sz
               << "\nCreating it without clients.\n";
      }

      auto map_width_ty = get_nt_map_width_ty(*m);
      auto* key_sz = ir.CreateZExtOrTrunc(mca.key_sz, map_width_ty);
      auto* value_sz = ir.CreateZExtOrTrunc(mca.value_sz, map_width_ty);
      auto* map = tap_map_create(mca.type, key_sz, value_sz, map_size,
                                 num_clients, id, ir, *m);

      /* Remember this map for threads to find */
      auto* gep = ir.CreateConstInBoundsGEP2_32(maparr_ty, maparr, 0, idx,
                                                "map_loc" + Twine(idx));
      ir.CreateStore(map, gep);

      /* Add the client pairs (send / receive contexts) to this map */
      auto& reqs = stage_function_t::map_to_reqs[id];
      auto& rcvs = stage_function_t::map_to_rcvs[id];

      for( unsigned cid = 0; cid < num_clients; cid++ ) {
        auto *req_stage = reqs[cid];
        auto *rcv_stage = rcvs[cid];
        Value *req_buf_sz = req_stage->map_req_buf_size;
        Value *rcv_buf_sz = rcv_stage->map_rcv_buf_size;
        req_buf_sz = ir.CreateZExtOrTrunc(req_buf_sz, map_width_ty);
        rcv_buf_sz = ir.CreateZExtOrTrunc(rcv_buf_sz, map_width_ty);
        tap_map_add_client(map, key_sz, req_buf_sz,
                           true, rcv_buf_sz,
                           contexts[req_stage->idx], MAP_REQ,
                           contexts[rcv_stage->idx], MAP_RESP,
                           cid, id, ir, *m);
      }

      /* Build the map */
      tap_map_build(map, id, ir, *m);
    }
  }

  /* Bind contexts to thread functions */
  for( unsigned i = 0; i < stages.size(); i++ ) {
    /* Thread per stage */
    auto* ctx = contexts[i];
    thread_create(ctx, stages[i]->stage_name, stages[i]->func, maparr, maparr_size,
                  ir, *m);
  }

  /* Remove old calls to the high-level nanotube_map_create */
  //XXX: There is probably a cleaner way to do this with the setup_func
  //magic
  SmallVector<Instruction*, 16> map_creates;
  SmallVector<Instruction*, 16> context_add_maps;
  for( auto& inst : instructions(setup->get_function()) ) {
    if( !isa<CallInst>(inst) )
      continue;
    auto id = get_intrinsic(&inst);
    if( id == Intrinsics::map_create )
      map_creates.push_back(&inst);
    else if( id == Intrinsics::context_add_map )
      context_add_maps.push_back(&inst);
  }

  for( auto* inst : context_add_maps )
    inst->eraseFromParent();
  for( auto* inst : map_creates )
    inst->eraseFromParent();

  /* Remove the old call to nanotube_add_plain_packet_kernel(capsule=true) */
  assert(add_plain_kernel->user_empty());
  add_plain_kernel->eraseFromParent();
}

void Pipeline::parse_max_access_sizes(Function& f) {
  /* Scan through the Nanotube accesses and compute maximum access sizes */
  for( auto iit = inst_begin(f); iit != inst_end(f); ++iit ) {
    auto* inst = &*iit;
    if( !isa<CallInst>(inst) )
      continue;
    auto* call = cast<CallInst>(inst);
    nt_api_call ntc(call);
    if( !ntc.is_packet() || (!ntc.is_read() && !ntc.is_write()) )
        continue;
    auto* size_arg = ntc.get_data_size_arg();
    int   max_size = ntc.get_max_data_size();
    size_arg_to_max[size_arg] = max_size;
    LLVM_DEBUG(
      dbgs() << "NT API Call:\n  " << *call
             << "\n  Size arg: " << *size_arg
             << "\n  Max data size: "
             << ntc.get_max_data_size() << '\n');
  }
}

/* Check whether an API call needs to be split */
static bool
needs_split(Intrinsics::ID id) {
  return (id == Intrinsics::packet_resize) ||
         (id == Intrinsics::map_op);
}

/**
 * Split a Nanotube packet resize operation.  This is needed because we
 * need to create multiple pipeline stages for a single resize.  Splitting
 * this early in the pass will make that easier by allowing creating one
 * pipeline stage per Nanotube API call.  It also separates the needed
 * parameters so that they do not have to stay live for too long.
 */
static bool
split_packet_resize(Instruction* call) {
  auto& m = *call->getFunction()->getParent();

  LLVM_DEBUG(dbgs() << "Splitting packet_resize " << *call << '\n');
  packet_resize_args pra(call);
  IRBuilder<> ir(call);
  auto* cword = ir.CreateAlloca(get_nt_tap_packet_resize_cword_ty(m),
                                nullptr, "cword");
  auto* packet_length_out = ir.CreateAlloca(get_nt_tap_offset_ty(m),
                                            nullptr, "packet_length_out");
  auto* resize_ingress    = create_nt_packet_resize_ingress(m);
  auto* resize_ingress_ty = get_nt_packet_resize_ingress_ty(m);
  Value* iargs[] = {cword, packet_length_out, pra.packet, pra.offset, pra.adjust};
  auto* ingress_call = ir.CreateCall(resize_ingress_ty, resize_ingress,
                                     iargs);

  auto* resize_egress    = create_nt_packet_resize_egress(m);
  auto* resize_egress_ty = get_nt_packet_resize_egress_ty(m);
  auto* new_length = ir.CreateLoad(get_nt_tap_offset_ty(m), packet_length_out,
                                   "new_length");
  Value* eargs[] = {pra.packet, cword, new_length};
  auto* egress_call = ir.CreateCall(resize_egress_ty, resize_egress,
                                    eargs, "resize_egress");

  LLVM_DEBUG(dbgs() << *cword << '\n'
                    << *ingress_call << '\n'
                    << *egress_call  << '\n');

  assert(egress_call->getType() == call->getType());
  call->replaceAllUsesWith(egress_call);
  call->eraseFromParent();
  return true;
}

/**
 * Split a Nanotube map operation.  These are split because the pipeline
 * boundary cuts the map_op into two: the send request part and the receive
 * response part, living in different pipeline stages.  Splitting this
 * early makes it easier to track live stage correctly, because the input
 * operands can live in pipeline stage N (map_op_send), while the outputs
 * can live in stage N+1 (map_op_receive).
 */
static bool
split_map_op(Instruction* call) {
  auto& m = *call->getFunction()->getParent();

  LLVM_DEBUG(dbgs() << "Splitting map_op " << *call << '\n');
  map_op_args moa(call);
  IRBuilder<> ir(call);

  auto zero_size = ConstantInt::get(moa.data_length->getType(), 0);
  auto req_buf_size = ( isa<ConstantPointerNull>(moa.data_in)
                        ? zero_size : moa.data_length );
  auto resp_buf_size = ( isa<ConstantPointerNull>(moa.data_out)
                        ? zero_size : moa.data_length );

  auto* map_id = ConstantInt::get(get_nt_map_id_type(m), moa.map);
  /* The send part captures all inputs needed for stage N */
  auto* send_ty = get_nt_map_op_send_ty(m);
  auto* send_f  = create_nt_map_op_send(m);
  std::array<Value*, 9> send_args = { moa.ctx, map_id, moa.type, moa.key,
    moa.key_length, moa.data_in, moa.mask, moa.offset, req_buf_size };
  auto* send = ir.CreateCall(send_ty, send_f, send_args);

  /* The receive part captures the outputs produced by the response and
   * inputs needed for the responds; it will replace the original map_op
   * call */
  auto* receive_ty = get_nt_map_op_receive_ty(m);
  auto* receive_f  = create_nt_map_op_receive(m);
  std::array<Value*, 4> receive_args = { moa.ctx, map_id, moa.data_out,
                                         resp_buf_size };
  auto* receive = ir.CreateCall(receive_ty, receive_f, receive_args);

  LLVM_DEBUG(dbgs() << "Replacing with\n" << *send << '\n' << *receive << '\n');
  call->replaceAllUsesWith(receive);
  call->eraseFromParent();
  return true;
}

/**
 * Convert a return with value into a call to nanotube_packet_drop + ret
 * void.
 */
static bool
convert_ret_to_drop(Instruction* inst) {
  auto* ret = cast<ReturnInst>(inst);
  auto* ret_val = ret->getReturnValue();
  if( ret_val == nullptr )
    return false;

  auto& m = *ret->getFunction()->getParent();
  auto* packet = ret->getFunction()->arg_begin() + 1;

  IRBuilder<> ir(ret);
  auto* drop_ty = get_nt_packet_drop_ty(m);
  auto* drop_f  = create_nt_packet_drop(m);
  std::array<Value*, 2> drop_args = {packet, ret_val};
  auto* drop = ir.CreateCall(drop_ty, drop_f, drop_args);

  auto* new_ret = ir.CreateRetVoid();
  LLVM_DEBUG(dbgs() << "Replacing " << *ret << " with\n"
                    << *drop << '\n' << *new_ret << '\n');
  ret->eraseFromParent();
  return true;
}

/**
 * Go through all Nanotube API calls that need splitting and split them.
 * Usually this is done for two phase accesses where the pipeline needs to
 * be split "in between" an access (nanotube_map_op), and when a single
 * Nanotube API call will need multiple pipeline stages
 * (nanotube_packet_resize).
 *
 * NOTE: Splitting instructions will invalidate liveness analysis data!
 * Returns true when changes were made.
 */
static
bool convert_api_calls(Function& f) {
  bool changes = false;
  for( auto iit = inst_begin(f); iit != inst_end(f); ) {
    /* NOTE: Use iterator-based stepping because we are changing the
     * underlying instructions! */
    auto* inst = &*iit; ++iit;

    if( isa<CallInst>(inst) ) {
      /* Convert Nanotube API calls */
      auto* call = cast<CallInst>(inst);
      auto  id   = get_intrinsic(call);
      if( !needs_split(id) )
        continue;

      switch( id ) {
        case Intrinsics::packet_resize:
          changes |= split_packet_resize(call);
          break;
        case Intrinsics::map_op:
          changes |= split_map_op(call);
          break;
        default:
          errs() << "ERROR: Unhandled call " << *call << " in "
                 << "convert_api_calls.\nAborting!\n";
          exit(1);
      };
      changes = true;
    } else if( isa<ReturnInst>(inst) ) {
      /* Convert returns with a value */
      changes |= convert_ret_to_drop(inst);
    }
  }
  return changes;
}

/**
 * Check the usage of return codes of Nanotube API functions which will
 * need to get changed when translated to the tap-level API.
 *
 * XXX: In the future, we might actually perform the translation to the
 * tap-level API before splitting the function into pipeline stages!
 */
static
bool check_api_call_usage(Function& f) {
  for( auto& inst : instructions(f)) {
    if( !isa<CallInst>(inst) )
      continue;

    /* For now: assume split points = call that will get translated to tap
     * interface! */
    if( !is_split_point(&inst) )
      continue;

    /* Calls where the return code is ignored are no issue */
    if( inst.user_empty() )
      continue;

    LLVM_DEBUG( dbgs() << "NT API Call " << inst << " has users:\n";
      for( auto* u : inst.users() ) {
        dbgs() << "  " << *u << '\n';
      }
    );

    switch( get_intrinsic(&inst) ) {
      case Intrinsics::packet_bounded_length:
      case Intrinsics::packet_read:
      case Intrinsics::packet_write:
        /* These all have perfect translations */
        break;
      case Intrinsics::packet_resize_egress:
        /* Known problematic, fake success code is generated! */
        break;
      case Intrinsics::map_op_receive:
        for( auto* u : inst.users() ) {
          auto* user = cast<Instruction>(u);
          if( (user == nullptr) || !isa<ICmpInst>(user) ) {
            errs() << "FIXME: map_op call " << inst
                  << " has unsupported user: " << *u
                  << " all users: \n";
            for( auto* u : inst.users() )
              errs() << "  " << *u << '\n';
            errs() << "Aborting!\n";
            exit(1);
          }
          /* We currently only handle icmp (N)EQ %map_op, 0 */
          auto* icmp = cast<ICmpInst>(user);
          auto* rhs  = dyn_cast<ConstantInt>(icmp->getOperand(1));
          assert(icmp->isEquality());
          assert((rhs != nullptr) && rhs->isZero());
        }
        break;
      default:
        errs() << "ERROR: Unhandled case in check_api_call_usage() for "
               << inst << "\nFix me in " << __FILE__ << ":" << __LINE__
               << "\nAborting!\n";
        exit(1);
    }
  }
  return false;
}

/**
 * Remove llvm.stacksave / llvm.stackrestore calls from the function as
 * they are not needed here and generally confuse the pass.
 *
 * NOTE: Removing these will invalidate liveness analysis data!
 * Returns true when changes were made.
 */
static
bool remove_stacksave_restore(Function& f) {
  LLVM_DEBUG(dbgs() << "Removing llvm.stacksave / llvm.stackrestore in "
                    << f.getName() << '\n');
  bool changes = false;
  std::vector<Instruction*> todo;

  /* Go backwards by removing the stackrestores and all values feeding into
   * them */
  for( auto& inst : instructions(f) ) {
    if( get_intrinsic(&inst) == Intrinsics::llvm_stackrestore )
      todo.push_back(&inst);
  }

  changes = (todo.size() > 0);

  while( todo.size() > 0 ) {
    auto* inst = todo.back();
    todo.pop_back();
    assert( inst->user_empty() );
    for( auto& u : inst->operands() ) {
      if( u->hasOneUse() && isa<Instruction>(u))
        todo.push_back(cast<Instruction>(u));
    }
    LLVM_DEBUG(dbgs() << "Deleting unused " << *inst << '\n');
    inst->eraseFromParent();
  }
  return changes;
}

Pipeline::stages_t Pipeline::pipeline(Function& f) {
  std::vector<stage_function_t*> stages;
  /* Scan the function for stages */
  determine_stages(&stages, f);

  /* Parse the type of each stage */
  for( unsigned i = 0; i < stages.size(); i++ ) {
    auto* stage = stages[i];
    stage->scan_nanotube_accesses(setup);
  }

  /* Create the actual code for each stage */
  for( auto* stage : stages ) {
    ValueToValueMapTy vmap;
    auto* pl_end = generate_prologue(stage, &vmap);
    copy_app_code(stage, pl_end, vmap);
    convert_to_taps(stage);
  }

  /* Check application state input / output */
  if( stages.front()->has_live_in() || stages.back()->has_live_out() ) {
    auto* li = stages.front()->get_live_in_ty();
    auto* lo = stages.back()->get_live_out_ty();
    errs() << "ERROR: The pipeline has application live state going in / "
           << "coming out of the pipeline, which is illegal.\n"
           << "  In: " << nullsafe(li) << '\n'
           << "  Out: " << nullsafe(lo) << "\nAborting!\n";
    abort();
  }

  return stages;
}

typedef std::unordered_map<PHINode*, SmallVector<Value*,4>>
phi_idx_to_ptr_ty;


/**
 * Collect all pointer PHI nodes that are complex and the split point which
 * they are before.  For that, we iterate over the function in CFG DAG
 * order, collect interesting PHI nodes and associate them with the next
 * split point.
 *
 * @param f          The function in which we want to find pointer phis and
 *                   split points.
 * @param split2phis Output: mapping of split point to a vector of phis
 *                   before that split point.
 */
static void
collect_pointer_phis_and_splits(Function& f,
  std::unordered_map<Instruction*, std::vector<PHINode*>>* split2phis) {
  std::vector<PHINode*> phis;

  typedef dep_aware_converter<BasicBlock> dac_t;
  dac_t dac;
  dac.insert_ready(&f.getEntryBlock());

  dac.execute([&](dac_t* dac, BasicBlock* bb) {
    for( auto& inst : *bb ) {
      if( is_split_point(&inst) && (phis.size() > 0) ) {
        split2phis->emplace(std::make_pair(&inst, std::move(phis)));
        //NOTE: Not needed because of std::move above:
        //phis.clear();
      }

      /* Ignore anything that is not a non-trivial pointer PHI */
      if( !isa<PHINode>(inst) || !inst.getType()->isPointerTy() )
        continue;
      auto* phi = cast<PHINode>(&inst);
      if( phi->hasConstantOrUndefValue() )
        continue;
      phis.emplace_back(phi);
    }

    /* Ready our successor basic blocks */
    for( auto* succ : successors(bb) )
      if( dac->contains(succ) )
        dac->mark_dep_ready(succ);
      else
        dac->insert(succ, pred_size(succ) - 1);
  });
  assert(dac.empty());
}

/**
 * Get pointer-typed instructions that derive from instruction start and
 * are not after a certain instruction.  This can be used to find all
 * values that derive from a PHI node inside a single pipeline stage.
 * @param start   Starting instruction, will be part of the results.
 * @param cut     Split point instruction, tracing will stop at any derived
 *                instruction that is dominated by cut.
 * @param oi      OrderedInstruction (wrapper around DominatorTree) to help
 *                answer ordering queries.
 * @return        Vector of instructions that are derived from start and
 *                not dominated by cut.
 */
static std::vector<Instruction*>
get_derived_ptrs_before(Instruction* start, Instruction* cut, OrderedInstructions* oi) {
  std::vector<Instruction*> todo(1, start);
  std::vector<Instruction*> derived;
  while( !todo.empty() ) {
    auto* cur = todo.back();
    todo.pop_back();
    derived.emplace_back(cur);

    /* Add users that are pointers and derived, and before cut */
    for(auto* u : cur->users()) {
      auto* inst = dyn_cast<Instruction>(u);
      /* Ignore non-instruction uses */
      if( inst == nullptr )
        continue;
      /* Ignore uses that don't keep pointer type */
      if( !inst->getType()->isPointerTy() ) {
        assert(inst->getOpcode() != Instruction::PtrToInt);
        continue;
      }
      /* Ignore users after the cut */
      if( oi->dominates(cut, inst) )
        continue;
      todo.emplace_back(inst);
    }
  }
  return derived;
}

/**
 * The pipeline pass does not support complex pointer PHI nodes that cross
 * a pipeline stage.  Some of these can be converted into a new memory
 * allocation and instead of selecting an underlying pointer in the PHI, we
 * simply copy the selected data to the new allocation.  In some cases,
 * however, this mechanism does not work; the code detects these cases and
 * aborts compilation with an error in those cases.
 *
 * @param f     Function to do the conversion on
 * @param livi  Liveness analysis info
 * @param aa    Alias analysis info
 * @param dt    DominatorTree analysis info
 * @return      Did this code make any modifications?
 */
static
bool convert_pointer_phis(Function& f, liveness_info_t* livi,
                          AliasAnalysis* aa,
                          DominatorTree* dt) {
  bool changes = false;
  OrderedInstructions oi(dt);

  std::unordered_map<Instruction*, std::vector<PHINode*>> split2phis;
  collect_pointer_phis_and_splits(f, &split2phis);

  std::vector<Instruction*> to_delete;

  /* We now have a list of split points and and interesting phi nodes just
   * before them.  Check whether the phi node is actually live at the split
   * point, and if so, check if we can convert it.  We need to check
   * live-in, because the split point is the start of the next pipeline
   * stage! */
  for( auto& split_phis : split2phis ) {
    auto* split = split_phis.first;
    std::vector<Value*>         liv;
    std::vector<MemoryLocation> lim;
    livi->get_live_in(split, &liv, &lim);
    std::unordered_set<Value*>  liv_set(liv.begin(), liv.end());

    LLVM_DEBUG(dbgs() << "Split: " << *split << '\n');
    for( auto* phi : split_phis.second ) {
      LLVM_DEBUG(dbgs() << "PHI: " << *phi << '\n');

      /* Check whether the PHI node itself, or a derived value is live in
       * the next stage */
      auto phi_derived = get_derived_ptrs_before(phi, split, &oi);
      bool used_in_next_stage = false;
      for( auto* phi_d : phi_derived ) {
        if( liv_set.count(phi_d) != 0 ) {
          LLVM_DEBUG(dbgs() << "Long-range usage via: "
                            << *phi_d << '\n');
          used_in_next_stage = true;
          break;
        }
      }
      if( !used_in_next_stage ) {
        LLVM_DEBUG(dbgs() << "PHI is not live outside this stage,"
                          << " ignoring!\n");
        continue;
      }

      /* We now know that this PHI is long-range, meaning that it is live
       * in a later stage than where it is defined => we need to handle it.
       */
      std::unordered_set<Value*> phivs(phi->value_op_begin(),
                                       phi->value_op_end());

      std::vector<Value*>         lov;
      std::vector<MemoryLocation> lom;
      livi->get_live_in(phi, &lov, &lom);

      /* We now want to answer the question: is any code after the PHI
       * using either of the locations selected by the PHI directly (rather
       * than the PHI node itself)?  Note that a direct comparison with the
       * PHI operands is not enough.  Instead, check if anything aliases
       * with the live-out values minus the PHI.  Note that all of this has
       * to work on the actual values, rather than memory locations,
       * because the memory locations will all be live due to the PHI node
       * itself! */
      LLVM_DEBUG(
        dbgs() << "Live Out Values (of PHI):\n";
        for( auto* v : lov )
          dbgs() << "  " << *v << " is_phi_value: " << phivs.count(v) << '\n';
      );

      /* Collapse live-out values into AliasSets (in case multiple operands
       * of the PHI point to the same / aliasing memory) */
      AliasSetTracker ast(*aa);
      for( auto* v : lov ) {
        /* Ignore the phi node itself */
        if( v == phi )
          continue;
        /* Non-pointer values cannot overlap */
        if( !v->getType()->isPointerTy() )
          continue;
        ast.getAliasSetFor(MemoryLocation(v));
      }

      LLVM_DEBUG(dbgs() << "Live-Out AliasSets:\n" << ast << '\n');

      /* Check that the underlying memory location pointed to by the PHI node
       * is not subsequently accessed through a pointer that does not
       * derive from the PHI node. For that, go through all the PHI
       * operands and check whether they intersect with any of the Live-Out
       * Alias Sets (aliases with any of the live-out values minus the PHI
       * itself).*/
      for( auto* u : phivs ) {
        MemoryLocation uloc(u);
        auto& as = ast.getAliasSetFor(uloc);
        LLVM_DEBUG(dbgs() << "AS: " << as << " Size: " << as.size() << '\n');
        if( as.size() > 1 ) {
          errs() << "ERROR: phi-of-addr value " << *u << " is live after "
                 << "being used in a complex phi " << *phi << " preventing "
                 << "collapsing the phi allocations.  This is not supported "
                 << "at the moment! See NANO-293.\nAborting!\n";
          exit(1);
          break;
        }
      }

      /* Find out which allocation corresponds to the input values and
       * whether they are live */
      std::unordered_map<Value*, MemoryLocation> v_to_loc;
      unsigned found = 0;
      for( auto* v : phivs ) {
        MemoryLocation* alloc = nullptr;
        for( auto& m :lom ) {
          MemoryLocation vloc(v);
          auto res = aa->alias(vloc, m);
          if( res == NoAlias )
            continue;
          if( alloc != nullptr ) {
            errs() << "ERROR: Unexpected additional match " << m
                   << " (we already have " << *alloc << " ) when "
                   << "inspecting  value " << *v
                   << " of complex pointer phi " << *phi << '\n'
                   << "Aborting!\n";
            exit(1);
          }
          assert(alloc == nullptr);
          alloc = &m;
          LLVM_DEBUG(dbgs() << "PHI Value: " << *v << " intersects " << m
                            << " AA: " << res << '\n');
          //NOTE: No early exit to ensure singe match property!
        }

        auto& dl = f.getParent()->getDataLayout();

        if( alloc != nullptr ) {
          v_to_loc[v] = *alloc;
          found++;
        } else {
          /* If we didn't find anything in the live memory objects, simply
           * inspect the entries themselves. */
          auto* ul = GetUnderlyingObject(v, dl, 0);
          LLVM_DEBUG(dbgs() << "Pointer PHI " << *phi << " value " << *v
                            << " is a dead allocation. Underlying object:"
                            << *ul << '\n');
          if( (ul == nullptr) || !isa<AllocaInst>(ul) ) {
            errs() << "ERROR: Unexpected object " << nullsafe(ul)
                   << " for value " << *v << " in PHI: " << *phi
                   << "\nAborting!\n";
            exit(1);
          }
          auto* alloca = cast<AllocaInst>(ul);
          auto  sz = alloca->getAllocationSizeInBits(dl).getValue() / 8;
          v_to_loc[v] = MemoryLocation(alloca, sz);
        }
      }
      assert(phivs.size() == v_to_loc.size());

      /* Find the right size for the new allocation */
      auto size = v_to_loc.begin()->second.Size;
      for( auto& v_loc : v_to_loc )
        size = size.unionWith(v_loc.second.Size);
      assert(size.hasValue());
      LLVM_DEBUG(dbgs() << "Size for replacement: " << size << '\n');

      /* Create a new alloca to hold the value */
      IRBuilder<> ir(&f.getEntryBlock().back());
      auto* new_alloc = ir.CreateAlloca(ir.getInt8Ty(),
                                        ir.getInt64(size.getValue()),
                                        phi->getName() + ".mem");
      auto* bc = ir.CreateBitCast(new_alloc, phi->getType(), phi->getName());
      LLVM_DEBUG(dbgs() << "New alloc: " << *new_alloc << '\n'
                        << "BC: " << *bc << '\n');
      phi->replaceAllUsesWith(bc);
      to_delete.emplace_back(phi);

      if( found == 0 ) {
        /* All memory locations are dead -> replace with single allocation */
        LLVM_DEBUG(dbgs() << "PHI selects between dead memory.\n"
                          << "Replacing with: " << *bc << '\n');
        changes = true;
      } else if ( found == phivs.size() ) {
        /* All memory locations are live -> create copies into new alloc */
        LLVM_DEBUG(dbgs() << "PHI selects between live memory.\n");
        unsigned n = phi->getNumIncomingValues();
        for( unsigned i = 0; i < n; i++ ) {
          auto* v   = phi->getIncomingValue(i);
          auto& loc = v_to_loc[v];
          auto* bb  = phi->getIncomingBlock(i);
          ir.SetInsertPoint(&bb->back());
          auto* cpy = ir.CreateMemCpy(new_alloc, 0,
                                      const_cast<Value*>(loc.Ptr), 0,
                                      ir.getInt64(loc.Size.getValue()));
          LLVM_DEBUG(
            dbgs() << "Copying value " << *loc.Ptr << " to " << *new_alloc
                   << " in BB: " << bb->getName() << " with " << *cpy
                   << '\n');
        }
        changes = true;
      } else {
        errs() << "ERROR: PHI node " << *phi << " selects between dead & "
               << "live memory locations which is not supported for now.\n"
               << "Aborting!\n";
        exit(1);
      }
    }
  }

  /* Clean up converted PHI nodes */
  for( auto* inst : to_delete ) {
    assert(inst->use_empty());
    inst->eraseFromParent();
  }
  return changes;
}

/**
 * Checks whether the provided kernel_info matches what we would expect in
 * this pass. Complains and exits the program, if not.
 */
static void check_kernel_info(const kernel_info& ki) {
  auto& args = ki.args();
  if( !args.is_capsule ) {
    errs() << "ERROR: Kernel " << args.kernel->getName()
           << " is not registered as a capsule kernel in "
           << *ki.creator() << '\n'
           << "Did you run the platform pass?\nAborting!\n";
    exit(1);
  }
  if ( !is_nt_packet_kernel(*args.kernel) ) {
    errs() << "ERROR: Kernel " << args.kernel->getName()
           << " does not have the signature of a packet kernel.\n"
           << "Aborting!\n";
    exit(1);
  }
  assert(args.is_capsule && is_nt_packet_kernel(*args.kernel));
}

bool Pipeline::runOnModule(Module& m) {
  setup = new setup_func(m);
  stage_function_t::set_setup_func(setup);

  bool changes = false;
  /* Go through all the kernels found and pipeline them */
  for( auto& kernel_info : setup->kernels() ) {
    auto& f = *kernel_info.args().kernel;

    check_kernel_info(kernel_info);

    get_all_analysis_results(f);
    parse_max_access_sizes(f);

    /* Split Nanotube API calls that are multi-phase / multi-stage */
    changes |= convert_api_calls(f);
    changes |= check_api_call_usage(f);
    /* Remove all llvm.stacksave / llvm.stackrestore calls */
    changes |= remove_stacksave_restore(f);
    /* Convert phi-of-pointer instructions so they can deal with changed
     * allocations and be sent from stage to stage */
    /* Need to get a fresh AA result here for some reason */
    aa = &getAnalysis<AAResultsWrapperPass>(f).getAAResults();
    changes |= convert_pointer_phis(f, livi, aa, dt);

    /* Recompute analysis if the code changed */
    if( changes ) {
      livi->recompute(f);
      dt->recalculate(f);
      pdt->recalculate(f);
    }

    /* Do the splitting */
    auto stages = pipeline(f);

    /* Rewrite the Nanotube setup function */
    extend_nanotube_setup(stages, kernel_info);

    print_pipeline_stats(f);
    f.eraseFromParent();
  }

  delete(setup);
  setup = nullptr;

  return changes;
}

setup_func* stage_function_t::setup = nullptr;

stage_function_t::map_to_insts_t stage_function_t::map_to_reqs;
stage_function_t::map_to_insts_t stage_function_t::map_to_rcvs;

char Pipeline::ID = 0;
static RegisterPass<Pipeline>
  X("pipeline", "Break up Nanotube packet kernels into a pipeline.",
    true,
    false
  );
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
