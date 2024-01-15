/*******************************************************/
/*! \file  Mem2req.h
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert Nanotube memory interface to requests
**   \date 2020-07-14
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"

#include "Dep_Aware_Converter.h"
#include "nanotube_api.h"

namespace {
  using llvm::BasicBlock;
  using llvm::CallInst;
  using llvm::Function;
  using llvm::IRBuilder;
  using llvm::Instruction;
  using llvm::IntToPtrInst;
  using llvm::LLVMContext;
  using llvm::Module;
  using llvm::Twine;
  using llvm::Type;
  using llvm::Value;

  using nanotube::dep_aware_converter;

  struct mem_to_req : public llvm::ModulePass {

    mem_to_req() : ModulePass(ID) {}
    bool runOnModule(Module& m) override;

    static char ID;
  }; // struct mem_to_req

  /* Information about a pointer root. */
  struct root_meta {
    nanotube_map_id_t map_id;

    /* The buffer holding the map entry */
    Value* buffer;
  };

  struct pointer_meta {
    typedef std::unordered_set<root_meta*> possible_roots_t;

    Value* base;      /* base package / map */
    Value* buffer;    /* The buffer being referenced */
    Value* key;       /* key for a map lookup; nullptr for packets */
    Value* key_sz;    /* size of the key as a value */
    Value* dummy_read_ret;  /* return value of the dummy map read */
    possible_roots_t possible_roots; /* A set of possible roots */

    bool   is_map;    /* true -> map; false -> packet */
  };
  typedef std::unordered_map<Value*, pointer_meta*> val_meta_map;
  typedef std::vector<pointer_meta*>                meta_vec;

  /* The state for a buffer at a single point in the program. */
  struct buffer_state {
    /* The buffer has been filled on some path to this point. */
    bool filled = false;

    /* There was a conflicting store on some path to this point. */
    Instruction *conflict = nullptr;
  };

  /* The abstract state of the program at a single point in the
   * code. */
  struct prog_state {
    typedef std::unordered_map<Value*,buffer_state> buf_states_t;

    buf_states_t buf_states;
  };

  struct flow_conversion {
    typedef dep_aware_converter<BasicBlock> dac_t;
    typedef std::unordered_map<BasicBlock*,prog_state> bb_state_t;

    /* Note: Pointers to the root_meta structures are held elsewhere
     * (e.g. pointer_meta), so these containers must not move them
     * when other elements are added. */
    typedef std::list<root_meta> root_meta_list_t;
    typedef std::unordered_map<nanotube_map_id_t, root_meta_list_t> map_metas_t;
    typedef std::unordered_map<Value*, root_meta> packet_metas_t;

    map_metas_t   map_metas;
    packet_metas_t packet_metas;
    meta_vec      meta_nodes;
    val_meta_map  val_to_meta;
    std::vector<Instruction*> deletion_candidates;
    dac_t dac;

    /* The program state at the current instruction. */
    prog_state    current_state;

    /* The program state on entry to each basic block.  The state for
     * the entry block is initialized when the pass starts.  The state
     * for any other basic block is populated as the predecessors are
     * encountered.  States are removed from this map when the basic
     * block is processed. */
    bb_state_t    bb_states;

    Module&       m;
    LLVMContext&  c;
    Function&     func;
    flow_conversion(Function& f);
    void flow(BasicBlock *bb);
    void flow(Instruction* inst);

    void convert_nanotube_root(CallInst* inst);
    void convert_call(CallInst* call);
    bool try_convert_special_case_call(CallInst* call);

    void convert_mem(Instruction* inst);
    bool try_convert_memcpy(Instruction* inst);
    void replace_and_cleanup(Instruction* inst, Value* repl,
                             IntToPtrInst* i2p);

    Instruction* typed_alloca(Type* ty, const Twine& name, Value* size,
                              IRBuilder<>* ir);
  };
}; //anonymous namespace
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
