/**
 * Nanotube compiler utility definitions.
 * Author: Stephan Diestelhorst <stephand@amd.com>
 * Date:   2020-02-26
 */

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef NANOTUBE_UTILS_HPP
#define NANOTUBE_UTILS_HPP
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>


#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

using llvm::raw_ostream;
using llvm::SmallVectorTemplateCommon;

namespace {
// NOTE: urgh! we need to wrap this in an anon namespace, otherwise, the
// compiler does not expand the right templates, and then the users will not
// find this :(
template <typename T>
raw_ostream &operator<<(raw_ostream &o, const std::set<T> &s) {
    o << "{ ";
    for (auto &e : s)
        o << e << " ";
    o << "}";
    return o;
}
template <typename T>
raw_ostream &operator<<(raw_ostream &o, const std::unordered_set<T> &s) {
    o << "{ ";
    for (auto &e : s)
        o << e << " ";
    o << "}";
    return o;
}
template <typename T, typename U>
raw_ostream &operator<<(raw_ostream &o, const std::unordered_map<T,U> &s) {
    o << "{ ";
    for (auto &e : s)
        o << "(" << e.first << "," << e.second  << ") ";
    o << "}";
    return o;
}

template <typename T>
raw_ostream &operator<<(raw_ostream &o, const std::vector<T> &v) {
    o << "{ ";
    for (auto &e : v)
        o << e << " ";
    o << "}";
    return o;
}

template <typename T>
raw_ostream &operator<<(raw_ostream &o,
                        const SmallVectorTemplateCommon<T> &ar) {
    o << "{ ";
    for (auto &e : ar)
        o << e << " ";
    o << "}";
    return o;
}
} // anonymous namespace
#endif // NANOTUBE_UTILS_HPP
