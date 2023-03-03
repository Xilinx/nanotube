/*******************************************************/
/*! \file graphs.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Data structures for graph operations.
**   \date 2020-02-11
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef __GRAPHS_HPP__
#define __GRAPHS_HPP__

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "graph"
using llvm::dbgs;
using llvm::errs;
using llvm::raw_ostream;

/**
 * Graph is a simple adjecency + weights directed graph class.  We are using
 * it to track the order of how maps are accessed.
 */
template <typename T>
struct Graph {
    std::map<T, std::map<T, unsigned>> adj;
    std::set<T> nodes;
    void add(T sid, T did);
    raw_ostream &print(raw_ostream &o) const;

    /**
     * Traverse the graph in depth first order, keeping track of which nodes
     * still have successors that need exploring (in_progress), and which ones
     * are already fully explored (done).  Also, keeps track of a linear order
     * that is compatible with the DAG, or a cycle, in case one is found.
     *
     * Returns true, if the graph is indeed acyclic.
     */
    bool dfs(T node, std::set<T> &unmarked, std::set<T> &in_progress,
             std::set<T> &done, std::vector<T> &sorted);

    /**
     * Do a topological sort of the Graph and check that it is cycle-free
     * (acyclic).  Returns a (one of the possible ones) linear order that is
     * compatible with the order in the graph if it is cycle-freeC in
     * *reverse* !
     *
     * Returns true, if the graph is a DAG, i.e., cycle-free
     */
    bool topo_sort(std::vector<T> &sorted);
};

/**
 * LLGraph is doubly linked and provides operations for removing nodes while
 * keeping the connectivity through them.  We are using that for representing
 * CFGs of BasicBlocks that are maintained out of band and can be modified /
 * filtered.
 *
 * This is independent of the type actually stored, but relies on
 * predecessor(T*) and successor(T*) being existing functions that give an
 * iterator for the predecessors / successors of a T node.
 */
template <typename T>
struct LLGraph {
    /**
     * This is a node in a doubly linked CFG where each Node corresponds to a
     * BasicBlock and tracks a separate set of successor and predecessor
     * BasicBlocks.  That way, we can represent CFG overlays that can be
     * manipulated without manipulating the actual CFG.
     *
     * XXX: Direct linkage between Nodes and then a link off to the
     *      T -> mmhm, but that makes the insertion harder, as we have to
     *      walk the bb_ps map then...
     *
     */
    struct Node {
        std::set<T*> preds;
        std::set<T*> succs;
        T*           node;
        Node() {}
        Node(T* n) :
            preds(predecessors(n).begin(), predecessors(n).end()),
            succs(successors(n).begin(), successors(n).end()),
            node(n)
        {
        }

        raw_ostream& print(raw_ostream& o) const;
    };


    void add(T* n) { T_pss.emplace(n, n); }

    void unlink_remove(T* n);

    std::set<T*>& preds(T* n) { return T_pss.at(n).preds; }
    std::set<T*>& succs(T* n) { return T_pss.at(n).succs; }
    const std::set<T*>& preds(T* n) const { return T_pss.at(n).preds; }
    const std::set<T*>& succs(T* n) const { return T_pss.at(n).succs; }

    // Connect T payload and Nodes through a map (for now)
    std::map<T*, Node> T_pss;

    raw_ostream& print(raw_ostream& o) const;
    /*
     * OH HOW I HATE C++!!!! This function obviously does not need to be a
     * friend... but otherwise, non-deduced contexts make this not work.  I
     * wanted to simply write rawo& operator<<(rawo&, const typename
     * LLGraph<T>i::Node&) but NO!i
     * */
    friend raw_ostream& operator<<(raw_ostream& o, const Node& n) {
      n.print(o);
      return o;
    };
};

/********* Implementations **********/
template <typename T>
void Graph<T>::add(T sid, T did) {
    LLVM_DEBUG(dbgs() << "Adding " << sid << " -> " << did << '\n');
    adj[sid][did]++;
    nodes.insert(sid);
    nodes.insert(did);
}

template <typename T>
raw_ostream& Graph<T>::print(raw_ostream& o) const {
    for (auto &src : adj) {
        o << src.first << " -> ";
        for (auto &dst : src.second) {
            o << dst.first << " (" << dst.second << ") ";
        }
        o << '\n';
    }
    return o;
}

template <typename T>
bool Graph<T>::dfs(T node, std::set<T>& unmarked, std::set<T>& in_progress,
         std::set<T>& done, std::vector<T>& sorted) {
    LLVM_DEBUG(
      dbgs() << "  In Node " << node << " unmarked: " << unmarked
             << " in_progress: " << in_progress
             << " done: " << done
             << " sorted: " << sorted << '\n');

    if (done.count(node))
        return true;
    if (in_progress.count(node)) {
        sorted.push_back(node);
        return false;
    }

    unmarked.erase(node);
    in_progress.insert(node);
    bool acyclic = true;
    for (auto &succ : adj[node]) {
        acyclic &= dfs(succ.first, unmarked, in_progress, done, sorted);
        if (!acyclic)
            break;
    }
    in_progress.erase(node);
    done.insert(node);
    sorted.push_back(node);

    LLVM_DEBUG(
      errs() << "  Out Node " << node << " unmarked: " << unmarked
             << " in_progress: " << in_progress
             << " done: " << done
             << " sorted: " << sorted << '\n');
    return acyclic;
}

template <typename T>
bool Graph<T>::topo_sort(std::vector<T>& sorted) {
    std::set<T>    unmarked = nodes;
    std::set<T>    in_progress;
    std::set<T>    done;
    std::vector<T> rev_sorted;
    rev_sorted.reserve(nodes.size());

    bool acyclic = true;
    while (!unmarked.empty() && acyclic) {
        auto it = unmarked.begin();
        T node   = *it;
        unmarked.erase(it);
        LLVM_DEBUG(dbgs() << "Looking at node " << node << '\n');

        acyclic &= dfs(node, unmarked, in_progress, done, rev_sorted);
    }
    sorted.resize(rev_sorted.size());
    std::reverse_copy(rev_sorted.begin(), rev_sorted.end(),
                      sorted.begin());
    return acyclic;
}

template <typename T>
raw_ostream& operator<<(raw_ostream& o, const Graph<T>& g) {
    return g.print(o);
}

template <typename T>
raw_ostream& LLGraph<T>::Node::print(raw_ostream& o) const {
    o << "{ ";
    for (auto p : preds)
        o << p << " ";
    o << "} " << *node << " { ";
    for (auto s : succs)
        o << s << " ";
    o << "}";
    /*
    for (auto s : succs)
      o << "  N" << node << " -> N" << s << '\n';
      */
    return o;
}

template <typename T>
raw_ostream& LLGraph<T>::print(raw_ostream& o) const {
    o << "digraph {\n";
    for (auto &T_ps : T_pss) {
        o << "  ";
        T_ps.second.print(o);
        o << '\n';
    }
    o << "}\n";
    return o;
}

template <typename T>
void LLGraph<T>::unlink_remove(T* n) {
    for (auto &pred : T_pss[n].preds) {
        // Add all of bb's successors to its predecessors
        T_pss[pred].succs.insert(T_pss[n].succs.begin(),
                                 T_pss[n].succs.end());
        // Erase bb from the successors
        T_pss[pred].succs.erase(n);
    }
    for (auto &succ : T_pss[n].succs) {
        // Add all of bb's predecessors to its successors
        T_pss[succ].preds.insert(T_pss[n].preds.begin(),
                                 T_pss[n].preds.end());
        // Erase bb from the predecessors
        T_pss[succ].preds.erase(n);
    }
    T_pss.erase(n);
}

template <typename T>
raw_ostream& operator<<(raw_ostream& o, const LLGraph<T>& g) {
    return g.print(o);
}
#undef DEBUG_TYPE
#endif //ifdef __GRAPHS_HPP__
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
