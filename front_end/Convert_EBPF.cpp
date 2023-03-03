/*******************************************************/
/*! \file Convert_EBPF.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Convert EBPF idioms to Nanotube
**   \date 2020-05-14
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <linux/bpf.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "../back_end/Intrinsics.h"
#include "../include/nanotube_private.hpp"
#include "../include/ebpf_nt_adapter.h"
#include "../back_end/utils.h"

using namespace llvm;
using namespace nanotube;

#define DEBUG_TYPE "ebpf2nanotube"

namespace {
struct ebpf_to_NT : public ModulePass {
  static char ID;

  ebpf_to_NT() : ModulePass(ID) { }

  /**
   * The declaration of bpf_Map_def looks as follows:
   *
   * struct bpf_map_def {
   *   unsigned int type;
   *   unsigned int key_size;
   *   unsigned int value_size;
   *   unsigned int max_entries;
   *   unsigned int map_flags;
   *   unsigned int inner_map_idx;   <- only in Katran?
   *   unsigned int numa_node;       <- only in Katran?
   * };
   */
  enum bpf_map_idx {
    BPF_TYPE = 0,
    BPF_KEY_SZ,
    BPF_VAL_SZ,
    BPF_MAX_ENT,
    BPF_FLAGS,
    BPF_INNER_IDX,
    BPF_NUMA_ND,
  };

  enum bpf_func_id {
    BPF_UNKNOWN         = -1,
    BPF_MAP_LOOKUP      = 1,
    BPF_MAP_UPDATE      = 2,
    BPF_KTIME_GET_NS    = 5,
    BPF_XDP_ADJUST_HEAD = 44,
    BPF_XDP_ADJUST_META = 54,
  };

  bpf_func_id get_ebpf_call_id(CallInst* call) {
    if( call == nullptr )
      return BPF_UNKNOWN;

    Value* v = call->getCalledOperand();
    auto* u = dyn_cast<User>(v);
    if( u == nullptr ) {
      //errs() << "Non-user " << *v << " in call " << *call << '\n';
      return BPF_UNKNOWN;
    }
    auto* c = dyn_cast<Constant>(u);
    if( c == nullptr ) {
      //errs() << "Non-constant user: " << *u << " in call " << *call <<'\n';
      return BPF_UNKNOWN;
    }
    auto res = get_int_casted_to_pointer(c);
    if( !res.first) {
      //errs() << "Unknown constant " << res.second << " in call " << *call << '\n';
      return BPF_UNKNOWN;
    }
    return (bpf_func_id)res.second;
  }

  /**
   * Checks whether a global variable defines an EBPF map in the classic
   * style.
   */
  bool is_classic_ebpf_map(GlobalVariable& g) {
    /* Classic map definitions are simple structs with 5 / 7 entries and
     * u32 values describing the map */
    Type* t = g.getType();
    assert( t->isPointerTy() );
    auto* st = dyn_cast<StructType>(t->getPointerElementType());
    if( st == nullptr )
      return false;

    //XXX: Can we reintroduce the struct type name check?
    auto* int32_ty = IntegerType::get(g.getParent()->getContext(), 32);
    unsigned count = st->getNumElements();
    if( (count != 5) && (count != 7) && (count != 9) )
      return false;

    for( auto* t : st->elements() ) {
      if( t != int32_ty )
        return false;
    }
    return true;
  }

  /**
   * Check whether a global variable is a BTF-style map definition.
   */
  bool is_btf_ebpf_map(GlobalVariable& g) {
    return g.getSection().equals(".maps");
  }

  /*!
  ** Checks wheter a global variable describes an EBPF map.
  ** @param g Global variable that is checked.
  ** @return True if the variable is an EBPF map description.
  **/
  bool is_ebpf_global(GlobalVariable& g) {
    bool classic = is_classic_ebpf_map(g);
    bool btf     = is_btf_ebpf_map(g);

    LLVM_DEBUG(
      if( classic || btf ) {
        dbgs() << "Global variable " << g << " is an EBF map.  "
               << "Classic: " << classic
               << " BTF: " << btf << '\n';
      });

    return classic || btf;
  }

  unsigned get_struct_entry(const ConstantStruct* cs, unsigned op) {
    return cast<ConstantInt>(cs->getOperand(op))->getZExtValue();
  }
  ConstantInt* get_const_int32(LLVMContext& C, uint32_t val) {
    return ConstantInt::get(Type::getInt32Ty(C), val);
  }
  ConstantInt* get_const_int64(LLVMContext& C, uint64_t val) {
    return ConstantInt::get(Type::getInt64Ty(C), val);
  }

  map_type_t ebpf_to_nt_map_type(unsigned ebpf_map_type) {
    switch( ebpf_map_type ) {
      /* Various types of hash */
      case BPF_MAP_TYPE_HASH:
        return NANOTUBE_MAP_TYPE_HASH;
      case BPF_MAP_TYPE_LRU_HASH:
        return NANOTUBE_MAP_TYPE_LRU_HASH;

      /* Arrays */
      case BPF_MAP_TYPE_ARRAY:
        return NANOTUBE_MAP_TYPE_ARRAY_LE;
      case BPF_MAP_TYPE_PERCPU_ARRAY:
        return NANOTUBE_MAP_TYPE_ARRAY_LE;

      /* Unhandled */
      default:
        errs() << "Unknown / unhandled EBPF map type " << ebpf_map_type
               << '\n';
        return NANOTUBE_MAP_TYPE_ILLEGAL;
    }
  }

  struct nt_map_info {
    size_t     key_sz;
    size_t     value_sz;
    uint32_t   max_entries;
    map_type_t type;
    uint32_t   flags;
    Value*     local_val;
    Value*     global_id;
  };

  bool parse_classic_ebpf_map_def(Module& m, LLVMContext& C, GlobalVariable& g,
                                  nt_map_info* nt_entry) {
    StructType* gty = cast<StructType>(g.getType()->getPointerElementType());
    unsigned n = gty->getNumElements();
    if( n != 5 && n != 7 && n != 9) {
      errs() << "Unknown number of elements " << n << " in " << g << '\n';
      return false;
    }

    if( !g.hasInitializer() ) {
      errs() << "No initialiser found for " << g << '\n';
      return false;
    }

    auto* cs = dyn_cast<ConstantStruct>(g.getInitializer());
    if( cs == nullptr ) {
      errs() << "No constant struct initialiser found for " << g << '\n';
      return false;
    }

    /* Pull out interesting data and store for our own consumption */
    assert(nt_entry != nullptr);
    nt_entry->key_sz      = get_struct_entry(cs, BPF_KEY_SZ);
    nt_entry->value_sz    = get_struct_entry(cs, BPF_VAL_SZ);
    nt_entry->max_entries = get_struct_entry(cs, BPF_MAX_ENT);
    nt_entry->type        = ebpf_to_nt_map_type(
                              get_struct_entry(cs, BPF_TYPE));
    nt_entry->flags       = get_struct_entry(cs, BPF_FLAGS);

    if( nt_entry->type == NANOTUBE_MAP_TYPE_ILLEGAL ) {
      errs() << "IMPLEMENT ME: Unhandled map type "
             << get_struct_entry(cs, BPF_TYPE) << " for map "
             << g << "\nContinuing!\n";
      return false;
    }

    return true;
  }

  template <typename T>
  T* parse_cast(DINode* di, StringRef ty, GlobalVariable& g) {
    if( !isa<T>(di) ) {
      errs() << "ERROR: Expecting a " << ty << " for BTF map "
             << "definition of " << g << " but instead got: ";
      di->print(errs());
      errs() << "\nAborting!\n";
      abort();
    }
    return cast<T>(di);
  }

  DIType* strip_type_aliases(DIType* ty) {
    while( isa<DIDerivedType>(ty) ) {
      if( ty->getTag() != dwarf::DW_TAG_typedef )
        break;
      ty = cast<DIDerivedType>(ty)->getBaseType().resolve();
    }
    return ty;
  }

  size_t get_btf_type_size(DIType* di_ty, GlobalVariable& g) {
    di_ty = strip_type_aliases(di_ty);
    auto sz  = di_ty->getSizeInBits();

    if( (sz & 7) != 0 ) {
      errs() << "WARNING: Key / value type not byte-sized (" << sz
             << " bits) when parsing map " << g << ".  Rounding up!\n";
    }
    if( sz == 0 ) {
      errs() << "ERROR: Unsized key / value type for map " << g
             << "\nAborting!\n";
      abort();
    }
    return (sz + 7) / 8;
  }

  size_t get_btf_num_val(DIType* di_ty, GlobalVariable& g) {
    auto* di_arrty =
      parse_cast<DICompositeType>(di_ty, "DICompositeType", g);
    assert(di_arrty->getTag() == dwarf::DW_TAG_array_type);

    auto di_elems = di_arrty->getElements();
    assert(di_elems.size() == 1);

    auto* di_arrsz = parse_cast<DISubrange>(di_elems[0], "DISubrange", g);
    auto  sz = di_arrsz->getCount();
    if( !sz.is<ConstantInt*>() ) {
      errs() << "ERROR: Expecting a constant array size for ";
      di_arrty->print(errs());
      errs() << " in map " << g << "\nAborting!\n";
      abort();
    }
    return sz.get<ConstantInt*>()->getZExtValue();
  }

  bool parse_btf_ebpf_map_def(Module& m, LLVMContext& C, GlobalVariable& g,
                              nt_map_info* nt_entry) {
    /* BTF map descriptions encode the map characteristics through debug /
     * type info annotated to an anonymous struct with pointers to fields
     * of various types. */
    SmallVector<DIGlobalVariableExpression*, 4> debug_info;
    g.getDebugInfo(debug_info);
    bool have_key_sz, have_val_sz, have_type, have_max_entries, have_flags =
      false;
    for( auto* di : debug_info ) {
      auto* di_gv = di->getVariable();
      //auto  name  = di_gv->getName();
      auto* di_ty = di_gv->getType().resolve();

      auto* di_compty  = parse_cast<DICompositeType>(di_ty,
                           "DICompositeType", g);
      auto  di_elemtys = di_compty->getElements();

      for( auto* di_elemty : di_elemtys ) {
        /* The first DIDerivedType wraps the member-of-struct */
        auto* di_derivty = parse_cast<DIDerivedType>(di_elemty,
                             "DIDerivedType", g);
        assert( di_derivty->getTag() == dwarf::DW_TAG_member );
        auto n = di_derivty->getName();

        /* The second DIDerivedType shows that these are pointers to sth */
        auto* di_memberty = parse_cast<DIDerivedType>(
                            di_derivty->getBaseType().resolve(),
                            "DIDerivedType", g);
        assert( di_memberty->getTag() == dwarf::DW_TAG_pointer_type );
        auto* di_basety = di_memberty->getBaseType().resolve();

        /* The type of the struct member elements defines the property that
         * is indicated by their name:
         *  - type(anon.key) = "u32*"  means that the key is a u32
         *  - type(anon.max_entries) = "(int [42])*" means that
         *                                           max_entries = 42
         */
        if( n.equals("key") ) {
          /* The type of the key */
          auto sz = get_btf_type_size(di_basety, g);
          nt_entry->key_sz = sz;
          have_key_sz = true;

          LLVM_DEBUG(
            dbgs() << "Key: ";
            di_basety->print(dbgs());
            dbgs() << " Size: " << sz << '\n');
        } else if( n.equals("key_size") ) {
          auto sz = get_btf_num_val(di_basety, g);
          if( have_key_sz && (nt_entry->key_sz != sz) ) {
            errs() << "ERROR: Incompatible key sizes (" << nt_entry->key_sz
                   << " vs " << sz << ") for map " << g << "\nAborting!\n";
            abort();
          }
          nt_entry->key_sz = sz;
          have_key_sz = true;
        } else if( n.equals("value") ) {
          /* The type of the value stored in the map */
          auto sz = get_btf_type_size(di_basety, g);
          nt_entry->value_sz = sz;
          have_val_sz = true;

          LLVM_DEBUG(
            dbgs() << "Value: ";
            di_basety->print(dbgs());
            dbgs() << " Size: " << sz << '\n');
        } else if( n.equals("value_size") ) {
          auto sz = get_btf_num_val(di_basety, g);
          if( have_val_sz && (nt_entry->value_sz != sz) ) {
            errs() << "ERROR: Incompatible value sizes (" << nt_entry->value_sz
                   << " vs " << sz << ") for map " << g << "\nAborting!\n";
            abort();
          }
          nt_entry->value_sz = sz;
          have_val_sz = true;
        } else if( n.equals("map_flags") ) {
          auto flags = get_btf_num_val(di_basety, g);
          nt_entry->flags = flags;
          have_flags = true;
        } else if( n.equals("type") ) {
          /* The size of the pointed to array is the type enum */
          auto  ty_enum = get_btf_num_val(di_basety, g);
          have_type = true;
          nt_entry->type = ebpf_to_nt_map_type(ty_enum);
          if( nt_entry->type == NANOTUBE_MAP_TYPE_ILLEGAL ) {
            errs() << "IMPLEMENT ME: Unhandled map type "
                   << ty_enum << " for map " << g << "\nContinuing!\n";
            return false;
          }

          LLVM_DEBUG(
            dbgs() << "Type: ";
            di_basety->print(dbgs());
            dbgs() << " Value: " << ty_enum << '\n');

        } else if( n.equals("max_entries") ) {
          /* The size of the pointed to array is the maximum number of
           * entries */
          auto  max_entries = get_btf_num_val(di_basety, g);
          have_max_entries = true;

          LLVM_DEBUG(
            dbgs() << "Max_entries: ";
            di_basety->print(dbgs());
            dbgs() << " Value: " << max_entries << '\n');
        } else {
          /**
           * Currently known and unhandled fields:
           *   - numa_node
           *   - values
           *   - pinning
           *   - map_extra
           */
          errs() << "WARNING: Ignoring unknown BTF map defintion "
                 << "attribute " << n << " for variable " << g << '\n';
        }
      }
    }

    if( debug_info.empty() ) {
      errs() << "ERROR: Empty debug info for BTF-style map " << g
             << " but that is needed to parse the map defintion.\n"
             << "Aborting!\n";
      abort();
    }

    if( !have_key_sz || !have_val_sz || !have_type ) {
      errs() << "ERROR: BTF-style map definition for " << g
             << " is missing essential fields: "
             << "key_sz: " << have_key_sz
             << " val_sz: " << have_val_sz
             << " type: " << have_type
             << " max_entries: " << have_max_entries
             << " flags: " << have_flags
             << "\nAborting!\n";
      abort();
    }
    return true;
  }

  /*!
  ** Parse single EBPF map description.
  **
  ** @param m Module that is being examined.
  ** @param C LLVM context.
  ** @param g Global variable to parse.
  ** @param nt_entry Entry in map description table that will be populated (out).
  ** @return True if parsing was successful.
  **/
  bool parse_ebpf_map_def(Module& m, LLVMContext& C, GlobalVariable& g,
                          nt_map_info* nt_entry) {
    if( is_classic_ebpf_map(g) )
      return parse_classic_ebpf_map_def(m, C, g, nt_entry);
    else if( is_btf_ebpf_map(g) )
      return parse_btf_ebpf_map_def(m, C, g, nt_entry);

    errs() << "ERROR: Map definition " << g << " is neither classic nor "
           << " BTF.  Aborting!\n";
    abort();
  }

  std::pair<bool,unsigned> get_int_casted_to_pointer(Constant* c) {
    if( c == nullptr ) {
      //errs() << "Null pointer!\n";
      return std::make_pair(false, 0);
    }

    auto* cexp = dyn_cast<ConstantExpr>(c);
    if( cexp == nullptr ) {
      //errs() << "Non-ConstantExpr in " << *c <<'\n';
      return std::make_pair(false, 0);
    }

    if( cexp->getOpcode() != Instruction::IntToPtr ) {
      //errs() << "Non-inttoptr " << cexp->getOpcode() <<  " for "
      //       << *c <<'\n';
      return std::make_pair(false, 0);
    }

    Value* v = cexp->getOperand(0);
    auto* ci = dyn_cast<ConstantInt>(v);
    if( ci == nullptr ) {
      //errs() << "Non-constant inttoptr argument " << *v << " for "
      //       << *c << '\n';
      return std::make_pair(false, 0);
    }

    unsigned num = ci->getValue().getLimitedValue();
    return std::make_pair(true, num);
  }

  Function *setup_func;

  /*!
  ** Convert all EBPF map description globals to Nanotube.
  **
  ** The conversion takes EBPF map defintions in global variables and
  ** converts them into Nanotube map_create calls.  And will also create a
  ** struct that contains pointers to all the maps.
  **
  ** @param m Module that is examined.
  ** @param name Name of the function that calls will perform the
  **             map_creates.
  ** @return True if there were changes.
  **/
  bool convert_ebpf_map_defs(Module& m) {
    LLVMContext& C = m.getContext();
    /* We need to do three things here:
     * (1) parse existing EBPF map descriptions
     * (2) create a struct / array that holds those created maps
     * (3) create a function XXX for the call below
     * (4) create calls to map_create
     * (5) put the created map pointers into a structure / array,
     *     remembering which map went where
     * (6) convert the EBPF kernel so that it accepts the map array /
     *     struct
     *
     * NOT HERE:
     * (7) create code that reads out the map pointers at the beginning of
     *     the kernel and puts them into named variables
     * (8) convert the map_lookups themselves
     */

    /* Review all globals and parse all EBPF maps into a vector, and also
     * into a lookup table that maps old EBPF varaible to the parsed map. */
    unsigned pos = 0;
    for( auto& g : m.globals() ) {
      if( !is_ebpf_global(g) )
        continue;

      nt_maps.resize(pos + 1);
      auto& nt_map = nt_maps[pos];
      bool success = parse_ebpf_map_def(m, m.getContext(), g, &nt_map);
      if( !success ) {
        nt_maps.resize(pos);
        continue;
      }

      /* Remember the translation */
      ebpf2nt_map.emplace(&g, pos);

      /* Create a nice global constant holding the map ID. */
      auto* map_id = ConstantInt::get(get_nt_map_id_type(m), pos);
      auto* map_id_gv = new GlobalVariable(m, get_nt_map_id_type(m),
                           true /*constant*/,
                           GlobalVariable::InternalLinkage,
                           map_id,
                           g.getName() + "_id");
      nt_map.global_id = map_id_gv;
      pos++;
    }

    /* Create function */
    auto* ret_ty = Type::getVoidTy(C);
    setup_func = Function::Create(FunctionType::get(ret_ty, false),
                                  Function::ExternalLinkage /* XXX */,
                                  "nanotube_setup", m);

    auto* bb = BasicBlock::Create(C, "", setup_func);
    IRBuilder<> ir(bb);
    /* Create a single context */
    auto* cr_ctx_ty = get_nt_context_create_ty(m);
    auto* cr_ctx_f  = create_nt_context_create(m);
    auto* ctx       = ir.CreateCall(cr_ctx_ty, cr_ctx_f, None, "context");

    /* Create maps and add them to context */
    auto* map_create = cast<Function>(create_nt_map_create(m));
    auto* map_add_ty = get_nt_context_add_map_ty(m);
    auto* map_add_f  = create_nt_context_add_map(m);

    for( auto& nt_map : nt_maps ) {
      auto* id = ir.CreateLoad(nt_map.global_id,
                               nt_map.global_id->getName() + "_");
      std::array<Value*,4> args = {
        id,
        ir.getInt32(nt_map.type),
        ir.getInt64(nt_map.key_sz),
        ir.getInt64(nt_map.value_sz)
      };

      CallInst* call = ir.CreateCall(map_create, args);
      assert( call != nullptr );
      /* Add to context */
      std::array<Value*,2> add_args = {ctx, call};
      ir.CreateCall(map_add_ty, map_add_f, add_args);
    }

    ir.CreateRetVoid();
    return true;
  }

  void remove_from_llvm_used(Module& m,  ArrayRef<GlobalValue*> vs) {
    /* Remove variables from the llvm.used meta-data! */
    /* (1) get the existing llvm.used variable and its initialiser
     *     (ConstantArray) */
    GlobalVariable* llvm_used = m.getGlobalVariable("llvm.used");
    if( llvm_used == nullptr)
      return;

    SmallSet<GlobalValue*, 16>    to_remove;
    SmallVector<GlobalValue*, 16> used_list;
    for( auto* v : vs )
      to_remove.insert(v);

    auto* ca = cast<ConstantArray>(llvm_used->getInitializer());
    if( ca != nullptr ) {
       /* (2) go through the entries, and build up a new initialiser list
        *     not adding those entries that are in ebpf2nt_map and have
        *     exactly one use */
      for( auto& op : ca->operands() ) {
        auto* ce = dyn_cast<ConstantExpr>(op);
        GlobalValue* used =
          (ce != nullptr) ? cast<GlobalValue>(ce->getOperand(0))
                          : cast<GlobalValue>(ce);
        if( to_remove.count(used) == 0 )
          used_list.push_back(used);
      }
    }

    /* (3) delete the old llvm.used */
    llvm_used->eraseFromParent();

    /* (4) do a simple appendToUsed list call with the new list :) */
    if( !used_list.empty() )
      appendToUsed(m, used_list);
  }
  void remove_from_llvm_used(Module& m, GlobalValue* v) {
    GlobalValue* vs[] = { v };
    remove_from_llvm_used(m, vs);
  }

  void erase_unsused_globals(Module& m, ArrayRef<GlobalValue*> gvs) {
    for( auto* gv : gvs ) {
      gv->removeDeadConstantUsers();
      if( gv->getNumUses() == 0 ) {
        gv->eraseFromParent();
        continue;
      }

      /* Those with uses get an honourable mention ;) */
      errs() << "EBPF variable " << *gv << " still has uses:\n";
      for( auto* u : gv->users() ) {
        if( u != nullptr )
          errs() << "  " << *u << '\n';
        else
          errs() << "  <nullptr>\n";
      }
      errs() << "Not deleting it.\n";
    }
  }

  void remove_ebpf_globals(Module& m) {
    SmallVector<GlobalValue*, 16> to_remove_maps;
    SmallVector<GlobalValue*, 16> to_remove_funcs;
    /* Spring cleaning step 1: make a list of things to throw out */
    for (auto& gref : m.globals() ) {
      auto* g = &gref;
      /* Remove EBPF maps that we converted */
      if( ebpf2nt_map.find(g) != ebpf2nt_map.end() )
        to_remove_maps.push_back(g);
    }
    for (auto& f : m.functions() ) {
      if( is_packet_kernel(m.getContext(), f))
        to_remove_funcs.push_back(&f);
    }

    /* Spring cleaning step 2: remove sticky llvm.used tape */
    remove_from_llvm_used(m, to_remove_funcs);
    remove_from_llvm_used(m, to_remove_maps);

    /* Spring cleaning step 3: remove unused items */
    erase_unsused_globals(m, to_remove_funcs);
    erase_unsused_globals(m, to_remove_maps);
  }

  /* Map EBPF map definitions to their Nanotube equivalents */
  SmallVector<nt_map_info, 10>        nt_maps;
  std::map<GlobalVariable*, unsigned> ebpf2nt_map;


  std::string spaces(unsigned depth) {
    std::string s;
    s.append(depth, ' ');
    return s;
  }
  void print_data_flow_tree(Value* v, unsigned depth = 0) {
    errs() << spaces(2 * depth) << "| " << *v << '\n';
    User* u = dyn_cast<User>(v);
    if( u == nullptr)
      return;
    for( Value* user : u->operand_values() )
      print_data_flow_tree(user, depth + 1);
  }

  GlobalVariable* ebpf_get_map_arg(CallInst* call) {
    Value* arg = call->getArgOperand(0);
    if( arg == nullptr ) {
      errs() << "WARNING: Did not get map arg from " << *call << '\n';
      return nullptr;
    }

    auto* cexp = dyn_cast<ConstantExpr>(arg);
    if( cexp == nullptr ) {
      errs() << "Non-ConstantExpr in " << *arg <<'\n';
      print_data_flow_tree(arg);
      return nullptr;
    }

    if( cexp->getOpcode() != Instruction::BitCast ) {
      errs() << "Non-bitcast " << cexp->getOpcode() <<  " for "
             << *arg <<'\n';
      return nullptr;
    }

    Value* v = cexp->getOperand(0);
    GlobalVariable* gv = dyn_cast<GlobalVariable>(v);
    if( gv == nullptr ) {
      errs() << "Non-global variable bitcast argument " << *v << " for "
             << *arg << '\n';
      return nullptr;
    }
    return gv;
  }

  bool get_nt_map_idx(GlobalVariable* gv, unsigned* idx) {
    auto res = ebpf2nt_map.find(gv);
    if( res == ebpf2nt_map.end() ) {
      errs() << "Did not find map defintion " << *gv << " in translation table.\n";
      return false;
    }
    *idx = res->second;
    return true;
  }

  ConstantInt* get_map_key_sz(LLVMContext& C, CallInst* call) {
    GlobalVariable* gv = ebpf_get_map_arg(call);
    unsigned idx;
    if( (gv != nullptr) && get_nt_map_idx(gv, &idx) )
      return get_const_int64(C, nt_maps[idx].key_sz);
    return nullptr;
  }

  ConstantInt* get_map_data_sz(LLVMContext& C, CallInst* call) {
    GlobalVariable* gv = ebpf_get_map_arg(call);
    unsigned idx;
    if( (gv != nullptr) && get_nt_map_idx(gv, &idx) )
      return get_const_int64(C, nt_maps[idx].value_sz);
    return nullptr;
  }

  Value* get_map_id(LLVMContext& C, CallInst* call) {
    GlobalVariable* gv = ebpf_get_map_arg(call);
    unsigned idx;
    /* Return the map pulled out from the Nanotube context */
    if( (gv != nullptr) && get_nt_map_idx(gv, &idx) )
      return nt_maps[idx].local_val;
    return nullptr;
  }

  Value* ebpf_get_map_key(LLVMContext& C, CallInst* call) {
    return call->getArgOperand(1);
  }

  GlobalValue* get_all_one_bitmask(Module* m, unsigned bytes_to_cover) {
    LLVMContext& C = m->getContext();
    unsigned width = (bytes_to_cover + 7) / 8;
    std::string name = "onemask" + std::to_string(width);
    auto* gv = m->getNamedValue(name);

    if( gv != nullptr )
      return gv;

    auto* one_type = ArrayType::get(Type::getInt8Ty(C), width);
    SmallVector<uint8_t, 64> ones(width, 0xff);

    auto* one_mask = ConstantDataArray::get(C, ones);

    gv = new GlobalVariable(*m, one_type, true,
        GlobalVariable::InternalLinkage, one_mask, name);
    return gv;
  }

  /*!
   * Convert an EBPF map_lookup call to its Nanotube API equivalent.
   * @param M Module where the call resides.
   * @param call Call instruction that will be convertedt.
   */
  CallInst* convert_to_nt_map_lookup(Module* M, CallInst* call) {
    LLVMContext& C = M->getContext();

    /**
      * uint8_t* nanotube_map_lookup(nanotube_context_t* ctx,
      *                              nanotube_map_id_t map, const uint8_t* key,
      *                              size_t key_length, size_t data_length);
     */
    auto* nt_ctx = call->getFunction()->arg_begin();
    Value* args[] = {nt_ctx, get_map_id(C, call),
                     ebpf_get_map_key(C, call), get_map_key_sz(C, call),
                     get_map_data_sz(C, call)};

    /* Make sure that all values could be obtained */
    for( auto* a : args ) {
      if( a != nullptr )
        continue;
      errs() << "ERROR: Could not convert " << *call
             << "\nAborting!\n";
      abort();
    }

    Constant* ml = create_nt_map_lookup(*M);
    //errs() << *ml << " " << *call << '\n';

    IRBuilder<> ir(call);
    auto* map_call = ir.CreateCall(ml, args, "map_lookup");
    //errs() << *map_call << '\n';
    return map_call;
  }

  CallInst* convert_to_nt_map_update(Module* M, CallInst* call) {
    LLVMContext& C = M->getContext();

    /**
     * int32_t map_op_adapter(nanotube_context_t* ctx, nanotube_map_id_t map_id,
     *                        enum map_access_t type, const uint8_t* key,
     *                        size_t key_length, const uint8_t* data_in,
     *                        uint8_t* data_out, const uint8_t* mask,
     *                        size_t offset, size_t data_length);
     */
    auto* data_sz  = get_map_data_sz(C, call);
    if( data_sz == nullptr ) {
      errs() << "ERROR: Could not parse map description in call\n"
             << *call
             << "\nAborting!\n";
      abort();
    }
    unsigned count = data_sz->getZExtValue();
    auto* null_ptr = Constant::getNullValue(Type::getInt8PtrTy(C));
    auto* null_u64 = Constant::getNullValue(Type::getInt64Ty(C));
    auto* all_one  = get_all_one_bitmask(M, count);
    auto* one_u8p  = ConstantExpr::getBitCast(all_one,
                                              Type::getInt8PtrTy(C));

    unsigned nt_op;
    unsigned ebpf_flag = cast<ConstantInt>(call->getOperand(3))->getZExtValue();
    switch( ebpf_flag ) {
      case 0: /* BPF_ANY */
        nt_op = NANOTUBE_MAP_WRITE; break;
      case 1: /* BPF_NOEXIST */
        nt_op = NANOTUBE_MAP_INSERT; break;
      case 2: /* BPF_EXIST */
        nt_op = NANOTUBE_MAP_UPDATE; break;
      default: assert(false);
    }
    auto* nt_op_arg = ConstantInt::get(Type::getInt32Ty(C), nt_op);
    auto* nt_ctx    = call->getFunction()->arg_begin();

    Value* args[] = {nt_ctx, get_map_id(C, call), nt_op_arg,
                     call->getOperand(1), get_map_key_sz(C, call),
                     call->getOperand(2), null_ptr,
                     one_u8p, null_u64,
                     get_map_data_sz(C, call)};

    /* Make sure that all values could be obtained */
    for( auto* a : args ) {
      if( a != nullptr )
        continue;
      errs() << "ERROR: Could not convert " << *call
             << "\nAborting!\n";
      abort();
      return nullptr;
    }

    Constant* mop = create_map_op_adapter(*M);
    LLVM_DEBUG(
      dbgs() << "Type of args: ";
      for( unsigned i = 0; i < 9; ++i )
        dbgs() << *args[i]->getType() << ", ";
      dbgs() << '\n';

      dbgs() << "Function Type: " << *mop->getType() << '\n';
      //dbgs() << *ml << " " << *call << '\n';
    );

    IRBuilder<> ir(call);
    auto* map_call = ir.CreateCall(mop, args, "map_op");
    //errs() << *map_call << '\n';
    return map_call;
  }

  CallInst* convert_to_nt_get_time_ns(Module* M, CallInst* call) {
    Constant* gtns = create_nt_get_time_ns(*M);
    IRBuilder<> ir(call);
    auto* nt_time_call = ir.CreateCall(gtns, None, "time_ns");
    return nt_time_call;
  }

  /*!
  ** Convert low-level Nanotube functions (EBPF) into Nanotube API L1
  ** @param F Function that is being traversed for EBPF map lookups.
  ** @return True if changes were made.
  **/
  bool convert_ebpf_map_calls(Function& F) {
    bool made_changes = false;
    for (auto &bbl : F) {
      for (auto ii = bbl.begin(); ii != bbl.end(); ++ii) {
        Instruction* ip = &(*ii);
        if( !isa<CallInst>(ip) )
          continue;
        auto* c = cast<CallInst>(ip);

        Value* conv = nullptr;

        switch( get_ebpf_call_id(c) ) {
          case BPF_MAP_LOOKUP:
            conv = convert_to_nt_map_lookup(F.getParent(), c);
            break;
          case BPF_MAP_UPDATE:
            conv = convert_to_nt_map_update(F.getParent(), c);
            break;
          case BPF_KTIME_GET_NS:
            conv = convert_to_nt_get_time_ns(F.getParent(), c);
            break;
          default:
            continue;
        }

        if( conv == nullptr )
          continue;

        made_changes = true;
        c->replaceAllUsesWith(conv);
        ii = c->eraseFromParent(); --ii;
      }
    }
    return made_changes;
  }

  bool is_packet_kernel(LLVMContext& C, const Function& f) {
    PointerType* pty;
    StructType* sty;

    return ( f.getReturnType() == Type::getInt32Ty(C) ) &&
             f.arg_size() == 1 &&
           ( (pty = dyn_cast<PointerType>(f.arg_begin()->getType())) != nullptr ) &&
           ( (sty = dyn_cast<StructType>(pty->getElementType())) != nullptr ) &&
             sty->hasName() &&
             sty->getName().equals("struct.xdp_md");
  }

  FunctionType* get_nt_packet_kernel_type(Module& m, unsigned num_maps) {
    LLVMContext& C = m.getContext();

    /**
     * int packet_kernel(nanotube_context_t* ctx,
     *                    nanotube_packet_t* * packet);
     */
    auto* nt_ctx_ty = get_nt_context_type(m);
    auto* nt_pkt_ty = get_nt_packet_type(m);
    Type* arg_tys[] = {nt_ctx_ty->getPointerTo(), nt_pkt_ty->getPointerTo()};
    return FunctionType::get(Type::getInt32Ty(C), arg_tys, false);
  }

  typedef std::unordered_map<Value*, int64_t> v2offs_ty;
  void convert_context_load(Module* m, LoadInst* ld, int64_t offset,
                            Value* nt_ctx, Value* nt_packet);
  void convert_context_call(Module* m, CallInst* call,
                            const v2offs_ty& v2offs,
                            Value* nt_ctx, Value* nt_packet);

  Function* convert_kernel_params(Function& f);

  bool register_kernel(Function *func, StringRef name)
  {
    Module *module = func->getParent();
    BasicBlock *bb = &(setup_func->getEntryBlock());
    Instruction *term = bb->getTerminator();
    Constant *add_kernel = create_nt_add_plain_packet_kernel(*module);
    auto ir = IRBuilder<>(term);
    GlobalValue *name_var = ir.CreateGlobalString(name);
    Type *uint8_type = ir.getInt8PtrTy();
    Value *name_ptr = ir.CreateBitCast(name_var, uint8_type);
    /* If setting capsules=1 (4th arg) here you'd need to set bus_type (3rd arg)
     * to get_bus_type() */
    Value* args[] = { name_ptr, func, ir.getInt32(-1), ir.getInt32(0)};
    ir.CreateCall(add_kernel, args);
    return true;
  }

  void check_all_calls(Function* f) {
    for( auto& inst : instructions(f) ) {
      if( !isa<CallInst>(inst) )
        continue;
      auto& call = cast<CallInst>(inst);
      auto* callee = call.getCalledFunction();
      if( callee != nullptr )
        continue;
      errs() << "ERROR: Unresolved calls remain after eBPF -> Nanotube "
             << "conversion:\n" << call
             << "\nContained in BB:" << *call.getParent()
             << "\nAborting!\n";
      abort();
    }
  }

  bool runOnModule(Module& m) override {
    const DataLayout &dl = m.getDataLayout();
    if (!dl.isLittleEndian()) {
      report_fatal_errorv("Big-endian machines are not currently supported.");
    }
    LLVMContext& C = m.getContext();
    bool made_changes = false;
    made_changes |= convert_ebpf_map_defs(m);
    get_nt_context_type(m);

    SmallVector<Function*, 4> converted_kernels;
    for( auto& f : m ) {
      if( !is_packet_kernel(C, f) )
        continue;
      Function* f_new = convert_kernel_params(f);
      made_changes |= convert_ebpf_map_calls(*f_new);
      made_changes |= register_kernel(f_new, f.getName());

      check_all_calls(f_new);
    }

    remove_ebpf_globals(m);

    return made_changes;
  }
};
} // anonymous namespace

static
void cleanup_graph(std::vector<Value*>& cleanup, Value* stop = nullptr) {
  /* Clean-up unused pieces and start at the leaves */
  while( !cleanup.empty() ) {
    auto* v = cleanup.back();
    cleanup.pop_back();
    if( v == stop )
      continue;
    if( !isa<Instruction>(v) )
      continue;
    auto* inst = cast<Instruction>(v);

    /* Ignore instructions that are still used */
    if( !inst->use_empty() ) {
      LLVM_DEBUG(
        dbgs() << "Inst " << *inst << " still has users:\n";
        for( auto* u : inst->users() )
          dbgs() << "  " << *u << '\n';
      );
      continue;
    }

    /* Erase this instruction and check all its operands for deadness */
    LLVM_DEBUG(dbgs() << "Removing " << *inst << '\n');
    for( auto* v : inst->operand_values() )
      if( isa<Instruction>(v) ) {
        LLVM_DEBUG(dbgs() << "  Checking " << *v << '\n');
        cleanup.emplace_back(v);
      }
    inst->eraseFromParent();
  }
}

Function* ebpf_to_NT::convert_kernel_params(Function& f) {
  Module&      m = *f.getParent();
  LLVMContext& C = m.getContext();

  /* Create a new function type with the context argument replaced */
  auto* f_new_ty  = get_nt_packet_kernel_type(m, nt_maps.size());
  auto* f_new     = Function::Create(f_new_ty, Function::ExternalLinkage,
                                     f.getName() + "_nt", m);
  Argument* ctx_arg = f_new->arg_begin();
  ctx_arg->setName("nt_ctx");
  Argument* pkt_arg = f_new->arg_begin() + 1;
  pkt_arg->setName("packet");

  auto* bb        = BasicBlock::Create(C, "", f_new);
  IRBuilder<> ir(bb);

  /* Get access to the packet (pointer) */
  auto* nt_packet   = pkt_arg;

  /* and FOR THE MOMENT!11!!!!!1!! cheat and cast to an XDP Context */
  auto* xdp_ctx_ty = m.getTypeByName("struct.xdp_md")->getPointerTo();
  auto* xdp_ctx    = ir.CreateBitCast(nt_packet, xdp_ctx_ty, "xdp_ctx");

  for( unsigned i = 0; i < nt_maps.size(); ++i ) {
    /* Create code to read each map pointer */
    auto* map_id  = ir.CreateLoad(nt_maps[i].global_id,
                                  nt_maps[i].global_id->getName() + "_");
    /* Update our internal tracking structure so that later
     * transormation passes can find this */
    nt_maps[i].local_val = map_id;
  }
  /* Use CloneFunctionInto to copy over the rest of the function and
   * replace references to the old context with the new value */
  ValueToValueMapTy vmap;
  vmap[f.arg_begin()] = xdp_ctx;
  SmallVector<ReturnInst*, 4> rets;
  CloneFunctionInto(f_new, &f, vmap, true, rets);

  /* Join the copied entry block and what we generated here */
  auto* entry_moved = cast<BasicBlock>(vmap[&f.getEntryBlock()]);
  ir.CreateBr(entry_moved);
  /* And then physically merge them together */
  MergeBlockIntoPredecessor(entry_moved);

  /* Follow all data flow from xdp_ctx, track computations, convert
   * leaves */
  std::vector<Value*> todo;
  v2offs_ty val_to_offset;
  todo.emplace_back(xdp_ctx);
  std::vector<Value*> cleanup;
  while( !todo.empty() ) {
    auto* v = todo.back();
    unsigned offset = 0;
    todo.pop_back();
    LLVM_DEBUG(dbgs() << "Tracing value " << *v << '\n');

    /* BitCasts don't change the offset and don't need conversion */
    if( isa<BitCastInst>(v) )
      goto cont;

    /* GEPs compute the constant offset and track it */
    if( isa<GetElementPtrInst>(v) ) {
      auto* gep = cast<GetElementPtrInst>(v);
      APInt gep_offs(64, 0, true);
      bool success = gep->accumulateConstantOffset(m.getDataLayout(),
                                                   gep_offs);
      if( !success ) {
        errs() << "ERROR: GEP " << *gep << " must have constant offset."
               << "\nAborting!\n";
        abort();
      }
      auto it = val_to_offset.find(gep->getPointerOperand());
      assert(it != val_to_offset.end());
      offset = it->second + gep_offs.getSExtValue();
      LLVM_DEBUG(dbgs() << "Parsing GEP " << *gep << " offset: " << offset
                        << '\n');
      goto cont;
    }

    /* Loads from the context need manual conversion */
    if( isa<LoadInst>(v) ) {
      auto* ld = cast<LoadInst>(v);
      auto  it = val_to_offset.find(ld->getPointerOperand());
      assert(it != val_to_offset.end());
      LLVM_DEBUG(dbgs() << "Load " << *ld << " from context with offset "
                        << it->second << '\n');
      convert_context_load(&m, ld, it->second, ctx_arg, pkt_arg);
      cleanup.emplace_back(ld);
      continue;
    }

    /* Calls that use the context need manual conversion */
    if( isa<CallInst>(v) ) {
      auto* call = cast<CallInst>(v);
      LLVM_DEBUG(dbgs() << "Call " << *call << " using context.\n");
      convert_context_call(&m, call, val_to_offset, ctx_arg, pkt_arg);
      cleanup.emplace_back(call);
      continue;
    }

    cont:
    val_to_offset.emplace(v, offset);
    for( auto* u : v->users() )
      todo.emplace_back(u);
  }
  cleanup_graph(cleanup, xdp_ctx);

  /* And delete the now useless xdp_ctx.  Hooray! */
  if( xdp_ctx->use_empty() ) {
    cast<Instruction>(xdp_ctx)->eraseFromParent();
  } else {
    errs() << "ERROR: Incomplete XDP -> NT conversion, xdp_ctx has users:\n";
    for( auto* u : xdp_ctx->users() )
      errs() << "  " << *u << '\n';
    errs() << "Aborting!\n";
    abort();
  }

  /* Replace return statements XDP_{DROP,PASS,...} with a call to a
   * helper function that will set the packet port and a plain return */
  auto* conv_func = create_packet_handle_xdp_result(m);
  for( BasicBlock& bb : *f_new ) {
    for( auto ii = bb.begin(); ii != bb.end(); ) {
      auto& inst = *(ii++);
      auto* ret = dyn_cast<ReturnInst>(&inst);
      if( ret == nullptr )
        continue;

      ir.SetInsertPoint(ret);
      auto* rv = ret->getReturnValue();
      assert( rv != nullptr );
      Value* args[] = {nt_packet, rv};
      auto* call    = ir.CreateCall(conv_func, args);
      auto* ret_new = ir.CreateRet(call);
      LLVM_DEBUG(dbgs() << "Translating return " << *ret << '\n'
                        << "into " << *call << *ret_new << '\n');
      ret->eraseFromParent();
    }
  }

  return f_new;
}


void
ebpf_to_NT::convert_context_load(Module* m, LoadInst* ld, int64_t offset,
                                 Value* nt_ctx, Value* nt_packet) {
  /**
   * We have a load that reads data from an offset in the context; we must
   * then track down the users of the loaded value and convert them:
   *
   * %ld = load (ctx + offset)
   * %foo = XYZ %ld
   * %bar = ABC %ld
   *
   * There are two interesting offsets in ctx:
   * 0 .. packet data start
   * 4 .. packet data end
   * 8 .. meta data start
   * so we trace the users of %ld all the way to when they are either
   * (1) creating a pointer to access packet data, or
   * (2) performing integer arithmetic.
   *
   * Note that the meta data end is implicitly the packet start.
   */

  if( offset != 0 && offset != 4 && offset != 8 ) {
    errs() << "ERROR: Unknown xdp_ctx offset " << offset
           << " in load " << *ld << "; not converting!\n";
    errs() << "Aborting!\n";
    abort();
  }

  std::vector<Use*> todo;
  std::vector<Use*> int_targets;
  std::vector<Use*> ptr_targets;
  for( auto& u : ld->uses() )
    todo.emplace_back(&u);

  while( !todo.empty() ) {
    auto* u = todo.back();
    auto* v = u->getUser();
    todo.pop_back();
    LLVM_DEBUG(dbgs() << "Tracing " << *v << " read from offset " << offset
                      << '\n');
    if( isa<ZExtInst>(v) ) {
      /* Fall-through and follow flow */
    } else if( isa<IntToPtrInst>(v) ) {
      /* Pointer users will be fed the correct packet pointer */
      //auto* i2p = cast<IntToPtrInst>(v);
      ptr_targets.emplace_back(u);
      continue;
    } else if( isa<BinaryOperator>(v) ) {
      /* Integer users will be fed ptr2int packet pointers. */
      //auto* bo = cast<BinaryOperator>(v);
      int_targets.emplace_back(u);
      continue;
    } else {
      errs() << "IMPLEMENT ME: Unknown user " << *v
             << " of context offset " << offset
             << "\nAborting!\n";
      abort();
    }
    /* Continue following the data flow */
    for( auto& u : v->uses() )
      todo.emplace_back(&u);
  }

  IRBuilder<> ir(ld);
  Constant*   nt_packet_func;
  StringRef   name;

  /* Call the respective Nanotube function */
  switch( offset ) {
    case 0:
      nt_packet_func = create_nt_packet_data(*m);
      name = "packet_data"; break;
    case 4:
      nt_packet_func = create_nt_packet_end(*m);
      name = "packet_end"; break;
    case 8:
      nt_packet_func = create_nt_packet_meta(*m);
      name = "packet_meta"; break;
    default:
      assert(false);
  }
  Value* args[] = { nt_packet };
  auto* packet_data = ir.CreateCall(nt_packet_func, args, name);

  std::vector<Value*> cleanup;

  /* Patch up pointer users, using bitcasts if necessary */
  for( auto* u : ptr_targets ) {
    auto* tgt = u->getUser();
    LLVM_DEBUG(dbgs() << "Ptr Target: Val: " << *u->get() << " User: "
                      << *u->getUser() << '\n');

    if( tgt->getType() == packet_data->getType() ) {
      /* Same pointer, just replace */
      LLVM_DEBUG(dbgs() << "Replacing\n" << *tgt << "\nwith\n"
                        << *packet_data << '\n');
      tgt->replaceAllUsesWith(packet_data);
      cleanup.emplace_back(tgt);
    } else {
      /* The pointers are different types.. we need to cast. */
      auto* bc = new BitCastInst(packet_data, tgt->getType(), name + ".c");
      LLVM_DEBUG(dbgs() << "Replacing\n" << *tgt << "\nwith\n" << *bc
                        << '\n');
      tgt->replaceAllUsesWith(bc);
      cleanup.emplace_back(tgt);

      /* If the user is an instruction, place the BC next to it and then
       * delete the original instruction */
      auto* inst = dyn_cast<Instruction>(tgt);
      if( inst != nullptr ) {
        bc->insertBefore(inst);
      } else {
        /* Otherwise, please the BC close to the call to nt_packet_* */
        ir.Insert(bc);
      }
    }
  }

  /* Patch up integer type users of the packet start / end */
  std::unordered_set<Value*> done;
  for( auto* u : int_targets ) {
    auto* val = u->get();
    /* We convert all uses of a single value in one go so skip additional
     * users! */
    if( done.count(val) != 0 )
      continue;
    done.insert(val);
    auto* user = u->getUser();
    LLVM_DEBUG(dbgs() << "Int Target: Val: " << *val << " User: " << *user
                      << '\n');
    auto* pdi = ir.CreatePtrToInt(packet_data, val->getType(), name + ".i");
    LLVM_DEBUG(dbgs() << "Replacing\n" << *val << "\nwith\n" << *pdi
                      << "\neverywhere!\n");
    val->replaceAllUsesWith(pdi);
    cleanup.emplace_back(val);
  }

  cleanup_graph(cleanup, ld);
}

void
ebpf_to_NT::convert_context_call(Module* m, CallInst* call,
                                 const ebpf_to_NT::v2offs_ty& v2offs,
                                 Value* nt_ctx, Value* nt_packet) {
  Function* repl_f = nullptr;
  std::vector<Value*> args;
  auto id = get_ebpf_call_id(call);
  switch( id ) {
    case BPF_XDP_ADJUST_HEAD: {
      auto* ctx = call->getOperand(0);
      auto it = v2offs.find(ctx);
      if( (it == v2offs.end()) || (it->second != 0) ) {
        /* Check that this is a plain ebpf ctx! */
        errs() << "ERROR: Unknown use of EBPF ctx " << *ctx << " in "
               << *call << "\nABorting!\n";
        abort();
      }
      repl_f = cast<Function>(create_packet_adjust_head_adapter(*m));
      args.emplace_back(nt_packet);
      args.emplace_back(call->getOperand(1));
      break; }
    case BPF_XDP_ADJUST_META: {
      auto* ctx = call->getOperand(0);
      auto it = v2offs.find(ctx);
      if( (it == v2offs.end()) || (it->second != 0) ) {
        /* Check that this is a plain ebpf ctx! */
        errs() << "ERROR: Unknown use of EBPF ctx " << *ctx << " in "
               << *call << "\nABorting!\n";
        abort();
      }
      repl_f = cast<Function>(create_packet_adjust_meta_adapter(*m));
      args.emplace_back(nt_packet);
      args.emplace_back(call->getOperand(1));
      break; }
    default:
      errs() << "ERROR: Unhandled call " << *call << " to EBPF function "
             << id << "\nAborting!\n";
      abort();
  }

  /* Call the equivalent NT function */
  assert(repl_f != nullptr);
  IRBuilder<> ir(call);
  auto* new_call = ir.CreateCall(repl_f , args, "");
  LLVM_DEBUG(dbgs() << "Converting " << *call << " to " << *new_call
                    << '\n');
  auto* conv = ir.CreateSExtOrTrunc(new_call, call->getType());
  call->replaceAllUsesWith(conv);
}

char ebpf_to_NT::ID = 0;
static RegisterPass<ebpf_to_NT>
    X("ebpf2nanotube", "Convert EBPF API to Nanotube L1 API.",
      false,
      false
      );

/* vim: set ts=8 et sw=2 sts=2 tw=75: */
