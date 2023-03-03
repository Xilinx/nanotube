/*******************************************************/
/*! \file aa_tie_in.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Testing external AA access from a module pass
**   \date 2021-09-06
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
 * Testing various combinations of out-of-tree custom AliasAnalysis, and
 * various users thereof.  In more detail:
 *
 * - custom_aa_pass: this is the custom AA pass that should live out-of-tree
 *   and make its insights available to other passes
 * - aa_function_pass_user: a FunctionPass that tries to use the
 *   custom_aa_pass via the AA aggregation mechanism in
 *   AAResultsWrapperPass
 * - aa_module_pass_user: a ModulePass that does the same via the
 *   LegacyAARGetter interface, going through the functions in the module
 * - aa_module_pass_indirect_user: a ModulePass that uses the custom AA
 *   indirectly via the aa_function_pass_user, in order to test out how to
 *   get access via the getAnalysis(F) interface
 * -aa_module_pass_gentle_indirect_user: a similar ModulePass, but being
 *  "gentle" in that it only asks whether an analysis is available by
 *  chance via getAnalysisIfAvailable; that does not seem to work from a
 *  ModulePass finding a FunctionPass.
 */

#include <map>
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"

using namespace llvm;
/********** The Custom AA **********/
namespace {
struct custom_aa_result_t : public AAResultBase<custom_aa_result_t> {
  friend AAResultBase<custom_aa_result_t>;
  using base_t = AAResultBase<custom_aa_result_t>;

  custom_aa_result_t(Function& f) {
    dbgs() << "Result constructor: " << f.getName() << '\n';
  }
  ~custom_aa_result_t() {
    dbgs() << "Result destructor\n";
  }

  AliasResult alias(const MemoryLocation& loc_a, const MemoryLocation& loc_b) {
    dbgs() << "custom_aa_result::alias(loc_a, loc_b))\n";
    return MayAlias;
  }
  ModRefInfo getArgModRefInfo(const CallBase* call, unsigned arg_idx) {
    dbgs() << "custom_aa_result::getArgModRefInfo(call, arg_idx)\n";
    return ModRefInfo::ModRef;
  }
  ModRefInfo getModRefInfo(const CallBase* call, const MemoryLocation& loc) {
    dbgs() << "custom_aa_result::getModRefInfo(call, loc)\n";
    return ModRefInfo::ModRef;
  }
  ModRefInfo getModRefInfo(const CallBase* call1, const CallBase* call2) {
    dbgs() << "custom_aa_result::getModRefInfo(call, loc)\n";
    return ModRefInfo::ModRef;
  }
  FunctionModRefBehavior getModRefBehavior(const CallBase* call) {
    dbgs() << "custom_aa_result::getModRefBehavior(call)\n";
    return FunctionModRefBehavior::FMRB_UnknownModRefBehavior;
  }
  FunctionModRefBehavior getModRefBehavior(const Function* func) {
    dbgs() << "custom_aa_result::getModRefBehavior(func)\n";
    return FunctionModRefBehavior::FMRB_UnknownModRefBehavior;
  }
};

struct custom_aa_pass : public FunctionPass {
  static char ID;
  custom_aa_result_t* result;
  custom_aa_pass() : FunctionPass(ID) {
    dbgs() << "custom_aa_pass Constructor\n";
  }
  ~custom_aa_pass() {
    dbgs() << "custom_aa_pass Destructor\n";
  }

  custom_aa_result_t& getResult() { return *result; }
  const custom_aa_result_t& getResult() const { return *result; }

  bool runOnFunction(Function& f) override {
    dbgs() << "custom_aa_pass::runOnFunction() " << f.getName() << '\n';

    result = new custom_aa_result_t(f);

    /**
     * Register a CB with the ExternalAAWrapper
     * This is an odd place to register the CB, but I could not find a
     * better place.  Suggestions welcome!  Also, I am unsure about the
     * lifetime and ownership of result here :|
     */
    auto& ext_aa = getAnalysis<ExternalAAWrapperPass>();
    ext_aa.CB = [this](Pass& p, Function& f, AAResults& aar) {
      dbgs() << "custom_aa_pass::callback() " << f.getName() << '\n';
      aar.addAAResult(*result);
    };

    return false;
  }

  void getAnalysisUsage(AnalysisUsage& AU) const override {
    AU.setPreservesAll();
    AU.addRequired<ExternalAAWrapperPass>();
  }
};
};

namespace llvm {
void initializecustom_aa_passPass(PassRegistry&);
void initializeaa_function_pass_userPass(PassRegistry&);
void initializeaa_module_pass_userPass(PassRegistry&);
void initializeaa_module_pass_indirect_userPass(PassRegistry&);
void initializeaa_module_pass_gentle_indirect_userPass(PassRegistry&);
};

char custom_aa_pass::ID = 0;
INITIALIZE_PASS_BEGIN(custom_aa_pass, "custom-aa-pass",
  "Custom AA Pass", false, true);
INITIALIZE_PASS_END(custom_aa_pass, "custom-aa-pass",
  "Custom AA Pass", false, true);

/********** FunctionPass User **********/
namespace {
struct aa_function_pass_user : public FunctionPass {
  static char ID;
  aa_function_pass_user() : FunctionPass(ID) {
    dbgs() << "aa_function_pass_user Constructor\n";
  }
  ~aa_function_pass_user() {
    dbgs() << "aa_function_pass_user Destructor\n";
  }

  typedef std::map<CallInst*, FunctionModRefBehavior> result_t;
  result_t call2fmrb;

  bool runOnFunction(Function& f) {
    dbgs() << "aa_function_pass_user::runOnFunction() " << f.getName() << '\n';

    auto* the_aa = &getAnalysis<AAResultsWrapperPass>().getAAResults();

    for( inst_iterator it = inst_begin(f); it != inst_end(f); ++it ) {
      if( !isa<CallInst>(*it) )
        continue;
      auto* call = &cast<CallInst>(*it);
      auto  fmrb = the_aa->getModRefBehavior(call);
      dbgs() << *call << " FMRB: " << fmrb << '\n';
      call2fmrb[call] = fmrb;
    }
    return false;
  }

  result_t& getResult() {
    dbgs() << "aa_function_pass_user::getResult()\n";
    return call2fmrb;
  }

  void getAnalysisUsage(AnalysisUsage& AU) const override {
    AU.setPreservesAll();
    AU.addRequired<custom_aa_pass>();
    AU.addRequired<AAResultsWrapperPass>();
  }
};
};

char aa_function_pass_user::ID = 0;
INITIALIZE_PASS_BEGIN(aa_function_pass_user, "aa-function-pass-user",
  "AA FunctionPass User", false, true);
INITIALIZE_PASS_END(aa_function_pass_user, "aa-function-pass-user",
  "AA FunctionPass User", false, true);

/********** ModulePass User: Directly querying AA **********/
namespace {
struct aa_module_pass_user : public ModulePass {
  static char ID;
  aa_module_pass_user() : ModulePass(ID) {
    dbgs() << "aa_module_pass_user Constructur\n";
  }
  ~aa_module_pass_user() {
    dbgs() << "aa_module_pass_user Destructur\n";
  }

  bool runOnModule(Module& m) {
    dbgs() << "aa_module_pass_user::runOnModule()\n";

    LegacyAARGetter legacy_aa(*this);

    for( auto& f : m) {
      if( f.isDeclaration() )
        continue;
      dbgs() << "  Func: " << f.getName() << '\n';

      /**
       * Trying to force the custom_aa_pass to exist and register itself
       * with ExternalAAWrapper, but it does not seem to work.  Instead, I
       * have to explicitly instantiate on the opt command line :(
       */
      getAnalysis<custom_aa_pass>(f);

      /* Query available AA for function F, hopefully including our
       * custom_aa_pass / custom_aa_result */
      auto* the_aa = &legacy_aa(f);

      for( inst_iterator it = inst_begin(f); it != inst_end(f); ++it ) {
        if( !isa<CallInst>(*it) )
          continue;
        auto* call = &cast<CallInst>(*it);
        auto  fmrb = the_aa->getModRefBehavior(call);
        dbgs() << "    " << *call << " FMRB: " << fmrb << '\n';
      }
    }
    return false;
  }

  void getAnalysisUsage(AnalysisUsage& AU) const override {
    AU.setPreservesAll();
    AU.addRequired<custom_aa_pass>();
    //AU.addRequired<ExternalAAWrapperPass>();
    AU.addRequired<AssumptionCacheTracker>();
    getAAResultsAnalysisUsage(AU);
  }
};
};

char aa_module_pass_user::ID = 0;
INITIALIZE_PASS_BEGIN(aa_module_pass_user, "aa-module-pass-user",
  "AA ModulePass User", false, true);
INITIALIZE_PASS_END(aa_module_pass_user, "aa-module-pass-user",
  "AA ModulePass User", false, true);

/********** ModulePass User: query via FunctionPass, query with getAnalysis  **********/
namespace {
struct aa_module_pass_indirect_user : public ModulePass {
  static char ID;
  aa_module_pass_indirect_user() : ModulePass(ID) {
    dbgs() << "aa_module_pass_indirect_user Constructur\n";
  }
  ~aa_module_pass_indirect_user() {
    dbgs() << "aa_module_pass_indirect_user Destructur\n";
  }

  bool runOnModule(Module& m) {
    dbgs() << "aa_module_pass_indirect_user::runOnModule()\n";

    for( auto& f : m) {
      if( f.isDeclaration() )
        continue;
      dbgs() << "  Func: " << f.getName() << '\n';

      /* Query via another function pass, emulating the case where the AA
       * is fed into another FunctionPass that we cannot use via the
       * LegacyAARGetter here.   This seems to pull in the custom_aa_pass
       * without having to specify it on the command line! */
      auto* aa_fpu = &getAnalysis<aa_function_pass_user>(f);
      auto& call2fmrb = aa_fpu->getResult();

      for( inst_iterator it = inst_begin(f); it != inst_end(f); ++it ) {
        if( !isa<CallInst>(*it) )
          continue;
        auto* call = &cast<CallInst>(*it);
        dbgs() << "    " << *call << " FMRB: " << call2fmrb[call] << '\n';
      }
    }
    return false;
  }

  void getAnalysisUsage(AnalysisUsage& AU) const override {
    AU.setPreservesAll();
    AU.addRequired<aa_function_pass_user>();
    AU.addRequired<AssumptionCacheTracker>();
    getAAResultsAnalysisUsage(AU);
  }
};
};

char aa_module_pass_indirect_user::ID = 0;
INITIALIZE_PASS_BEGIN(aa_module_pass_indirect_user,
    "aa-module-pass-indirect-user", "AA ModulePass Indirect User",
    false, true);
INITIALIZE_PASS_END(aa_module_pass_indirect_user,
    "aa-module-pass-indirect-user", "AA ModulePass Indirect User",
    false, true);

/********** ModulePass User: query via FunctionPass, query with IfAvailable **********/
namespace {
struct aa_module_pass_gentle_indirect_user : public ModulePass {
  static char ID;
  aa_module_pass_gentle_indirect_user() : ModulePass(ID) {
    dbgs() << "aa_module_pass_gentle_indirect_user Constructur\n";
  }
  ~aa_module_pass_gentle_indirect_user() {
    dbgs() << "aa_module_pass_gentle_indirect_user Destructur\n";
  }

  bool runOnModule(Module& m) {
    dbgs() << "aa_module_pass_gentle_indirect_user::runOnModule()\n";

    for( auto& f : m) {
      if( f.isDeclaration() )
        continue;
      dbgs() << "  Func: " << f.getName() << '\n';

      /* Query via another FunctionPass, but using the "gentle" interface
       * of getAnalysisIfAvailable.  This does not seem to work from a
       * ModulePass trying to get to a FunctionPass. */
      auto* aa_fpu = getAnalysisIfAvailable<aa_function_pass_user>();
      if( aa_fpu == nullptr ) {
        dbgs() << "  No available result from the AA_FPU for " << f.getName() << '\n';
        continue;
      }
      auto& call2fmrb = aa_fpu->getResult();

      for( inst_iterator it = inst_begin(f); it != inst_end(f); ++it ) {
        if( !isa<CallInst>(*it) )
          continue;
        auto* call = &cast<CallInst>(*it);
        dbgs() << "    " << *call << " FMRB: " << call2fmrb[call] << '\n';
      }
    }
    return false;
  }

  void getAnalysisUsage(AnalysisUsage& AU) const override {
    AU.setPreservesAll();
    AU.addRequired<aa_function_pass_user>();
    AU.addRequired<AssumptionCacheTracker>();
    getAAResultsAnalysisUsage(AU);
  }
};
};

char aa_module_pass_gentle_indirect_user::ID = 0;
INITIALIZE_PASS_BEGIN(aa_module_pass_gentle_indirect_user,
    "aa-module-pass-gentle-indirect-user", "AA ModulePass Gentle Indirect User",
    false, true);
INITIALIZE_PASS_END(aa_module_pass_gentle_indirect_user,
    "aa-module-pass-gentle-indirect-user", "AA ModulePass Gentle Indirect User",
    false, true);

/********** Call the pass initialisers **********/
struct InitMyPasses {
  InitMyPasses() {
    initializecustom_aa_passPass(*PassRegistry::getPassRegistry());
    initializeaa_function_pass_userPass(*PassRegistry::getPassRegistry());
    initializeaa_module_pass_userPass(*PassRegistry::getPassRegistry());
    initializeaa_module_pass_indirect_userPass(*PassRegistry::getPassRegistry());
    initializeaa_module_pass_gentle_indirect_userPass(*PassRegistry::getPassRegistry());
  }
};
static InitMyPasses X;
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
