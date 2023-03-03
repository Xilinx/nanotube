/*******************************************************/
/*! \file Dep_Aware_Converter.h
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Converting values forming an acyclic directed graph in dep order.
**   \date 2020-07-22
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef __DEP_AWARE_CONVERTER_H__
#define __DEP_AWARE_CONVERTER_H__
#include <functional>
#include <unordered_set>
#include <unordered_map>

#include <llvm/IR/CFG.h>
#include "llvm/Support/Debug.h"
#include <llvm/Support/raw_ostream.h>

using llvm::errs;
using llvm::dbgs;
using llvm::predecessors;
using llvm::successors;
#define DEBUG_TYPE "dep-aware-converter"

namespace nanotube {
template<typename T>
class dep_aware_converter {
  public:
    typedef std::unordered_set<T*> items_t;
    typedef std::unordered_map<T*, unsigned> pending_t;

    void insert_ready(T *item) {
      ready.insert(item);
    }

    void insert(T *item, unsigned deps) {
      if( deps == 0)
        insert_ready(item);
      else
        pending.emplace(item, deps);
    }

    void clear() {
      ready.clear();
      pending.clear();
    }

    void erase(T* item) {
      ready.erase(item);
      pending.erase(item);
    }

    bool contains(const T *item) {
      return (ready.find(const_cast<T*>(item)) != ready.end()) ||
             (pending.find(const_cast<T*>(item)) != pending.end());
    }

    void mark_dep_ready(const T *item) {
      const auto it = pending.find(const_cast<T*>(item));
      if( it == pending.end() ) {
        errs() << "Dep-Aware Converter: Item " << *item
               << " not pending!\n";
        return;
      }
      if( --it->second == 0 ) {
        insert_ready(it->first);
        pending.erase(it);
      }
    }

    /* Simple style that processes entries one by one */
    typedef void simple_callback_t(dep_aware_converter<T>*, T*);
    void execute(std::function<simple_callback_t> cb) {
      while( !ready.empty() ) {
        auto it = ready.begin();
        if( cb )
          cb(this, *it);
        ready.erase(it);
      }
      LLVM_DEBUG(
        if( !empty() ) {
          dbgs() << "Dep-Aware Converter: Some items were not converted:\n";
          for( auto& i : pending )
            dbgs() << "  " << *i.first << " Pending: " << i.second << '\n';
        }
      );
    }

    /* More complex style API that gives a ready list */
    typedef void frontier_callback_t(dep_aware_converter<T>*,
                                     const items_t& candidates,
                                     items_t* processed);
    void execute(std::function<frontier_callback_t> cb) {
      items_t processed;
      while( !ready.empty() ) {
        cb(this, ready, &processed);

        /* Remove the processed elements from the ready queue */
        for( T* p : processed ) {
          auto n_removed = ready.erase(p);
          assert(n_removed == 1);
        }
        processed.clear();
      }
      LLVM_DEBUG(
        if( !pending.empty() ) {
          dbgs() << "Dep-Aware Converter: Some items were not converted:\n";
          for( auto& i : pending )
            dbgs() << "  " << *i.first << " Pending: " << i.second << '\n';
        }
      );
    }
    bool empty() { return ready.empty() && pending.empty(); }

    /**
     * Simple helpers that initialise the dep-aware-converter from an
     * iterable element that supports also iterating through the
     * predecessors / successors and querying their count.
     * In LLVM, this allows simple filling from a Function and iterating
     * through the basic blocks */
    template<typename U>
    void init_forward(U& parent) {
      for( auto& item : parent )
        insert(&item, llvm::pred_size(&item));
    }
    void ready_forward(T* item) {
      for( auto* succ : llvm::successors(item) )
        mark_dep_ready(succ);
    }

    template<typename U>
    void init_backward(U& parent) {
      for( auto& item : parent )
        insert(&item, llvm::succ_size(&item));
    }
    void ready_backward(T* item) {
      for( auto* pred : llvm::predecessors(item) )
        mark_dep_ready(pred);
    }

  private:
    items_t   ready;
    pending_t pending;
};
}; // namespace Nanotube

#undef DEBUG_TYPE
#endif //ifdef __DEP_AWARE_CONVERTER_H__
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
