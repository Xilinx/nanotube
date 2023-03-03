/*******************************************************/
/*! \file  Map_Packet_CFG.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert Nanotube memory interface to requests
**   \date 2020-09-28
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "llvm/IR/Function.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
struct map_packet_focus : public llvm::Function {
};

template <>
struct DOTGraphTraits<const map_packet_focus*> : public DOTGraphTraits<const Function*> {
  DOTGraphTraits(bool simple = false);
  std::string getNodeLabel(const BasicBlock* bb, const map_packet_focus* g);
};

template <>
struct GraphTraits<const map_packet_focus*> : public GraphTraits<const Function*> {
};
}
void write_map_packet_cfg(llvm::Function& f, bool use_count = true);
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
