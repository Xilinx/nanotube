/*******************************************************/
/*! \file back_end_main.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief The Nanotube compiler back end main program.
**   \date 2020-06-17
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "HLS_Printer.h"
#include "llvm_common.h"
#include "llvm_pass.h"

#include "llvm/InitializePasses.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>

namespace nanotube {
  namespace cl = llvm::cl;
};
using namespace nanotube;

///////////////////////////////////////////////////////////////////////////
// Command line options.

static cl::opt<std::string>
opt_input_filename(cl::Positional, cl::desc("<input bitcode>"),
                   cl::Required);

static cl::opt<std::string>
opt_output_directory("o", cl::desc("<output directory>"),
                     cl::Required);

static cl::opt<bool>
opt_overwrite("overwrite", cl::desc("Overwrite the output directory."));

static cl::opt<std::string>
opt_passes("passes", cl::desc("A pipeline of passes to run."),
           cl::init("-"));

///////////////////////////////////////////////////////////////////////////
// The pass definitions.

class unknown_pass_error: public llvm::ErrorInfo<unknown_pass_error>
{
public:
  static char ID;

  explicit unknown_pass_error(llvm::StringRef &pass_name):
    m_pass_name(pass_name)
  {
  }

  void log(raw_ostream &os) const
  {
    os << "Unknown pass '" << m_pass_name << "'.";
  }

  std::error_code convertToErrorCode() const
  {
    return std::error_code(EINVAL, std::generic_category());
  }

private:
  llvm::StringRef m_pass_name;
};
char unknown_pass_error::ID = 0;

llvm::Error add_passes(llvm::legacy::PassManager &pm)
{
  // Set the default list of passes.
  if (opt_passes == "-") {
    opt_passes = "";
  }

  llvm::PassRegistry &registry = *llvm::PassRegistry::getPassRegistry();

  llvm::SmallVector<llvm::StringRef, 8> passes;
  llvm::StringRef(opt_passes).split(passes, ',', -1, false);
  for (auto &pass_name: passes) {
    const llvm::PassInfo *pi = registry.getPassInfo(pass_name);
    if (pi == nullptr)
      return llvm::make_error<unknown_pass_error>(pass_name);

    pm.add(pi->createPass());
  }

  return llvm::Error::success();
}

///////////////////////////////////////////////////////////////////////////
// The main program.

int main(int argc, char *argv[])
{
  llvm::InitLLVM init_llvm(argc, argv);

  // Initialize passes.  See llvm/tools/opt/opt.cpp
  llvm::PassRegistry &registry = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeCore(registry);
  llvm::initializeAnalysis(registry);
  llvm::initializeTransformUtils(registry);

  cl::ParseCommandLineOptions(argc, argv, "Nanotube back end\n");

  llvm::LLVMContext context;
  llvm::SMDiagnostic sm_diag;

  llvm::legacy::PassManager pm;
  llvm::Error err = add_passes(pm);
  if (err) {
    llvm::errs() << argv[0] << ": " << toString(std::move(err)) << "\n";
    return 1;
  }

  std::unique_ptr<llvm::Module> module;
  module = parseIRFile(opt_input_filename, sm_diag, context, false);
  if (!module) {
    sm_diag.print(argv[0], llvm::WithColor::error(llvm::errs(), argv[0]));
    return 1;
  }

  Triple the_triple = Triple(module->getTargetTriple());
  std::cout << "Target triple: " << the_triple.getTriple() << "\n";

  pm.add(create_hls_printer(opt_output_directory, opt_overwrite));

  pm.run(*module);

  return 0;
}

///////////////////////////////////////////////////////////////////////////
