/*******************************************************/
/*! \file flatten_cfg.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Flatten the control flow in LLVM IR.
**   \date 2022-11-28
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
 * _Goal_
 *
 * The goal of this pass is to convert a program with control flow into a fully
 * flattened representation where all control flow has been removed.
 *
 * _Algorithm_
 *
 * The algorithm of the pass visits basic blocks in control-flow compatible
 * order in breadth-first traversal and moves instructions from the current
 * basic block to the single flattened basic block.  Special care has to be
 * taken for:
 *   - instructions with side effects: these will be turned into conditional
 *     variants with the condition consisting of whether the parent basic block
 *     was executed in the original program or not
 *
 *   - phi nodes: phi nodes will be converted into intrinsics which take a
 *     one-hot encoding of which edge we used to get to the basic block
 *     containing the phi and will then perform a selection operation of the
 *     provided values
 *
 *   - conditional branches: branches (condintional / unconditional / switch
 *     statements) are removed; they order the basic blocks of the program;
 *     conditional branches / switch statements update the conditions for
 *     tracking which basic block of the program is executing
 */
#include "flatten_cfg.hpp"

#include <string>
#include <utility>
#include <vector>

#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"

#include "../include/nanotube_api.h"
#include "common_cmd_opts.hpp"
#include "Dep_Aware_Converter.h"
#include "printing_helpers.h"
#include "setup_func.hpp"
#include "unify_function_returns.hpp"


#define DEBUG_TYPE "flatten-cfg"
using namespace llvm;
using namespace nanotube;

cl::opt<bool> spec_reads("flatten-spec-reads", cl::desc("Allow the "\
                         "flatten-cfg pass to speculatively execute "\
                         "packet and map reads rather than predicating "\
                         "them."));

const bool inline_helpers = true;

typedef DenseMap<BasicBlock*,Value*> block_to_val_t;
typedef SmallVector<std::pair<BasicBlock*, Value*>, 4> block_val_vec_t;
typedef DenseMap<BasicBlock*, block_val_vec_t> block_to_blockval_t;

/**
 * Return a function type that has N boolean inputs and a single boolean output
 */
static FunctionType*
get_nway_ty(LLVMContext& c, unsigned n) {
  auto* ty = Type::getInt1Ty(c);
  std::vector<Type*> arg_ty(n, ty);
  auto* fty = FunctionType::get(ty, arg_ty, false);
  return fty;
}

/**
 * Creates a reduction tree for N input values and reduction operation op.
 * NOTE: values is being overwritten in the process.
 */
template<typename T>
static Value*
create_reduction_insts(LLVMContext& c, SmallVectorImpl<Value*>& values,
                       Instruction::BinaryOps op, T* ip) {
  unsigned n = values.size();
  assert(n > 0);
  for( unsigned step = 1; step < n; step *= 2 ) {
    for( unsigned i = 0; i + step < n; i += 2 * step ) {
      auto* lhs = values[i];
      auto* rhs = values[i + step];
      values[i] = BinaryOperator::Create(op, lhs, rhs, "", ip);
    }
  }
  return values[0];
}

static Value*
create_nway_function(Module* m, LLVMContext& c, FunctionType* fty,
                     const Twine& name, Instruction::BinaryOps op, unsigned n) {
  std::string name_str = (name + Twine(n)).str();
  auto* f = m->getFunction(name_str);
  if( f != nullptr )
    return f;

  /* We need to declare ( &define!) the function first */
  f = Function::Create(fty, GlobalValue::ExternalLinkage, name_str, m);

  /* Define the function */
  assert(f->arg_size() == n);
  auto* bb = BasicBlock::Create(c, "", f);
  SmallVector<Value*, 8> values;
  for( auto& arg : f->args() )
    values.emplace_back(&arg);
  create_reduction_insts(c, values, op, bb);
  ReturnInst::Create(c, values[0], bb);
  return f;
}
static Value*
create_nway_or(Module* m, LLVMContext& c, FunctionType* fty, unsigned n) {
  return create_nway_function(m, c, fty, "or", Instruction::Or, n);
}
// static Value*
// create_nway_and(Module* m, LLVMContext& c, FunctionType* fty, unsigned n) {
//   return create_nway_function(m, c, fty, "and", Instruction::And, n);
// }

/**
 * Compute the OR of the provided values (one to many).
 *
 * Small cases (1, 2) are handled inline, a helper function will be called for
 * the larger cases.
 */
static Value*
create_or(Instruction* ip, SmallVectorImpl<Value*>& vals, const Twine& name) {
  auto sz = vals.size();
  assert(sz > 0);

  /* Just one entry -> return that */
  if( sz == 1 )
    return vals.front();

  /* Two entries -> synthesise OR instruction directly */
  IRBuilder<> ir(ip);
  if( sz == 2 ) {
    auto* v1 = vals[0];
    auto* v2 = vals[1];
    return ir.CreateOr(v1, v2, name);
  }

  /* >2 enties: call a specialised helper function */
  auto& c = ip->getContext();
  if( !inline_helpers ) {
    auto* m   = ip->getModule();
    auto* fty = get_nway_ty(c, sz);
    auto* f   = create_nway_or(m, c, fty, sz);
    return ir.CreateCall(fty, f, vals, name);
  } else {
    return create_reduction_insts(c, vals, Instruction::Or, ip);
  }
}
/**
 * Compute the OR of the provided values; unpack the BB*, Val* pairs
 */
static Value*
create_or(Instruction* ip, block_val_vec_t& bb_vals, const Twine& name) {
  SmallVector<Value*, 8> vals;
  for( auto& bb_val : bb_vals )
    vals.emplace_back(bb_val.second);
  return create_or(ip, vals, name);
}

/**
 * Returns the type for an nary phi-helper, which looks as follows:
 * ty phi_helperN(i1 sel0, i1 sel1, ..., i1 selN-1,
 *                ty val0, ty val1, ..., ty valN-1);
 */
static FunctionType*
get_nary_phi_ty(LLVMContext& c, Type* ty, unsigned n) {
  std::vector<Type*> arg_ty(2 * n);
  auto* bool_ty = Type::getInt1Ty(c);
  for( unsigned i = 0; i < n; i++ ) {
    arg_ty[i]     = bool_ty;
    arg_ty[i + n] = ty;
  }
  auto* fty = FunctionType::get(ty, arg_ty, false);
  return fty;
}

/**
 * Creates the instructions for emulating a PHI instruction.
 *
 * preds .. predicates in one-hot encoding (only one is true)
 * vals  .. values to pick from
 * ip    .. insertion point
 * l, r  .. range of values / predicates to operate on [l, r)
 * whole_pred .. is a combined predicate for the entire range needed?
 *
 * returns: value selected from vals[l, r) according to preds[l, r)
 */
template<typename T>
static Value*
create_phi_insts(LLVMContext& c, Type* ty, SmallVectorImpl<Value*>& preds,
                 SmallVectorImpl<Value*>& vals, T* ip,
                 unsigned l, unsigned r, Value** whole_pred = nullptr) {
  assert(l <= preds.size());
  assert(l <= vals.size());
  assert(r <= preds.size());
  assert(r <= vals.size());
  assert(l < r);

  unsigned n = r - l;
  bool wp = (whole_pred != nullptr);

  LLVM_DEBUG(dbgs() << "L: " << l << " R: " << r
                    << " N: " << n << " WP: " << wp << '\n');

  /* Trivial case: single entry - just pick that */
  if( n == 1 ) {
    if( wp )
      *whole_pred = preds[l];
    return vals[l];
  }

  IRBuilder<> ir(ip);
  /* Two entries: do a simple select. */
  if( n == 2 ) {
    if( wp )
      *whole_pred = ir.CreateOr(preds[l], preds[l + 1]);
    return ir.CreateSelect(preds[l + 1], vals[l + 1], vals[l]);
  }

  /* N-entries: option 1: split in the middle */
  //unsigned left = (n + 1) / 2;

  /* N-entries: option 2: split off largest power-of-two */
  auto n_bits = std::numeric_limits<unsigned>::digits;
  auto ldz = countLeadingZeros(n - 1);
  unsigned left = 1 << (n_bits - ldz - 1);

  /* Generate the left half of the tree.  If we need a whole predicate,
   * generate one for the left tree, too. */
  Value* left_pred = nullptr;
  Value** left_predp = wp ? &left_pred : nullptr;
  auto* left_v = create_phi_insts(c, ty, preds, vals, ip, l, l + left,
                                  left_predp);
  /* Right half of the tree; always needs to give us a predicate */
  Value* right_pred = nullptr;
  auto* right_v = create_phi_insts(c, ty, preds, vals, ip, l + left, r,
                                   &right_pred);

  LLVM_DEBUG(
    dbgs() << "Left pred: " << nullsafe(left_pred) << '\n';
    dbgs() << "Right pred: " << nullsafe(right_pred) << '\n');

  if( wp )
    *whole_pred = ir.CreateOr(left_pred, right_pred);
  return ir.CreateSelect(right_pred, right_v, left_v);


#ifdef USE_MASKED_PHIS
  /* Sign extend the i1 conditions to full masks */
  for( unsigned i = 0; i < n; i++ )
    preds[i] = ir.CreateSExt(preds[i], ty);
  for( unsigned i = 0; i < n; i++ )
    vals[i] = ir.CreateAnd(preds[i], vals[i]);
  auto* res = create_reduction_insts(c, vals, Instruction::And, ip);
  return res;
#endif
}

static Function*
create_nary_phi(Module* m, LLVMContext& c, FunctionType* fty, Type* ty,
                unsigned n) {
  std::string name_str = "phi" + std::to_string(n);
  auto* int_ty = dyn_cast<IntegerType>(ty);
  if( int_ty != nullptr )
    name_str += "_i" + std::to_string(int_ty->getBitWidth());
  auto* ptr_ty = dyn_cast<PointerType>(ty);
  if( ptr_ty != nullptr )
    name_str += "_ptr";

  auto* f = m->getFunction(name_str);
  if( f != nullptr )
    return f;

  /* We need to declare ( &define!) the function first */
  f = Function::Create(fty, GlobalValue::ExternalLinkage, name_str, m);

  /* Define the function */
  auto* bb = BasicBlock::Create(c, "", f);
  assert(f->arg_size() == 2 * n);

  SmallVector<Value*, 8> preds;
  SmallVector<Value*, 8> vals;
  auto it = f->arg_begin();
  for( unsigned i = 0; i < n; i++ ) {
    it->setName("cond" + Twine(i));
    preds.emplace_back(it++);
  }
  for( unsigned i = 0; i < n; i++ ) {
    it->setName("val" + Twine(i));
    vals.emplace_back(it++);
  }
  auto* res = create_phi_insts(c, ty, preds, vals, bb, 0, n);
  ReturnInst::Create(c, res, bb);
  return f;
}

static void
flatten_phi(PHINode* phi, Instruction* ip, block_val_vec_t& in_edge_preds) {
  auto& c = ip->getContext();
  auto* m = ip->getModule();

  auto* ty = phi->getType();
  auto  sz = phi->getNumIncomingValues();

  /* Uniquify the values present in the PHI and record conditions for each */
  auto* undef = UndefValue::get(ty);
  DenseMap<Value*, SmallVector<Value*,8>> val_to_preds;
  for( auto& bb_pred : in_edge_preds ) {
    auto* bb   = bb_pred.first;
    auto* pred = bb_pred.second;
    auto* val  = phi->getIncomingValueForBlock(bb);
    val_to_preds[val].emplace_back(pred);
  }

  /* Merge undef values with the largest class and collapse them */
  unsigned max_sz = 0;
  Value* max_val = nullptr;
  if( val_to_preds.count(undef) != 0 ) {
    for( auto& val_preds : val_to_preds ) {
      if( val_preds.getFirst() == undef )
        continue;
      auto pred_size = val_preds.getSecond().size();
      if( pred_size <= max_sz )
        continue;
      max_sz  = pred_size;
      max_val = val_preds.getFirst();
    }
    auto& ud_preds = val_to_preds[undef];
    val_to_preds[max_val].append(ud_preds.begin(), ud_preds.end());
    val_to_preds.erase(undef);
  }
  sz = val_to_preds.size();

  /* If there is only a single meaningful value left, return that */
  if( sz == 1 ) {
    phi->replaceAllUsesWith(val_to_preds.begin()->getFirst());
    return;
  }

  /* Tease out the actual predicates and values */
  SmallVector<Value*, 8> preds;
  for( auto& val_preds : val_to_preds ) {
    /* OR together multiple predicates for the same value */
    auto* pred = create_reduction_insts(c, val_preds.getSecond(),
                                        Instruction::Or, ip);
    preds.emplace_back(pred);
  }

  SmallVector<Value*, 8> vals;
  for( auto& val_preds : val_to_preds )
    vals.emplace_back(val_preds.getFirst());

  /* Generate the code either inline or as a separate function */
  Value* phi_repl = nullptr;
  if( !inline_helpers ) {
    auto* fty = get_nary_phi_ty(c, ty, sz);
    auto* f   = create_nary_phi(m, c, fty, ty, sz);
    vals.append(preds.begin(), preds.end());
    IRBuilder<> ir(ip);
    phi_repl = ir.CreateCall(fty, f, vals, phi->getName());
  } else {
    phi_repl = create_phi_insts(c, ty, preds, vals, ip, 0, sz);
  }
  phi->replaceAllUsesWith(phi_repl);
}

static void
flatten_branch(BranchInst* br, Instruction* ip, Value* bb_pred,
               block_to_blockval_t& edge_preds) {
  auto* bb      = br->getParent();
  auto* bb_true = br->getSuccessor(0);

  /* Unconditional branches: edge_pred[src->dst] = bb_pred */
  if( br->isUnconditional() ) {
    edge_preds[bb_true].emplace_back(bb, bb_pred);
    return;
  }

  /* Conditional branches:
   *   edge_pred[src->dst_true]  = bb_pred & cond *
   *   edge_pred[src->dst_false] = bb_pred & !cond */
  IRBuilder<> ir(ip);
  auto* bb_false = br->getSuccessor(1);
  auto* cond     = br->getCondition();
  auto* not_cond = ir.CreateNot(cond, "not_" + cond->getName());

  Value* edge_t_pred, *edge_f_pred = nullptr;
  if( bb_pred == ConstantInt::getTrue(bb_pred->getContext()) ) {
    /* Special case blocks that will always get executed */
    edge_t_pred = cond;
    edge_f_pred = not_cond;
  } else {
    /* Compute the edge predicates normally */
    edge_t_pred = ir.CreateAnd(bb_pred, cond,
                               "pred_" + bb->getName() + "_" +
                               bb_true->getName());
    edge_f_pred = ir.CreateAnd(bb_pred, not_cond,
                               "pred_" + bb->getName() + "_" +
                               bb_false->getName());
  }
  edge_preds[bb_true].emplace_back(bb, edge_t_pred);
  edge_preds[bb_false].emplace_back(bb, edge_f_pred);
}

static void
flatten_switch(SwitchInst* sw, Instruction* ip, Value* bb_pred,
               block_to_blockval_t& edge_preds) {
  auto* bb = sw->getParent();
  auto  sz = sw->getNumCases();
  auto* bb_default = sw->getDefaultDest();

  if( sz == 0 ) {
    /* Just a default case... treat as an unconditional branch */
    edge_preds[bb_default].emplace_back(bb, bb_pred);
    return;
  }

  /* Each case needs its own predicate */
  auto* val = sw->getCondition();
  IRBuilder<> ir(ip);
  SmallVector<Value*, 8> case_preds;
  unsigned n = 0;
  for( auto& cays : sw->cases() ) {
    auto* bb_dest = cays.getCaseSuccessor();
    auto* case_pred = ir.CreateICmpEQ(val, cays.getCaseValue(),
                                      "case_" + Twine(n));
    n++;
    auto* edge_pred = ir.CreateAnd(bb_pred, case_pred,
                                   "pred_" + bb->getName() + "_" +
                                   bb_dest->getName());
    edge_preds[bb_dest].emplace_back(bb, edge_pred);
    case_preds.emplace_back(case_pred);
  }
  auto* not_default  = create_or(ip, case_preds, "not_default");
  auto* default_pred = ir.CreateNot(not_default, "case_default");
  auto* edge_pred    = ir.CreateAnd(bb_pred, default_pred,
                                    "pred_" + bb->getName() + "_" +
                                    bb_default->getName());
  edge_preds[bb_default].emplace_back(bb, edge_pred);
}

/**
 * Generate a call to a conditional store helper intrinsic.
 */
static void
generate_conditional_store(StoreInst* st, Instruction* ip, Value* pred) {
  Module*      m = st->getModule();
  LLVMContext& c = st->getContext();

  auto* val = st->getValueOperand();
  auto* ptr = st->getPointerOperand();
  auto* valty  = cast<IntegerType>(val->getType());
  auto* ptrty  = ptr->getType();
  auto* predty = pred->getType();

  Type* tys[] = {predty, valty, ptrty};
  auto* fty = FunctionType::get(Type::getVoidTy(c), tys, false);
  std::string n = ("cond_store" + Twine(valty->getBitWidth())).str();
  auto* f = m->getFunction(n);

  if( f == nullptr ) {
    /* Generate a very dinky conditional store:
     * entry:
     *   br %pred, %store, %done
     * store:
     *   store %val, %ptr
     *   br %done
     * done:
     *   ret void
     */
    f = Function::Create(fty, GlobalValue::ExternalLinkage, n, m);
    auto* entry = BasicBlock::Create(c, "entry", f);
    auto* store = BasicBlock::Create(c, "store", f);
    auto* done  = BasicBlock::Create(c, "done",  f);
    auto* f_args = f->arg_begin();
    f_args[0].setName("pred");
    f_args[1].setName("val");
    f_args[2].setName("ptr");
    BranchInst::Create(store, done, &f_args[0], entry);
    new StoreInst(&f_args[1], &f_args[2], store);
    BranchInst::Create(done, store);
    ReturnInst::Create(c, done);
  }

  /* Call the conditional store helper with the right arguments and use it
   * to replace the old store. */
  IRBuilder<> ir(ip);
  Value* args[] = {pred, val, ptr};
  auto* call = ir.CreateCall(fty, f, args);
  call->copyMetadata(*st);
  st->eraseFromParent();
}

/**
 * Turns a packet operation into a predicated one and move it.
 */
static void
predicate_packet_access(nt_api_call* pkt_op, Instruction* ip, Value* pred) {
  assert(pkt_op->is_packet());
  IRBuilder<> ir(ip);
  auto* call = pkt_op->get_call();
  int idx = -1;
  Value* new_arg = nullptr;

  bool hoist_read = spec_reads && pkt_op->is_read();
  if( pkt_op->is_access() ) {
    /* Execute reads speculatively if that is okay */
    if( hoist_read ) {
      call->moveBefore(ip);
      return;
    }

    /* For packet accesses, set the length to zero */
    idx = pkt_op->get_data_size_arg_idx();
    if( idx < 0 ) {
      errs() << "ERROR: Could not find the size argument of packet access "
             << *call << "\nAborting!\n";
      exit(1);
    }
    auto* sz = call->getArgOperand(idx);

    /* Replace the size with a either the orignal size or zero */
    auto* zero = Constant::getNullValue(sz->getType());
    new_arg = ir.CreateSelect(pred, sz, zero);
  } else if( pkt_op->get_intrinsic() == Intrinsics::packet_resize ) {
    /* Packet resizes set the resize amount to zero */
    idx = PACKET_RESIZE_ADJUST_ARG;
    auto* adj = call->getArgOperand(idx);

    auto* zero = Constant::getNullValue(adj->getType());
    new_arg = ir.CreateSelect(pred, adj, zero);
  } else {
    errs() << "ERROR: Unhandled packet op " << *call
           << " when flattening.\nAborting!\n";
    exit(1);
  }

  assert(idx >= 0);
  assert(new_arg != nullptr);
  call->setArgOperand(idx, new_arg);

  /* Move the instruction and pick the right meta-data */
  call->moveBefore(ip);
  if( isa<Instruction>(new_arg) )
    cast<Instruction>(new_arg)->copyMetadata(*call);
}

/**
 * Turns a map access into a predicated one and moves it.
 */
static void
predicate_map_access(nt_api_call* map_acc, Instruction* ip, Value* pred) {
  assert(map_acc->is_access() && map_acc->is_map());
  auto* call = map_acc->get_call();

  /* For map accesses, set the operation to a no-op */
  auto idx = map_acc->get_map_op_arg_idx();
  if( idx < 0 ) {
    errs() << "ERROR: Could not find the op argument of map access "
           << *call << "\nAborting!\n";
    exit(1);
  }
  auto* op = call->getArgOperand(idx);

  /* If it is a map read and allowed, execute it speculatively. */
  auto* op_const = dyn_cast<ConstantInt>(op);
  if( spec_reads && (op_const != nullptr) &&
      (op_const->getZExtValue() == NANOTUBE_MAP_READ) ) {
    call->moveBefore(ip);
    return;
  }
  /* Replace the operand with a either the orignal op or no-op */
  IRBuilder<> ir(ip);
  auto* nop    = ConstantInt::get(op->getType(), NANOTUBE_MAP_NOP);
  auto* select = ir.CreateSelect(pred, op, nop);
  call->setArgOperand(idx, select);

  /* Move the instruction and pick the right meta-data */
  call->moveBefore(ip);
  if( isa<Instruction>(select) )
    cast<Instruction>(select)->copyMetadata(*call);
}

/**
 * Predicate a memcpy operation by adjusting the length.
 */
static void
predicate_memcpy(nt_api_call* nac, Instruction* ip, Value* pred) {
  assert(nac->get_intrinsic() == Intrinsics::llvm_memcpy);
  auto* call = nac->get_call();

  const unsigned LLVM_MEMCPY_LEN = 2;
  auto* len = call->getArgOperand(LLVM_MEMCPY_LEN);

  /* Replace the operand with a either the orignal op or no-op */
  IRBuilder<> ir(ip);
  auto* zero = ConstantInt::get(len->getType(), 0);
  auto* select = ir.CreateSelect(pred, len, zero);
  call->setArgOperand(LLVM_MEMCPY_LEN, select);

  /* Move the instruction and pick the right meta-data */
  call->moveBefore(ip);
  if( isa<Instruction>(select) )
    cast<Instruction>(select)->copyMetadata(*call);
}

/**
 * Removes a llvm.stacksave and all associated llvm.stackrestore
 * operations.
 */
static void
remove_stacksave_and_restores(nt_api_call* nac) {
  assert(nac->get_intrinsic() == Intrinsics::llvm_stacksave);
  auto* call = nac->get_call();

  /* With a slightly weird loop go through all users and delete them.
   * Deleting the users updates the users so we can always look at the
   * first entry.
   */
  for( auto ub = call->user_begin(); ub != call->user_end();
       ub = call->user_begin() ) {
    auto* user = cast<Instruction>(*ub);
    auto intr = get_intrinsic(user);
    assert(intr == Intrinsics::llvm_stackrestore);
    user->eraseFromParent();
  }
  nac->get_call()->eraseFromParent();
}

static void
flatten_side_effect(Instruction* inst, Instruction* ip, Value* bb_pred) {

  /* If the basic block will always be executed, no problem */
  auto* pred_const = dyn_cast<ConstantInt>(bb_pred);
  if( (pred_const != nullptr) && (pred_const->isOne()) ) {
    LLVM_DEBUG(dbgs() << "Moving unconditional side-effect " << *inst
                      << '\n');
    inst->moveBefore(ip);
    return;
  }

  /* Stores get turned into a call to a helper function */
  auto* st = dyn_cast<StoreInst>(inst);
  if( st != nullptr ) {
    generate_conditional_store(st, ip, bb_pred);
    return;
  }

  /* Calls need handling per function called:
     - they either need to be known to have no side effects -> move them
     - or we know how to predicate them -> do that
     - otherwise, complain to the user */
  auto* call = dyn_cast<CallInst>(inst);
  if( call != nullptr ) {
    nt_api_call ntc(call);
    /* Turn an NT API access into a predicated version */
    if( ntc.is_packet() ) {
      predicate_packet_access(&ntc, ip, bb_pred);
      return;
    } else if( ntc.is_map() ) {
      predicate_map_access(&ntc, ip, bb_pred);
      return;
    } else if( ntc.get_intrinsic() == Intrinsics::llvm_memcpy ) {
      predicate_memcpy(&ntc, ip, bb_pred);
      return;
    } else if( ntc.get_intrinsic() == Intrinsics::llvm_stacksave ) {
      remove_stacksave_and_restores(&ntc);
      return;
    } else {
      errs() << "ERROR: Trying to move conditional call " << *call
             << " executed with condition " << *bb_pred
	     << " but it's not known to be side-effect free and unknown "
             << "how to predicate.\nAborting!\n";
      exit(1);
    }
  }

  dbgs() << "IMPLEMENT ME: flatten_side_effect(" << *inst << ", " << *ip <<
    ")\n";
  exit(1);
}

/**
 * Moves an instruction and makes it conditional if needed.
 */
BasicBlock::iterator
flatten_instruction(BasicBlock::iterator it, Value* pred, Instruction* ip,
                    block_to_blockval_t& edge_preds,
                    block_val_vec_t& in_edge_preds, Value* bb_pred) {
  auto* inst = &(*it);
  auto nxt = ++it;

  LLVM_DEBUG(dbgs() << "Moving " << *inst << '\n');
  if( inst->isLifetimeStartOrEnd() || isa<DbgInfoIntrinsic>(inst) ) {
    inst->moveBefore(ip);
    return nxt;
  }

  switch( inst->getOpcode() ) {
    case Instruction::Br:
      flatten_branch(cast<BranchInst>(inst), ip, pred, edge_preds);
      break;
    case Instruction::Switch:
      flatten_switch(cast<SwitchInst>(inst), ip, pred, edge_preds);
      break;
    case Instruction::PHI:
      flatten_phi(cast<PHINode>(inst), ip, in_edge_preds);
      break;
    case Instruction::Ret:
      if( inst != ip ) {
        inst->moveBefore(ip);
        ip->eraseFromParent();
      }
      break;
    case Instruction::Load:
      /* We are exception safe and stores will be handled correctly.
       * Therefore, we can move the loads. */
      inst->moveBefore(ip);
      break;
    case Instruction::Store:
    case Instruction::Call:
      flatten_side_effect(inst, ip, bb_pred);
      break;
    default:
      if( !isSafeToSpeculativelyExecute(inst) ) {
        errs() << "ERROR: Unsafe move of instruction " << *inst << '\n';
        exit(1);
      }
      inst->moveBefore(ip);
  }
  return nxt;
}


/**
 * Flatten basic block bb and append instructions before ip.
 */
static bool
flatten_basic_bloc(BasicBlock* bb, Instruction* ip, block_to_val_t& bb_preds,
                   block_to_blockval_t& edge_preds, DominatorTree* dt,
                   PostDominatorTree* pdt) {
  //auto& c = ip->getContext();
  auto& in_edge_preds = edge_preds[bb];

  LLVM_DEBUG(dbgs() << "Flattening BB: " << bb->getName() << '\n');

  /* For merge nodes, we can check whether we have a control flow
   * equivalent BB: one that we post-dominate and that dominates us.  That
   * means both blocks will be executed in the same cases and we can simply
   * reuse the predicate of that block.  That can remove lengthy OR
   * computation for the pred_bb */
  auto* bb_dom = dt->getNode(bb)->getIDom()->getBlock();
  LLVM_DEBUG(dbgs() << "Dominator: " << bb_dom->getName() << '\n');
  assert(dt->dominates(bb_dom, bb));
  Value* pred_bb = nullptr;
  if( pdt->dominates(bb, bb_dom) ) {
    pred_bb = bb_preds[bb_dom];
    LLVM_DEBUG(dbgs() << "Control flow equivalent, reusing predicate: "
                      << *pred_bb << '\n');
  } else {
    /* Compute whether we are in this BB:
     * we are if we came from any of the edges leading to us.
     * pred_bb = pred_edge1 | pred_edge2 | ... */
    pred_bb = create_or(ip, in_edge_preds, "pred_" + bb->getName());
    LLVM_DEBUG(dbgs() << "Computing new predicate: " << *pred_bb << '\n');
  }
  bb_preds[bb]  = pred_bb;

  auto it = bb->begin();
  while( it != bb->end() ) {
    it = flatten_instruction(it, pred_bb, ip, edge_preds, in_edge_preds,
                             pred_bb);
  }
  return false;
}

/**
 * Perform an in-place flattening of function f.
 */
static bool
flatten_function(Function& f, DominatorTree* dt, PostDominatorTree* pdt) {
  bool changes = false;
  auto& bb_entry = f.getEntryBlock();
  auto* ip = bb_entry.getTerminator();

  LLVM_DEBUG(dbgs() << "Flattening function " << f.getName() << '\n'
                    << "Insertion point " << *ip << '\n');

  /* Record for each BB a boolean whether that block was executed */
  block_to_val_t      bb_preds;
  /* Record edges in the CFG and whether they were executed.
   * edge_preds[bb_dest] = (bb_src1, cond1), (bb_src2, cond2), ... */
  block_to_blockval_t edge_preds;

  /* The entry block is always executed. The edge predicates depend on the
   * terminator -> handle that specially here. */
  auto* true_val = ConstantInt::getTrue(f.getContext());
  bb_preds[&bb_entry] = true_val;
  auto* entry_term = bb_entry.getTerminator();
  flatten_instruction(entry_term->getIterator(), true_val, entry_term,
                      edge_preds, edge_preds[&bb_entry], true_val);

  /* Go through all other basic blocks in CFG-compatible order and flatten them
   * into the entry block */
  typedef dep_aware_converter<BasicBlock> dac_t;
  dac_t dac;
  dac.init_forward(f);
  dac.ready_forward(&bb_entry);
  dac.erase(&bb_entry);

  SmallVector<BasicBlock*, 16> to_delete;
  dac.execute([&](dac_t* dac, BasicBlock* bb) {
    changes |= flatten_basic_bloc(bb, ip, bb_preds, edge_preds, dt, pdt);
    dac->ready_forward(bb);
    to_delete.emplace_back(bb);
  });
  for( auto* bb : to_delete)
    bb->eraseFromParent();
  return changes;
}

void flatten_cfg::get_all_analysis_results(Function& f) {
  dt  = &getAnalysis<DominatorTreeWrapperPass>(f).getDomTree();
  pdt = &getAnalysis<PostDominatorTreeWrapperPass>(f).getPostDomTree();
}

void flatten_cfg::getAnalysisUsage(AnalysisUsage &info) const {
  info.addRequired<DominatorTreeWrapperPass>();
  info.addRequired<PostDominatorTreeWrapperPass>();
}

bool flatten_cfg::runOnModule(Module& m) {
  bool changes = false;
  setup_func setup(m, nullptr, false);
  for( auto& thread : setup.threads() ) {
    auto& f = *thread.args().func;
    get_all_analysis_results(f);
    changes |= unify_function_returns(f, dt, pdt);
    changes |= flatten_function(f, dt, pdt);
  }
  for( auto& kernel : setup.kernels() ) {
    auto& f = *kernel.args().kernel;
    get_all_analysis_results(f);
    changes |= unify_function_returns(f, dt, pdt);
    changes |= flatten_function(f, dt, pdt);
  }
  return changes;
}


char flatten_cfg::ID = 0;
static RegisterPass<flatten_cfg>
  X("flatten-cfg", "Flatten the control-flow graph of Nanotube packet "\
    "kernels.",
    true,
    true
  );

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
