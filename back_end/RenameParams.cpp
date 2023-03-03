/**
 * Very simple pass that renames parameters of functions in the IR for easier
 * depug and code gen later on.  Useful after optimisations threw away the
 * names.
 *
 * Author: Stephan Diestelhorst
 * Date:   2020-02-23
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
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {
cl::opt<std::string> RenameParamString("rps",
    cl::desc("Specify rename description"),
    cl::value_desc("func1:par1,par2;func2:par3,par4[;]"));

struct RenameParams : public FunctionPass {
    static char ID;
    RenameParams() : FunctionPass(ID) {
        parseArgs();
    }
    std::map<std::string, std::vector<std::string>> names;

    void parseArgs() {
        // Use LLVM regex to parse the command line
        std::string param = RenameParamString;
        unsigned    pos = 0;
        Regex       re("([^:,;]*)([:,;]?)");
        std::string cur_func;
        SmallVector<StringRef, 3> m;

        while (pos < param.size()) {
            if (!re.match(param.substr(pos), &m))
                break;
            if (m[2].equals(":")) {
                // Functions look as follows "func:"
                cur_func = m[1];
            } else {
                // Arguments look like "arg1,arg2,arg3;" or "arg4,arg5"
                assert(!cur_func.empty());
                names[cur_func].push_back(m[1]);
            }
            pos += m[0].size();
        }
    }

    bool runOnFunction(Function &F) override {
        errs().write_escaped(F.getName());
        for (auto a = F.arg_begin(); a != F.arg_end(); ++a)
            errs() << " | " << *a;
        errs() << "\n";


        // Check if we have a matching entry for this function
        auto res = names.find(F.getName());
        if (res == names.end())
            return false;
        if (res->second.size() != F.arg_size())
            return false;

        // Rename the arguments
        auto arg = F.arg_begin();
        auto name = res->second.begin();
        for (; arg != F.arg_end(); ++arg, ++name)
            arg->setName(*name);

        errs().write_escaped(F.getName());
        for (auto a = F.arg_begin(); a != F.arg_end(); ++a)
            errs() << " | " << *a;
        errs() << "\n";
        return true;
    }
};
} // anonymous namespace

char RenameParams::ID = 0;


static RegisterPass<RenameParams>
    X("rename_params", "Rename parameters of selected functions",
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
