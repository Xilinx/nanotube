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

#include <algorithm>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

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
    typedef std::vector<T*> ready_t;
    typedef std::unordered_map<T*, unsigned> dep_counts_t;

    void insert_ready(T *item) {
      ready.push_back(item);
      dep_counts.emplace(item, 0);
    }

    void insert(T *item, unsigned deps) {
      if( deps == 0)
        ready.push_back(item);
      dep_counts.emplace(item, deps);
    }

    void clear() {
      ready.clear();
      dep_counts.clear();
    }

    void erase(T* item) {
      erase_ready(item);
      dep_counts.erase(item);
    }

    bool contains(const T *item) {
      auto it = dep_counts.find(const_cast<T*>(item));
      return it != dep_counts.end();
    }

    void mark_dep_ready(const T *item) {
      const auto it = dep_counts.find(const_cast<T*>(item));
      if( it == dep_counts.end() ) {
        errs() << "Dep-Aware Converter: Item " << *item
               << " not pending!\n";
        return;
      }
      if( --it->second == 0 ) {
        ready.push_back(it->first);
      }
    }

    /* Simple style that processes entries one by one */
    typedef void simple_callback_t(dep_aware_converter<T>*, T*);
    void execute(std::function<simple_callback_t> cb) {
      while( !ready.empty() ) {
        auto item = ready.back();
        ready.pop_back();
        dep_counts.erase(item);
        if( cb )
          cb(this, item);
      }
      LLVM_DEBUG(
        if( !empty() ) {
          dbgs() << "Dep-Aware Converter: Some items were not converted:\n";
          for( auto& i : dep_counts )
            dbgs() << "  " << *i.first << " Pending: " << i.second << '\n';
        }
      );
    }

    /* More complex style API that gives a ready list */
    typedef std::unordered_set<T*> items_t;
    typedef void frontier_callback_t(dep_aware_converter<T>*,
                                     const items_t& candidates,
                                     items_t* processed);
    void execute(std::function<frontier_callback_t> cb) {
      items_t ready_set;
      items_t processed;

      while( !ready.empty() ) {
        ready_set.clear();
        ready_set.insert(ready.begin(), ready.end());

        cb(this, ready_set, &processed);

        /* Remove the processed elements from the ready queue */
        typename ready_t::size_type n_keep = 0;
        for( typename ready_t::size_type i = 0; i < ready.size(); ++i ) {
          auto item = ready[i];
          if( processed.find(item) != processed.end() ) {
            auto n_removed = dep_counts.erase(item);
            assert(n_removed == 1);
          } else {
            ready[n_keep++] = item;
          }
        }
        assert(processed.size() == ready.size() - n_keep);
        ready.resize(n_keep);
        processed.clear();
      }
      LLVM_DEBUG(
        if( !dep_counts.empty() ) {
          dbgs() << "Dep-Aware Converter: Some items were not converted:\n";
          for( auto& i : dep_counts )
            dbgs() << "  " << *i.first << " Pending: " << i.second << '\n';
        }
      );
    }

    bool empty() { return dep_counts.empty(); }

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
    void erase_ready(T *item) {
      auto it = std::find(ready.begin(), ready.end(), item);
      if( it != ready.end() )
        ready.erase(it);
    }

    ready_t      ready;
    dep_counts_t dep_counts;
};
}; // namespace Nanotube

#undef DEBUG_TYPE
#endif //ifdef __DEP_AWARE_CONVERTER_H__
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
