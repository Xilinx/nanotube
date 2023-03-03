#ifndef __SET_OPS_HPP__
#define __SET_OPS_HPP__
/*******************************************************/
/*! \file set_ops.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Various set operations on containers
**   \date 2020-04-09
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <set>
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;
using namespace nanotube;

/**
 * Compare two sets of pointers using the associative lookup option.
 *
 * NOTE: The decltype magic below really just checks that the provided
 * class has an associative count function.
 */
template <typename T>
auto overlap(const T& set_a, const T& set_b)
  -> decltype(set_a.count(*set_a.begin()), bool())
{
  if( &set_a == &set_b )
    return true;
  /* Make sure set_b is the smaller one! */
  if( set_a.size() < set_b.size() )
    return overlap(set_b, set_a);

  for( auto e : set_b )
    if( set_a.count(e) != 0 )
      return true;
  return false;
}

/**
 * Check whether set_b is a subset of set_a
 */
template <typename T>
auto subset(const T& set_a, const T& set_b)
  -> decltype(set_a.count(*set_a.begin()), bool())
{
  if( &set_a == &set_b )
    return true;

  for( auto e : set_b )
    if( set_a.count(e) == 0 )
      return false;
  return true;
}

/**
 * Compare two vectors if they have common entries. Sort these vectors and
 * then ely on the sortedness and perform an in-order comparison.
 */
template <typename T>
bool overlap(SmallVectorImpl<T>& vec_a, SmallVectorImpl<T>& vec_b) {
  if( &vec_a == &vec_b )
    return true;

  sort(vec_a); sort(vec_b);

  unsigned idx_a = 0;
  unsigned idx_b = 0;

  while( idx_a < vec_a.size() && idx_b < vec_b.size() ) {
    if( vec_a[idx_a] == vec_b[idx_b] )
      return true;
    if( vec_a[idx_a] < vec_b[idx_b] )
      idx_a++;
    else
      idx_b++;
  }
  return false;
}

template <typename T>
bool subset(SmallVectorImpl<T>& vec_a, SmallVectorImpl<T>& vec_b) {
  if( &vec_a == &vec_b )
    return true;

  sort(vec_a); sort(vec_b);

  unsigned idx_a = 0;
  unsigned idx_b = 0;

  /* Make sure all entries of vec_b are in vec_a */
  while( idx_a < vec_a.size() && idx_b < vec_b.size() ) {
    /* Wind vector A forward until we have a chance of mathing */
    while( vec_a[idx_a] < vec_b[idx_b] )
      idx_a++;
    if( vec_a[idx_a] != vec_b[idx_b] )
      return false;
    idx_b++;
  }
  return (idx_b == vec_b.size());
}

template <typename T> bool identical(const T& set_a, const T& set_b) {
  if( &set_a == &set_b )
    return true;
  if( set_a.size() != set_b.size() )
    return false;
  return overlap(set_a, set_b);
}

template <typename T> bool disjoint(const T& set_a, const T& set_b) {
  return !overlap(set_a, set_b);
}
#endif //__SET_OPS_HPP__

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
