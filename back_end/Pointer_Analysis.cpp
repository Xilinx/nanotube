/**************************************************************************\
*//*! \file Pointer_Analysis.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube pointer analysis
**   \date  2019-06-24
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "Pointer_Analysis.h"
#include "Intrinsics.h"

#include "llvm_common.h"
#include "llvm_insns.h"
#include "llvm_pass.h"

#include <cassert>
#include <iostream>
#include <utility>

#define DEBUG_TYPE "nanotube-pointer-analysis"

using namespace nanotube;

char Pointer_Analysis_Pass::ID = 0;

static RegisterPass<Pointer_Analysis_Pass>
reg("nanotube-pointer-analysis", "Nanotube pointer analysis",
    /*CFGOnly*/false, /*is_analysis*/true);

Pointer_Analysis_Pass::Pointer_Analysis_Pass():
  FunctionPass(ID)
{
}

Pointer_Analysis_Pass::~Pointer_Analysis_Pass()
{
}

/**
 * Inspect all function arguments and add them as roots to the pointer info
 * structure
 */
void Pointer_Analysis_Pass::processPointerArgs(Function &F) {
  Module *M = F.getParent();
  const DataLayout &DL = M->getDataLayout();

  for(auto arg_it=F.arg_begin(); arg_it!=F.arg_end(); arg_it++) {
    Value *val = &(*arg_it);

    LLVM_DEBUG(
      dbgs() << "Handling argument: ";
      val->print(dbgs(), true);
      dbgs() << "\n";
      );

    Type *t = arg_it->getType();
    if (!t->isPointerTy())
      continue;

    addPointer(val, val, APInt(DL.getTypeSizeInBits(t), 0, true), false);
  }
}

/**
 * Traces origin through instructions that consume a pointer.  The idea is to
 * check if results produced by this instruction are effectively
 * addresses of the same origin as the pointer.
 *
 * Note(SD):
 * This is similar to the original addPointer code below, and some of
 * the functionality is / will be split between this pointer consumer call and
 * the pointer producer part.  XXX: Merge / fuse these
 */
void Pointer_Analysis_Pass::tracePointerConsumer(Instruction &inst, Value
  *ptr_op, const Pointer_Info *pin) {

  Pointer_Info pi;

  switch (inst.getOpcode()) {
    case Instruction::PtrToInt:
      /* The resulting int should be tracked as a pointer-like entity */
      assert(!pin->ptr_like);
      pi.ptr_like   = true;
      pi.type_id    = pin->type_id;
      pi.def_base   = pin->def_base;
      pi.def_offset = pin->def_offset;
      m_pointer_info_map[&inst] = pi;
      break;
    case Instruction::GetElementPtr:
      /* The resulting entity should be a proper pointer of the same type */
      pi.type_id    = pin->type_id;
      pi.def_base   = pin->def_base;
      //pi.def_offset = pin->def_offset;  //XXX: Do the maths here?
      m_pointer_info_map[&inst] = pi;
      break;
    case Instruction::IntToPtr:
      /* Back to a proper pointer */
      assert(pin->ptr_like);
      pi.ptr_like   = false;
      pi.type_id    = pin->type_id;
      pi.def_base   = pin->def_base;
      pi.def_offset = pin->def_offset;
      m_pointer_info_map[&inst] = pi;
      break;
    //XXX: case Instruction::bitcast => hand through
    case Instruction::Load:
      /* A load from a (proper) pointer => translate accordingly:

         Load from stack / memory
           => fuse with store data producer (or let an optimisation pass do
              that?)
         Load from packet type
           => transform to PacketRead(offset) (width stuff from Byteify pass?)
           => easy / lazy: offset = ptrtoint(%ptr) - %ctxt (rely on optimiser /
                                                            backend to give us
                                                            a compact value)
           => hard(er): offset = map[%ptr].def_offset (careful tracking by us)
         Load from map type
           => transform to a map read with the key pulled from the
              bpf_map_lookup call; offset calculation similar to Packet */
      assert(!pin->ptr_like);
      // XXX: Do this in a later pass?
      break;
    case Instruction::Store:
      /* A store to a (proper) pointer => translate accordingly

         Store to stack / memory
           => record the stored data relative to the location for later load
              fusing
         Store to packet type
           => transform to PacketWrite(offset, data) (width stuff from Byteify pass?)
           => easy / lazy: offset = ptrtoint(%ptr) - %ctxt (rely on optimiser /
                                                            backend to give us
                                                            a compact value)
           => hard(er): offset = map[%ptr].def_offset (careful tracking by us)
         Store to map type
           => transform to a map write with the key pulled from the
              bpf_map_lookup call; offset calculation similar to Packet */
      assert(!pin->ptr_like);
      // XXX: Do this in a later pass?
      break;
    case Instruction::Call:
      /* Should only be calls to BPF helpers!
         bpf_map_lookup => ABI uses stack-based calls so this accepts a pointer
         to memory for the key; remember the actual fused value for this as the
         key for the lookup and associate with the result pointer value */
      break;
    case Instruction::Sub:
      /* XXX: Special case where we lose pointer-ness if both arguments are of
         the right pointer origin */
    default:
      if (pin->ptr_like && inst.isBinaryOp()) {
        /* Other instructions could manipulate the pointer value through maths */
        pi = *pin;
        //pi.def_offset = pin->def_offset;  //XXX: For now don't mirror pointer computation
        m_pointer_info_map[&inst] = pi;
      }
     /* All other instructions do not carry through the pointer origin */
  }
}

/**
 * Traces origin through instructions that produce a pointer and record the
 * characteristics of this pointer.
 *
 * Note(SD):
 * The split between this and tracePointerConsumer and the addPointer function
 * below is somewhat murky.  In many cases, instructions that produce a pointer
 * also consume one etc. XXX: Merge / fuse these
 */
void Pointer_Analysis_Pass::tracePointerProducer(Instruction &inst) {
  /*
     Per type actions:
     Pointer load from stack
        => fuse the value with the previous store in case we have stack-based
           spill / fill all other cases: probably assert(false)
     Pointer load from packet => no pointer should come from the packet
     Pointer load from map => for now (?) should probably not have pointers
                              come from a map

     GEP / inttoptr / bitcast => mark destination same origin as source, see
                                 tracePointerConsumer for more detail

     Arithmetic etc => should never generate a pointer; we trace these rather
                       through the inputs being pointer or pointer-like
                       integers
     Call => Only one handled here should be bpf_map_lookup which should mark
             the resulting pointer as map_type and record the value of the key
             used with that map type
  */
}

bool Pointer_Analysis_Pass::runOnFunction(Function &F)
{
  assert(m_pointer_info_map.empty());

  Module *M = F.getParent();
  const DataLayout &DL = M->getDataLayout();

  LLVM_DEBUG(dbgs() << "Running Pointer_Analysis_Pass on function "
             << F.getName() << "\n");

  // Process pointer arguments.
  processPointerArgs(F);


  // Process pointer instructions.

  for(auto bb_it=F.begin(); bb_it!=F.end(); bb_it++) {
    for(auto ins_it=bb_it->begin(); ins_it!=bb_it->end(); ins_it++) {
      Type *t = ins_it->getType();
      Instruction *ins = &(*ins_it);
      Value *val = ins;

      LLVM_DEBUG(
        dbgs() << "Handling instruction:";
        ins->print(dbgs(), true);
        dbgs() << "\n"
        );

      // Only process pointer types.
      if (!t->isPointerTy())
        continue;

      // Handle a single load.
      bool indirect = false;
      if (ins->getOpcode() == Instruction::Load) {
        val = ins->getOperand(0);
        ins = dyn_cast<Instruction>(val);
        indirect = true;

        LLVM_DEBUG(
          dbgs() << "  Load address:";
          val->print(dbgs(), true);
          dbgs() << "\n"
          );
      }

      // Follow the chain of bitcasts and GEPs.
      APInt offset = APInt(DL.getTypeSizeInBits(t), 0, true);
      while(ins != NULL) {
        auto opcode = ins->getOpcode();
        if (opcode == Instruction::BitCast) {
          val = ins->getOperand(0);

          LLVM_DEBUG(
            dbgs() << "  Bit cast from:";
            val->print(dbgs(), true);
            dbgs() << "\n"
            );

        } else if (opcode == Instruction::GetElementPtr) {
          GetElementPtrInst *gep = cast<GetElementPtrInst>(ins);
          bool succ = gep->accumulateConstantOffset(DL, offset);
          if (!succ)
            report_fatal_error("Cannot handle variable pointer arithmetic"
                               " in '" + F.getName().str() + "'.");
          val = ins->getOperand(0);

          LLVM_DEBUG(
            dbgs() << "  Get element pointer from:";
            val->print(dbgs(), true);
            dbgs() << "\n"
            );

        } else {
          break;
        }

        ins = dyn_cast<Instruction>(val);
      }

      addPointer(&(*ins_it), val, offset, indirect);
    }
  }

  LLVM_DEBUG(
    dbgs() << "Ran Pointer_Analysis_Pass on function "
    << F.getName() << "\n"
    );

  return false;
}

void Pointer_Analysis_Pass::print(llvm::raw_ostream &O, const Module *M) const
{
  O << "Dump of Pointer_Analysis_Pass\n";
}

void Pointer_Analysis_Pass::releaseMemory()
{
  m_pointer_info_map.clear();
}

const Pointer_Info *Pointer_Analysis_Pass::getPointerInfo(Value &V)
{
  auto it = m_pointer_info_map.find(&V);
  if (it != m_pointer_info_map.end())
    return &(it->second);
  return NULL;
}

void Pointer_Analysis_Pass::addPointer(Value *val, Value *def_base,
                                       const APInt &def_offset,
                                       bool indirect)
{
  auto res = m_pointer_info_map.try_emplace(val, Pointer_Info());
  assert(res.second);
  auto &pi = res.first->second;
  pi.type_id = Pointer_Type::unknown;
  pi.def_base = def_base;
  pi.def_offset = def_offset;
  pi.use = NULL;

  Instruction *ins = dyn_cast<Instruction>(def_base);
  if (ins != NULL) {
    switch(ins->getOpcode()) {
    case Instruction::Alloca:
      pi.type_id = Pointer_Type::stack;
      break;

    case Instruction::Call: {
      switch(get_intrinsic(ins)) {
      case Intrinsics::channel_create:
        pi.type_id = Pointer_Type::channel_handle;
        break;

      default: {
        Function *callee = cast<CallBase>(ins)->getCalledFunction();
        if (callee->getName().equals("bpf_map_lookup_elem")) {
            pi.type_id = Pointer_Type::map_data;
        } else {
            report_fatal_error("Cannot handle function '" +
                               callee->getName() + "' returning a pointer.");
        }
      }
      }
      break;
    }

    default:
      break;
    }
  } else {
    Argument *arg = dyn_cast<Argument>(def_base);
    if (arg != NULL) {
      res.first->second.type_id = Pointer_Type::argument;
    }
  }

  if (indirect) {
    if (ins)
        llvm::errs() << *ins << '\n';
    if (res.first->second.type_id != Pointer_Type::argument) {
      //report_fatal_error("Only loads from argument types are supported.");
        llvm::errs() << "Ignoring load from non-argument.\n";
    }
  }

  LLVM_DEBUG(
    dbgs() << "Defined pointer:\n  Value:      ";
    res.first->first->print(dbgs(), true);
    dbgs() << "\n  Type:       ";
    switch(res.first->second.type_id) {
    case Pointer_Type::unknown: dbgs() << "(unknown)"; break;
    case Pointer_Type::argument: dbgs() << "argument"; break;
    case Pointer_Type::stack: dbgs() << "stack"; break;
    case Pointer_Type::channel_handle: dbgs() << "channel_handle"; break;
    case Pointer_Type::map_data: dbgs() << "map_data"; break;
    default: dbgs() << "UNKNOWN"; break;
    }
    dbgs() << "\n  def_base:   ";
    res.first->second.def_base->print(dbgs(), true);
    dbgs() << "\n  def_offset: " << res.first->second.def_offset;
    dbgs() << "\n  use:        ";
    if (res.first->second.use == NULL) {
      dbgs() << "(null)";
    } else {
      res.first->second.use->print(dbgs(), true);
    }
    dbgs() << "\n";
    );
}
