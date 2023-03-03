/**************************************************************************\
*//*! \file print_setup.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube utility pass to print stages.
**   \date  2020-08-18
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#define DEBUG_TYPE "print-setup"

#include "Intrinsics.h"
#include "llvm_common.h"
#include "llvm_pass.h"
#include "setup_func.hpp"

#include <iostream>

using namespace nanotube;

///////////////////////////////////////////////////////////////////////////

namespace {
  class print_setup: public llvm::ModulePass {
  public:
    static char ID;

    print_setup();
    StringRef getPassName() const override {
      return "Nanotube print processing functions";
    }
    void getAnalysisUsage(AnalysisUsage &info) const override;

    bool runOnModule(Module &m) override;
  };
};

///////////////////////////////////////////////////////////////////////////

char print_setup::ID;

print_setup::print_setup():
  ModulePass(ID)
{
}

void print_setup::getAnalysisUsage(AnalysisUsage &info) const
{
  info.setPreservesAll();
}

static std::string to_string(nanotube_channel_type_t t)
{
  switch(t) {
  case NANOTUBE_CHANNEL_TYPE_NONE: return "None";
  case NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET: return "Simple packet";
  case NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET: return "Softhub packet";
  default: return "*Invalid*";
  }
}

bool print_setup::runOnModule(Module &m)
{
  // Setup the output.
  raw_os_ostream output(std::cout);

  // Create the setup_func.
  auto setup = setup_func(m);

  output << formatv("Setup function: {0}\n", setup.get_function().getName());

  setup.print_memory_contents(output);

  channel_index_t num_channels = setup.channels().size();
  for (channel_index_t channel_index = 0; channel_index < num_channels;
       channel_index++) {
    channel_info &channel = setup.get_channel_info(channel_index);

    output << formatv("Channel {0}\n", channel_index);
    output << formatv("  Name:              {0}\n", channel.get_name());
    output << formatv("  Element size:      {0}\n", channel.get_elem_size());
    output << formatv("  Num Elements:      {0}\n", channel.get_num_elem());
    output << formatv("  Write export type: {0}\n",
                      to_string(channel.get_write_export_type()));
    output << formatv("  Read export type:  {0}\n",
                      to_string(channel.get_read_export_type()));
    output << "\n";
  }

  context_index_t num_contexts = setup.contexts().size();
  for (context_index_t context_index = 0; context_index < num_contexts;
       context_index++) {
    context_info &context = setup.get_context_info(context_index);
    output << formatv("Context: {0}\n", context_index);
    port_index_t num_ports = context.ports().size();
    for (port_index_t port_index = 0; port_index < num_ports; port_index++) {
      stage_port &port = context.get_port(port_index);
      output << formatv("  Port {0}: {1} channel {2}\n", port_index,
                        (port.is_read() ? "reads " : "writes"),
                        port.channel_index());
    }
    output << "\n";
  }

  thread_id_t num_threads = setup.threads().size();
  for (thread_id_t id = 0; id < num_threads; id++) {
    thread_info &thread = setup.get_thread_info(id);
    auto &args = thread.args();
    output << formatv("Thread {0}\n", id);
    output << formatv("  Context:  {0}\n", thread.context_index());
    output << formatv("  Name:     {0}\n", args.name);
    output << formatv("  Function: {0}\n", args.func->getName());

    output << "\n";
  }

  // Nothing was modified.
  return  false;
}

///////////////////////////////////////////////////////////////////////////

static RegisterPass<print_setup>
register_pass("print-setup", "Print setup function contents",
               false,
               false
  );

///////////////////////////////////////////////////////////////////////////
