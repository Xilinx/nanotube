/**************************************************************************\
*//*! \file thread_const.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube thread constant propagation.
**   \date  2021-08-19
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

// Thread constant propagation
// ===========================
//
// The thread constant propagation pass identifies memory locations
// which are constant and propagates the values into thread functions.
//
// Input conditions
// ----------------
//
// The setup function is valid according to the setup_func class.  The
// program only uses threads and channels.  Each memory location is
// read-only or accessed by only a single thread.  Pointers in memory
// are not modified.  Pointer arithmetic does not cross boundaries
// between memory allocations.
//
// Output conditions
// -----------------
//
// The input conditions are preserved.  Values which are loaded from
// memory and can be identified as constant are replaced with their
// constant values.
//
// Theory of operation
// -------------------
//
// The pass operates in three phases.  The first phase determines the
// possible values of each pointer and works out which memory
// locations are known to be constant.  The second phase locates load
// instructions which can be replaced with constant values and adds
// them to a map.  The third phase uses the LLVM function cloning
// machinery to create a new copy of the function with the load
// instructions replaced with their constant values.
//
// The first phase works by iterating over the program, building up
// possible values of each pointer and identifying memory locations
// which could be written.  Pointers are produced by bitcast,
// getelementptr, load and phi instructions.  Initial pointers are
// global variables.  For each thread function, it iterates over the
// instructions and updates the set of possible values of each pointer
// producing instruction it encounters.  This process is repeated
// until no set is updated.
//
// The instructions are processed as follows:
//
//   bitcast: The set of pointers produced by the instruction is the
//   same as the set of pointers of the operand.
//
//   call: If the call is not an intrinsic then the pass fails.  If
//   the function does not modify memory then it is ignored.  Each
//   argument is probed to determine whether it points to memory which
//   is written by the call.  If it does then the size of the memory
//   is determined and each possible region is marked as written.
//   Calls which return pointer values are not supported.
//
//   getelementptr: Sets of pointers are created for each of the
//   intermediate results.  This allows offsets to be handled one at a
//   time.  If an offset is constant then each candidate pointer is
//   offset by required amount and inserted into the new set if it is
//   within the same allocation.  If a resulting pointer is outside
//   the allocation of the candidate pointer and the "inbounds" flag
//   is clear then the result set is the all-values set.  If the
//   offset is not constant then propagate the set bits up and down by
//   multiples of the type size up to the edge of the allocation.  If
//   the "inbounds" flag is clear then the result set is the
//   all-values set.
//
//   load: Each possible value of the pointer operand which is part of
//   an allocation is dereferenced.  If all the bytes are either
//   unknown or written then the all-values set if returned.  If all
//   the known read-only bytes come from a single pointer value then
//   the pointer value is added to the result set.  Otherwise the
//   pointer value is ignored.
//
//   phi: The set of pointers produced by the instruction is the union
//   of the sets associated with the operands.
//
//   store: For each possible value of the pointer operand, the stored
//   memory region is identified and marked as written.  If the
//   possible value set is the all-values set then the pass is aborted
//   since any memory location could be written.
//
//   unsupported: The result set is the all-values set.
//
// The second phase works by examining each load instruction.  For
// each possible value of the pointer argument, the referenced memory
// location is then examined to determine whether it is constant.  All
// possible load values are checked to make sure they are the same.
// If they are and the value stored by the setup function can be
// converted to an LLVM constant value then a map entry is added
// associating the load instruction with the constant.
//
// The third phase uses the LLVM function cloning machinery to create
// a new copy of the function with the constant loads replaced with
// constant values.  The function argument of the thread create
// intrinsic is then set to reference the new function.

///////////////////////////////////////////////////////////////////////////

#define DEBUG_TYPE "thread-const"

#include "llvm_common.h"
#include "llvm_pass.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "setup_func.hpp"
#include "utils.h"


using namespace nanotube;

///////////////////////////////////////////////////////////////////////////

namespace {
  class pointer_set_iterator;

  // Represents a set of pointers, either as a set of in-bounds
  // pointers or as the all-values set.
  class pointer_set {
  private:
    typedef std::vector<bool> vec_t;
    friend class pointer_set_iterator;

  public:
    typedef pointer_set_iterator iterator;

    pointer_set();
    pointer_set(const pointer_set &src) = default;
    pointer_set(pointer_set &&src) = default;
    pointer_set &operator=(const pointer_set &src) = default;
    pointer_set &operator=(pointer_set &&src) = default;

    // An iterator which references the first pointer in the set.
    // This must not be called on the all-values set.
    pointer_set_iterator begin() const;

    // An iterator which beyond the last pointer in the set.
    pointer_set_iterator end() const;

    bool operator==(const pointer_set &other) const;
    bool operator!=(const pointer_set &other) const { return !(*this == other); }

    // Return true if the set contains the specified pointer.
    bool contains(setup_ptr_value_t ptr) const;

    // Returns true if the set is all-values set.
    bool is_all_values() const { return m_all_values; }

    // Remove all pointers from the set.
    void clear();

    // Add a pointer to the set.
    void add_pointer(setup_ptr_value_t ptr);

    // Add the members of src_set to this set.
    void add_set(const pointer_set src_set);

    // Make this set equal to the all-values set.
    void add_all();

    // Swap this set with other.
    void swap(pointer_set &other);

  private:
    // The in-bounds pointers which are contained in the set.  This
    // member is ignored if m_all_values is set.
    vec_t m_contents;

    // True if this set is the all-values set.
    bool m_all_values;
  };

  // An iterator over a pointer_set.
  class pointer_set_iterator {
  private:
    typedef pointer_set::vec_t vec_t;

  public:
    typedef vec_t::iterator iterator;

    pointer_set_iterator(const pointer_set *set, setup_ptr_value_t val):
      m_set(set), m_val(val) {}

    pointer_set_iterator() = default;
    pointer_set_iterator(const pointer_set_iterator &src) = default;
    pointer_set_iterator(pointer_set_iterator &&src) = default;
    pointer_set_iterator &operator=(const pointer_set_iterator &src) = default;
    pointer_set_iterator &operator=(pointer_set_iterator &&src) = default;

    bool operator==(const pointer_set_iterator &other) const;
    bool operator!=(const pointer_set_iterator &other) const { return !(*this == other); }
    pointer_set_iterator &operator++();
    const setup_ptr_value_t &operator*() { return m_val; }

  private:
    // The set being iterated over.
    const pointer_set *m_set;

    // The current pointer value.
    setup_ptr_value_t m_val;
  };
}

namespace llvm {
  template <>
  struct format_provider<pointer_set> {
    static void format(const pointer_set &val, raw_ostream &os,
                       StringRef style);
  };
}

pointer_set::pointer_set():
  m_all_values(false)
{
}

pointer_set_iterator pointer_set::begin() const
{
  // Iteration over all values is not supported.
  assert(!m_all_values);

  // Start with a null pointer and then increment if necessary to find
  // the first set bit.
  auto it = iterator(this, 0);
  if (m_contents.size() != 0 && !contains(0))
    ++it;

  return it;
}

pointer_set_iterator pointer_set::end() const
{
  return iterator(this, m_contents.size());
}

bool pointer_set::operator==(const pointer_set &other) const
{
  return (m_all_values == other.m_all_values &&
          m_contents == other.m_contents);
}

bool pointer_set::contains(setup_ptr_value_t ptr) const
{
  if (m_all_values)
    return true;
  if (ptr >= m_contents.size())
    return false;
  return m_contents[ptr];
}

void pointer_set::clear()
{
  m_all_values = false;
  m_contents.clear();
}

void pointer_set::add_pointer(setup_ptr_value_t ptr)
{
  if (m_all_values)
    return;
  if (m_contents.size() <= ptr)
    m_contents.resize(ptr+1, false);
  m_contents[ptr] = true;
}

void pointer_set::add_set(const pointer_set src_set)
{
  if (m_all_values)
    return;

  if (src_set.m_all_values) {
    add_all();
    return;
  }

  for (setup_ptr_value_t ptr: src_set) {
    add_pointer(ptr);
  }
}

void pointer_set::add_all()
{
  m_all_values = true;
  m_contents.clear();
}

void pointer_set::swap(pointer_set &other)
{
  std::swap(m_all_values, other.m_all_values);
  m_contents.swap(other.m_contents);
}

bool pointer_set_iterator::operator==
  (const pointer_set_iterator &other) const
{
  return ( m_set == other.m_set &&
           m_val == other.m_val );
}

pointer_set_iterator &pointer_set_iterator::operator++()
{
  auto *contents = &(m_set->m_contents);
  do {
    ++m_val;
  } while (m_val < contents->size() && (*contents)[m_val] == false);
  return *this;
}

void llvm::format_provider<pointer_set>::format(const pointer_set &set,
                                                raw_ostream &os,
                                                StringRef style)
{
  os << "{ ";
  if (set.is_all_values()) {
    os << "all";
  } else {
    setup_ptr_value_t range_low;
    setup_ptr_value_t range_high;
    bool range_valid = false;
    for (auto p: set) {
      if (!range_valid) {
        range_low = p;
        range_valid = true;
      } else if (p != range_high+1) {
        if (range_low != range_high)
          os << range_low << "..";
        os << range_high << ", ";
        range_low = p;
      }
      range_high = p;
    }
    if (range_valid) {
      if (range_low != range_high)
        os << range_low << "..";
      os << range_high;
    } else {
      os << "none";
    }
  }
  os << " }";
}

///////////////////////////////////////////////////////////////////////////

namespace {
  class thread_const: public llvm::ModulePass {
  public:
    static char ID;

    thread_const();
    StringRef getPassName() const override {
      return "Nanotube thread constant propagation.";
    }
    void getAnalysisUsage(AnalysisUsage &info) const override;

  private:
    // Get the possible values of alloca instruction insn into set
    // set_out.  If the add parameter is true then the set is not
    // cleared before adding the possible values.
    void get_alloca_set(pointer_set *set_out, AllocaInst *insn,
                        bool add=false);

    // Get the possible values of instruction insn into set set_out.
    // If the add parameter is true then the set is not cleared before
    // adding the possible values.
    void get_insn_set(pointer_set *set_out, Instruction *insn,
                      bool add=false);

    // Get the possible values of constant cst into set set_out.  If
    // the add parameter is true then the set is not cleared before
    // adding the possible values.
    void get_const_set(pointer_set *set_out, Constant *cst,
                       bool add=false);

    // Get the possible values of operand op into set set_out.  If the
    // add parameter is true then the set is not cleared before adding
    // the possible values.
    void get_operand_set(pointer_set *set_out, Value *op,
                         bool add=false);

    // Add the possible values of operand op into set set_out.
    void add_operand_set(pointer_set *set_out, Value *op) {
      get_operand_set(set_out, op, true);
    }

    // Add all the in-bound pointers to a set.
    void add_in_bound_pointers(pointer_set *set_out);

    // Update the pointer value map to associate insn with ptr_set.
    // Returns true if a modification was made.
    bool update_pointer(Instruction *insn, pointer_set *ptr_set);

    // Mark a region of memory as modified.  Return true if any
    // changes were made.
    bool modify_region(setup_ptr_value_t region_ptr,
                       setup_ptr_value_t region_size);

    // Mark the regions of memory indentified by the pointer set and
    // size as modified.  Return true if any changes were made.
    bool modify_region_set(const pointer_set &ptr_set,
                           setup_ptr_value_t region_size);

    // Mark the regions of memory indentified by the pointer set and
    // size as modified.  Return true if any changes were made.
    bool modify_region_set_unsized(const pointer_set &ptr_set);

    // Update the pointer value map for insn.  Returns true if any
    // modification was made.
    bool process_bitcast(BitCastInst *insn);

    // Update the pointer value map for call instruction insn.
    // Returns true if any modification was made.
    bool process_call(CallBase *insn);

    // Update ptr_set_inout for the constant offset const_int
    // associated with the type referenced by GTI.  The GEP in-bounds
    // flag is in_bounds.
    void handle_gep_const_offset(
      pointer_set *ptr_set_inout, llvm::gep_type_iterator GTI,
      const ConstantInt *const_int, bool in_bounds);

    // Update ptr_set_inout for a dynamic offset associated with GTI.
    // The GEP in-bounds flag is in_bounds.
    void handle_gep_dynamic_offset(
      pointer_set *ptr_set_inout, llvm::gep_type_iterator GTI,
      bool in_bounds);

    // Update the pointer value map for insn.  Returns true if any
    // modification was made.
    bool process_gep(GetElementPtrInst *insn);

    // Update the pointer value map for insn.  Returns true if any
    // modification was made.
    bool process_load(LoadInst *insn);

    // Update the pointer value map for insn.  Returns true if any
    // modification was made.
    bool process_phi(PHINode *insn);

    // Update the set of modified memory locations for insn.  Returns
    // true if any modification was made.
    bool process_store(StoreInst *insn);

    // Update the pointer value map and set of modified memory
    // locations for func.  Returns true if any modification was made.
    bool update_pointers(Function *func);

    // Load a value of type ty from ptr.  Returns unknown on failure.
    setup_value load_value(Type *ty, setup_ptr_value_t ptr);

    // Find the constant value of a load instruction.  Returns nullptr
    // if the value cannot be determined to be constant.
    Value *find_constant(LoadInst *insn);

    // Find the constant values in the specified function and insert
    // them into map.
    void find_constants(llvm::ValueToValueMapTy *map,
                        Function *func);

    // Update the thread by cloning the thread function and updating
    // the thread_create call instruction.
    void update_thread(const thread_info *thread,
                       llvm::ValueToValueMapTy *updates);

    // Run the pass on the thread function indicated by args.  Returns
    // true if any modification was made.
    bool process_thread(const thread_info *thread);

    // Run the pass on module m.  Returns true if any modification was
    // made.
    bool runOnModule(Module &m) override;

    // The DataLayout of the module.
    const DataLayout *m_data_layout;

    // The maximum number of bits required for a pointer.
    unsigned int m_max_ptr_bits;

    // The local memory allocations.
    std::map<const AllocaInst *, setup_ptr_value_t> m_local_allocs;

    // The set of possible values associated with each instruction
    // which produces a pointer.
    std::map<const Instruction *, pointer_set> m_value_map;

    // The setup_func object.
    setup_func *m_setup_func;

    // The context type.
    Type *m_context_type;

    // The set of modified memory locations.
    pointer_set m_modified_memory;
  };
};

///////////////////////////////////////////////////////////////////////////

char thread_const::ID;

thread_const::thread_const():
  ModulePass(ID)
{
}

void thread_const::getAnalysisUsage(AnalysisUsage &info) const
{
}

///////////////////////////////////////////////////////////////////////////

void thread_const::get_alloca_set(pointer_set *set_out, AllocaInst *insn,
                                  bool add)
{
  auto a_size = insn->getAllocationSizeInBits(*m_data_layout);
  if (!a_size.hasValue()) {
    report_fatal_errorv("Allocation has a dynamic size: {0}", *insn);
  }
  auto size = APInt(m_max_ptr_bits, a_size.getValue());

  auto res = m_local_allocs.emplace(insn, 0);
  auto it = res.first;
  if (res.second) {
    it->second = m_setup_func->do_alloc(size, insn);
  }

  auto ptr = m_setup_func->get_alloc_base(it->second);
  LLVM_DEBUG(dbgv("    Alloca {0} is at {1}.\n",
                  as_operand(insn), ptr););
  if (!add)
    set_out->clear();
  set_out->add_pointer(ptr);
}

void thread_const::get_insn_set(pointer_set *set_out, Instruction *insn,
                                   bool add)
{
  // Check whether the instruction is in the map.
  auto it = m_value_map.find(insn);
  if (it != m_value_map.end()) {
    if (add)
      set_out->add_set(it->second);
    else
      *set_out = it->second;
    return;
  }

  // Determine whether the instruction is supported.  The branches
  // of this switch statement must match those in update_pointers().
  switch (insn->getOpcode()) {
  case Instruction::Alloca:
    get_alloca_set(set_out, cast<AllocaInst>(insn), add);
    return;

  case Instruction::BitCast:
  case Instruction::GetElementPtr:
  case Instruction::Load:
  case Instruction::PHI:
    // The instruction is supported but not in the map.  It
    // currently has no valid values.
    if (!add)
      set_out->clear();
    return;

  default:
    // The instruction is not supported.  Any value is possible.
    set_out->add_all();
    return;
  }
}

void thread_const::get_const_set(pointer_set *set_out, Constant *cst,
                                   bool add)
{
  LLVM_DEBUG(dbgs() << formatv("  Constant: {0}\n", *cst));

  // Strip operators and accumulate offsets.
  APInt offset = APInt(m_max_ptr_bits, 0);
  while (true) {
    auto *const_expr = dyn_cast<ConstantExpr>(cst);
    if (const_expr == nullptr)
      break;

    switch (const_expr->getOpcode()) {
    case Instruction::BitCast:
      cst = const_expr->getOperand(0);
      break;

    case Instruction::GetElementPtr: {
      auto *gep = cast<GEPOperator>(const_expr);
      APInt gep_offset = APInt(m_max_ptr_bits, 0);
      bool succ =
        gep->accumulateConstantOffset(*m_data_layout, gep_offset);
      offset += gep_offset;

      LLVM_DEBUG(dbgv("    GEP offset {0}\n", gep_offset););

      // Constant expressions should have constant offsets.
      assert(succ);

      cst = cast<Constant>(gep->getPointerOperand());
      break;
    }

    default:
      // The operator is not supported.  Any value is possible.
      LLVM_DEBUG(dbgv("    Unsupported {0}\n", *cst););
      set_out->add_all();
      return;
    }
  }

  auto *global_var = dyn_cast<GlobalVariable>(cst);
  if (global_var != nullptr) {
    auto var_alloc = m_setup_func->find_alloc_of_var(global_var);
    auto base = m_setup_func->get_alloc_base(var_alloc);
    auto end = m_setup_func->get_alloc_end(var_alloc);
    auto offset_val = offset.getZExtValue();
    auto ptr = base + offset_val;
    LLVM_DEBUG(dbgv("    Variable {0}, alloc {1}..{2} offset {3} is {4}.\n",
                    as_operand(global_var), base, end, offset_val, ptr););

    // Add the pointer to the set if it is within the same allocation.
    if (ptr >= base && ptr <= end) {
      if (!add)
        set_out->clear();
      set_out->add_pointer(ptr);
      return;
    }

    // Otherwise, the pointer is beyond the allocation so it is not
    // supported.
    LLVM_DEBUG(dbgv("    Out of range {0}\n", *cst););
    set_out->add_all();
    return;
  }

  if (isa<ConstantPointerNull>(cst) && offset.isNullValue()) {
    auto alloc_id = m_setup_func->get_null_alloc();
    auto base = m_setup_func->get_alloc_base(alloc_id);
    LLVM_DEBUG(dbgv("    Null {0} is {1}\n", *cst, base););
    set_out->add_pointer(base);
    return;
  }

  // The expression is not supposed.  Any value is possible.
  LLVM_DEBUG(dbgv("    Unsupported {0}\n", *cst););
  set_out->add_all();
}

void thread_const::get_operand_set(pointer_set *set_out, Value *op,
                                   bool add)
{
  auto *insn = dyn_cast<Instruction>(op);
  if (insn != nullptr) {
    get_insn_set(set_out, insn, add);
    return;
  }

  auto *cst = dyn_cast<Constant>(op);
  if (cst != nullptr) {
    get_const_set(set_out, cst, add);
    return;
  }

  // The expression is not supposed.  Any value is possible.
  set_out->add_all();
}

void thread_const::add_in_bound_pointers(pointer_set *set_out)
{
  for (auto alloc: m_setup_func->allocs()) {
    setup_ptr_value_t base = alloc.base;
    setup_ptr_value_t end = alloc.end;
    for (auto ptr=base; ptr<=end; ptr++) {
      set_out->add_pointer(ptr);
    }
  }
}

bool thread_const::update_pointer(Instruction *insn,
                                  pointer_set *ptr_set)
{
  // Try to insert the instruction into the map.  This will fail if it
  // has already been inserted.
  auto ins = m_value_map.emplace(insn, pointer_set());
  auto it = ins.first;
  auto succ = ins.second;

  // If the insert happened then copy the pointer set and return true
  // to indicate that a change was made.
  if (succ) {
    LLVM_DEBUG(dbgs() << formatv("  new: {0} = {1}\n",
                                 as_operand(*insn), *ptr_set));
    it->second = *ptr_set;
    return true;
  }

  // If the pointer sets are the same then no changes need to be made.
  if (it->second == *ptr_set) {
    LLVM_DEBUG(dbgs() << formatv("  keep: {0} = {1}\n", *insn, *ptr_set));
    return false;
  }

  // If the pointer sets are not the same, copy the source set to the
  // destination set and return true to indicate that a change was
  // made.
  LLVM_DEBUG(dbgs() << formatv("  update: {0} = {1}\n", *insn, *ptr_set));
  it->second = *ptr_set;
  return true;
}

bool thread_const::modify_region(setup_ptr_value_t region_ptr,
                                 setup_ptr_value_t region_size)
{
  // Ignore stores which overflow the allocation.
  auto s_base = m_setup_func->get_alloc_base_of_ptr(region_ptr);
  auto s_end = m_setup_func->get_alloc_end_of_ptr(region_ptr);
  auto region_end = region_ptr + region_size;
  if (region_end < s_base || region_end > s_end)
    return false;

  // Mark all the bytes as modified.
  bool any_change = false;
  for (setup_ptr_value_t i=0; i<region_size; i++) {
    auto p = region_ptr+i;
    if (!m_modified_memory.contains(p)) {
      m_modified_memory.add_pointer(p);
      any_change = true;
    }
  }

  return any_change;
}

bool thread_const::modify_region_set(const pointer_set &ptr_set,
                                     setup_ptr_value_t region_size)
{
  bool any_change = false;

  if (ptr_set.is_all_values()) {
    if (!m_modified_memory.is_all_values()) {
      m_modified_memory.add_all();
      any_change = true;
    }
    return any_change;
  }

  for (auto ptr: ptr_set) {
    any_change |= modify_region(ptr, region_size);
  }

  return any_change;
}

bool thread_const::modify_region_set_unsized(const pointer_set &ptr_set)
{
  bool any_change = false;

  if (ptr_set.is_all_values()) {
    if (!m_modified_memory.is_all_values()) {
      m_modified_memory.add_all();
      any_change = true;
    }
    return any_change;
  }

  for (auto ptr: ptr_set) {
    auto alloc_end = m_setup_func->get_alloc_end_of_ptr(ptr);
    for (auto p=ptr; p<alloc_end; p++) {
      if (!m_modified_memory.contains(p)) {
        m_modified_memory.add_pointer(p);
        any_change = true;
      }
    }
  }

  return any_change;
}

bool thread_const::process_bitcast(BitCastInst *insn)
{
  // Only process pointer bitcasts.
  Type *ty = insn->getType();
  if (!ty->isPointerTy())
    return false;

  // Find the set of possible source pointers.
  pointer_set src_set;
  get_operand_set(&src_set, insn->getOperand(0));

  // Update the pointer if necessary.
  return update_pointer(insn, &src_set);
}

bool thread_const::process_call(CallBase *insn)
{
  auto iid = get_intrinsic(insn);
  if (iid == Intrinsics::none ||
      iid == Intrinsics::llvm_unknown) {
    report_fatal_errorv("thread-const: Unsupported call: {0}",
                        *insn);
  }

  // Do nothing if the call does not access memory.
  if (intrinsic_is_nop(iid))
    return false;

  // Check whether the call writes to memory.
  auto fmrb = get_nt_fmrb(iid);
  LLVM_DEBUG(dbgv("fmrb {0}: {1}\n", fmrb, *insn););
  switch (fmrb) {
  case llvm::FMRB_DoesNotAccessMemory:
  case llvm::FMRB_OnlyReadsMemory:
  case llvm::FMRB_OnlyReadsArgumentPointees:
  case llvm::FMRB_OnlyAccessesInaccessibleMem:
    // These do not write to accessible memory.
    return false;

  case llvm::FMRB_OnlyAccessesArgumentPointees:
  case llvm::FMRB_OnlyAccessesInaccessibleOrArgMem:
    // These write to accessible memory in a predictable way.
    break;

  default:
    // These may behave in an unpredictable way.
    report_fatal_errorv("thread-const: Unsupported memory behaviour of"
                        " call: {0}", *insn);
  }

  // Check all the arguments.
  bool any_change = false;
  unsigned num_args = insn->arg_size();
  for (unsigned i=0; i<num_args; i++) {
    // Check whether the pointer argument is written.
    llvm::ModRefInfo mri = get_nt_arg_info(iid, i);

    LLVM_DEBUG(
      pointer_set ptr_set;
      get_operand_set(&ptr_set, insn->getArgOperand(i));
      dbgv("arg{0}: mri{1}{2}{3}: {4}\n", i,
                    isMustSet(mri) ? " Must" : "",
                    isModSet(mri) ? " Mod" : "",
                    isRefSet(mri) ? " Ref" : "",
                    ptr_set);
    );

    if (!llvm::isModSet(mri))
      continue;

    Value *arg_val = insn->getArgOperand(i);

    // Ignore context arguments.  They do not refer to user-accessible
    // memory.
    auto *arg_ptr_ty = dyn_cast<PointerType>(arg_val->getType());
    if (arg_ptr_ty != nullptr) {
      auto *elem_ty = arg_ptr_ty->getElementType();
      if (elem_ty == m_context_type)
        continue;
    }

    bool is_bits = false;
    int size_arg = get_size_arg(insn, i, &is_bits);
    if (size_arg < 0) {
      report_fatal_errorv("thread-const: Could not identify size"
                          " for argument {0}: {1}", i, *insn);
    }

    pointer_set ptr_set;
    get_operand_set(&ptr_set, arg_val);

    Value *size_val = insn->getArgOperand(size_arg);
    auto *size_ci = dyn_cast<ConstantInt>(size_val);
    if (size_ci == nullptr) {
      LLVM_DEBUG(dbgv("  Modifying region {0} unsized.\n", ptr_set););
      any_change |= modify_region_set_unsized(ptr_set);
    } else {
      uint64_t size_int = size_ci->getZExtValue();
      LLVM_DEBUG(dbgv("  Modifying region {0} size {1}.\n",
                      ptr_set, *size_ci););
      any_change |= modify_region_set(ptr_set, size_int);
    }
  }

  return any_change;
}

void thread_const::handle_gep_const_offset(
  pointer_set *ptr_set_inout, llvm::gep_type_iterator GTI,
  const ConstantInt *const_int, bool in_bounds)
{
  // Calculate the constant offset since it will be the same across
  // all the pointers.
  setup_ptr_value_t offset = 0;
  StructType *struct_type = GTI.getStructTypeOrNull();
  setup_ptr_value_t index = const_int->getZExtValue();
  if (struct_type != nullptr) {
    auto *layout = m_data_layout->getStructLayout(struct_type);
    offset = layout->getElementOffset(index);
  } else {
    auto *ty = GTI.getIndexedType();
    offset = ( m_data_layout->getTypeAllocSize(ty) * index );
  }

  // Iterate over the pointers and build up the resulting set.
  pointer_set result;
  for (setup_ptr_value_t ptr: *ptr_set_inout) {
    // Find the old and new pointers and allocations.
    setup_ptr_value_t old_ptr = ptr;
    setup_ptr_value_t new_ptr = ptr + offset;
    auto old_alloc = m_setup_func->find_alloc_of_ptr(old_ptr);
    auto new_alloc = m_setup_func->find_alloc_of_ptr(new_ptr);

    // Check whether the arithmetic overflows the allocation.
    if (old_alloc != new_alloc) {
      // If in_bounds is set then the result is a undefined value which
      // cannot be dereferenced so ignore it.
      if (in_bounds)
        continue;

      // An overflow with the in_bounds flag clear is not supported.
      // Return the all-values set.
      ptr_set_inout->add_all();
      return;
    }

    result.add_pointer(ptr + offset);
  }

  // Return the set which has been constructed.
  ptr_set_inout->swap(result);
}

void thread_const::handle_gep_dynamic_offset(
  pointer_set *ptr_set_inout, llvm::gep_type_iterator GTI,
  bool in_bounds)
{
  // The operand should be an index into an array.
  assert(GTI.isSequential());

  // Adjust each pointer value by multiples of the type size.
  Type *index_type = GTI.getIndexedType();
  setup_ptr_value_t type_size = m_data_layout->getTypeAllocSize(index_type);

  // If the type size is zero then the offset has no effect.
  if (type_size == 0)
    return;

  // Dynamic offsets can always overflow the allocation.  If the
  // in_bounds flag is clear then overflow is not supported.  Return
  // the all-values set.
  if (!in_bounds) {
    ptr_set_inout->add_all();
    return;
  }

  LLVM_DEBUG(dbgv("    Dynamic offset {0} with set {1}\n",
                  type_size, *ptr_set_inout););

  pointer_set result;

  // Adjust the pointers by all multiples of the type size.
  setup_ptr_value_t base = 0;
  setup_ptr_value_t end = 0;
  for (auto ptr: *ptr_set_inout) {
    if (ptr > end || ptr < base) {
      setup_ptr_value_t p = ptr;
      base = m_setup_func->get_alloc_base_of_ptr(p);
      end = m_setup_func->get_alloc_end_of_ptr(p);
    }
    LLVM_DEBUG(dbgv("    Pointer {0} is in range {1}..{2}\n",
                    ptr, base, end););

    assert(ptr >= base);
    assert(ptr <= end);

    // Propagate the bit down in steps of the type size until we reach
    // the base of the allocation or a bit which is already set.
    setup_ptr_value_t new_ptr = ptr;
    while (true) {
      if (result.contains(new_ptr))
        break;

      result.add_pointer(new_ptr);

      if (type_size > new_ptr - base)
        break;
      new_ptr -= type_size;
    }

    // Propagate the bit up in steps of the type size until we reach
    // the end of the allocation or a bit which is already set.
    new_ptr = ptr;
    while (true) {
      if (type_size > end - new_ptr)
        break;
      new_ptr += type_size;
      if (result.contains(new_ptr))
        break;

      result.add_pointer(new_ptr);
    }
  }

  LLVM_DEBUG(dbgv("    Result {0}\n", result););
  ptr_set_inout->swap(result);
}

bool thread_const::process_gep(GetElementPtrInst *insn)
{
  bool in_bounds = insn->isInBounds();

  // Start with the set associated with the pointer operand.
  auto result = pointer_set();
  get_operand_set(&result, insn->getPointerOperand());

  // Iterate over the GEP index operands and update the set for each
  // one.
  for (auto GTI=llvm::gep_type_begin(insn), GTE=llvm::gep_type_end(insn);
       GTI != GTE; ++GTI) {
    // Handle the all-values set.
    if (result.is_all_values()) {
      // If it is not in-bounds then all subsequent pointers will be
      // all-values too.
      if (!in_bounds)
        break;

      // Restrict to the in-bound pointers.
      result.clear();
      add_in_bound_pointers(&result);
    }

    const Value *operand = GTI.getOperand();
    auto *const_int = dyn_cast<ConstantInt>(operand);
    if (const_int != nullptr) {
      handle_gep_const_offset(&result, GTI, const_int, in_bounds);
    } else {
      handle_gep_dynamic_offset(&result, GTI, in_bounds);
    }
  }

  // Update the pointer if necessary.
  return update_pointer(insn, &result);
}

bool thread_const::process_load(LoadInst *insn)
{
  // Only process pointer loads.
  Type *ty = insn->getType();
  if (!ty->isPointerTy())
    return false;

  // Determine the possible values of the pointer operand.
  pointer_set src_set;
  get_operand_set(&src_set, insn->getPointerOperand());

  // Any value if possible when loading from an unknown pointer.
  if (src_set.is_all_values()) {
    LLVM_DEBUG(dbgv("    Pointer operand is unknown.\n"););
    return update_pointer(insn, &src_set);
  }

  // Determine the loaded type and its size.
  auto type_size = m_data_layout->getTypeStoreSize(ty);

  // Build up the result set by considering all possible values of the
  // pointer operand.
  pointer_set result;
  for (auto ptr: src_set) {
    auto load_base = setup_ptr_value_t(ptr);
    auto alloc_end = m_setup_func->get_alloc_end_of_ptr(load_base);
    setup_ptr_value_t max_size = (alloc_end - load_base);

    // Ignore a load which extends beyond the end of an allocation.
    // This is invalid so it must never happen.
    if (type_size > max_size)
      continue;

    // Determine the possible loaded value.
    bool not_valid = false;
    bool have_pointer = false;
    setup_ptr_value_t loaded_ptr;
    auto it = m_setup_func->memory_at(load_base);
    for (setup_ptr_value_t offset=0; offset<type_size; ++offset, ++it) {
      auto byte_ptr = ptr + offset;

      // Ignore bytes which can be modified.
      if (m_modified_memory.contains(byte_ptr) ||
          it->write_value.is_unknown())
        continue;

      // If either a non-pointer byte or a pointer byte with the wrong
      // offset is loaded, the result is not a valid pointer.
      if (!(it->write_value.is_ptr())) {
        not_valid = true;
        break;
      }

      if (!have_pointer) {
        // If this is a new pointer then take note.
        have_pointer = true;
        loaded_ptr = it->write_value.get_ptr();
      } else if (loaded_ptr != it->write_value.get_ptr()) {
        // Mixing pointers is not valid.
        not_valid = true;
        break;
      } else {
        // The pointers are equal so do nothing.
      }
    }

    // Ignore the value if it is not valid.
    if (not_valid)
      continue;

    // Add the pointer if there is one.  Otherwise add all values
    // since the result could be any pointer.
    if (have_pointer) {
      result.add_pointer(loaded_ptr);
    } else {
      result.add_all();
      break;
    }
  }

  // Update the pointer if necessary.
  return update_pointer(insn, &result);
}

bool thread_const::process_phi(PHINode *insn)
{
  // Only process pointer phis.
  Type *ty = insn->getType();
  if (!ty->isPointerTy())
    return false;

  // Find the union of the sets of all the operands.
  pointer_set result;
  for (Value *v: insn->incoming_values()) {
    add_operand_set(&result, v);
  }

  // Update the pointer if necessary.
  return update_pointer(insn, &result);
}

bool thread_const::process_store(StoreInst *insn)
{
  bool any_change = false;

  // Determine the possible values of the pointer operand.
  pointer_set ptr_set;
  get_operand_set(&ptr_set, insn->getPointerOperand());

  if (ptr_set.is_all_values()) {
    any_change = !ptr_set.is_all_values();
    m_modified_memory.add_all();
    return any_change;
  }

  // Determine the type being stored and its size.
  Type *ty = insn->getValueOperand()->getType();
  setup_ptr_value_t type_size = m_data_layout->getTypeStoreSize(ty);

  // Mark the overlapping memory as modified.
  return modify_region_set(ptr_set, type_size);
}

bool thread_const::update_pointers(Function *func)
{
  bool any_change = false;

  LLVM_DEBUG(dbgs() << "Updating pointers.\n");

  for (auto &bb: *func) {
    // Ignore basic blocks which end in an unreachable instruction.
    // This skips assertion failures.
    if (bb.getTerminator()->getOpcode() == Instruction::Unreachable)
      continue;

    LLVM_DEBUG(dbgs() << formatv("  Basic block: {0}\n",
                                 as_operand(bb)));

    for (auto &insn: bb) {
      auto opc = insn.getOpcode();
      LLVM_DEBUG(dbgs() << formatv("  Processing {0}\n", insn));

      // The branches of this switch statement should match the ones
      // in get_insn_set().
      switch (opc) {
      case Instruction::Alloca:
        // Processed on demand.
        break;

      case Instruction::BitCast:
        any_change |= process_bitcast(&cast<BitCastInst>(insn));
        break;

      case Instruction::Call:
        any_change |= process_call(&cast<CallBase>(insn));
        break;

      case Instruction::GetElementPtr:
        any_change |= process_gep(&cast<GetElementPtrInst>(insn));
        break;

      case Instruction::Load:
        any_change |= process_load(&cast<LoadInst>(insn));
        break;

      case Instruction::PHI:
        any_change |= process_phi(&cast<PHINode>(insn));
        break;

      case Instruction::Store:
        any_change |= process_store(&cast<StoreInst>(insn));
      }
    }
  }

  return any_change;
}

setup_value thread_const::load_value(Type *ty, setup_ptr_value_t ptr)
{
  auto type_size = m_data_layout->getTypeStoreSize(ty);

  LLVM_DEBUG(dbgv("    Loading value at {0} with type {1}.\n",
                  ptr, *ty););

  // Check whether the whole value is within the allocation.
  setup_ptr_value_t alloc_end = m_setup_func->get_alloc_end_of_ptr(ptr);
  if (type_size > alloc_end - ptr) {
    LLVM_DEBUG(dbgv("    Value extends beyond allocation.\n"););
    return setup_value::undefined();
  }

  // Check whether any part of the value is written.
  for (setup_ptr_value_t offset=0; offset<type_size; offset++) {
    if (m_modified_memory.contains(ptr+offset)) {
      LLVM_DEBUG(dbgv("    Value is partly written.\n"););
      return setup_value::unknown();
    }
  }

  return m_setup_func->load_value(ty, ptr);
}

Value *thread_const::find_constant(LoadInst *insn)
{
  // Find the possible values of the pointer operand.
  pointer_set src_set;
  get_operand_set(&src_set, insn->getPointerOperand());

  if (src_set.is_all_values()) {
    LLVM_DEBUG(dbgv("    Unknown pointer operand.\n"););
    return nullptr;
  }

  // Get the type of the value being loaded.
  Type *ty = insn->getType();

  // Find the loaded value.
  bool have_value = false;
  auto value = setup_value::undefined();
  for (auto ptr: src_set) {
    setup_value loaded = load_value(ty, ptr);

    LLVM_DEBUG(dbgs() << formatv("    Value at {0} is {1}\n",
                                 ptr, loaded););

    // Ignore undefined values since they cannot be loaded.
    if (loaded.is_undefined())
      continue;

    if (have_value == false) {
      have_value = true;
      value = loaded;
    } else if (loaded != value) {
      // Do not replace the instruction if there are multiple values.
      LLVM_DEBUG(dbgs() << "Multiple values found.\n");
      return nullptr;
    }
  }

  LLVM_DEBUG(dbgs() << formatv("  Setup value is {0}\n", value););

  return m_setup_func->convert_value(ty, value);
}

void thread_const::find_constants(llvm::ValueToValueMapTy *map,
                                  Function *func)
{
  LLVM_DEBUG(dbgs() << "Finding constants:\n";);
  for (auto &bb: *func) {
    // Ignore basic blocks which end in an unreachable instruction.
    // This skips assertion failures.
    if (bb.getTerminator()->getOpcode() == Instruction::Unreachable)
      continue;

    for (auto &insn: bb) {
      if (insn.getOpcode() != Instruction::Load)
        continue;

      LLVM_DEBUG(dbgs() << formatv("  Processing {0}\n", insn););
      Value *val = find_constant(dyn_cast<LoadInst>(&insn));
      if (val == nullptr) {
        LLVM_DEBUG(dbgs() << formatv("  No value found.\n"));
        continue;
      }

      LLVM_DEBUG(dbgs() << formatv("  Value is {0}\n", *val));
      (*map)[&insn] = val;
    }
  }
}

void thread_const::update_thread(const thread_info *thread,
                                 llvm::ValueToValueMapTy *updates)
{
  CallBase *call = thread->call();
  Function *old_func = thread->args().func;

  // Clone the function instead of modifying the original in case the
  // original has another user.
  llvm::ValueToValueMapTy values;
  Function *new_func = CloneFunction(old_func, values);
  new_func->setLinkage(Function::PrivateLinkage);

  // Apply the updates to the new function.  The keys in updates refer
  // to instructions in the old function, so use the values map to
  // locate the equivalent instructions in the new module.
  for (auto entry: *updates) {
    const Instruction *old_insn = cast<Instruction>(entry.first);
    Constant *cst = cast<Constant>(entry.second);
    auto it = values.find(old_insn);
    if (it == values.end())
      report_fatal_errorv("Failed to find equivalent of: {0}.", *old_insn);

    Instruction *new_insn = cast<Instruction>(it->second);
    new_insn->replaceAllUsesWith(cst);
    new_insn->eraseFromParent();
  }

  // Update the argument to nanotube_thread_create.
  LLVM_DEBUG(
    Value *old_arg = call->getArgOperand(thread_create_args::func_arg_num);
    dbgs() << formatv("Updating call {0}\n  old_arg: {1}\n  new_arg: {2}\n",
                      *call, old_arg->getName(), new_func->getName());
  );
  call->setArgOperand(thread_create_args::func_arg_num, new_func);

  // Delete the old function if possible.
  if (old_func->user_begin() == old_func->user_end()) {
    LLVM_DEBUG(dbgv("Removing unused function '{0}'.\n", as_operand(old_func)););
    std::string old_name = old_func->getName();
    old_func->eraseFromParent();
    new_func->setName(old_name);
  }
}

bool thread_const::process_thread(const thread_info *thread)
{
  const thread_create_args *args = &(thread->args());
  Function *func = args->func;
  LLVM_DEBUG(
    dbgs() << "\n"
           << "Thread: "
           << args->name
           << "\nfunc: "
           << std::string(func->getName())
           << "\n");

  // Determine possible values of all the pointers.
  while (update_pointers(func))
    continue;

  LLVM_DEBUG(
    dbgs() << formatv("Modified memory: {0}\n", m_modified_memory)
  );

  // Determine the values of constant loads.
  llvm::ValueToValueMapTy map;
  find_constants(&map, func);

  LLVM_DEBUG(
    dbgs() << "Value map:\n";
    for (auto elem: map) {
      dbgs() << formatv("  {0} -> {1}\n", as_operand(elem.first),
                        as_operand(elem.second));
    }
  );

  // If the map is entry then no change is required.
  if (map.size() == 0)
    return false;

  // Update the thread definition.
  update_thread(thread, &map);

  LLVM_DEBUG(
    dbgs() << "Value map:\n";
    for (auto elem: map) {
      dbgs() << formatv("  {0} -> {1}\n", as_operand(elem.first),
                        as_operand(elem.second));
    }
  );

  return true;
}

bool thread_const::runOnModule(Module &m)
{
  // Create the setup_func.
  auto setup = setup_func(m);

  // Initialise the temporary data.
  m_data_layout = &(m.getDataLayout());
  m_max_ptr_bits = m_data_layout->getMaxPointerSizeInBits();
  if (m_max_ptr_bits > 64) {
    report_fatal_error("Pointers wider than 64 bits are not"
                       " supported.");
  }
  if (!(m_data_layout->isLittleEndian() || m_data_layout->isBigEndian())) {
    report_fatal_error("Target machine is not little-endian or big-endian!");
  }
  m_setup_func = &setup;
  m_context_type = get_nt_context_type(m);
  m_modified_memory.clear();

  LLVM_DEBUG(dbgs() << "Running the thread_const pass.\n");

  bool any_modified = false;
  for (auto &thread_info: setup.threads()) {
    m_local_allocs.clear();
    m_value_map.clear();
    any_modified |= process_thread(&thread_info);
  }

  // Free the temporary data.
  m_data_layout = nullptr;
  m_local_allocs.clear();
  m_value_map.clear();
  m_setup_func = nullptr;
  m_modified_memory.clear();

  return any_modified;
}

///////////////////////////////////////////////////////////////////////////

static RegisterPass<thread_const>
register_pass("thread-const", "Propagate constants into Nanotube threads.",
               false,
               false
  );

///////////////////////////////////////////////////////////////////////////
