/*******************************************************/
/*! \file  Map_Packet_CFG.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert Nanotube memory interface to requests
**   \date 2020-09-28
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "Map_Packet_CFG.hpp"

#include <string>

#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/raw_ostream.h"

#include "Intrinsics.h"

using namespace llvm;
using namespace nanotube;

cl::opt<bool> show_maps("cfg-show-maps",
                        cl::desc("Show maps in the restricted CFG"),
                        cl::init(true));
cl::opt<bool> show_packets("cfg-show-packets",
                           cl::desc("Show packets in the restricted CFG"),
                           cl::init(true));


namespace llvm {
DOTGraphTraits<const map_packet_focus*>::DOTGraphTraits(bool simple) :
  DOTGraphTraits<const Function*>(simple) {}

std::string
DOTGraphTraits<const map_packet_focus*>::getNodeLabel(const BasicBlock* bb,
                                                      const map_packet_focus* g) {
  std::string ret;
  for( auto& inst : *bb ) {
    auto i = get_intrinsic(&inst);
    if( (show_maps && (i == Intrinsics::map_read || i == Intrinsics::map_write)) ||
        (show_packets && (i == Intrinsics::packet_read ||
                          i == Intrinsics::packet_write ||
                          i == Intrinsics::packet_write_masked ||
                          i == Intrinsics::packet_resize ))) {

      std::string str;
      raw_string_ostream OS(str);
      OS << inst;
      ret += OS.str() + "\\l  ";
    }
  }
  return ret;
}
}

void write_map_packet_cfg(Function& f, bool use_count) {
  std::string dot = f.getName().str() + "." + (show_maps ? "m" : "") +
                    (show_packets ? "p" : "");
  static unsigned count = 0;
  if( use_count )
    dot += "." + std::to_string(count++);
  dot += ".dot";
  WriteGraph((const map_packet_focus*)&f, f.getName(), false,
             "CFG for " + f.getName(), dot);
}

namespace {
  struct map_packet_cfg_pass : public FunctionPass {
    static char ID;

    map_packet_cfg_pass() : FunctionPass(ID) {}
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesAll();
    }
    bool runOnFunction(Function& f) override {
      write_map_packet_cfg(f);
      return false;
    }
  }; // struct map_packet_cfg
} // anonymous namespace
char map_packet_cfg_pass::ID = 0;
static RegisterPass<map_packet_cfg_pass>
    X("map-packet-cfg", "Dump CFGs with only Nanotube map / packet accesses",
      false,
      true
      );

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
