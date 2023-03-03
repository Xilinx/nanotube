/**************************************************************************\
*//*! \file Pointer_Analysis.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube pointer analysis
**   \date  2019-06-24
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_POINTER_ANALYSIS_H
#define NANOTUBE_POINTER_ANALYSIS_H

#include "llvm_common.h"
#include "llvm_pass.h"

namespace nanotube {
  namespace Pointer_Type {
    typedef enum {
      unknown,
      argument,
      stack,
      channel_handle,
      // packet_handle,
      // packet_data,
      map_data,
    } ID;
  }

  struct Pointer_Info
  {
    Pointer_Type::ID type_id;
    Value *def_base;
    APInt def_offset;
    User *use;
    bool ptr_like;
    Pointer_Info() : type_id(Pointer_Type::unknown), def_base(nullptr), def_offset(),
      use(nullptr), ptr_like(false) {}
    Pointer_Info(Value *base, const APInt& off) : type_id(Pointer_Type::unknown), def_base(base),
      def_offset(off), use(nullptr), ptr_like(false) {}
  };

  class Pointer_Analysis_Pass: public llvm::FunctionPass
  {
  public:
    static char ID;

    Pointer_Analysis_Pass();
    ~Pointer_Analysis_Pass();
    StringRef getPassName() const override { return "Nanotube pointer analysis"; }

    bool runOnFunction(Function &F);
    void print(llvm::raw_ostream &O, const Module *M) const;
    void releaseMemory();

    const Pointer_Info *getPointerInfo(Value &V);

  private:
    typedef DenseMap<Value *, Pointer_Info> map_t;
    map_t m_pointer_info_map;

    void addPointer(Value *val, Value *def, const APInt &offset,
                    bool indirect);
    void processPointerArgs(Function &F);
    void tracePointerConsumer(Instruction &inst, Value *ptr_op,
        const Pointer_Info *pin);
    void tracePointerProducer(Instruction &inst);
  };
}

#endif // NANOTUBE_POINTER_ANALYSIS_H
