/*******************************************************/
/*! \file Liveness.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Liveness analysis of values and stack allocations.
**   \date 2021-01-11
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "Liveness.hpp"

#define DEBUG_TYPE "liveness"

namespace std {
  template<> struct hash<llvm::MemoryLocation> {
    std::size_t operator()(const llvm::MemoryLocation& mloc) const noexcept {
      std::size_t h1 = std::hash<const llvm::Value*>{}(mloc.Ptr);
      std::size_t h2 = mloc.Size.toRaw();
      return h1 ^ h2;
    }
  };
}

void liveness_info_t::get_live_in(Instruction *inst,
                                  std::vector<Value*>* values,
                                  std::vector<MemoryLocation>* stack) {
  auto mem_it = live_mem_in_inst.find(inst);
  auto val_it = live_in_inst.find(inst);

  if( mem_it != live_mem_in_inst.end() ) {
    stack->insert(stack->end(), mem_it->second.begin(),
                  mem_it->second.end());
  }

  if( val_it != live_in_inst.end() ) {
    values->insert(values->end(), val_it->second.begin(),
                   val_it->second.end());
  }
}

void liveness_info_t::get_live_out(Instruction* inst,
                                   std::vector<Value*>* values,
                                   std::vector<MemoryLocation>* stack) {
  auto mem_it = live_mem_out_inst.find(inst);
  auto val_it = live_out_inst.find(inst);

  if( mem_it != live_mem_out_inst.end() ) {
    stack->insert(stack->end(), mem_it->second.begin(),
                  mem_it->second.end());
  }

  if( val_it != live_out_inst.end() ) {
    values->insert(values->end(), val_it->second.begin(),
                   val_it->second.end());
  }
}

void
liveness_info_t::find_producers_in_def(Instruction* inst, MemoryAccess* acc,
                                       MemoryLocation mloc,
                                       std::vector<memory_use_t>* memory_uses)
{
  aa_helper aah(aa);
  auto* walker = mssa->getWalker();
  while( true ) {
    LLVM_DEBUG(dbgs() << "    Memory access " << *acc << '\n');

    /* Nothing before us... remaining mlocs must be from elsewhere */
    if( mssa->isLiveOnEntryDef(acc) ) {
      LLVM_DEBUG(dbgs() << "    LiveOnEntry\n");
      memory_uses->emplace_back(nullptr, inst, mloc);
      return;
    }

    /* Special case: this is a MemoryPHI -> simply continue tracing
     * through the children */
    if( isa<MemoryPhi>(acc) ) {
      LLVM_DEBUG(
        dbgs() << "    Memory Phi following\n";
        for( auto it = acc->defs_begin(); it != acc->defs_end(); ++it)
          dbgs() << "      " << **it << '\n';
      );

      for( auto it = acc->defs_begin(); it != acc->defs_end(); ++it)
        find_producers_in_def(inst, *it, mloc, memory_uses);
      return;
    }

    /* Bail if there is more madness :) */
    if( !isa<MemoryDef>(acc) ) {
      errs() << "ERROR: Liveness Analysis - Unusual MemoryAccess:" << *acc
             << "\nAborting!\n";
      assert(false);
    }

    auto* memdef  = cast<MemoryDef>(acc);
    auto* meminst = memdef->getMemoryInst();
    LLVM_DEBUG(dbgs() << "    Inst: " << *meminst << '\n');
    if( isa<CallInst>(meminst) && ignore_function(cast<CallInst>(meminst)) ) {
      acc = walker->getClobberingMemoryAccess(memdef);
      continue;
    }

    /* Check overlap with the defining store */
    auto mri = aah.get_mri(meminst, mloc);
    if( isModSet(mri) ) {
      /* Special case: Check the base of the mloc for being (derived from)
       * a PHI node */
      auto* ptr_base =
        const_cast<Value*>(mloc.Ptr->stripPointerCasts());
      if( isa<PHINode>(ptr_base) || isa<SelectInst>(ptr_base) ) {
        std::vector<Value*> incoming_values;
        if( isa<PHINode>(ptr_base) ) {
          LLVM_DEBUG(dbgs() << "    PHI Base: " << *ptr_base << '\n');
          auto* phi = cast<PHINode>(ptr_base);
          for( auto& u : phi->incoming_values() )
            incoming_values.emplace_back(u.get());
        } else {
          LLVM_DEBUG(dbgs() << "    Select Base: " << *ptr_base << '\n');
          auto* sel = cast<SelectInst>(ptr_base);
          incoming_values.emplace_back(sel->getTrueValue());
          incoming_values.emplace_back(sel->getFalseValue());
        }

        /* We now know that the consumer reads from a "split" memory
         * location.  Let's check the defined constituents and see if they
         * are ALL overwritten, which suggests the store also writes to the
         * "split" location.*/
        bool all_bases_stored = true;
        for( auto* v : incoming_values ) {
          if( isa<UndefValue>(v) )
            continue;
          auto part_mri = aah.get_mri(meminst, mloc.getWithNewPtr(v));
          all_bases_stored &= isModSet(part_mri);
          LLVM_DEBUG(dbgs() << "    Checking " << *v
                            << " Res: " << part_mri << '\n');
        }
        if( !all_bases_stored ) {
          /* Create separate memory uses for the
           *   producerS -> phi
           * and
           *   phi -> original consumer */

          LLVM_DEBUG(dbgs() << "    Not all covered => splitting "
                            << "MemoryLocation!\n");
          memory_uses->emplace_back(ptr_base, inst, mloc);
          for( auto* v : incoming_values ) {
            if( isa<UndefValue>(v) )
              continue;
            find_producers_in_def(cast<Instruction>(ptr_base), acc,
                mloc.getWithNewPtr(v), memory_uses);
          }
          return;
        }
      }

      /* Normal case: simply add the use */
      memory_uses->emplace_back(meminst, inst, mloc);

      /* XXX: Only return when we are sure that there are no possible other
       * stores to the memory location (for example due to MayAlias / Mod)
       * or PartialAlias */
      return;
    }

    /* Continue looking if our memory location did not alias */
    acc = walker->getClobberingMemoryAccess(memdef);
  }
}

void liveness_info_t::memory_liveness_analysis_backward(Function& f) {
  /* Liveness analysis for all memory locations */
  errs() << f.getName() << '\n';

  std::vector<memory_use_t> memory_uses;

  for( auto& inst : instructions(f) ) {
    std::vector<MemoryLocation> in_mlocs;
    std::vector<MemoryDef*>     in_mdef;
    std::vector<MemoryLocation> out_mlocs;
    std::vector<MemoryDef*>     out_mdef;

    /* Process potential memory producers */
    //XXX: Do we actually need to do anything?
    if( inst.mayWriteToMemory() ) {
    }

    /* Trace back memory locations from consumers */
    if( inst.mayReadFromMemory() ) {
      LLVM_DEBUG(dbgs() << inst << '\n');
      bool succ = get_memory_inputs(&inst, &in_mlocs, &in_mdef);
      if( !succ ) {
        errs() << "ERROR: Liveness Analysis - Could not properly analyse "
               << "memory for\n" << inst << '\n';
        assert(false);
        //continue;
      }

      // dbgs() << "    Memory Locations:\n";
      // for( auto& mloc : in_mlocs )
      //   dbgs() << "      " << mloc << '\n';

      /* Walk the MemorySSA and collect writers */
      auto* walker = mssa->getWalker();
      MemoryAccess* memacc = mssa->getMemoryAccess(&inst);
      memacc = walker->getClobberingMemoryAccess(memacc);

      for( auto mloc : in_mlocs )
        find_producers_in_def(&inst, memacc, mloc, &memory_uses);
    } /* if inst.mayReadFromMemory() */
  }

  std::sort(memory_uses.begin(), memory_uses.end(), memory_use_t::cmp);
  LLVM_DEBUG(
    for( auto& mu : memory_uses ) {
      dbgs() << "Memory Use\n  Prod: ";
      if( mu.producer != nullptr )
        dbgs() << *mu.producer;
      else
        dbgs() << "LiveOnEntry";
      dbgs() << "\n  Cons: " << *mu.consumer
             << "\n  Loc: " << mu.loc << '\n';
    }
  );
}

template <typename M, typename V>
using map_to_vec_t = std::unordered_map<M, std::vector<V>>;
template <typename M, typename S>
using map_to_set_t = std::unordered_map<M, std::unordered_set<S>>;

/**
 * Compute live memory locations for a function in a forward pass.
 *
 * The algorithm consists of multiple steps:
 * (1) collect found allocations (memory locations) and record them per
 *     basic block.  collect instructions (loads / stores / calls) that
 *     access these allocations => this flows forwards and builds the lists
 *     of available (visible) memory locatins per basic block and then
 *     intersects these locations with the instructions in the block
 * (2) flow the information of readers backwards through the program on the
 *     basic blocks to capture liveness that way; and record when a read is
 *     the last one to access the location (this can be multiple loads per
 *     location in case control flow diverges)
 * (3) trace backwards on the instruction level and mark all instruction
 *     between the last read and the first write as having the memory
 *     location live.
 *
 * TOOD:
 *   - instead of always querying all (detected) memory locations with all
 *     loads / stores / calls, we could flow forward the allocations
 *     through the program (and through pointer transformations and phis)
 *     and mark each load / store with the allocations it could potentially
 *     access => that is basically again what the mem2req pass does
 */
void liveness_info_t::memory_liveness_analysis_forward(Function& f) {
  aa_helper aah(aa);
  const auto& dl = f.getParent()->getDataLayout();

  /* Known memory locations coming out of / going into a basic block */
  map_to_set_t<BasicBlock*, MemoryLocation> out_locs;

  /* Capture all potential(!) readers / writers per mem loc */
  map_to_vec_t<MemoryLocation, Instruction*> writers, readers;

  /* Track the outgoing and incoming written mem locs per bb */
  map_to_set_t<BasicBlock*, MemoryLocation> out_written;

  /* Record first / last accesses that limit the lifetime.  Note that this
   * can be multiple, e.g., in parallel basic blocks. */
  map_to_vec_t<MemoryLocation, Instruction*> first_writes, last_reads;

  /* Collect readers per BB for backwards traversal */
  map_to_vec_t<BasicBlock*, Instruction*> reads;

  /* Collect mem locs per read for backwards traversal */
  map_to_vec_t<Instruction*, MemoryLocation> read_locs;

  typedef dep_aware_converter<BasicBlock> dac_t;
  dac_t dac;
  for( auto& bb : f )
    dac.insert(&bb, pred_size(&bb));

  /* Walk through all basic blocks in CFG compatible order, and collect
   * information about allocas and how they are accessed */
  dac.execute([&](dac_t* dac, BasicBlock* bb) {
    LLVM_DEBUG(dbgs() << bb->getName() << '\n');

    for( auto* pred : predecessors(bb) ) {
      /* Compute in-memloc as the union over the previous out-memloc */
      auto& pred_out = out_locs[pred];
      out_locs[bb].insert(pred_out.begin(), pred_out.end());

      /* Union of previous outs is our in */
      auto& pred_wr = out_written[pred];
      out_written[bb].insert(pred_wr.begin(), pred_wr.end());
    }

    for( auto& inst : *bb ) {
      /* Record interesting memory locations; from allocas only (for now) */
      auto* alloca = dyn_cast<AllocaInst>(&inst);
      if( alloca != nullptr ) {
        auto size   = alloca->getAllocationSizeInBits(dl);
        auto tysize = LocationSize::unknown();
        if( size.hasValue() ) {
          tysize = LocationSize::precise(size.getValue() / 8);
        } else {
          errs() << "Unknown size for alloca: " << *alloca << '\n';
        }
        out_locs[bb].emplace(alloca, tysize);
      }

      if( !inst.mayReadFromMemory() && !inst.mayWriteToMemory() )
        continue;

      /* Skip calls to uninteresting / confusing functions */
      if( isa<CallInst>(&inst) && ignore_function(cast<CallInst>(&inst)) )
        continue;

      /* Intersect the current instruction with all known memory locations */
      bool inst_reads = false;
      bool print_ins  = true;
      for( auto& mloc : out_locs[bb] ) {
        auto res    = aah.get_mri(&inst, mloc);
        if( isNoModRef(res) )
          continue;
        bool rd = inst.mayReadFromMemory() && isRefSet(res);
        bool wr = inst.mayWriteToMemory()  && isModSet(res);

        if( rd ) {
          readers[mloc].emplace_back(&inst);
          read_locs[&inst].emplace_back(mloc);
          inst_reads = true;
        }
        if( wr ) {
          writers[mloc].emplace_back(&inst);
          bool new_one = out_written[bb].insert(mloc).second;
          if( new_one )
            first_writes[mloc].emplace_back(&inst);
        }

        /* Debug output */
        if( print_ins ) {
          LLVM_DEBUG(dbgs() << inst << "\n");
          print_ins = false;
        }
        LLVM_DEBUG(
          dbgs() << "    " << (rd ? "R" : "") << (wr ? "W" : "");
          dbgs() << mloc << '\n';
        );
      } /* for( auto* alloca : out_allocas[bb] ) */

      /* Collect instructions per BB that read (at least) one of our
       * known locations */
      if( inst_reads )
        reads[bb].emplace_back(&inst);
    } /* for( auto& inst : *bb ) */

    for( auto* succ : successors(bb) )
      dac->mark_dep_ready(succ);
  });


  /* Record per-BB which memory locations are known to be live, aka, read
   * in a successor */
  map_to_set_t<BasicBlock*, MemoryLocation> known_live;

  /* Go backwards through the CFG and track the last reads per alloca */
  for( auto& bb : f )
    dac.insert(&bb, succ_size(&bb));

  dac.execute([&](dac_t* dac, BasicBlock* bb) {
    /* Our known-live values are the union of the successors */
    for( auto* succ : successors(bb) ) {
      auto& succ_known = known_live[succ];
      known_live[bb].insert(succ_known.begin(), succ_known.end());
    }

    /* Add any reads / read memory locations to the known-live list, and
     * find the last read (by going reverse and remembering the first) for
     * each location */
    for( auto it = reads[bb].rbegin(); it != reads[bb].rend(); ++it ) {
      auto* read = *it;
      for( auto& mloc : read_locs[read] ) {
        bool new_one = known_live[bb].insert(mloc).second;
        if( new_one )
          last_reads[mloc].emplace_back(read);
      }
    }

    for( auto* pred : predecessors(bb) )
      dac->mark_dep_ready(pred);
  });

  /* Record live memory per instruction */
  /* XXX: Instead of tracing back through the program once for every single
   * mloc, we should be backtracking through the program once, and have a
   * map instruction -> vector of mloc that captures the last reads, and
   * the same for instruction -> vector of mloc to capture the first write.
   * Then go through all instructions once, and keep a set of active mlocs
   * and only do delta updates, here.  That might be a little tricky with
   * branches and joins, but should be doable.
   * IIRC, I wrote that to begin with but then got royally confused.  It
   * also feels like the bactracking on the BBs would be enough to capture
   * this?!?
   *
   * Alternatively / in addition, it would make sense to track this only on
   * a per-BB level for those blocks where the liveness does not change
   * (i.e. those that are not first_write->getParent() or
   * last_read->getParent().
   */
  for( auto& mloc_vec : last_reads ) {
    auto& mloc   = mloc_vec.first;
    auto& ends   = mloc_vec.second;
    std::unordered_set<Instruction*> starts(first_writes[mloc].begin(),
                                            first_writes[mloc].end());

    std::vector<Instruction*>       todo(ends.begin(), ends.end());
    std::unordered_set<BasicBlock*> done_bb;

    while( todo.size() != 0 ) {
      auto* inst = todo.back();
      auto* bb   = inst->getParent();
      todo.pop_back();

      /* We have found a livetime start -> done */
      if( starts.count(inst) != 0)
        continue;

      /* Keep tracing */
      mark(live_mem_in_inst, inst, mloc);
      if( inst != &bb->front() ) {
        /* Mark previous inst live out, and add to worklist */
        auto* pred_inst = inst->getPrevNode();
        mark(live_mem_out_inst, pred_inst, mloc);
        todo.push_back(pred_inst);
      } else {
        /* Close this BB and trace through the predecessors */
        mark(live_mem_in_bb, bb, mloc);
        done_bb.insert(bb);
        for( auto* pred : predecessors(bb) ) {
          /* Skip blocks we have already processed */
          if( done_bb.count(pred) != 0 )
            continue;
          /* Skip blocks that don't have the location live */
          if( out_written[pred].count(mloc) == 0 )
            continue;
          mark(live_mem_out_bb, pred, mloc);
          auto* pred_inst = &pred->back();
          mark(live_mem_out_inst, pred_inst, mloc);
          todo.push_back(pred_inst);
        }
      }
    }
  }

  /* Print out our findings */
  LLVM_DEBUG(
    for( auto& mloc_vec : first_writes ) {
      auto& mloc = mloc_vec.first;
      dbgs() << "MLoc: " << mloc << "\n  First write(s):\n";
      for( auto* wr : mloc_vec.second )
        dbgs() << "  " << *wr << '\n';
      dbgs() << "  Last read(s):\n";
      for( auto* rd : last_reads[mloc] )
        dbgs() << "  " << *rd << '\n';
    }
  );

  /* Print out liveness per instruction in CFG-friendly order */
  LLVM_DEBUG(
    for( auto& bb : f )
      dac.insert(&bb, pred_size(&bb));
    dac.execute([&](dac_t* dac, BasicBlock* bb) {
      dbgs() << '\n' << bb->getName() << ":\n";
      for ( auto& inst : *bb ) {
        dbgs() << inst << '\n';
        if( live_mem_in_inst[&inst].size() > 0 )
          dbgs() << "    Live In:\n";
        for( auto& mloc : live_mem_in_inst[&inst] )
          dbgs() << "    " << mloc << '\n';
        if( live_mem_out_inst[&inst].size() > 0 )
          dbgs() << "    Live Out:\n";
        for( auto& mloc : live_mem_out_inst[&inst] )
          dbgs() << "    " << mloc << '\n';
      }

      for( auto* succ : successors(bb) )
        dac->mark_dep_ready(succ);
    });
  );
}

void liveness_info_t::reset_state() {
  live_mem_in_inst.clear();
  live_mem_out_inst.clear();
  live_mem_in_bb.clear();
  live_mem_out_bb.clear();
  live_in_inst.clear();
  live_out_inst.clear();
  live_in_bb.clear();
  live_out_bb.clear();
}

void liveness_info_t::recompute(Function& f) {
  reset_state();

  /* Liveness analysis for all values */
  for( auto& arg : f.args() )
    liveness_analysis(&arg);
  for( auto& g : f.getParent()->globals() )
    liveness_analysis(&g);
  for( auto& inst : instructions(f) )
    liveness_analysis(&inst);

  memory_liveness_analysis_forward(f);
  //memory_liveness_analysis_backward(f);
  LLVM_DEBUG(print_liveness_results(dbgs(), f));
}

bool liveness_info_t::ignore_function(CallBase* call) {
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
    exit(1);
  }
  StringRef name = callee->getName();
  if( name.equals("printf") ||
      name.equals("__assert_fail") )
    return true;
  return false;
}

bool liveness_info_t::get_memory_inputs(Value* val,
                          std::vector<MemoryLocation>* input_mlocs,
                          std::vector<MemoryDef*>* input_mdefs) {
  assert(input_mlocs != nullptr);
  assert(input_mdefs != nullptr);
  /* Special memory-sensitive instructions */
  /* Calls may read pointer arguments */
  if( isa<CallInst>(val) ) {
    auto* call = cast<CallInst>(val);

    /* Skip functions that don't read memory */
    auto fmrb = aa->getModRefBehavior(call);
    if( AAResults::doesNotReadMemory(fmrb) )
      return true;

    if( ignore_function(call) )
      return true;

    /* Give up on functions that are all over memory */
    if( !AAResults::onlyAccessesInaccessibleOrArgMem(fmrb) ) {
      errs() << "XXX: Not analysing memory-crazy call: " << *call << '\n';
      return false;
    }

    /* Record memory locations that this function reads from */
    for( auto& u : call->args() ) {
      auto* arg = u.get();
      if( !arg->getType()->isPointerTy() )
        continue;

      unsigned idx = u.getOperandNo();
      auto  mri = aa->getArgModRefInfo(call, idx);
      if( !isRefSet(mri) )
        continue;
      auto mloc = get_memory_location(call, idx, *tli);
      input_mlocs->emplace_back(mloc);
      LLVM_DEBUG(dbgs() << "    Arg: " << idx << " Mem: " << mloc << '\n');
    }

    // Disable this for testing.  It is usually better for the caller to do
    // something with the MemorySSA.
#if 0
    /* We now have a sensible function that accesses memory.  Go forth and
     * try to make sense of the madness that is memory dataflow :) */
    auto* mem = mssa->getMemoryAccess(call);
    LLVM_DEBUG(
      dbgs() << "  Call: " << *call << " MemSSA: " << *mem << '\n');

    /* Go through all potential memory definitions for this and try to find
     * the previous writer */
    for( auto it = mem->defs_begin(); it != mem->defs_end(); ++it ) {
      auto* def = dyn_cast<MemoryDef>(*it);
      if( def == nullptr ) {
        errs() << "XXX: Unhandled memory def " << **it << " for " << *call
               << '\n';
        return false;
      }
      input_mdefs->push_back(def);
      LLVM_DEBUG(dbgs() << "    Def: " << **it << '\n');
    }
#endif

  } else if( isa<LoadInst>(val) ){
    auto* ld = cast<LoadInst>(val);

    auto mloc = MemoryLocation::get(ld);
    input_mlocs->emplace_back(mloc);
    LLVM_DEBUG(dbgs() << "    Mem: " << mloc << '\n');

    auto* mem = mssa->getMemoryAccess(ld);
    LLVM_DEBUG(
      dbgs() << "  Load: " << *ld << " MemSSA: " << *mem << '\n');

    // Disable this for testing.  It is usually better for the caller to do
    // something with the MemorySSA.
#if 0
    /* Go through all potential memory definitions for this and try to find
     * the previous writer */
    for( auto it = mem->defs_begin(); it != mem->defs_end(); ++it ) {
      auto* def = dyn_cast<MemoryDef>(*it);
      if( def == nullptr ) {
        errs() << "XXX: Unhandled memory def " << **it << " for " << *ld
               << '\n';
        return false;
      }
      input_mdefs->push_back(def);
      LLVM_DEBUG(dbgs() << "    Def: " << **it << '\n');
    }
#endif

  } else if( isa<StoreInst>(val) ){
    /* Stores have no memory INPUT dependency -> fall through! */
  } else {
    errs() << "XXX: unknown memory inputs for " << *val << '\n';
    return false;
  }
  return true;
}

void liveness_info_t::print_liveness_results(raw_ostream& os, Function& f) {
  typedef dep_aware_converter<BasicBlock> dac_t;
  dac_t dac;
  for( auto& bb : f )
    dac.insert(&bb, pred_size(&bb));

  os << "Function: " << f.getName() << " Liveness\n";
  dac.execute([&](dac_t* dac, BasicBlock* bb) {
    dbgs() << '\n' << bb->getName() << ":\n";
    for ( auto& inst : *bb ) {
      os << inst << '\n';
      os << "    Live In: ";
      for( auto* v : live_in_inst[&inst] ) {
        v->printAsOperand(os, false);
        os << " ";
      }
      os << '\n';
      os << "    Live Out: ";
      for( auto* v : live_out_inst[&inst] ) {
        v->printAsOperand(os, false);
        os << " ";
      }
      os << '\n';
    }

    for( auto* succ : successors(bb) )
      dac->mark_dep_ready(succ);
  });
}

void liveness_info_t::liveness_analysis(Value* v) {
  done.clear();
  for( auto& u : v->uses() ) {
    auto* inst = dyn_cast<Instruction>(u.getUser());
    if( inst == nullptr )
      continue;
    if( isa<PHINode>(inst) ) {
      auto* phi = cast<PHINode>(inst);
      live_out_at_block(phi->getIncomingBlock(u), v);
    } else {
      live_in_at_statement(inst, v);
    }
  }
}
void liveness_info_t::live_out_at_block(BasicBlock* bb, Value* v) {
  mark(live_out_bb, bb, v);
  if( done.count(bb) > 0 )
    return;
  done.insert(bb);
  live_out_at_statement(&bb->back(), v);
}
void liveness_info_t::live_in_at_statement(Instruction* inst, Value* v) {
  mark(live_in_inst, inst, v);
  auto* bb = inst->getParent();
  if( inst == &bb->front() ) {
    mark(live_in_bb, bb, v);
    for( auto* pred : predecessors(bb) )
      live_out_at_block(pred, v);
  } else {
    live_out_at_statement(inst->getPrevNode(), v);
  }
}
void liveness_info_t::live_out_at_statement(Instruction* inst, Value* v) {
  mark(live_out_inst, inst, v);
  Value* def = inst; // XXX: This will need changes for memory locations!
  if( v != def )
    live_in_at_statement(inst, v);
}
char liveness_analysis::ID = 0;
static RegisterPass<liveness_analysis>
  X("liveness", "Analyse liveness of values and stack allocations.",
    true,
    false
  );
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
