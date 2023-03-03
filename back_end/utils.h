/**************************************************************************\
*//*! \file utils.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube utilities.
**   \date  2020-07-08
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_UTILS_H
#define NANOTUBE_UTILS_H

#include "llvm_common.h"
#include "llvm_pass.h"

#include <utility>

namespace nanotube
{
  template<typename... TYPES>
    static inline LLVM_ATTRIBUTE_NORETURN
    void report_fatal_errorv(TYPES&&... args)
  {
    report_fatal_error(std::string(formatv(std::forward<TYPES>(args)...)));
  }

  template<typename... TYPES>
    static inline
    void dbgv(TYPES&&... args)
  {
    dbgs() << formatv(std::forward<TYPES>(args)...);
  }

  class as_operand
  {
  public:
    as_operand(const Value *value): m_value(value) {}
    as_operand(const Value &value): m_value(&value) {}
    const Value *get_value() const { return m_value; }
    void print(raw_ostream &os) {
      m_value->printAsOperand(os);
    }

  private:
    const Value *m_value;
  };

  static inline
  llvm::raw_ostream &operator <<(llvm::raw_ostream &os,
                                 const nanotube::as_operand &x)
  {
    x.get_value()->printAsOperand(os);
    return os;
  }
};

#endif // NANOTUBE_UTILS_H
