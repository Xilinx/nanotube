/**
 * Add HLS annotations through LLVM metadata or annotations.  This needs to be
 * picked up in the C back-end and serialised out into the right #pragmas.
 *
 * XXX: Converge on whether metadata or attributes should be used
 *
 * Author: Stephan Diestelhorst
 * Date:   2020-02-24
 */

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <map>
#include <vector>

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct AnnotateHLS : public FunctionPass {
    static char ID;
    AnnotateHLS() : FunctionPass(ID) { }
    static const int PACK_THRESH = 8;

    /**
     * Get HLS annotation for the argument pointed to.
     */
    MDNode *getMDHLSNode(LLVMContext &C, std::string s, Argument *arg,
                         int arg_pos = 0) {
        auto *dp   = MDString::get(C, s);
        auto *pos  = ValueAsMetadata::get(
                ConstantInt::get(C, APInt(16, arg_pos)) );
        auto *name = MDString::get(C, arg->getName());

        Metadata *a[] = {dp, name, pos};

        return MDNode::get(C, a);
    }
    MDNode *getMDHLSDataPack(LLVMContext &C, Argument *arg, int arg_pos = 0) {
        return getMDHLSNode(C, "DATA_PACK variable=", arg, arg_pos);
    }
    MDNode *getMDHLSInterface(LLVMContext &C, std::string iface, Argument *arg,
            int arg_pos = 0) {
        return getMDHLSNode(C, "INTERFACE " + iface + " port=", arg, arg_pos);
    }

    /**
     * Determine per function argument whether they need specific annotations
     */
    std::string getInterface(Argument *arg) {
        return "axis";
    }
    bool needsPacking(Argument *arg) {
        auto *ty = arg->getType();

        // Strip pointers of arguments
        while (ty->isPointerTy())
            ty = dyn_cast<PointerType>(ty)->getElementType();


        // Check size for composite type
        auto &DL = arg->getParent()->getParent()->getDataLayout();

        //if (ty->isCompositeType)
            if (DL.getTypeStoreSize(ty) > PACK_THRESH)
                return true;

        return false;
    }

    /**
     * Add parameter annotations for this function.
     *
     * XXX: Make this configurable (which function, which parameter)
     */
    bool runOnFunction(Function &F) override {
        auto &C = F.getContext();

        errs().write_escaped(F.getName());
        for (auto a = F.arg_begin(); a != F.arg_end(); ++a)
            errs() << " | " << *a;
        errs() << "\n";

        // Option 1: use LLVM metadata to add the HLS annotations
        SmallVector<Metadata *, 6> func_info;
        int argc = 0;
        for (auto arg = F.arg_begin(); arg != F.arg_end(); ++arg) {
            if (needsPacking(arg))
                func_info.push_back( getMDHLSDataPack(C, arg, argc) );
            func_info.push_back(
                getMDHLSInterface(C, getInterface(arg), arg, argc) );
            argc++;
        }
        F.addMetadata("HLS", *MDNode::get(C, func_info));
        errs() << F << "\n";
        return true;
    }
};
} // anonymous namespace

char AnnotateHLS::ID = 0;


static RegisterPass<AnnotateHLS>
    X("annotateHLS", "Add HLS annotations to function parameters",
      true,
      false
      );
#if(0) //does not work with LLVM 8, but instead needs 9
// Run at the end of most passes so the names do not disappear :)
static RegisterStandardPasses Y(
    PassManagerBuilder::EP_OptimizerEnd,
    [](const llvm::PassManagerBuilder &Builder,
        llvm::legacy::PassManagerBase &PM) { PM.add(new Hello()); });
#endif
