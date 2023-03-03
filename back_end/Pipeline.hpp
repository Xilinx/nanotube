#ifndef __PIPELINE_HPP__
#define __PIPELINE_HPP__
/*******************************************************/
/*! \file Pipeline.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert a properly structured Nanotube fragement into pipeline stages.
**   \date 2021-01-11
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <unordered_map>
#include <unordered_set>

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "../include/nanotube_api.h"
#include "Dep_Aware_Converter.h"
#include "Intrinsics.h"
#include "Liveness.hpp"
#include "Nanotube_Alias.hpp"
#include "printing_helpers.h"
#include "setup_func.hpp"

namespace {
class stage_function_t;

struct Pipeline : public llvm::ModulePass {
  Pipeline() : ModulePass(ID) {
  }
  void get_all_analysis_results(Function& f);

  /* LLVM-specific */
  static char ID;

  /* Used analysis passes */
  llvm::DominatorTree*     dt;
  llvm::PostDominatorTree* pdt;
  llvm::MemorySSA*         mssa;
  llvm::AliasAnalysis*     aa;
  llvm::TargetLibraryInfo* tli;
  nanotube::liveness_info_t*   livi;

  setup_func* setup;

  void getAnalysisUsage(AnalysisUsage &info) const override;

  bool runOnModule(Module& m) override;

  /* Spliting into pipeline functions */
  typedef std::vector<stage_function_t*> stages_t;
  void print_pipeline_stats(llvm::Function& f) __attribute__((unused));
  stages_t pipeline(llvm::Function& f);
  void parse_max_access_sizes(llvm::Function& f);
  void determine_stages(stages_t* stages, Function& f);
  stage_function_t* get_stage(Instruction* start, Instruction** end, unsigned id, Type* in_state_ty);
  void copy_app_code(stage_function_t* stage, BasicBlock* last_prologue, ValueToValueMapTy& vmap);
  void collect_liveness_data(stage_function_t* stage,
                             llvm::Type* in_state_ty, llvm::LLVMContext& c);

  bool is_control_flow_converged(llvm::Instruction* start, llvm::Instruction* end);
  void extract_live_data(llvm::Instruction* inst, std::vector<llvm::Value*>* liv,
                         std::vector<llvm::MemoryLocation>* lim, bool is_out);
  void convert_to_taps(stage_function_t* stage);
  void extend_nanotube_setup(const stages_t& stages, const kernel_info& ki);
  void create_stage_epilogue(stage_function_t* stage, Value* packet_word,
                             std::vector<llvm::Value*>& live_out_val,
                             std::vector<llvm::MemoryLocation>& live_out_mem,
                             llvm::BasicBlock** out_app_entry,
                             llvm::BasicBlock** out_bypass_entry);
};

class stage_function_t {
  public:
    static stage_function_t* create(const llvm::Twine& base_name,
                                    const llvm::Twine& stage_name,
                                    unsigned idx,
                                    llvm::Module* m, llvm::LLVMContext& c);

    static FunctionType* get_function_type(llvm::Module* m, llvm::LLVMContext& c);
    static void set_setup_func(setup_func* sf) {
      setup = sf;
      map_to_rcvs.clear();
      map_to_reqs.clear();
    }

    llvm::Value* get_packet_arg();
    llvm::Value* get_context_arg();
    llvm::Value* get_user_arg();
    llvm::Value* get_in_state_arg();
    llvm::Value* get_out_state_arg();

    llvm::StructType* get_live_in_ty()  { return live_in_ty; }
    llvm::StructType* get_live_out_ty() { return live_out_ty; }
    bool has_live_in() {  return live_in_ty  != nullptr; }
    bool has_live_out() { return live_out_ty != nullptr; }

    llvm::Type* get_map_resp_data_ty() { return map_resp_data_ty; }

    /* Code generators */
    llvm::BasicBlock* marshal_live_state(std::vector<llvm::Value*> live_values,
                                   std::vector<llvm::MemoryLocation> live_mem,
                                   llvm::Type* live_state_ty,
                                   llvm::Value* buffer,
                                   llvm::BasicBlock* marshal_bb = nullptr);
    llvm::BasicBlock* unmarshal_live_state(llvm::ArrayRef<llvm::Value*> live_values,
                                     llvm::ArrayRef<MemoryLocation> live_mem,
                                     llvm::ValueToValueMapTy* vmap,
                                     llvm::Type* live_state_ty,
                                     llvm::Value* buffer);

    llvm::BasicBlock* read_app_state(llvm::Value* dest_state,
                                     llvm::BasicBlock* insert_bb);
    llvm::BasicBlock* read_map_response(llvm::Value* dest_state,
                                        llvm::Value* result,
                                        llvm::BasicBlock* insert_bb);
    llvm::BasicBlock* read_packet_word(llvm::Value* dest_word,
                                 llvm::Instruction* insert_before,
                                 bool checked = false);
    llvm::BasicBlock* try_call(llvm::Instruction* insert_before,
                               std::function<llvm::Value*(llvm::IRBuilder<>&)> gen);
    llvm::BasicBlock* read_from_channel(llvm::Constant* channel_id, llvm::Value* data,
                                  llvm::Constant* data_size, llvm::Instruction* insert_before);
                                  // XXX: Or don't these have to be constants?
    llvm::BasicBlock* checked_read(llvm::Value* dest, llvm::Type* ty,
                                   llvm::Value* static_check,
                                   unsigned channel, llvm::StringRef name,
                                   llvm::Instruction* insert_before,
                                   llvm::Instruction** good_read_ip = nullptr);
    llvm::BasicBlock* checked_call(llvm::Value* static_check,
                                   llvm::Instruction* insert_before,
                                   llvm::Instruction** call_insert_place);

    void write_app_state(llvm::Value* src_state, llvm::Instruction* insert_before);
    void write_packet_word(llvm::Value* src_word, llvm::Instruction* insert_before);
    void write_cword(llvm::Value* cword, llvm::Instruction* insert_before);
    void write_to_channel(Constant* channel_id, llvm::Value* data,
                          Constant* data_size, llvm::Instruction* insert_before); // XXX: Or don't these have to be constants?

    llvm::BasicBlock*     get_thread_wait_exit(BasicBlock* close_to);

    /* State of the stage that must persist across cycles / invocations */
    /* Application live-In state */
    llvm::GlobalVariable* get_static_have_app_state();
    llvm::GlobalVariable* get_static_sent_app_state();
    llvm::GlobalVariable* get_static_app_state();

    /* Map response state */
    llvm::GlobalVariable* get_static_have_map_resp();
    llvm::GlobalVariable* get_static_map_resp_data();
    llvm::GlobalVariable* get_static_map_result();

    /* Packet word data */
    llvm::GlobalVariable* get_static_have_packet_word();
    llvm::GlobalVariable* get_static_packet_word();
    llvm::GlobalVariable* get_static_word_is_eop();

    /* Tap state */
    llvm::GlobalVariable* get_static_packet_read_tap_state();
    llvm::GlobalVariable* get_static_packet_read_data(unsigned size);
    llvm::GlobalVariable* get_static_packet_write_tap_state();
    llvm::GlobalVariable* get_static_packet_length_tap_state();
    llvm::GlobalVariable* get_static_packet_resize_ingress_tap_state();
    llvm::GlobalVariable* get_static_packet_eop_tap_state();

    llvm::GlobalVariable* get_static_packet_resize_egress_tap_state();
    llvm::GlobalVariable*
      get_static_packet_resize_egress_tap_state_packet_word();
    llvm::GlobalVariable* get_static_packet_resize_egress_cword();
    llvm::GlobalVariable* get_static_packet_resize_egress_have_cword();

    /* Code manipulators */
    void copy_partial_bbs(llvm::Instruction* start, Instruction* end,
                          llvm::ValueToValueMapTy* vmap);
    void copy_entire_bbs(ArrayRef<llvm::BasicBlock*> bbs, llvm::ValueToValueMapTy* vmap);
    void recreate_allocas(llvm::ValueToValueMapTy* vmap);
    void clean_up_dangling_debug();
    void remap_values(llvm::ValueToValueMapTy* vmap);

    void convert_packet_read(llvm::Value* packet_word, llvm::BasicBlock* bypass_bb);
    llvm::Value* convert_packet_write(Value* packet_word);
    void convert_packet_length(llvm::Value* packet_word, llvm::BasicBlock* bypass_bb);
    llvm::Value* convert_packet_drop(llvm::Value* packet_word);
    void convert_packet_resize_ingress(llvm::Value* packet_word);
    llvm::Value* convert_packet_resize_egress(Value* packet_word, llvm::BasicBlock* bypass_bb);
    void convert_map_op_receive();
    void convert_packet_drop();

    /* Translating accesses/ requests to taps */
    void scan_nanotube_accesses(setup_func* setup);
    void translate_packet_read(llvm::Instruction* inst);
    void translate_packet_write(llvm::Instruction* inst);

    /* Helpers */
    bool is_defined_here(const llvm::Value* v);
    bool value_used(llvm::Value* v, std::vector<Value*>::iterator* live_out_it);
    std::string get_parent_name(const llvm::Value* v);
    llvm::Instruction* get_stage_exit() { return stage_exit->getTerminator(); }
    Value* get_eop(IRBuilder<>& ir, Value* packet_word, Module* m);

    enum channel {
      PACKETS_IN,
      PACKETS_OUT,
      STATE_IN,
      STATE_OUT,
      CWORD_IN,
      CWORD_OUT,
      MAP_REQ,
      MAP_RESP,
    };

    Function* func;
    std::string stage_name;
    unsigned idx;

    static setup_func* setup;

    /* Content of the stage and live-in / live-out values.  Note that these
     * may refer to content of the original function before the stage is
     * carved out.  Once stage creation is complete, however, these will
     * refer to content in the stage.
     * The stage contains the interval [start, end) */
    Instruction*             start;
    Instruction*             end;
    std::vector<BasicBlock*> bbs;

    StructType* live_in_ty;
    StructType* live_out_ty;
    std::vector<llvm::Value*>   live_in_val;
    std::vector<MemoryLocation> live_in_mem;

    std::vector<llvm::Value*>   live_out_val;
    std::vector<MemoryLocation> live_out_mem;

    /* Map related information */
    std::unordered_map<nanotube_map_id_t, llvm::Value*> map_id_to_val;
    llvm::Type* map_resp_data_ty;
    llvm::Value* get_map_ptr(nanotube_map_id_t id);
    unsigned get_client_id(nanotube_map_id_t id, bool rcv);
    typedef std::unordered_map<nanotube_map_id_t,
              std::vector<stage_function_t*>> map_to_insts_t;
    static map_to_insts_t map_to_reqs;
    static map_to_insts_t map_to_rcvs;

    /* Keep all static variables for this stage in a hash rather than
     * declaring them here.  The hash is much easier to extend and just
     * reduces boilerplate (constructor, getters).
     */
    std::unordered_map<std::string, llvm::GlobalVariable*> static_vars;

    llvm::Value* packet_word;
    llvm::Value* word_is_eop;

    /* Interesting basic blocks inside this function*/
    llvm::BasicBlock*  unmarshal_bb;
    llvm::BasicBlock*  app_logic_entry;
    llvm::BasicBlock*  app_logic_exit;
    std::vector<llvm::BasicBlock*> app_logic;

    llvm::BasicBlock* split_app_bb(llvm::BasicBlock* app_bb,
        llvm::Instruction* split_before, const Twine& name = "") {
      auto* bb = app_bb->splitBasicBlock(split_before, name);
      if( app_bb == app_logic_exit )
        app_logic_exit = bb;
      return bb;
    }

    llvm::BasicBlock* thread_wait_exit; // exit when channel read fails, invokes thread_wait
    llvm::BasicBlock* stage_exit;       // normal stage exit

    /* Special accesses in this function */
    llvm::Instruction*   nt_call;

    llvm::CallInst*    map_op_req;
    llvm::ConstantInt* map_req_buf_size;
    llvm::CallInst*    map_op_rcv;
    llvm::ConstantInt* map_rcv_buf_size;
    llvm::Value*       drop_val;

    void set_nt_id(Intrinsics::ID id) { nt_id = id; }
    void set_nt_call(Instruction* call, Intrinsics::ID id);

    bool is_none()           { return nt_id == Intrinsics::none; }
    bool is_packet_read()    { return nt_id == Intrinsics::packet_read;}
    bool is_packet_write()   { return nt_id == Intrinsics::packet_write ||
                                      nt_id == Intrinsics::packet_write_masked;}
    bool is_resize_ingress() { return nt_id == Intrinsics::packet_resize_ingress;}
    bool is_resize_egress()  { return nt_id == Intrinsics::packet_resize_egress;}
    bool is_packet_length()  { return nt_id == Intrinsics::packet_bounded_length;}
    bool is_map_request()    { return map_op_req != nullptr; }
    bool is_map_receive()    {
      /* For now, map_receives have to be their own stage! */
      bool map_field = (map_op_rcv != nullptr);
      bool nt_op     = (nt_id == Intrinsics::map_op_receive);
      assert(map_field == nt_op);
      return map_field;
    }
    bool is_packet_drop()    { return nt_id == Intrinsics::packet_drop; }
    bool needs_app_send_guard() {return !is_packet_read() && !is_packet_length(); }
    bool needs_static_checked_packet_word() {return is_resize_egress(); }
    bool unset_app_state_eop() {return has_live_in() && !is_resize_egress(); }
    bool unset_map_resp_eop() {return is_map_receive(); }

  private:
    Intrinsics::ID nt_id;
    stage_function_t();
    llvm::GlobalVariable* get_static(StringRef var_name, Type* type);
};
} // anonymous namespace
#endif //#ifndef __PIPELINE_HPP__
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
