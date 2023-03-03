/*******************************************************/
/*! \file Destruct.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert structs and other types into byte arrays.
**   \date 2020-01-31
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/**
** LLVM optimisation pass that converts structs (and other complex types)
** into byte arrays of the same size
**/

#include <unordered_map>
#include <unordered_set>

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "../include/nanotube_utils.hpp"
#include "Dep_Aware_Converter.h"

#define DEBUG_TYPE "destruct"
using namespace llvm;
using namespace nanotube;

namespace {
struct Destruct : public FunctionPass {
    static char ID;
    Destruct() : FunctionPass(ID) {}

    typedef std::unordered_map<Value *, Value *>       V2V;
    typedef std::unordered_map<Function *, Function *> F2F;
    typedef std::unordered_set<Value *>                Vs;
    typedef std::unordered_map<Value *, unsigned>      V2u;

    F2F new2old;

    /**
     * Check whether Value v needs conversion of its output.
     */
    bool convert_output(const DataLayout &DL, Value *v) {
        return !skip_value(DL, v);
    }
    /**
     * Check and count the inputs / operands of Value v that need
     * conversion.
     */
    unsigned convert_input(const DataLayout &DL, User *u) {
        unsigned count = 0;
        for (auto v : u->operand_values())
            if (convert_output(DL, v))
                count++;
        return count;
    }

    /**
     * Return the value of destructed type that is being proxied through
     * the specified BitCast.
     */
    Value *get_bc_op(Value *bc, V2u *bc_users) {
        auto it = bc_users->find(bc);
        assert(it != bc_users->end());
        auto *bc_inst = cast<Instruction>(bc);
        Value *orig = bc_inst->getOperand(0);
        return orig;
    }

    /**
     * Remove a user of a proxy BitCast, decrementing its use count and
     * removing it if it is not used anymore. */
    void pop_bc_use(Value *bc, V2u *bc_users) {
        auto it = bc_users->find(bc);
        assert(it != bc_users->end());
        LLVM_DEBUG(dbgs() << "Popping BC: " << *bc
                          << " Users: " << it->second << '\n');
        if (--it->second == 0) {
            LLVM_DEBUG(dbgs() << "  Deleting it\n");
            auto *bc_inst = cast<Instruction>(bc);
            assert(bc_inst->use_empty());
            bc_inst->eraseFromParent();
            bc_users->erase(it);
        }
    }

    /**
     * Destruct handlers for various things.  They will all take the value
     * that needs conversion, return the new value that they have converted
     * "into", and will add all new instructions they generate to v_new.
     */
    /**
     * GEPs are replaced as follows:
     *     Value *v <- structure / array type
     *     ..
     *     w = GEP v, ...
     * =>
     *     *v' = bitcast byte[]x* , v       XXX: or simply to a byte* ?
     *     ..
     *     x  = GEP, v', 0, offset
     *     w' = bitcast x
     */
    Value *destruct_GEP(const DataLayout &DL, GetElementPtrInst *gep,
                        V2u *bc_users) {
        LLVMContext &C = gep->getContext();
        APInt off(64 /* bits */, 0 /* val */);
        if (!gep->accumulateConstantOffset(DL, off)) {
            errs() << "Cannot get constant byte offset for "
                   << *gep << '\n';
            assert(false && "Need constant GEP offset!");
        }

        // In a GEP we always must convert the source, but sometimes also the
        // destination! XXX: Mmhmh, we could have opaque types as the source..
        // meh!
        Value *ptr    = gep->getPointerOperand();
        Type *src     = ptr->getType();
        Type *src_new = get_simple_ty(C, DL, src);
        assert(src_new);
        Type *dst     = gep->getType();
        Type *dst_new = get_simple_ty(C, DL, dst);

        Value *ptr_new = get_bc_op(ptr, bc_users);
        LLVM_DEBUG(dbgs() << "Src_new: " << *src_new << " ptr_new->t(): "
                          << *ptr_new->getType() << '\n');
        assert(ptr_new->getType() == src_new);

        // Replace the GEP so it dereferences v'
        auto  *idx  = ConstantInt::get(C, off);
        //auto  *zero = ConstantInt::get(Type::getInt32Ty(C), 0);
        //Value *idx_list[] = {zero, idx};
        Value *idx_list[] = {idx};
        Type  *new_ty = src_new->getPointerElementType();
        GetElementPtrInst *new_gep = GetElementPtrInst::CreateInBounds(new_ty,
            ptr_new, idx_list, "", gep);

        LLVM_DEBUG(dbgs() << "Replacing " << *gep << " with "
                          << *new_gep << '\n');

        if (!dst_new || new_gep->getType() == dst_new)
            return new_gep;
        // %x = new_GEP
        // %w = GEP  -> w' = bitcast %x
        auto *bc_back =
            new BitCastInst(new_gep, dst_new, "bc_gep", gep);
        return bc_back;
    }
    /**
     * Allocas are simply rewritten to allocate a simplified type of same size
     */
    Value *destruct_alloca(const DataLayout &DL, AllocaInst *alloca,
                           V2u *bc_users) {
        LLVMContext &C = alloca->getContext();
        Type   *new_ty = get_simple_ty(C, DL,
                             alloca->getType()->getPointerElementType());
        auto *newa   = new AllocaInst(new_ty,
            alloca->getType()->getAddressSpace(), alloca->getArraySize(),
            alloca->getAlignment(), "", alloca);

        LLVM_DEBUG(dbgs() << "Replacing " << *alloca << " with "
                          << *newa << '\n');

        // for simple u8* do a cast-back here
        Type *u8p_ty = get_simple_ty(C, DL, alloca->getType());
        if (newa->getType() != u8p_ty) {
            auto *bc = new BitCastInst(newa, u8p_ty, "bc_u8p", alloca);
            return bc;
        } else {
            return newa;
        }

    }

    /**
     * Loads of pointers to the changed type, i.e.,
     *
     *   w = load struct_complex*, struct_complex** v
     *   =>
     *   w' = load simple*, simple** v'
     *
     * Other actual data loads should get their value from a GEP and not be
     * touched.
     */
    Value *destruct_LD(const DataLayout &DL, LoadInst *ld,
                       V2u *bc_users) {
        LLVMContext &C  = ld->getContext();
        assert(ld->getType()->isPointerTy());

        Value *ptr_new = get_bc_op(ld->getPointerOperand(), bc_users);

        LLVM_DEBUG(
          dbgs() << "ptr_ty: " << *ld->getPointerOperand()->getType()
                 << " ptr_new_ty: " << *ptr_new->getType()
                 << " ld_t: " << *ld->getType()
                 << " ld_new_t: " << *get_simple_ty(C, DL, ld->getType())
                 << '\n');

        auto *new_ld = new LoadInst(get_simple_ty(C, DL, ld->getType()),
                                    ptr_new, "", ld->isVolatile(),
                                    ld->getAlignment(), ld);
        LLVM_DEBUG(dbgs() << "Replacing " << *ld << " with "
                          << *new_ld << '\n');
        return new_ld; // we need follow-on processing
    }

    /**
     * Stores of pointers to the changed type, i.e.,
     *
     *   store struct_complex*, struct_complex** v
     *   =>
     *   store simple*, simple** v'
     *
     * Stores of non-pointer data should have the destination pointer
     * bitcasted back just before the store.
     */
    Value *destruct_ST(const DataLayout &DL, StoreInst *st,
                       V2u *bc_users) {
        Value *ptr_new  = get_bc_op(st->getPointerOperand(), bc_users);
        Value *data_new = get_bc_op(st->getValueOperand(), bc_users);

        auto *new_st = new StoreInst(data_new, ptr_new, st->isVolatile(),
                                     st->getAlignment(), st);
        LLVM_DEBUG(dbgs() << "Replacing " << *st << " with "
                          << *new_st << '\n');

        return nullptr;
    }

    /**
     * Bitcasts
     *     w = bitcast v, ...
     * =>
     *     *v' = bitcast byte[]x* , v       XXX: or simply to a byte* ?
     *     ..
     *     w' = bitcast v', ... :)
     */
    Value *destruct_BC(const DataLayout &DL, BitCastInst *bc,
                       V2u *bc_users) {
        LLVMContext &C  = bc->getContext();

        Type *src     = bc->getSrcTy();
        Type *dst     = bc->getDestTy();
        Type *src_new = get_simple_ty(C, DL, src);
        Type *dst_new = get_simple_ty(C, DL, dst);
        if (!src_new) src_new = src;
        if (!dst_new) dst_new = dst;
        bool conv_src = (src != src_new);
        bool conv_dst = (dst != dst_new);
        assert(conv_src || conv_dst);

        Value *v = bc->getOperand(0);

        LLVM_DEBUG(
          dbgs() << "BC " << " src: " << *src << " dst: " << *dst
                 << " src_new: " << *src_new << " dst_new:" << *dst_new
                 << " conv_src: " << conv_src << " conv_dst: " << conv_dst
                 << " Operand: " << *v);

        if (conv_src) {
            v = get_bc_op(v, bc_users);
        }
        LLVM_DEBUG(dbgs() << " Operand: " << *v
                          << " Type: " << *v->getType() << '\n');
        assert(v->getType() == src_new);

        auto *bc_new = new BitCastInst(v, dst_new, "", bc);
        LLVM_DEBUG(dbgs() << "Replacing " << *bc << " with "
                          << *bc_new << '\n');

        return bc_new;
    }

    Value *destruct_I2P(const DataLayout &DL, IntToPtrInst *i2p,
                        V2u *bc_users) {
        LLVMContext &C  = i2p->getContext();

        Type *dst     = i2p->getDestTy();
        Type *dst_new = get_simple_ty(C, DL, dst);
        assert(dst_new != nullptr);
        Value *v = i2p->getOperand(0);
        auto *i2p_new = new IntToPtrInst(v, dst_new, "", i2p);
        return i2p_new;
    }

    Value *destruct_ICMP(const DataLayout &DL, ICmpInst *icmp,
                         V2u *bc_users) {
        LLVMContext &C  = icmp->getContext();

        Value *v0      = icmp->getOperand(0);
        Value *v1      = icmp->getOperand(1);
        Type *src0     = v0->getType();
        Type *src1     = v1->getType();
        Type *src0_new = get_simple_ty(C, DL, src0);
        Type *src1_new = get_simple_ty(C, DL, src1);
        if (!src0_new) src0_new = src0;
        if (!src1_new) src1_new = src1;
        bool conv_src0 = (src0 != src0_new);
        bool conv_src1 = (src1 != src1_new);
        assert(conv_src0 || conv_src1);

        if( conv_src0 )
          v0 = get_bc_op(v0, bc_users);

        if( conv_src1 )
          v1 = get_bc_op(v1, bc_users);

        auto *icmp_new = new ICmpInst(icmp, icmp->getPredicate(), v0, v1,
                                      "");

        return icmp_new;
    }

    Value *destruct_PHI(const DataLayout &DL, PHINode *phi,
                        V2u *bc_users) {
        LLVMContext &C  = phi->getContext();

        Type *dst     = phi->getType();
        Type *dst_new = get_simple_ty(C, DL, dst);
        assert(dst_new != nullptr);

        auto *phi_new = PHINode::Create(dst_new,
                                        phi->getNumIncomingValues(), "",
                                        phi);

        for( unsigned i = 0; i < phi->getNumIncomingValues(); ++i ) {
          Value *v       = phi->getIncomingValue(i);
          BasicBlock *bb = phi->getIncomingBlock(i);

          assert(v->getType() == dst);
          Value *v_new = get_bc_op(v, bc_users);
          assert(v_new != nullptr);
          assert(v_new->getType() == dst_new);
          phi_new->addIncoming(v_new, bb);
        }

        return phi_new;
    }

    /**
     * Destruct a function argument; as a first step, this only adds a BC to
     * convert it.  In another pass, we will rewrite the function proper.
     * Return: new value
     */
    BitCastInst *destruct_arg(const DataLayout &DL, Argument *v,
                              V2u *bc_users) {
        LLVMContext &C = v->getContext();
        Type       *ty = v->getType();
        Type   *new_ty = get_simple_ty(C, DL, ty);
        if(!new_ty)
            return nullptr;

        // Arguments will be converted through bitcasts
        BasicBlock &bb = v->getParent()->getEntryBlock();
        auto *bc = new BitCastInst(v, new_ty, v->getName() + "_dstruct");
        bc->insertBefore(bb.getFirstNonPHI());

        // Insert a (temporary) cast-back
        if (v->use_empty())
            return nullptr;
        auto *bc_back = new BitCastInst(bc, v->getType(), "bc_proxya");
        bc_back->insertAfter(bc);
        v->replaceAllUsesWith(bc_back);
        return bc_back;
    }


    /**
     * Destructure a single instruction so that it will read converted
     * operands (if it was using complex ones previously), and generates
     * simply-typed values (if it was generating complex ones previously).
     * When this is called, all operands that needed conversion will have been
     * converted!
     */
    BitCastInst *destruct_inst(const DataLayout &DL, Instruction *inst,
                               V2u *bc_users) {
        if (!convert_output(DL, inst) && !convert_input(DL, inst))
            return nullptr;

        Value *destruct = nullptr;
        BitCastInst *bc = nullptr;

        // Convert instruction specifics
        if (auto *gep = dyn_cast<GetElementPtrInst>(inst)) {
            destruct = destruct_GEP(DL, gep, bc_users);
        } else if (auto *alloca = dyn_cast<AllocaInst>(inst)) {
            destruct = destruct_alloca(DL, alloca, bc_users);
        } else if (auto *bc = dyn_cast<BitCastInst>(inst)) {
            destruct = destruct_BC(DL, bc, bc_users);
        } else if (auto *st = dyn_cast<StoreInst>(inst)) {
            destruct =  destruct_ST(DL, st, bc_users);
        } else if (auto *ld = dyn_cast<LoadInst>(inst)) {
            destruct = destruct_LD(DL, ld, bc_users);
        } else if (auto *i2p = dyn_cast<IntToPtrInst>(inst)) {
            destruct = destruct_I2P(DL, i2p, bc_users);
        } else if (auto *icmp = dyn_cast<ICmpInst>(inst)) {
            destruct = destruct_ICMP(DL, icmp, bc_users);
        } else if (auto *phi = dyn_cast<PHINode>(inst)) {
            destruct = destruct_PHI(DL, phi, bc_users);
        } else {
            errs() << "Unknown unhandled user instruction " << *inst
                   << '\n';
            //assert(false);
            return nullptr;
        }

        // Convert output with a proxy BitCast
        if (!inst->use_empty()) {
            if (inst->getType() != destruct->getType()) {
                bc = new BitCastInst(destruct, inst->getType(),
                                     "bc_proxyi", inst);
                inst->replaceAllUsesWith(bc);
            } else {
                inst->replaceAllUsesWith(destruct);
            }
        }

        // Clean-up input BitCasts
        std::vector<Value *> bcs_to_pop;
        // Collect operands that came from BCs, first
        for (auto &op : inst->operands()) {
            if (skip_value(DL, op))
                continue;
            bcs_to_pop.push_back(op);
        }
        // Delete the original instruction
        inst->eraseFromParent();
        // Notify proxy BitCasts that they have one less user
        for (auto *op : bcs_to_pop)
            pop_bc_use(op, bc_users);

        return bc;
    }

    /**
     * Destruct Value v and update the readiness tracking structures and proxy
     * BiCast information
     */
    void destruct(dep_aware_converter<Value> *dac, Value* v,
                  const DataLayout &DL, V2u *bc_users) {
        Type *ty = v->getType();

        LLVM_DEBUG(dbgs() << "Destructing " << *v << " Type: " << *ty
                          << " Size: " << (ty->isSized() ?
                            DL.getTypeStoreSize(ty) : -1)
                          << '\n');

        BitCastInst *bc = nullptr;

        // basic idea: convert, cast-back, replace, delete
        if (Instruction *inst = dyn_cast<Instruction>(v)) {
            bc = destruct_inst(DL, inst, bc_users);
        } else if (Argument *arg = dyn_cast<Argument>(v)) {
            bc = destruct_arg(DL, arg, bc_users);
        } else {
            errs() << "Cannot destruct value " << *v << '\n';
            assert(false && "Cannot destruct");
        }

        if (!bc)
            return;

        // Add tracking if a proxy BitCast was added
        LLVM_DEBUG(dbgs() << "Adding BC: " << *bc
                          << " Uses: " << bc->getNumUses() << '\n');
        bc_users->emplace(bc, bc->getNumUses());

        for (User *user : bc->users())
            dac->mark_dep_ready(user);
    }

    /**
     * Should this value be converted or skipped?
     */
    bool skip_value(const DataLayout &DL, const Value *v) {
        Type *ty    = v->getType();
        auto &C     = v->getContext();

        // skip dead values!
        if (v->use_empty())
            return true;
        return get_simple_ty(C, DL, ty) == nullptr;
        return false;
    }

    std::unordered_map<Type *, Type *> simple_type_cache;
    /**
     * Creates an equivalent converted type that is a byte array with the same
     * depth of pointers slapped on to it.
     */
    Type *get_simple_ty(LLVMContext &C, const DataLayout &DL, Type *ty) {
        // Implement a small cache for faster type conversions
        auto it = simple_type_cache.find(ty);
        if (it != simple_type_cache.end())
            return it->second;

        Type *u8_ty  = IntegerType::getInt8Ty(C);
        Type *orig   = ty;
        Type *arr_ty = nullptr;
        uint64_t n_bytes;
        StructType *struct_ty;

        int ptr_depth = 0;
        // Strip and count pointer depth
        while (ty->isPointerTy()) {
            ty = ty->getPointerElementType();
            ptr_depth++;
        }

        // skip non-aggregate types being pointed to
        if (!ty->isAggregateType())
            goto done;

        // Skip types that are already a byte array
        // if (ty->isArrayTy() && ty->getArrayElementType() == u8_ty)
        //    goto done;

        // Skip Nanotube opaque types
        struct_ty = dyn_cast<StructType>(ty);
        if (struct_ty && struct_ty->hasName() &&
            struct_ty->getName().startswith("struct.nanotube_"))
        {
            LLVM_DEBUG(dbgs() << "Skipping Nanotube type "
                              << *orig << '\n');
            goto done;
        }

        // Skip types that are not sized
        if (!ty->isSized()) {
            LLVM_DEBUG(dbgs() << "Skipping unsized type "
                              << *orig << '\n');
            goto done;
        }
        n_bytes = DL.getTypeStoreSize(ty);

        // If the type was a straight aggregate, return an array so that the
        // size matches (for alloca's etc)
        if (ptr_depth == 0)
            arr_ty  = ArrayType::get(u8_ty, n_bytes);
        else
            arr_ty = u8_ty;

        while (ptr_depth--)
            arr_ty = arr_ty->getPointerTo();

        LLVM_DEBUG(dbgs() << "Converting " << *orig
                          << " to " << *arr_ty << '\n');
done:
        simple_type_cache.emplace(orig, arr_ty);
        return arr_ty;
    }

    bool split_function(Function &F) {
        LLVMContext &C = F.getContext();
        // Skip newly created functions
        if (new2old.find(&F) != new2old.end())
            return false;

        // Step 1: Go through all arguments and check if they are only used
        // through bitcasts; collect the target types, and create a new
        // function acepting those
        std::vector<Type *> arg_tys;
        for (auto &arg : F.args()) {
            Type *ty = arg.getType();
            // Keep everything that is not a pointer
            if (!ty->isPointerTy()) {
                arg_tys.push_back(ty);
                continue;
            }

            // Keep everything that has more than one (a bitcast!) user
            if (!arg.hasOneUse()) {
                arg_tys.push_back(ty);
                continue;
            }

            BitCastInst *bc = dyn_cast<BitCastInst>(arg.user_back());
            if (!bc) {
                LLVM_DEBUG(
                  dbgs() << "Ignoring user: " << *arg.user_back() << '\n');
                arg_tys.push_back(ty);
                continue;
            }
            arg_tys.push_back(bc->getDestTy());
            continue;
        }

        FunctionType *fnew_ty = FunctionType::get(F.getReturnType(), arg_tys,
                                                  F.isVarArg());
        Function *fnew = Function::Create(fnew_ty, F.getLinkage(),
                         F.getName() + "_destruct", F.getParent());
        fnew->copyAttributesFrom(&F);
        new2old[fnew] = &F;

        LLVM_DEBUG(
          dbgs() << "Old function type: " << *F.getType()
                 << " New function type: " << *fnew_ty << '\n'
                 << "New function " << *fnew << '\n');

        // Step 2: Copy the instructions over
        fnew->getBasicBlockList().splice(fnew->begin(),
                                         F.getBasicBlockList());

        // Step 3: rewire the arguments
        for (Function::arg_iterator new_arg = fnew->arg_begin(),
             old_arg = F.arg_begin();
             new_arg != fnew->arg_end(); ++new_arg, ++old_arg) {

            LLVM_DEBUG(dbgs() << "Old arg: " << *old_arg << '\n'
                              << "New arg: " << *new_arg << '\n');

            new_arg->setName(old_arg->getName());
            for (auto u : old_arg->users()) {
                BitCastInst *bc = dyn_cast<BitCastInst>(u);
                if (!bc) {
                    LLVM_DEBUG(dbgs() << "Unknown user " << *u << '\n');
                    continue;
                }
                assert(bc->getSrcTy()  == old_arg->getType() &&
                       bc->getDestTy() == new_arg->getType());

                LLVM_DEBUG(dbgs() << "Replacing " << *bc << " with "
                                  << *new_arg << '\n');

                bc->replaceAllUsesWith(new_arg);
                bc->eraseFromParent();
            }
        }

        // Step 4: Create shim from the old to the new function that casts the
        // arguments
        BasicBlock *bb = BasicBlock::Create(C, "", &F);
        SmallVector<Value *, 4> args;
        for (Function::arg_iterator new_arg = fnew->arg_begin(),
             old_arg = F.arg_begin();
             new_arg != fnew->arg_end(); ++new_arg, ++old_arg) {
            if (old_arg->getType() == new_arg->getType()) {
                // No translation needed -> simply forward
                args.push_back(old_arg);
                continue;
            }
            // Different types must be pointers
            assert(old_arg->getType()->isPointerTy() &&
                   new_arg->getType()->isPointerTy());
            // and will be bitcasted
            BitCastInst *bc = new BitCastInst(old_arg, new_arg->getType(), "",
                                              bb);
            args.push_back(bc);
        }
        // and then call the new function
        CallInst::Create(fnew_ty, fnew, args, None, "", bb);
        ReturnInst::Create(C, bb);
        return true;
    }

    void insert_instruction(dep_aware_converter<Value>* dac, const DataLayout &DL,
                            Instruction *inst) {
        unsigned n = convert_input(DL, inst);
        bool consumes = (n > 0);
        bool produces = convert_output(DL, inst);

        LLVM_DEBUG(
          dbgs() << "Instruction " << *inst << " "
                 << (produces ? "P" :"_")
                 << (consumes ? "C" : "_") << '\n');

        if (consumes) {
            LLVM_DEBUG(dbgs() << "Inserting " << inst << " as pending "
                              << n << '\n');
            dac->insert(inst, n);
        } else if (produces) {
            LLVM_DEBUG(dbgs() << "Inserting " << *inst << " as ready!\n");
            dac->insert_ready(inst);
        }
    }

    /**
     * Convert instructions one by one, in data-flow compatible order.  Every
     * converted instance has an output BitCast instruction that ensures the
     * users of the value can stay.  BitCasts are deleted once all users have
     * been converted, which may not happen if we cannot convert all users
     * (e.g., calls).  That way, we can delete the original instruction
     * always. */
    bool depaware_conversion(Function &F) {
        auto &DL = F.getParent()->getDataLayout();
        V2u bc_users;
        dep_aware_converter<Value> dac;
        // Go through arguments and the function body and convert pointers to
        // complex types to pointers to byte[] with the same size
        for (auto &arg : F.args()) {
            if (!skip_value(DL, &arg)) {
                dac.insert_ready(&arg);
                LLVM_DEBUG(dbgs() << "Inserting " << arg
                                  << " as ready!\n");
            }
        }
        // Insert instructions that produce or consume a value that needs
        // conversion
        for (auto &bbl : F) {
            for (auto ii = bbl.begin(); ii != bbl.end(); ++ii) {
                Instruction &inst = *ii;
                insert_instruction(&dac, DL, &inst);
            }
        }

        dac.execute(
            [&](dep_aware_converter<Value>* dac, Value* v) {
                destruct(dac, v, DL, &bc_users);
            });

        return true;
    }

    bool runOnFunction(Function &F) override {
        LLVM_DEBUG(dbgs().write_escaped(F.getName());
                   dbgs() << '\n');

        bool changes_made = false;

        changes_made |= depaware_conversion(F);

        // Separate out another function that contains the body of the
        // previous one but has also the arguments replaced
        //changes_made |= split_function(F);
        return changes_made;
    }

    bool doInitialization(Module &M) override {
        LLVM_DEBUG(
          dbgs() << "Hi there, this is your friendly Destruct pass :)\n");
        return false;
    }
};
} // anonymous namespace

char Destruct::ID = 0;
static RegisterPass<Destruct>
    X("destruct", "Convert structs into byte arrays",
      false,
      false
      );


/* vim: set ts=8 et sw=2 sts=2 tw=75: */
