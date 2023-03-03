/*******************************************************/
/*! \file printing_helpers.h
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Helpers for easy printing of types.
**   \date 2021-01-20
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef __PRINTING_HELPERS_H__
#define __PRINTING_HELPERS_H__
#include "llvm/Analysis/AliasAnalysis.h"
#include <unordered_set>

llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const llvm::ModRefInfo& i);
llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const llvm::FunctionModRefBehavior& i);
llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const llvm::AAMDNodes& aa);
llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const llvm::MemoryLocation& loc);

template <class T>
struct nullsafe_wrapper {
    const T* item;
    nullsafe_wrapper(const T* i) : item(i) {}
};
template <typename T>
llvm::raw_ostream &operator<<(llvm::raw_ostream &o, const nullsafe_wrapper<T> &s) {
    if( s.item != nullptr )
        o << *s.item;
    else
        o << "<nullptr>";
    return o;
}
// This function is needed because apparently class template argument deduction
// (CTAD) is only part of C++17. Can you imagine that?!?!?  So we have to use
// function template argument deduction for now...
template <typename T>
nullsafe_wrapper<T> nullsafe(const T* i) { return nullsafe_wrapper<T>(i);}

template <typename T>
llvm::raw_ostream &operator<<(llvm::raw_ostream &o, const std::unordered_set<T> &s) {
    o << "{ ";
    for (auto &e : s)
        o << e << " ";
    o << "}";
    return o;
}

template <typename T>
llvm::raw_ostream &operator<<(llvm::raw_ostream &o, const std::vector<T> &v) {
    o << "{ ";
    for (auto &e : v)
        o << e << " ";
    o << "}";
    return o;
}
#endif //__PRINTING_HELPERS_H__
