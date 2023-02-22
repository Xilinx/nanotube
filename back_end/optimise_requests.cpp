/*******************************************************/
/*! \file optimise_requests.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Optimise Nanotube requests by rearranging and merging them.
**   \date 2021-10-21
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
 * _Goal_
 *
 * The goal of the Optimise Requests (optreq) pass is to combine individual
 * packet / map access to reduce the number of accesses in the program and
 * thus the overal number of pipeline stages and taps needed.
 *
 * In order to do that, it tries to combine multiple smaller, adjacent
 * accesses and combine them into a larger one.
 *
 * _Theory of Operation_
 *
 * For each type of access (currently packet reads and packet writes), the
 * pass performs two steps: merge group collection and the actual merge
 * step.
 *
 * _Merge Group Collection_
 *
 * The merge group collection step identifies accesses which are (1) legal
 * to merge and (2) are beneficial to merge.
 *
 * Accesses are only legal to merge if:
 * - they are at a known distance from one another:
 *   - packet_read(%base, 2) and packet_read(%base + 3, 1) can be merged
 *   - packet_read(%base, 2) and packet_read(14, 1) cannot be merged (it is
 *     not clear where they are in relation to one another)
 * - they are to the same map entry (not implemented yet)
 * - they are of the same type (packet reads with packet reads, ...)
 * - EITHER: they are adjacent (no interleaving incompatible access)
 *   - packet_read(3, 4); packet_write(4, 2); packet_read(7, 1) cannot
 *     merge due to intermediate write
 * - OR: they can pass interleaving incompatible accesses
 *   - packet_read(3, 4); packet_write(17, 2); packet_read(7,1) can merge
 *     the reads, as the write is non-overlapping!
 * - NOTE: overlapping writes must not pass / reorder with one another,
 *   while reads can be arbitrarily reordered with one another
 *
 * Beneficial merging has various "soft" factors:
 * - minimise the gaps between accesses (keeps the amount of state small
 *   that needs to be passed around!)
 *   - example: packet_read(0, 14) and packet_read(64, 3) might not be a
 *              good choice
 *   - example: packet_read(7,2) and packet_read(9,1) are great to merge
 * - merge as much as possible (shortest pipeline)
 * - avoid far reaching merges (front and end of pipeline) as data needs to
 *   stay live between those and will increase live state cost (not
 *   implemented, yet), but not a problem if there are no other
 *   intermediate stages as the merge will then collapse into a single
 *   pipeline stage!
 * - limit the size of merged accesses (not implemented, yet)
 *
 * Currently, the merge groups are constructed as follows:
 * - collect all accesses of one specific type (packet read) in one code
 *   unit (we can do per-basic block or entire packet kernel)
 * - split groups based on hard requirements
 *   - aggregate around identical offset bases
 *   - determine potential insertion point for merged access
 *   - for each access in the group
 *     - trace from each individual access in group to insertion point
 *     - block at any incompatible access
 *   - create (sub-)groups based on block points so that each sub-group has
 *     the same block point
 * - review size and gaps of the group (size of holes vs overall size), and
 *   split if necessary
 *
 * Then, for each resulting merge group, try to find a place to insert the
 * merged access, observing the dominate / post-dominate conditions
 * outlined above.  That is not always possible and needs work in the
 * algorithm (graceful decay, for example splitting the group based on
 * problems around finding an insertion point!).
 *
 * Note that the algorithm aggressively tries to merge and will do so over
 * long distances (if fed with accesses from the entire function, for
 * example); there is no way of expressing locality in the code other than
 * limiting the selection of original accesses for the moment.  That can
 * cause issues around finding an inserstion point, as above, and others.
 *
 * _Limitations and Further Development_
 * I think a better way instead of tracing every group member to the
 * insertion point is to do a frontier-based approach, especially for
 * writes: start at each write and go forwards in the program until another
 * write or incompatible access is found; if compatible, grow the merge
 * group and continue tracing; if incompatible, stop and merge just before.
 *
 * Care has to be taken that overlapping writes are either not merged at
 * all, merged to the same big write, or their merged writes are in the
 * same order as the overlapping writes.
 * In the following example:
 *
 *    %A = packet_write( offs = 14, len = 2)
 *    %B = packet_write( offs = 15, len = 2)
 *    ...
 *    %C =  packet_read( offs = 16, len = 1)
 *    %D = packet_write( offs = 12, len = 2)
  *
 * we MUST NOT merge %A down into %D, even though %A can bypass %C. The
 * reason is that %A and %B MUST NOT be reordered, but %B cannot bypass
 * %C. In this example, it seems better to merge %A + %B into a merged
 * access just before %C. The can_bypass code will now return (MERGE_SAME
 * + %B) when trying to push %A past %B; the traversal code does not,
 * however handle that properly. Instead of traversing each access
 * individually (%A -> end, %B -> end, %D -> end), it might be better to
 * traverse starting at %A and collect writes and mop them up into a
 * bigger merge set: trying %A -> end, hitting %B, trying to traverse the
 * combined %A + %B write (offset = 14, length = 3) to the end (and
 * stopping at %C).
 *
 * A challenge with the frontier-flowing approach is that in some cases it
 * might be too eager to merge two access, for example when then colliding
 * with a small read.  If that read only prevents pushing down of a small
 * write, it might be beneficial to separate that small write out from the
 * group:
 *
 * There is a decision, however, that needs to be made: some writes MUST be
 * "mopped up" (overlapping ones!), while others can be bypassed or can be
 * mopped up, as well. That makes a difference checking overlap with later
 * conflicting accesses, as this example (slightly different from the
 * previous one!) shows:
 *
 *     %A = packet_write( offs = 14, len = 2)
 *     %B = packet_write( offs = 16, len = 2)
 *     ...
 *     %C =  packet_read( offs = 16, len = 1)
 *     %D = packet_write( offs = 12, len = 2)
 *
 * Here, two legal merge plans are possible:
 * Greedy mopping up:
 *
 *     %A_B = packet_write( offs = 14, len = 4)   // combined %A + %B
 *     ...
 *     %C =  packet_read( offs = 16, len = 1)
 *     %D = packet_write( offs = 12, len = 2)
  *
 * Only mopping when needed, allowing further delaying some writes and
 * having potential later merge opportunities:
 *
 *     %B = packet_write( offs = 16, len = 2)
 *     ...
 *     %C =  packet_read( offs = 16, len = 1)
 *     %A_D = packet_write( offs = 12, len = 4)  // combined %A + %D
 *
 * That functionality remains to be implemented, however.  One option would
 * be to separate the collection of the constraints and opportunities from
 * making the decision.  I am thinking a graph with edges of different
 * types might be a useful thing, here.
 *
 * _Actual Merge Step_
 *
 * Multiple packet reads are merged as follows:
 * - allocate buffer on stack big enough to cover the entire group
 * - create single, merged read that covers all reads of the merge group
 *    - both buffer and merged read are created early in the program, i.e.,
 *      either at the first read access of the group, a block point, or
 *      similar
 * - replace each original read with a memcpy from the big read buffer to
 *   the application provided read buffer of the small read; with the
 *   correct offset and size
 * - care has to be taken that the wide read dominates all merged reads
 *   (comes strictly before them!)
 *
 * Packet writes work similarly, but have a few nuances:
 * - we need to allocate both a data buffer and a mask buffer covering the
 *   entire size of the merged group early in the program (the entry block
 *   is good for that!)
 * - each small write is replaced with:
 *   - an update to the merged mask (by ORing its mask at the right offset)
 *   - an update to the merged data field (by writing its masked data at
 *     the right offset)
 * - the merged write is pushed to the end of the application and has to
 *   happen after all original small writes; and perfoms a masked write
 *   with the aggregated data and mask
 * - care has to be taken that the merged write post-dominates all writes
 *   of the merge group (comes strictly after them!)
 */

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Pass.h"
#include "llvm/Support/KnownBits.h"

#include "Intrinsics.h"
#include "Map_Packet_CFG.hpp"

#define DEBUG_TYPE "optreq"
using namespace llvm;
using namespace nanotube;

/**
 * A scaled value is a thin wrapper around llvm::Value and captures
 * arithmetic done to the value: multiplication / division / shifts by
 * constant integers.  It also tracks (scaled) constant offset from said
 * base value and keeps them correctly scaled so that they correspond to
 * bytes.
 */
struct scaled_value {
  Value* base;
  unsigned factor;
  unsigned divider;
  signed   shr_amount;
  int64_t  offs;

  scaled_value(Value* v) : base(v), factor(1), divider(1), shr_amount(0),
                           offs(0) {}
  scaled_value(const scaled_value& other) : base(other.base),
                                            factor(other.factor),
                                            divider(other.divider),
                                            shr_amount(other.shr_amount),
                                            offs(other.offs) {}
  void multiply(unsigned m) { factor *= m; }
  void divide(unsigned d) { divider *= d; }
  void shl(unsigned s) { shr_amount -= s; }
  void shr(unsigned s) { shr_amount += s; }

  bool offset(Value* v, int64_t inc, const DataLayout&);
  void update(Value* v) { base = v; }
  int64_t transform(int64_t raw) const;

  bool comparable(const scaled_value& other) const {
    return (factor == other.factor) &&
           (divider == other.divider) &&
           (shr_amount == other.shr_amount) &&
           (base == other.base);
  }
  bool equals(const scaled_value& other) const {
    return comparable(other) && (offs == other.offs);
  }
  typedef size_t hash_type;
  hash_type hash() const {
    return (hash_type)base ^
           ((hash_type)factor << 6) ^
           ((hash_type)divider << 12) ^
           ((hash_type)shr_amount << 18) ^
           ((hash_type)offs << 24);
  }

  Value* to_ir(IRBuilder<>* ir) const;
};

namespace std {
template<>
class hash<scaled_value> {
public:
  size_t operator()(const scaled_value& scv) const {
    return scv.hash();
  }
};
};

llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const scaled_value& scv);
bool operator==(const scaled_value& lhs, const scaled_value& rhs) {
  return lhs.equals(rhs);
}

typedef std::vector<Instruction*>                         ivec_t;
typedef std::tuple<Instruction*, int64_t, uint16_t>       inst_rng_t;
typedef std::vector<inst_rng_t>                           inst_rng_vec_t;
typedef std::unordered_map<scaled_value, inst_rng_vec_t>  v2inst_rng_vec_t;

/**
 * A Merge Group contains accesses that can be merged into one, and the
 * following additional information:
 * - key: the base value used for offset calculations (for constant
 *   offsets, this is constant zero)
 * - insert_point: the instruction before which the merged access should be
 *   placed
 * - accesses: a vector of (instruction, start, length) tuples
 */
struct merge_group {
  scaled_value   key;
  Instruction*   insert_point;
  inst_rng_vec_t accesses;
  merge_group() : key(nullptr), insert_point(nullptr) {}
  merge_group(const scaled_value& k, Instruction* ip) : key(k), insert_point(ip),
    accesses() {}
  merge_group(const scaled_value& k, inst_rng_vec_t& acc) : key(k),
    accesses(acc) {}
  merge_group(const scaled_value& k, Instruction* ip, inst_rng_vec_t& acc) : key(k),
    insert_point(ip), accesses(acc) {}
};

/* Helper to easily print a merge group */
static raw_ostream& operator<<(raw_ostream& os, const merge_group& g) {
  os << "Group Key: " << g.key;
  os << " IP: ";
  if( g.insert_point != nullptr )
    os << *g.insert_point;
  else
    os << "<nullptr>";
  os << '\n';
  for( auto& inst_rng : g.accesses ) {
    os << "  " << std::get<1>(inst_rng) << " " << std::get<2>(inst_rng)
       << " " << *std::get<0>(inst_rng) << '\n';
  }
  return os;
}

static bool is_reachable(BasicBlock* from, BasicBlock* to);

struct optimise_requests : public FunctionPass {
  optimise_requests() : FunctionPass(ID) {
  }
  void get_all_analysis_results(Function& f) {
    dt  = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    pdt = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  }
  void getAnalysisUsage(AnalysisUsage &info) const override {
    info.addRequired<DominatorTreeWrapperPass>();
    info.addRequired<PostDominatorTreeWrapperPass>();
  }

  void collect_nt_calls(Function& f, ivec_t* prds, Intrinsics::ID id);
  void collect_nt_calls(BasicBlock& bb, ivec_t* prds, Intrinsics::ID id);
  void group_nt_calls(const ivec_t& prds,
                      std::vector<merge_group>* prd_groups);

  DominatorTree* dt;
  PostDominatorTree* pdt;
  /* LLVM-specific */
  static char ID;
  bool runOnFunction(Function& f) override;
};

static void merge_packet_group(merge_group* g);

bool optimise_requests::runOnFunction(Function& f) {
  if( !is_nt_packet_kernel(f) )
    return false;

  get_all_analysis_results(f);

  const bool function_level = true;

  if( function_level ) {
  /* Per Function style */
    LLVM_DEBUG(dbgs() << "Performing function-level analysis:\n");
    bool changes = false;

    /* Group and merge reads, first */
    ivec_t prds;
    collect_nt_calls(f, &prds, Intrinsics::packet_read);
    std::vector<merge_group> groups;
    group_nt_calls(prds, &groups);

    for( auto& g : groups )
      merge_packet_group(&g);

    changes |= !groups.empty();

    /* Then try and merge the writes */
    prds.clear(); groups.clear();
    collect_nt_calls(f, &prds, Intrinsics::packet_write_masked);
    LLVM_DEBUG(
      dbgs() << "Packet writes:\n";
      for( auto *i : prds )
        dbgs() << "  " << *i << '\n';
    );
    group_nt_calls(prds, &groups);

    LLVM_DEBUG(
      dbgs() << "Write merge groups:\n";
      for( auto& g : groups )
        dbgs() << g;
    );
    for( auto& g : groups )
      merge_packet_group(&g);

    return changes;
  } else {
  /* Per BB style */
    for( auto& bb : f) {
      LLVM_DEBUG(dbgs() << "Performing basic-block-level analysis on "
                        << bb.getName() << '\n');
      ivec_t prds;
      collect_nt_calls(bb, &prds, Intrinsics::packet_read);
      std::vector<merge_group> groups;
      group_nt_calls(prds, &groups);
      for( auto& g : groups )
        merge_packet_group(&g);
    }
  }
  return false;
}

void optimise_requests::collect_nt_calls(Function& f, ivec_t* prds,
                                         Intrinsics::ID id) {
  for( auto& bb : f)
    collect_nt_calls(bb, prds, id);
}
void optimise_requests::collect_nt_calls(BasicBlock& bb, ivec_t* prds,
                                         Intrinsics::ID id) {
  for( auto& inst : bb ) {
    auto cur_id = get_intrinsic(&inst);
    if( cur_id == id )
      prds->push_back(&inst);
  }
}

static raw_ostream& operator<<(raw_ostream& os, const KnownBits& known) {
  unsigned bw = known.getBitWidth();
  for( unsigned i = bw; i > 0; i-- ) {
    auto m = APInt::getOneBitSet(bw, i - 1);
    if( known.Zero.intersects(m) )
      os << "0";
    else if( known.One.intersects(m) )
      os << "1";
    else
      os << "X";
  }
  return os;
}

#if 0
static std::unordered_map<const Value*, KnownBits> known_bits_cache;
static unsigned max_depth_ckb = 10;
/**
 * A simple wrapper around llvm::computeKnownBits that removes a few
 * limitations; mainly the limited PHI node recursion, and adds result
 * caching.
 *
 * Sadly, there is a major limitation in that I cannot hook into the
 * recursion that happens in llvm::computeKnownBits and
 * llvm::computeKnownBitsFromOperator.  And as for the cache, it would be
 * good to have one, but the challenge is that it would need to get
 * invalidated when (1) different modules / functions are being compiled,
 * and (2) when people change the code.
 */
static compute_known_bits(const Value* v, KnownBits& known, const DataLayout& dl) {
  compute_known_bits(v, known, 0);
}
static compute_known_bits(Value* v, KnownBits& known, unsigned depth) {
  auto it = known_bits_cache.find(v);
  if( it != known_bits_cache.end() ) {
    known = it->second;
    return;
  }
  if( depth > max_depth_ckb )
    return;

  /* Our special case handling here */
  if( isa<PHINode>(v) ) {
  }
  computeKnownBits
}
#endif


llvm::raw_ostream&
operator<<(llvm::raw_ostream& os, const scaled_value& scv) {
  if( scv.base != nullptr )
    os << *scv.base;
  else
    os << "<nullptr>";

  if( scv.factor != 1 )
    os << " * " << scv.factor;
  if( scv.divider != 1 )
    os << " / " << scv.divider;
  if( scv.shr_amount > 0 )
    os << " >> " << scv.shr_amount;
  if( scv.shr_amount < 0 )
    os << " << " << -scv.shr_amount;
  if( scv.offs > 0 )
    os << " + " << scv.offs;
  if( scv.offs < 0 )
    os << " - " << -scv.offs;
  return os;
}

bool scaled_value::offset(Value* v, int64_t inc, const DataLayout& dl) {
  if( (shr_amount <= 0) && ((factor * inc) % divider == 0) ) {
    /* Easy case: clean division, no right shift
     *   -> no rounding issues :) */
    offs += transform(inc);
    base = v;
    return true;
  }

  if( divider != 1 ) {
    errs() << "IMPLEMENT ME: Safe rounding of divided offsets and their "
           << "increments.\nTreating as untraceable for now.\n"
           << "This: " << *this << " New val: " << *v << " Inc: " << inc
           << '\n';
    return false;
  }

  /**
   * Shift right has rounding challenges:
   *   - imagine a shr = 3 (bit -> byte)
   *   - and the new base value has knownBits xxx000
   *   - and the increment is 12
   *   => the byte offset will be (0 + 12) >> 3 == 1
   *
   * Or:
   *   - new base value has knownBits xxx100
   *   - and the increment is 12
   *   => the byte offset will be (4 + 12) >> 3 == 2
   *
   * => we need to check the knownBits and adjust the offset
   */
  uint64_t mask = (1L << shr_amount) - 1;
  if( ((factor * inc) & mask) == 0 ) {
    /* Increment does not change the lower bits / remainder there is no
     * rounding issue! */
    offs += transform(inc);
    base = v;
    return true;
  }

  if( factor != 1 ) {
    errs() << "IMPLEMENT ME: Safe rounding of multiplied offsets and their "
           << "increments.\nTreating as untraceable for now.\n"
           << "This: " << *this << " New val: " << *v << " Inc: " << inc
           << '\n';
    return false;
  }

  /* factor = divider = 1 !*/
  KnownBits known(v->getType()->getScalarSizeInBits());
  computeKnownBits(v, known, dl);
  known = known.trunc(shr_amount);
  if( !known.isConstant() ) {
    dbgs() << "Not knowing enough bits for " << *this << " (known: "
           << known << ") to understand rounding behaviour when adding "
           << "constant " << inc << ".  Treating as not traceable!\n";
    return false;
  }

  /* We know enough bits, so we can adjust the increment! */
  offs += transform(inc + known.getConstant().getZExtValue());
  base = v;
  return true;
}

int64_t scaled_value::transform(int64_t raw) const {
  if( shr_amount > 0 )
    return raw * factor / divider >> shr_amount;
  else if( shr_amount < 0 )
    return raw * factor / divider << (-shr_amount);
  else
    return raw * factor / divider;
}

Value* scaled_value::to_ir(IRBuilder<>* ir) const {
  Value* out = base;
  if( factor != 1 )
    out = ir->CreateMul(out, ir->getInt32(factor));
  if( divider != 1 )
    out = ir->CreateUDiv(out, ir->getInt32(divider));
  if( shr_amount > 0 )
    out = ir->CreateLShr(out, ir->getInt32(shr_amount));
  if( shr_amount < 0 )
    out = ir->CreateShl(out, ir->getInt32(-shr_amount));
  out = ir->CreateZExtOrTrunc(out, ir->getInt64Ty());
  out->setName(base->getName() + ".scaled");
  return out;
}

/**
 * Given a value, strip off all constant offsets that this value has (i.e.,
 * additions / subtractions of constant values); basically the equivalent
 * of GetElementPtrInst::accumulateConstantOffset for non-GEPs, but
 * integers.
 */
static scaled_value strip_all_constants(Value* in, const DataLayout& dl) {
  scaled_value scv(in);
  bool cont = true;
  while( cont ) {
    in = scv.base;

    /* Constants first: simple / expressions */
    auto* cnst = dyn_cast<Constant>(in);
    if( cnst != nullptr ) {
      /* ConstantInt returns a constant 0, and adds the offset */
      auto* ci = dyn_cast<ConstantInt>(in);
      if( ci == nullptr ) {
        errs() << "FIXME: unhandled non-integer constant case in "
               << "strip_all_constants: " << *in << '\n';
        goto done;
      }
      scv.offset(Constant::getNullValue(in->getType()),
                 ci->getSExtValue(), dl);
      goto done;
    }

    /* Instructions: add / sub, ... */
    auto* inst = dyn_cast<Instruction>(in);
    if( inst == nullptr ) {
      /* Non-instruction / non-constant cases are not handled and we stop
       * having a look at them */
      goto done;
    }

    /* It is an instruction.. do some parsing! */
    auto opc = inst->getOpcode();
    switch( opc ) {
      case Instruction::Add: {
        auto* op0 = inst->getOperand(0);
        auto* op1 = inst->getOperand(1);
        if( isa<ConstantInt>(op1) ) {
          cont = scv.offset(op0, cast<ConstantInt>(op1)->getSExtValue(), dl);
        } else if( isa<ConstantInt>(op0) ) {
          cont = scv.offset(op1, cast<ConstantInt>(op0)->getSExtValue(), dl);
        } else {
          /* Give up on more complicated adds */
          LLVM_DEBUG(dbgs() << "Cannot trace ADD with two non-constants.\n");
          goto done;
        }
        break;
      }
      case Instruction::Sub: {
        auto* op0 = inst->getOperand(0);
        auto* op1 = inst->getOperand(1);
        if( isa<ConstantInt>(op1) ) {
          cont = scv.offset(op0, -cast<ConstantInt>(op1)->getSExtValue(), dl);
        } else {
          /* Give up on more complicated subs */
          LLVM_DEBUG(dbgs() << "Cannot trace unhandled SUB.\n");
          goto done;
        }
        break;
      }
      case Instruction::ZExt:
      case Instruction::SExt:
        scv.update(inst->getOperand(0));
        break;
      case Instruction::Or: {
        auto* op0 = inst->getOperand(0);
        auto* op1 = inst->getOperand(1);
        Value* val;
        ConstantInt* ci;
        if( isa<ConstantInt>(op1) ) {
          val = op0;
          ci  = cast<ConstantInt>(op1);
        } else if( isa<ConstantInt>(op0) ) {
          val = op1;
          ci  = cast<ConstantInt>(op0);
        } else {
          LLVM_DEBUG(dbgs() << "Cannot trace OR with two non-constants.\n");
          goto done;
        }

        /* If bits are disjoint, the OR is an ADD (LLVM likes these) */
        if( haveNoCommonBitsSet(val, ci, dl) ) {
          cont = scv.offset(val, ci->getZExtValue(), dl);
        } else {
          LLVM_DEBUG(dbgs() << "Cannot trace OR that cannot prove "
                            << "constant non-ovrelapping. \n");
          goto done;
        }
        break;
      }

      case Instruction::Mul:
      case Instruction::Shl:
      case Instruction::UDiv:
      case Instruction::SDiv:
      case Instruction::LShr:
      case Instruction::AShr:
        /* Handle scaled base / offset values */
        {
        auto* val = inst->getOperand(0);
        auto* op1 = inst->getOperand(1);
        if( !isa<ConstantInt>(op1) ) {
          LLVM_DEBUG(dbgs() << "Cannot trace through non-constant mul / "
                            << "div / shift.\n");
          goto done;
        }
        auto* ci = cast<ConstantInt>(op1);
        KnownBits known(val->getType()->getScalarSizeInBits());
        computeKnownBits(val, known, dl);
        LLVM_DEBUG(
          dbgs() << "Mul / Div " << *inst << '\n';
          dbgs() << "  Val: " << *val << " CI: " << *ci << '\n';
          dbgs() << "Before: " << scv << '\n';
        );
        if( opc == Instruction::Mul ) {
          scv.multiply(ci->getZExtValue());
        } else if( opc == Instruction::Shl ) {
          scv.shl(ci->getZExtValue());
        } else if( opc == Instruction::UDiv ) {
          scv.divide(ci->getZExtValue());
          LLVM_DEBUG(dbgs() << "  Known: " << known << '\n');
        } else if( opc == Instruction::SDiv ) {
          scv.divide(ci->getSExtValue());
          LLVM_DEBUG(dbgs() << "  Known: " << known << '\n');
        } else if( opc == Instruction::LShr ) {
          scv.shr(ci->getZExtValue());
          LLVM_DEBUG(dbgs() << "  Known: " << known << '\n');
        } else if( opc == Instruction::AShr ) {
          scv.shr(ci->getZExtValue());
          LLVM_DEBUG(dbgs() << "  Known: " << known << '\n');
        }
        scv.update(val);
        LLVM_DEBUG(dbgs() << "After: " << scv << '\n');
        break;
        }
      case Instruction::And:
      case Instruction::Xor:
        /* Known unhandled instructions: other arithmetic */
        {
        Value* lhs = inst->getOperand(0);
        Value* rhs = inst->getOperand(1);
        unsigned bw = inst->getType()->getScalarSizeInBits();
        KnownBits lhs_known(bw), rhs_known(bw);
        computeKnownBits(lhs, lhs_known, dl);
        computeKnownBits(rhs, rhs_known, dl);
        LLVM_DEBUG(
          dbgs() << "Inst: " << *inst << " LHS: " << lhs_known
                 << " RHS: " << rhs_known << '\n';
          dbgs() << "Cannot trace through unhandled AND / XOR.\n";
        );
        goto done;
        }
      case Instruction::PHI:
        {
        auto* phi = cast<PHINode>(inst);
        //XXX: Could trace through constant or undef PHIs here!
        KnownBits known(phi->getType()->getScalarSizeInBits());
        computeKnownBits(phi, known, dl);
        LLVM_DEBUG(
          dbgs() << "PHI: " << phi->getName() << " Known: " << known << '\n';
          for( auto& val : phi->incoming_values() ) {
            computeKnownBits(val, known, dl);
            dbgs() << "  Val: " << *val << " Known: " << known << '\n';
          }
          dbgs() << "Cannot trace through PHI.\n";
        );
        goto done;
        }
      case Instruction::Select:
        {
        auto* select = cast<SelectInst>(inst);
        //XXX: Could trace through constant or undef PHIs here!
        LLVM_DEBUG(
          KnownBits known(select->getType()->getScalarSizeInBits());
          computeKnownBits(select, known, dl);
          dbgs() << "Select: " << select->getName()
                 << " Known: " << known << '\n';
          computeKnownBits(select->getTrueValue(), known, dl);
          dbgs() << "  Val: " << *select->getTrueValue()
                 << " Known: " << known << '\n';
          known.resetAll();
          computeKnownBits(select->getFalseValue(), known, dl);
          dbgs() << "  Val: " << *select->getFalseValue()
                 << " Known: " << known << '\n';
          dbgs() << "Cannot trace through SELECT.\n";
        );
        goto done;
        }
      case Instruction::Load:
        LLVM_DEBUG(dbgs() << "Cannot trace through LD.\n");
        goto done;
      default:
        errs() << "WARNING: unhandled instruction case in "
               << "strip_all_constants: " << *in
               << "\nAssuming it cannot be merged.  Better reults might "
               << "be achieved when handling this instruction.\n";

        goto done;
    }
    LLVM_DEBUG(dbgs() << "Parsing " << *inst << " updated value: " << scv
                      << " Cont: " << cont << '\n');
  }
done:
#if 0
  dbgs() << "Most reduced value is: " << *in << " some more info: \n";
  unsigned bw = in->getType()->getScalarSizeInBits();
  KnownBits known(bw);
  computeKnownBits(in, known, dl);
  dbgs() << "  Known bits: " << known << '\n';
#endif
  return scv;
};


/* Sort based on starting offset */
static void sort_group_start_offset(merge_group* g) {
  auto& offs_ivec = g->accesses;
  std::sort(offs_ivec.begin(), offs_ivec.end(),
            [](inst_rng_t& a, inst_rng_t& b) {
               return std::get<1>(a) < std::get<1>(b);
            });
}

static void get_accessed_mask(const merge_group& g, std::vector<bool>* accessed,
                              int64_t* start, int64_t* end, unsigned* count) {
  auto& offs_ivec = g.accesses;
  /* Determine which bytes are actually being read */
  *start = std::get<1>(offs_ivec.front());
  *end   = std::get<1>(offs_ivec.back()) +
           std::get<2>(offs_ivec.back());

  accessed->resize(*end - *start);
  for( auto& inst_rng : offs_ivec ) {
    auto& s = std::get<1>(inst_rng);
    auto& e = std::get<2>(inst_rng);
    for( int64_t i = s; i < s + e; i++ )
      (*accessed)[i - *start] = true;
  }

  *count = 0;
  for( auto a : *accessed )
    if( a )
      (*count)++;
}

/* Find the largest gap */
static void find_largest_gap(const std::vector<bool>& mask, unsigned *gap, unsigned *gap_len ) {
  unsigned cur_gap_start = 0;
  unsigned cur_gap_len  = 0;
  *gap = *gap_len = 0;

  LLVM_DEBUG(
    dbgs() << "Finding gap in mask: ";
    for( auto m : mask )
      dbgs() << m;
    dbgs() << '\n';
  );
  for( unsigned i = 0; i < mask.size(); i++ ) {
    if( mask[i] ) {
      if( cur_gap_len >= *gap_len ) {
        *gap_len = cur_gap_len;
        *gap     = cur_gap_start;
      }
      cur_gap_len   = 0;
      cur_gap_start = i + 1;
    } else {
      cur_gap_len++;
    }
  }
#if 1
  LLVM_DEBUG(dbgs() << "Gap start: " << *gap << " of length "
                    << *gap_len << '\n');
#endif
}

static void split_on_offset(const merge_group& g, int64_t offs,
                            std::vector<merge_group>* out) {
  if( g.key.base == nullptr )
    errs() << "Group " << &g << " has nullptr key!\n";
  assert(g.key.base != nullptr);

  LLVM_DEBUG(dbgs() << "Split group " << g << "at offset: " << offs
                    << '\n');

  /* Split the current group along the largest gap */
  merge_group g1, g2;
  g1.key = g.key;
  g2.key = g.key;
  for( auto& a : g.accesses ) {
    if( std::get<1>(a) < offs )
      g1.accesses.emplace_back(a);
    else
      g2.accesses.emplace_back(a);
  }
  assert(!g1.accesses.empty());
  out->emplace_back(g1);
  assert(!g2.accesses.empty());
  out->emplace_back(g2);
}

/**
 * Check whether a group of accesses should be split because of holes and
 * overlaps etc.
 */
static bool split_group_with_holes(merge_group* g, unsigned empty_factor,
                                   unsigned total_factor,
                                   unsigned max_empty_bytes,
                                   std::vector<merge_group>* out) {
  int64_t  start, end;
  unsigned count;
  bool     needs_split = false;
  std::vector<bool> mask;

  sort_group_start_offset(g);
  get_accessed_mask(*g, &mask, &start, &end, &count);

  LLVM_DEBUG(dbgs() << "start: " << start << " end: " << end << " count: "
                    << count << '\n');
  unsigned len   = end - start;
  unsigned empty = len - count;

  if( empty > max_empty_bytes )
    needs_split = true;
  else if( empty_factor * empty >= total_factor * len )
    needs_split = true;

  LLVM_DEBUG(dbgs() << "Empty: " << empty << " max_empty_bytes: " << max_empty_bytes
                    << " empty_factor * empty: " << empty * empty_factor
                    << " total_factor * len: " << total_factor * len << '\n');
  if( needs_split ) {
    unsigned gap_start, gap_len;
    find_largest_gap(mask, &gap_start, &gap_len);
    split_on_offset(*g, gap_start + start, out);
  }

#if 0
    LLVM_DEBUG(
      dbgs() << "Group of value " << *g->key
             << " " << start << " - " << end
             << " " << count << "/" << end - start
             << ":\n";
      for( auto& offs_inst : g->accesses )
        dbgs() << "  " << std::get<1>(offs_inst)
               << " " << std::get<2>(offs_inst)
               << " " << *std::get<0>(offs_inst) << '\n';
    );
    LLVM_DEBUG(dbgs() << "Needs split: " << needs_split << '\n');
#endif

  return needs_split;
}

//#define DETAILED_DEBUG_UNSUPPORTED
static Instruction* prev_inst(Instruction* cur, ivec_t* todo) {
  auto* prev = cur->getPrevNode();
  if( prev != nullptr ) {
    /* Advance the scan: simple predecessor */
#ifdef DETAILED_DEBUG_UNSUPPORTED
    LLVM_DEBUG(dbgs() << "Direct predecessor: " << *prev << '\n');
#endif
    return prev;
  }

  /* Advance the scan: beginning of BB -> follow first, and enqueue other
   * entries */
  for( auto* pred : predecessors(cur->getParent()) ) {
    auto* t = pred->getTerminator();
    if( prev == nullptr ) {
      prev = t;
      LLVM_DEBUG(dbgs() << "Predecessor BB tail:" << *prev << '\n');
      continue;
    }
    todo->emplace_back(t);
    LLVM_DEBUG(dbgs() << "Fan-in node, remembering " << *t << " for later.\n");
  }
  return prev;
}

static Instruction* next_inst(Instruction* cur, ivec_t* todo) {
  auto* next = cur->getNextNode();
  if( next != nullptr ) {
    /* Advance the scan: simple successor */
#ifdef DETAILED_DEBUG_UNSUPPORTED
    LLVM_DEBUG(dbgs() << "Direct successor: " << *next << '\n');
#endif
    return next;
  }

  /* Advance the scan: end of BB -> follow first, and enqueue other
   * entries */
  for( auto* succ : successors(cur->getParent()) ) {
    auto* t = succ->getFirstNonPHI();
    if( next == nullptr ) {
      next = t;
      LLVM_DEBUG(dbgs() << "Successor BB head:" << *next << '\n');
      continue;
    }
    todo->emplace_back(t);
    LLVM_DEBUG(dbgs() << "Fan-out node, remembering " << *t << " for later.\n");
  }
  return next;
}

/**
 * Check if the ranges [b1, b1 + l1) and [b2, b2 + l2) overlap or not.
 */
static bool range_overlap(int64_t b1, uint16_t l1, int64_t b2, uint16_t l2) {
  bool left_of  = (b1 + l1 <= b2);
  bool right_of = (b2 + l2 <= b1);
  return !(left_of || right_of);
}

/**
 * Check whether the Nanotube access specified in inst_rng can be hoisted
 * past the target instruction.  No specific direction is specified, so we
 * are really asking whether inst_rng and target commute.  Actual data flow
 * does not have to be tracked between the two calls as that is done by the
 * code around can_bypass.  The only question is whether the "behind the
 * scenes" aspect of the access (accessing the packet / map) does commute.
 *
 * \return Enum that says whether the two accesses commute (BYPASS), or can
 *  combined together but must stay in sequence (MERGE_SAME), or do not
 *  commute (BLOCK).
 */
enum bypass_result_t {
  BYPASS,
  MERGE_SAME,
  BLOCK,
};
static bypass_result_t
can_bypass_packet_packet(inst_rng_t* inst_rng, scaled_value& base2,
                         nt_api_call nt_tgt ) {
  auto* inst = cast<CallInst>(std::get<0>(*inst_rng));
  nt_api_call nt_ins(inst);

  assert(nt_ins.is_packet() && nt_tgt.is_packet());

  auto id_ins = nt_ins.get_intrinsic();
  auto id_tgt = nt_tgt.get_intrinsic();

  /* Resizes block everything */
  if( (id_ins == Intrinsics::packet_resize) ||
      (id_tgt == Intrinsics::packet_resize) )
    return BLOCK;

  /* Packet length operations commute with everything (but resizes) */
  if( (id_ins == Intrinsics::packet_bounded_length) ||
      (id_tgt == Intrinsics::packet_bounded_length) )
    return BLOCK;

  /* Two reads always commute with one another */
  if( (id_ins == Intrinsics::packet_read) &&
      (id_tgt == Intrinsics::packet_read) )
    return BYPASS;

  /* The remaining cases are:
   *   - rd + wr
   *   - wr + rd (same as rd + wr!)
   *   - wr + wri
   */
  assert((nt_ins.is_read()  && nt_tgt.is_write()) ||
         (nt_ins.is_write() && nt_tgt.is_read())  ||
         (nt_ins.is_write() && nt_tgt.is_write()));

  /* These may potentially conflict, so check the range accessed */
  /* Read off the offset + length of the target access */
  auto* target = nt_tgt.get_call();
  Value* vlen = nullptr;
  Value* voff = nullptr;
  if( nt_tgt.is_write() ) {
    packet_write_args pwa(target);
    vlen = pwa.length;
    voff = pwa.offset;
  } else if( nt_tgt.is_read() ) {
    packet_read_args pra(target);
    vlen = pra.length;
    voff = pra.offset;
  }
  assert((vlen != nullptr) && (voff != nullptr));
  uint16_t len  = (uint16_t)cast<ConstantInt>(vlen)->getZExtValue();
  scaled_value base = strip_all_constants(voff,
                        inst->getModule()->getDataLayout());
  auto  offs = base.offs;

  /* Get the offset + length from the left access */
  auto  offs2 = std::get<1>(*inst_rng);
  auto  len2  = std::get<2>(*inst_rng);

  bool same_id   = (id_ins == id_tgt);
  bool same_base = base.comparable(base2);
  bool overlap   = range_overlap(offs, len, offs2, len2);

  /* Comparable (same base), non-overlapping accesses commute */
  if( same_base && !overlap ) {
    LLVM_DEBUG(dbgs() << "Allowing " << *inst << " past access "
                      << "that does not overlap: " << *target
                      << " DONE.\n");
    return BYPASS;
  }

  /* Overlapping accesses of the same type (writes) can be merged together,
   * but must not commute */
  if( same_id && same_base ) {
    LLVM_DEBUG(dbgs() << "Cannot bypass but merge with " << *target
                      << " DONE.\n");
    return MERGE_SAME;
  }

  LLVM_DEBUG(dbgs() << "Mustn't bypass access " << *target
                    << " Base: " << same_base
                    << " ID: " << same_id
                    << " Overlap: " << overlap << " DONE.\n");
  return BLOCK;
}

static bypass_result_t
can_bypass_map_map(inst_rng_t* inst_rng, scaled_value& base2, nt_api_call nt_tgt ) {
  return BLOCK;
}

static bypass_result_t
can_bypass(inst_rng_t* inst_rng, scaled_value& base2, Instruction* target)  {
  bypass_result_t res = BYPASS;
  auto* inst = cast<CallInst>(std::get<0>(*inst_rng));
  /* An instruction can always bypass itself */
  if( inst == target )
    return BYPASS;

  /* Only call instructions can fiddle with the "behind the scenes" data.
   * Everything else cannot access packet / map data at this point! */
  if( !isa<CallInst>(target) )
    return BYPASS;

  nt_api_call nt_ins(inst), nt_tgt(cast<CallInst>(target));

  /* Packet accesses commute with map accesses */
  if( (nt_ins.is_packet() && nt_tgt.is_map()) ||
      (nt_ins.is_map() && nt_tgt.is_packet()) )
    return BYPASS;

  /* Outsource the interesting same-type commute checks */
  if( nt_ins.is_packet() && nt_tgt.is_packet() )
    return can_bypass_packet_packet(inst_rng, base2, nt_tgt);
  if( nt_ins.is_map() && nt_tgt.is_map() )
    return can_bypass_map_map(inst_rng, base2, nt_tgt);

  /* Check the left-overs not caught above */
  switch( nt_tgt.get_intrinsic() ) {
    /* All of these should be checked above */
    case Intrinsics::packet_read:
    case Intrinsics::packet_write:
    case Intrinsics::packet_write_masked:
    case Intrinsics::packet_bounded_length:
    case Intrinsics::packet_resize:
    case Intrinsics::map_op:
      errs() << "ERROR: Unexpected operation " << *target << " in can_bypass"
             << ". Aborting!\n";
      exit(1);
      break;

    /* Memory manipulating instructions -> need to check the alias info */
    case Intrinsics::llvm_memcpy:
    case Intrinsics::llvm_memset:
      // XXX: Actually check for aliasing dependencies; do we actually??
      break;

    /* Known compatible accesses across which we can merge! */
    case Intrinsics::llvm_bswap:
    case Intrinsics::llvm_dbg_value:
    case Intrinsics::llvm_lifetime_end:
    case Intrinsics::llvm_lifetime_start:
    case Intrinsics::llvm_stackrestore:
    case Intrinsics::llvm_stacksave:
      break;

    case Intrinsics::none:
      /* XXX: We should probably call one of LLVM's built in alias
       * instructions?  Rather than hoping for the best ;) */
      break;

    /* Unknown: just moan at the programmer ;) */
    default:
      errs() << "IMPLEMENT ME: Unknown intrinsic "
             << intrinsic_to_string(nt_tgt.get_intrinsic())
             << " for instruction " << *target
             << " starting walk at " << *inst << '\n';
      errs() << "Treating as a blocker!\n";
      res = BLOCK;
  }
  return res;
}

/**
 * Trace a single access backwards to a starting point, stopping at and
 * returning incompatible accesses.  Tracing backwards may encounter
 * control flow divergence (convergence in the normal forwards direction!).
 * This function will trace through all potential code paths and identify
 * any incompatible access if it exists.  The caller should check and
 * adjust the returned access for domination relationships.
 */
static Instruction*
trace_to(inst_rng_t* inst_rng, scaled_value& base, Instruction* to, bool to_front = true) {
  auto* inst = std::get<0>(*inst_rng);
  ivec_t todo;
  todo.emplace_back(inst);

  std::unordered_set<Instruction*> visited;
  Instruction* block = nullptr;

  while( !todo.empty() ) {
    auto* cur = todo.back();
    todo.pop_back();

    while( true ) {
#ifdef DETAILED_DEBUG_UNSUPPORTED
      LLVM_DEBUG(dbgs() << "Walking at " << *cur << " from "
                        << *inst << '\n');
#endif
      /* If we reached the insertion point without hitting anything
       * nefarious, then we are done with this instruction. */
      if( cur == to ) {
        LLVM_DEBUG(dbgs() << "Hit target " << *to << " DONE.\n");
        block = to;
        break;
      }

      /* If we find a node that was already visited, this means that we
       * have control flow convergence and it because we are walking only a
       * single access backwards (rather than a group), it is safe to stop
       * the walk here.
       */
      auto it = visited.find(cur);
      if( (it != visited.end()) ) {
        assert(block != nullptr);
        LLVM_DEBUG(dbgs() << "Hit visited node " << *cur
                          << " with termination at " << *block
                          << " DONE.\n");
        break;
      }

      /* We are the first ones to have a look at this instruction, mark and
       * analyse it */
      visited.insert(cur);
      auto res = can_bypass(inst_rng, base, cur);
      if( res == BYPASS ) {
        /* We can push inst past cur -> keep pushing further in the
         * direction of travel :) */
        cur = to_front ? prev_inst(cur, &todo) : next_inst(cur, &todo);
      } else if( res == BLOCK ) {
        /* Access inst must not be pushed past the cur instruction ->
         * remember this as the blocker */
        auto* prev = to_front ? cur->getNextNode() : cur;
        assert(prev != nullptr);
        block = prev;
        break;
      } else if( res == MERGE_SAME ) {
        /* We cannot push the access in inst past the one in cur (must not
         * be reordered), but they can be merged together (or stay ordered
         * in some other way).
         *
         * XXX: This is not implemented, yet.  The idea is to collect these
         * groups of writes that must stay together when traversing the
         * program and building up an incremental merge set instead of
         * tracing each instruction to the potential merge point.  That
         * should also reduce the algorthmic complexity from O(N^2) to O(N)
         * in terms of program size N.*/
        errs() << "IMPLEMENT ME: Currently writes that overlap are not "
               << "optimised, but are treated as a block.  Tracked as "
               << "NANO-344.\n" << "Pushed write: " << *inst << " blocker:"
               << *cur << "\n";
               //<< "\nAborting!\n";
        //abort();

        /* Instead of aborting, treat as a blocking access! */
        auto* prev = to_front ? cur->getNextNode() : cur;
        assert(prev != nullptr);
        block = prev;
        break;
      }

      if( cur == nullptr ) {
        LLVM_DEBUG(dbgs() << "Walk from " << *inst << " did not target "
                          << *to << " but instead an instruction without "
                          << "predecessors. DONE.\n");
        break;
      }
    }
  }
#ifdef DETAILED_DEBUG_UNSUPPORTED
  LLVM_DEBUG(dbgs() << "Done walking from " << *inst << '\n');
#endif
  return block;
}

static BasicBlock* find_nearest_common_dom(BasicBlock* a, BasicBlock* b,
                                           DominatorTree& dt,
                                           PostDominatorTree& pdt,
                                           bool use_post = false) {
  if( a == nullptr )
    return b;
  if( b == nullptr )
    return a;
  return !use_post ? dt.findNearestCommonDominator(a, b)
                   : pdt.findNearestCommonDominator(a, b);
}

/**
 * Check that there are no incompatible accesses between the accesses of
 * the group and the insertion point.
 * Returns true if the group g cannot be merged and instead was split into
 * new entries in out.
 * Returns false if the group stayed as one (and may have been changed).
 */
static bool
split_group_unsupported_intermediate(merge_group* g,
                                     std::vector<merge_group>* out,
                                     DominatorTree* dt,
                                     PostDominatorTree* pdt, bool to_front = true) {
  LLVM_DEBUG(dbgs() << "Checking for unsupported intermediates in group " << *g
                    << '\n');
  std::vector<merge_group> res;

  auto& base = g->key;
  auto* ip   = g->insert_point;
  inst_rng_vec_t todo;
  for( auto& inst_rng : g->accesses )
    todo.emplace_back(inst_rng);

  /* Keep track of blocker instructions; vector for stable order, map for
   * fast contains checks and mapping from blocker -> associated accesses */
  std::vector<Instruction*> block_vec;
  std::unordered_map<Instruction*, inst_rng_vec_t> block_map;

  /* Trace every instruction back to planned insertion point and check that
   * there are no incompatible access on the way there.
   *
   * Block will either be the insertion point, or an instruction before
   * inst that prevents hoisting. */
  while( !todo.empty() ) {
    auto& inst_rng = todo.back();
    auto* inst     = std::get<0>(inst_rng);

    auto* block = trace_to(&inst_rng, base, ip, to_front);
    LLVM_DEBUG(
      if( block == ip ) {
        dbgs() << "Can merge " << *inst << " back to " << *ip << '\n';
      } else {
        dbgs() << "Cannot merge " << *inst << " back to " << *ip
               << " getting blocked at " << *block << " instead.\n";
        if( to_front )
          dbgs() << "(Right after: " << *block->getPrevNode() << ")\n";
      });

    /* Collect the instructions per blocking point */
    auto it = block_map.find(block);
    if( it == block_map.end() ) {
      block_vec.emplace_back(block);
      block_map.emplace(std::piecewise_construct,
                        std::forward_as_tuple(block),
                        std::forward_as_tuple(1, inst_rng));
    } else {
      it->second.emplace_back(inst_rng);
    }
    todo.pop_back();
  }

  LLVM_DEBUG(
    dbgs() << "Per-block accesses:\n";
    for( auto* block : block_vec ) {
      dbgs() << "  Blocker: " << *block << " Accesses:\n";
      for( auto& inst_rng : block_map[block] )
        dbgs() << "    " << *std::get<0>(inst_rng) << '\n';
    }
  );

  /* Go through all blocking / insertion points and the accesses that were
   * associated with them.  Make sure each block point is a valid insertion
   * point, as well, and if it is not, find a better insertion point!
   */
  std::unordered_map<Instruction*, Instruction*> block2ip;
  std::vector<Instruction*> block_res;
  for( auto* block : block_vec ) {
    auto it = block_map.find(block);
    assert(it != block_map.end());
    auto& accesses = it->second;

    LLVM_DEBUG(dbgs() << "Block point " << *block << '\n');

    /* If this (sub-)group is okay to merge at the original insertion
     * point, then we are done here. */
    if( block == g->insert_point ) {
      LLVM_DEBUG(dbgs() << "Block point is original insert point.\n");
      block2ip[block] = block;
      /* If we have other blockers, remember the "normal" group as one of
       * the block results so that it gets output later on.  If it is just
       * a single group, we simply tell the caller to keep it, instead. */
      if( block_vec.size() > 1 )
        block_res.emplace_back(block);
      continue;
    }

    /* Check whether all accesses are properly dominated by the block
     * instruction, and looks as follows:
     *
     *       block
     *         |
     *         |
     *     lowest_dom
     *       /   \
     *      /     o
     *     /      |\
     *    /       | \
     *  acc1   acc2  acc3
     */
    bool block_doms = true;
    BasicBlock* lowest_dom = nullptr;
    for( auto& i : accesses ) {
      auto* inst = std::get<0>(i);
      auto* bb   = inst->getParent();
      LLVM_DEBUG(dbgs() << "  " << *inst << '\n');

      /* Eagerly compute the lowest dominator for all the accesses */
      lowest_dom = find_nearest_common_dom(lowest_dom, bb, *dt, *pdt,
                                           !to_front);

      /* Check whether block actually dominates the access in inst */
      auto* block_bb = block->getParent();
      bool fwd_dom = to_front ? dt->dominates(block_bb, bb)
                               : pdt->dominates(block_bb, bb);
      /* Remember cases where the blocker does not dominate the access */
      if( !fwd_dom ) {
        block_doms = false;
        LLVM_DEBUG( dbgs() << "Block " << *block << " does not dominate "
                           << "access " << *inst << " so cannot be used "
                           << "as a merge point!\n");
      }
    }

    /**
     * If the block node does not dominate all the accesses blocked by it,
     * check whether the lowest dominating BB (that does dominate all the
     * accesses!) is "after" the block node:
     *
     *   block   other
     *      \     /
     *       \   /
     *     lowest_dom
     *       /   \
     *      /     o
     *     /      |\
     *    /       | \
     *  acc1   acc2  acc3
     *
     *  this does not have to be the case, see here:
     *
     *      lowest_dom
     *       /  \    \
     *      /    \    \
     *   block  other  \
     *      \    /      |
     *       \  /       |
     *  not_lowest_dom  |
     *       /   \      |
     *      /     o     |
     *     /      |\    |
     *    /       | \  /
     *  acc1   acc2  acc3
     *
     * (and vice versa where we are going the other direction!)
     */
    auto* block_bb = block->getParent();
    /* Dominance is too strong.  We just need to know  the lowest_dom is
     * between the accesses and the block! */
    bool rev_dom = to_front ? pdt->dominates(lowest_dom, block_bb)
                            : dt->dominates(lowest_dom, block_bb);

    /* Case that needs thought: block_bb == dom_bb.  We know that the
     * accesses are before the block, so there is never a risk of
     * overtaking the block in the same BB as the accesses.  We also know
     * that the dom_bb is not one of the access blocks, so the accesses are
     * fine where they are, in other BBs.  That means that even if block_bb
     * = dom_bb, we can use dom_bb here. */
    bool between_acc_block = to_front ? is_reachable(block_bb, lowest_dom)
                                      : is_reachable(lowest_dom, block_bb);
    LLVM_DEBUG(dbgs() << "rev_dom: " << rev_dom << " between_acc_block: "
                      << between_acc_block << '\n');
    if( !block_doms && !between_acc_block ) {
      /* The lowest_dom is in fact not between the accesses and block, so
       * we CANNOT use that as an insertion point, because that would move
       * the merged access in past the blocker which breaks the idea of
       * blocking.
       *
       * The goal is now to split the accesses based on where they are and
       * find a split and an associated insertion point where the insertion
       * point will:
       * - (post-)dominate the accesses it subsumes
       * - lie between the accesses and the block point
       *
       * We will do this as follows:
       * - group all accesses based on their basic block
       * - for all such basic blocks:
       *   - group those together that (post-)dominate one another, i.e.,
       *     for A, B from access_set: if dominates(A,B) or dominates(B,A)
       *     then group A, B
       *   - the resulting groups fulfill the requirements: they (1)
       *     (post-)dominate all accesses inside of them, and (2) they are
       *     before the block (assuming we remove the block_bb from the
       *     initial set and treat cases special where accesses and blocker
       *     are in the same BB!)
       *
       * - for all resulting groups
       *   - perform a pairwise search: for gA and gB from groups, check if
       *     getLowestCommonDominator is between the group and the block
       *     - if yes: merge the groups, record getLowestCommonDominator as
       *       the group's insert point
       *     - if no: continue comparing
       *
       * - split the original merge group according to the groups that were
       *   found here
       *
       * NOTE: This algorithm will take O(N^2) steps due to the pairwise
       * comparison!
       */

      /* Group accesses based on their containing BB */
      std::unordered_map<BasicBlock*, inst_rng_vec_t> bb2accs;
      for( auto& i : accesses ) {
        auto* inst = std::get<0>(i);
        auto* bb = inst->getParent();
        bb2accs[bb].emplace_back(i);
      }

      /* Group blocks based on whether they dominate one another */
      LLVM_DEBUG(dbgs() << "---- Subsuming BBs Among Each Other ----\n");
      std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> bb2dom_grp;
      std::unordered_map<BasicBlock*, BasicBlock*> bb2dom_lead;
      for( auto& bb_ivec : bb2accs ) {
        auto* bb   = bb_ivec.first;
        bb2dom_grp[bb].emplace_back(bb);
        bb2dom_lead[bb] = bb;
      }
      /* NOTE: I am iterating through the actual BBs here, but it might be
       * faster to iterate through the groups while they are built /
       * reshuffled.  Interator invalidation does make that a little
       * unpretty, however. */
      for( auto& bb_ivec : bb2accs ) {
        auto* bb   = bb_ivec.first;
        //auto& ivec = bb_ivec.second;

        for( auto& other : bb2accs ) {
          auto* other_bb = bb2dom_lead[other.first];
          auto* group_bb = bb2dom_lead[bb];
          if( other_bb == bb || other_bb == group_bb)
            continue;
          //auto& other_ivec = other.second;
          /* Check whether other_bb is a valid insert point for the
           * accesses in bb, as well.  That means, other_bb has to
           * (post-)dominate bb so that we can place a merged access in it.
           */
          bool dom = to_front ? dt->dominates(other_bb, group_bb)
                              : pdt->dominates(other_bb, group_bb);
          LLVM_DEBUG(
            dbgs() << "  This BB: " << bb->getName()
                   << " This Lead: " << group_bb->getName()
                   << " Other BB: " << other.first->getName()
                   << " Other Lead: " << other_bb->getName()
                   << " dom: " << dom << '\n');
          if( !dom )
            continue;

          /* Subsume the group_bb group into the other_bb group */
          LLVM_DEBUG(dbgs() << "  Subsuming group " << group_bb->getName()
                            << " in group " << other_bb->getName()
                            << '\n');
          auto& other_grp = bb2dom_grp[other_bb];
          auto& grp       = bb2dom_grp[group_bb];
          other_grp.insert(other_grp.end(), grp.begin(), grp.end());
          for( auto* bb : grp ) {
            assert(bb2dom_lead[bb] == group_bb);
            bb2dom_lead[bb] = other_bb;
          }
          bb2dom_grp.erase(group_bb);
        }
      }

      LLVM_DEBUG(
        dbgs() << "---- After Subsuming BBs Among Each Other ----\n";
        for( auto& bb_bbvec : bb2dom_grp ) {
          auto* lead = bb_bbvec.first;
          auto& bbs  = bb_bbvec.second;
          dbgs() << "Merge group headed by " << lead->getName() << ":\n";
          for( auto* bb : bbs ) {
            dbgs() << "  " << bb->getName() << ":\n";
            for( auto& acc : bb2accs[bb] )
              dbgs() << "    " << *std::get<0>(acc) << '\n';
          }
        }
      );

      /* If we have multiple groups, see if further BBs dominate them while
       * being before the block bb; if yes, simply group by and merge into
       * them */
      LLVM_DEBUG(dbgs() << "---- Finding Better Dominators ----\n");
      std::vector<BasicBlock*> grp_leads;
      for( auto& bb_bbvec : bb2dom_grp )
        grp_leads.emplace_back(bb_bbvec.first);
      for( auto it = grp_leads.begin(); it != grp_leads.end(); ++it ) {
        auto* bb1 = bb2dom_lead[*it];

        auto it2 = it;
        for( ++it2; it2 != grp_leads.end(); ++it2 ) {
          auto* bb2 = bb2dom_lead[*it2];
          if( bb1 == bb2 )
            continue;
          auto* dom_bb = to_front ? dt->findNearestCommonDominator(bb1, bb2)
                                  : pdt->findNearestCommonDominator(bb2, bb1);
          assert(dom_bb != bb1 && dom_bb != bb2);

          /* Case that needs thought: block_bb == dom_bb. See comment
           * above. It is safe. */
          bool between = to_front ? is_reachable(block_bb, dom_bb)
                                  : is_reachable(dom_bb, block_bb);

          LLVM_DEBUG(
            dbgs() << "  BB1: " << (*it)->getName()
                   << " Lead1: " << bb1->getName()
                   << " BB2: " << (*it2)->getName()
                   << " Lead2: " << bb2->getName()
                   << " Dom BB: " << dom_bb->getName()
                   << " Block BB: " << block_bb->getName()
                   << " between: " << between << '\n');
          if( !between )
            continue;

          /* Subsume both groups (led by bb1 and bb2) under a group under
           * dom_bb */
          LLVM_DEBUG(
            dbgs() << "  Subsuming group1: " << bb1->getName()
                   << " and group2 : " << bb2->getName()
                   << " under dom-BB: " << dom_bb->getName() << '\n');
          auto& dom_grp = bb2dom_grp[dom_bb];
          auto& grp1 = bb2dom_grp[bb1];
          auto& grp2 = bb2dom_grp[bb2];
          dom_grp.insert(dom_grp.end(), grp1.begin(), grp1.end());
          dom_grp.insert(dom_grp.end(), grp2.begin(), grp2.end());
          for( auto* bb : grp1 )
            bb2dom_lead[bb] = dom_bb;
          for( auto* bb : grp2 )
            bb2dom_lead[bb] = dom_bb;
          bb2dom_grp.erase(bb1);
          bb2dom_grp.erase(bb2);
        }
      }

      LLVM_DEBUG(
        dbgs() << "---- After Trying to Find Better Dominators ----\n";
        for( auto& bb_bbvec : bb2dom_grp ) {
          auto* lead = bb_bbvec.first;
          auto& bbs  = bb_bbvec.second;
          dbgs() << "Merge group headed by " << lead->getName() << ":\n";
          for( auto* bb : bbs ) {
            dbgs() << "  " << bb->getName() << ":\n";
            for( auto& acc : bb2accs[bb] )
              dbgs() << "    " << *std::get<0>(acc) << '\n';
          }
        }
      );

      /* Add these resulting groups as block / insertion nodes. */
      for( auto& bb_bbvec : bb2dom_grp ) {
        auto* dom_bb = bb_bbvec.first;
        auto& bb_vec = bb_bbvec.second;
        Instruction* ip_and_block = nullptr;
        if( dom_bb == block_bb ) {
          /* If this group can use the block node, do it. */
          ip_and_block = to_front ? block->getNextNode() : block;
        } else {
          /* This is not the BB of the block inst, so we can push as far as
           * possible in the direction of travel */
          ip_and_block = to_front ? dom_bb->getFirstNonPHI()
                                  : dom_bb->getTerminator();
        }
        res.emplace_back(g->key, ip_and_block);
        for( auto* bb : bb_vec )
          for( auto& acc : bb2accs[bb] )
            res.back().accesses.emplace_back(acc);
      }
      /* Do not add block to block_res, because we do our own adding to out
       * here */
      continue;
    }

    /* We have a (sub-)group that got stuck at a block node that was not
     * the original intended insertion point => update the insertion point.
     */
    if( block_doms ) {
      /* If the block node dominates all blocked accesses, we will use it
       * as an insertion point */
      LLVM_DEBUG(dbgs() << "Updating insert point of group " << g->key
                        << " from " << *g->insert_point << " to block"
                        << *block << '\n');
      block2ip[block] = block;
    } else {
      /* If the block node does not dominate all accesses, we will use the
       * lowest_dom BB as the insertion point.  In there, place the merged
       * access at the end. */
      auto* ip = to_front ? lowest_dom->getTerminator()
                          : lowest_dom->getFirstNonPHI();
      LLVM_DEBUG(dbgs() << "Updating insert point of group " << g->key
                        << " from " << *g->insert_point
                        << " to BB start / end " << *ip
                        << '\n');
      block2ip[block] = ip;
    }
    block_res.emplace_back(block);
  }

  /* Export all the remaining block-based groups */
  for( auto* block : block_res ) {
    auto* ip       = block2ip[block];
    auto it        = block_map.find(block);
    assert(it != block_map.end());
    auto& accesses = it->second;
    assert(ip != nullptr);
    res.emplace_back(g->key, ip, accesses);
  }

  if( res.size() == 0 ) {
    /* We did not create any new groups -> the input one was fine */
    return false;
  }
  if( res.size() == 1 ) {
    /* Still a single group, maybe with a new insert point -> update input
     */
    *g = res.front();
    return false;
  }

  /* We actually had to split the group -> tell the caller */
  out->insert(out->end(), res.begin(), res.end());
  return true;
}

std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>
reachability_cache;
static bool is_reachable(BasicBlock* from, BasicBlock* to) {
  /* NOTE: This is not really correct. If there is no loop around the BB,
   * then it is not reachable from itself. */
  if( from == to )
    return true;
  auto it = reachability_cache.find(from);
  if( it != reachability_cache.end() ) {
    return it->second.count(to);
  } else {
    std::unordered_set<BasicBlock*> visited, todo;
    todo.insert(from);
    while(!todo.empty()) {
      auto it = todo.begin();
      auto* bb = *it;
      visited.insert(bb);
      for( auto* succ : successors(bb) ) {
        if( visited.count(succ) > 0 )
          continue;
        reachability_cache[from].insert(succ);
        todo.insert(succ);
      }
      todo.erase(it);
    }
  }
  return reachability_cache[from].count(to);
}

static bool is_reachable(Instruction* from, Instruction* to) {
  auto* from_bb = from->getParent();
  auto* to_bb   = to->getParent();

  if( from_bb != to_bb )
    return is_reachable(from_bb, to_bb);
  else {
    /* NOTE: Just checking order in the current BB implicitly relies on
     * there not being any loops in the program */
    // return from.comesBefore(to);
    auto it = from->getIterator();
    while( ++it != from_bb->end() ) {
      if( &*it == to )
        return true;
    }
    return false;
  }
}
static Instruction* group_insertion_point(const merge_group& g,
                                          DominatorTree& dt,
                                          PostDominatorTree& pdt,
                                          bool to_front = true) {
  auto& inst_rng_v = g.accesses;
  BasicBlock* insert_bb = nullptr;

  /* Find the lowest common dominator for all the accesses */
  for( auto& inst_rng : inst_rng_v ) {
    auto* bb = std::get<0>(inst_rng)->getParent();
    insert_bb = find_nearest_common_dom(insert_bb, bb, dt, pdt, !to_front);
  }

  /* Find the insertion point in that BB */
  auto* ip = to_front ? insert_bb->getTerminator()
                     : insert_bb->getFirstNonPHI();
  for( auto& inst_rng : inst_rng_v ) {
    auto* inst = std::get<0>(inst_rng);
    if( !to_front )
      inst = inst->getNextNode();
    assert(inst != nullptr);
    if( insert_bb != inst->getParent() )
      continue;
    /* Same BB: find the earliest spot */
    //if( inst.comesBefore(ip) )
    bool llvm_fwd = isPotentiallyReachable(inst, ip, &dt);
    bool llvm_bwd = isPotentiallyReachable(ip, inst, &dt);
    bool my_fwd = is_reachable(inst, ip);
    bool my_bwd = is_reachable(ip, inst);

    if( my_fwd && my_bwd ) {
      errs() << "ERROR: Something fishy is going on. Intruction " << *ip
             << " is both before and after " << *inst << '\n'
             << "That is unsupported. Aborting!\n";
      errs() << "LLVM Fwd: " << llvm_fwd << " LLVM Bwd: " << llvm_bwd << '\n';
      abort();
    }

    if( (to_front && my_fwd) || (!to_front && my_bwd) ) {
      ip = inst;
    }
  }
  return ip;
}

void optimise_requests::group_nt_calls(const ivec_t& prds,
                                       std::vector<merge_group>* prd_groups) {
  if( prds.size() == 0 )
    return;

  auto id = get_intrinsic(prds[0]);
  if( (id != Intrinsics::packet_read) &&
      (id != Intrinsics::packet_write_masked) ) {
    errs() << "ERROR: Trying to group unknown intrinsic "
           << intrinsic_to_string(id) << " for " << *prds[0]
           << "\nAborting!\n";
    abort();
  }

  /* Group packet reads based on the base value used for their offset */
  std::vector<scaled_value> todo;
  v2inst_rng_vec_t          scv2todo;
  for( auto* prd : prds ) {
    Value* vlen = nullptr;
    Value* voff = nullptr;
    /* The logic for reads and writes is the same, just parse out the right
     * fields. */
    assert(get_intrinsic(prd) == id);
    if( id == Intrinsics::packet_read ) {
      packet_read_args pra(prd);
      voff = pra.offset;
      vlen = pra.length;
    } else if( id == Intrinsics::packet_write_masked ) {
      packet_write_args pwa(prd);
      voff = pwa.offset;
      vlen = pwa.length;
    }

    assert((vlen != nullptr) && (voff != nullptr));
    if( !isa<ConstantInt>(vlen) ) {
      errs() << "Unexpected non-constant length " << *vlen << " in "
             << *prd << "\nAborting!\n";
      abort();
    }

    uint16_t len = (uint16_t)cast<ConstantInt>(vlen)->getZExtValue();
    scaled_value scv = strip_all_constants(voff,
                         prd->getModule()->getDataLayout());
    auto offs = scv.offs;

    LLVM_DEBUG(dbgs() << "Grouping access " << *prd << " offset: "
                      << *voff << " simplified: " << scv
                      << '\n');

    /* Add / insert to simple_off -> vec<(inst, offs, len)>
     * For tracking purposes, collapse the constant offsets and track them
     * per access, and as zero for the entire group. */
    scv.offs = 0;
    auto it = scv2todo.find(scv);
    if( it == scv2todo.end() ) {
      scv2todo.emplace(scv,
                   inst_rng_vec_t(1, std::make_tuple(prd, offs, len)));
      todo.emplace_back(scv);
    } else {
#if 0
      LLVM_DEBUG(
        dbgs() << "Adding to group " << *simple_off << '\n';
        for( auto& i : it->second )
          dbgs() << "  " << i.first << " " << *i.second << '\n';
      );
#endif
      it->second.push_back(std::make_tuple(prd, offs, len));
    }
  }

  /* Output groups that have more than one entry */
  std::vector<merge_group> groups;
  for( auto& scv : todo ) {
    auto it = scv2todo.find(scv);
    assert(it != scv2todo.end());
    auto& key_ise = *it;
    if( key_ise.second.size() <= 1 )
      continue;
    groups.emplace_back(key_ise.first, key_ise.second);
  }

  /* Check groups and adjust them if necessary */
  while( !groups.empty() ) {
    auto g = groups.back();
    groups.pop_back();

    /* Ignore (drop) groups with only a single entry */
    if( g.accesses.size() <= 1 )
      continue;

    bool needs_split = false;

    /* Compute and check the insertion point for the merged access */
    bool to_front = (id == Intrinsics::packet_read);
    g.insert_point = group_insertion_point(g, *dt, *pdt, to_front);
    LLVM_DEBUG(dbgs() << "Mergeability check for group " << g << '\n');
    needs_split = split_group_unsupported_intermediate(&g, &groups, dt,
                                                       pdt, to_front);
    if( needs_split ) {
      LLVM_DEBUG(dbgs() << "Group must be split due to unsupported "
                        << "intermediate.\n");
      continue;
    }

    /* Never let the empty space be larger than total_factor / empty_factor
     * of the entire space. */
    const unsigned empty_factor = 8;
    const unsigned total_factor = 1;
    const unsigned max_empty_bytes = 4;
    /* Split groups that have too much free space! */
    needs_split = split_group_with_holes(&g, empty_factor,
                                         total_factor,
                                         max_empty_bytes, &groups);
    if( needs_split ) {
      LLVM_DEBUG(dbgs() << "Group must be split due to holes.\n");
      continue;
    }

    /* We have a group that can be merged! */
    LLVM_DEBUG(dbgs() << "Group can be merged!\n");
    assert(g.key.base != nullptr);
    assert(g.insert_point != nullptr);
    prd_groups->emplace_back(g);
  }
}

/**
 * Merge a group of packet read accesses into a single access and perform
 * memcpy operations at the original read sites.
 */
static void merge_packet_read_group(merge_group* g, Value* buffer,
                                    int64_t start, unsigned len) {
  auto* f = g->insert_point->getFunction();
  IRBuilder<> ir(g->insert_point);

  /* Do the read */
  packet_read_args pra(std::get<0>(g->accesses[0]));
  auto* nt_packet_read = create_nt_packet_read(*f->getParent());
  /* NOTE: One could also try to pull out the offset argument of the
   * left-most / first access in the group and use that offset rather than
   * creating redundant instructions, here. */
  auto* scaled_base = g->key.to_ir(&ir);
  Value* args[] = { pra.packet,
                    buffer,
                    ir.CreateAdd(
                      scaled_base,
                      ir.getInt64(start)),
                    ir.getInt64(len) };
  auto* new_read = ir.CreateCall(nt_packet_read, args);
  LLVM_DEBUG(dbgs() << "New buffer: " << *buffer
                    << "\nNew read: " << *new_read << '\n');

  /* Replace every access with a memcpy out from the big enough buffer */
  for( auto& inst_rng : g->accesses ) {
    auto* inst = std::get<0>(inst_rng);
    auto  from = std::get<1>(inst_rng);
    packet_read_args pra(inst);
    ir.SetInsertPoint(inst);

    auto* buf_gep = ir.CreateConstInBoundsGEP1_32(ir.getInt8Ty(),
                      buffer, from - start);
    auto* memcpy = ir.CreateMemCpy(pra.data_out, 1, buf_gep, 1,
                                   pra.length);
    LLVM_DEBUG(dbgs() << "Replacing " << *inst << " with\n"
                      << *buf_gep << '\n' << *memcpy << '\n');

    if( !inst->use_empty() ) {
      errs() << "IMPLEMENT ME: Merged packet_read " << *inst
             << " has uses which we do not support here:\n";
      for( auto* u : inst->users() )
        errs() << "  " << *u << '\n';
      exit(1);
    }
    inst->eraseFromParent();
  }
}

/**
 * Merge a group of masked writes into one.  The general scheme is that we
 * will allocate a data and a mask buffer, and will merge the data written
 * and mask where the original write would have happened.  We then do a
 * single masked write with the complete data and mask closer to the end of
 * the program.
 */
static void merge_packet_write_group(merge_group* g, Value* buffer,
                                     int64_t start, unsigned len) {
  auto* f = g->insert_point->getFunction();
  IRBuilder<> ir(&*f->getEntryBlock().getFirstInsertionPt());
  unsigned mask_len = (len + 7) / 8;
  auto* mask = ir.CreateAlloca(ir.getInt8Ty(), ir.getInt32(mask_len),
                               "write_mask_off" + Twine(start));
  ir.CreateMemSet(mask, ir.getInt8(0), mask_len, 0);

  /* Create the final, merged write */
  ir.SetInsertPoint(g->insert_point);
  packet_write_args pwa(std::get<0>(g->accesses[0]));
  auto* nt_packet_write = create_nt_packet_write_masked(*f->getParent());
  auto* scaled_base = g->key.to_ir(&ir);
  Value* args[] = { pwa.packet,
                    buffer,
                    mask,
                    ir.CreateAdd(
                      scaled_base,
                      ir.getInt64(start)),
                    ir.getInt64(len) };
  auto* new_write = ir.CreateCall(nt_packet_write, args);
  LLVM_DEBUG(dbgs() << "New buffer: " << *buffer
                    << "\nMask: " << *mask
                    << "\nNew write: " << *new_write << '\n');

  /* Replace every original access with a write into both the data and the
   * mask buffer => instead of writing IR directly, simply call an adapter
   * function that will be inlined later. */
  auto* merge_data_mask = create_nt_merge_data_mask(*f->getParent());
  Value* merge_args[] = { buffer,
                          mask,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr };
  for( auto& inst_rng : g->accesses ) {
    auto* inst = std::get<0>(inst_rng);
    auto  from = std::get<1>(inst_rng);
    packet_write_args pwa(inst);
    ir.SetInsertPoint(inst);

    merge_args[2] = pwa.data_in;
    merge_args[3] = pwa.mask;
    merge_args[4] = ir.getInt64(from - start);
    merge_args[5] = pwa.length;
    auto* mrg = ir.CreateCall(merge_data_mask, merge_args);

    LLVM_DEBUG(dbgs() << "Replacing " << *inst << " with\n"
                      << *mrg << '\n');

    if( !inst->use_empty() ) {
      errs() << "IMPLEMENT ME: Merged packet_write " << *inst
             << " has uses which we do not support here:\n";
      for( auto* u : inst->users() )
        errs() << "  " << *u << '\n';
      exit(1);
    }
    inst->eraseFromParent();
  }
}

static void merge_packet_group(merge_group* g) {
  LLVM_DEBUG(dbgs() << "Merging group " << *g);

  /* Get total size */
  int64_t  start, end;
  unsigned count;
  std::vector<bool> mask;
  sort_group_start_offset(g);
  get_accessed_mask(*g, &mask, &start, &end, &count);
  unsigned len = end - start;

  /* Allocate big enough buffer */
  auto id = get_intrinsic(std::get<0>(g->accesses[0]));
  auto* f = g->insert_point->getFunction();
  IRBuilder<> ir(&*f->getEntryBlock().getFirstInsertionPt());
  auto* buffer = ir.CreateAlloca(ir.getInt8Ty(), ir.getInt32(len),
                                 intrinsic_to_string(id) + "_buf_off" +
                                 Twine(start));

  if( id == Intrinsics::packet_read ) {
    merge_packet_read_group(g, buffer, start, len);
  } else if( id == Intrinsics::packet_write_masked ) {
    merge_packet_write_group(g, buffer, start, len);
  } else {
    errs() << "ERROR: Unknown Intrinsic " << intrinsic_to_string(id)
           << "for group " << *g << "\nAborting!\n";
    abort();
  }
}


char optimise_requests::ID = 0;
static RegisterPass<optimise_requests>
  X("optreq", "Optimise Nanotube requests by rearranging and merging them.",
    true,
    false
  );
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
