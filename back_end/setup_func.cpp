/**************************************************************************\
*//*! \file setup_func.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube setup function handling.
**   \date  2020-08-18
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

// Setup functions
// ===============
//
// This setup function class represents the code in the user's setup
// function.
//
// Input conditions
// ----------------
//
// The IR must contain a setup function which consists of a single
// basic block which allocates resources, spawns threads and returns.
//
// When a thread is spawned, it is passed a Nanotube context and the
// base and size of a block of memory to be copied and passed to the
// thread.
//
// The only allowed pointers in the setup function are:
//   Pointers to channels.
//   Pointers to user data areas allocated on the stack.
//   Pointers into user data areas.
//   Pointers to processing stage functions.
//
// Since there is only one basic block, there can be no phi nodes.  In
// addition, all pointer offsets must be constant.  There must be no
// computations other than GEP.
//
// Output conditions
// -----------------
//
// No changes.
//
// Theory of operation
// -------------------
//
// The setup function instance is constructed using the
// setup_func_builder class.  This class iterates over the
// instructions in the setup function and updates its state as it
// goes.
//
// The builder keeps track of the contents of all memory allocations
// used by the setup function.  Whenever a store instruction is
// encountered it updates the contents of the affected memory
// allocation.  When a thread is created, the contents of the
// referenced memory block are copied for use by the thread.

#include "setup_func.hpp"
#include "setup_func_builder.hpp"

#include "Intrinsics.h"
#include "utils.h"

#define DEBUG_TYPE "nanotube-setup"

namespace nanotube {
  namespace cl = llvm::cl;
};
using namespace nanotube;

static cl::opt<std::string>
opt_nanotube_setup("nanotube-setup", cl::init("nanotube_setup"));

///////////////////////////////////////////////////////////////////////////

context_info::context_info(context_index_t context_index):
  m_context_index(context_index),
  m_thread_id(thread_id_none)
{
}

void context_info::set_thread_id(thread_id_t thread_id)
{
  assert(m_thread_id == thread_id_none);
  m_thread_id = thread_id;
}

void
context_info::add_channel(setup_func &setup, const Instruction &call,
                          nanotube_channel_id_t channel_id,
                          channel_index_t channel_index,
                          nanotube_channel_flags_t flags)
{
  LLVM_DEBUG(
    dbgs() << "Context " << m_context_index
           << ": Adding channel " << channel_index
           << " with ID " << channel_id
           << " and flags " << flags << "\n";
  );

  if (m_thread_id != thread_id_none)
    report_fatal_errorv("Cannot add a channel after creating the"
                        " thread: {0}.", call);

  auto &channel_info = setup.get_channel_info(channel_index);

  if ((flags & NANOTUBE_CHANNEL_READ) != 0) {
    port_index_t port_index = add_port(channel_index, channel_id,
                                       true, call);
    channel_info.set_reader(m_context_index, port_index);
  }

  if ((flags & NANOTUBE_CHANNEL_WRITE) != 0) {
    port_index_t port_index = add_port(channel_index, channel_id,
                                       false, call);
    channel_info.set_writer(m_context_index, port_index);
  }
}

port_index_t
context_info::add_port(channel_index_t channel_index,
                       nanotube_channel_id_t channel_id, bool is_read,
                       const Instruction &insn)
{
  port_index_t port_index = m_ports.size();
  if (port_index == port_index_none)
    report_fatal_errorv("Too many channels for context at {0}.",
                        insn);

  LLVM_DEBUG(
    dbgs() << "Context " << m_context_index
           << " adding " << (is_read ? "read" : "write")
           << " port for channel index " << channel_index
           << " with port ID " << port_index
           << " and channel ID " << channel_id << "\n";
  );

  auto ins = m_port_map.insert(
    std::make_pair(std::make_pair(channel_id, is_read),
                   port_index));
  if (!ins.second)
    report_fatal_errorv("Channel is already added for {0} at {1}.",
                        (is_read ? "reading" : "writing"), insn);
  m_ports.emplace_back(channel_index, is_read);

  return port_index;
}

port_index_t
context_info::get_port_index(nanotube_channel_id_t channel_id,
                             bool is_read) const
{
  auto it = m_port_map.find(std::make_pair(channel_id, is_read));
  if (it == m_port_map.end())
    return port_index_none;
  return it->second;
}

stage_port &
context_info::get_port(port_index_t port_index)
{
  assert(port_index < m_ports.size());
  return m_ports[port_index];
}

void
context_info::add_map(setup_func &setup, const Instruction &call,
                      map_index_t map_idx)
{
  auto &map_info = setup.get_map_info(map_idx);
  auto map_id    = map_info.args().id;

  LLVM_DEBUG(
    dbgs() << "Context " << m_context_index
           << ": Adding map " << map_idx
           << " with ID " << map_id
           << " key_sz " << *map_info.args().key_sz
           << " value_sz " << *map_info.args().value_sz
           << " type " << (unsigned)map_info.args().type << '\n';
  );

  auto it = m_map_map.find(map_id);
  if (it != m_map_map.end()) {
    auto &other = setup.get_map_info(it->second);
    errs() << "ERROR: Context " << m_context_index
           << "already has a map with user ID " << map_id
           << ": created by " << *other.creator()
           << " key_sz " << other.args().key_sz
           << " value_sz " << other.args().value_sz
           << " type " << other.args().type
           << "\nAborting!\n";
    exit(1);
  }

  m_map_map.emplace(map_id, map_idx);
}

map_index_t
context_info::map_user_id_to_idx(nanotube_map_id_t uid) const
{
  auto it = m_map_map.find(uid);
  if (it == m_map_map.end()) {
    errs() << "ERROR: Asking for non-existent map id " << uid
           << " which has not been added to this context.\n"
           << "Aborting!\n";
    exit(1);
  }
  return it->second;
}

///////////////////////////////////////////////////////////////////////////

stage_port::stage_port(channel_index_t channel_index, bool is_read):
  m_channel_index(channel_index),
  m_is_read(is_read)
{
}

///////////////////////////////////////////////////////////////////////////

channel_info::channel_info(channel_index_t channel_index,
                           const CallBase *call,
                           setup_func_builder *builder):
  m_channel_index(channel_index),
  m_creator(call),
  m_args(call),
  m_sideband_size(0),
  m_sideband_signals_size(0),
  m_writer_context_index(context_index_none),
  m_writer_port_index(port_index_none),
  m_write_export_type(NANOTUBE_CHANNEL_TYPE_NONE),
  m_reader_context_index(context_index_none),
  m_reader_port_index(port_index_none),
  m_read_export_type(NANOTUBE_CHANNEL_TYPE_NONE)
{
  auto elem_size = builder->eval_operand(m_args.elem_size);
  if (!elem_size.is_int()) {
    report_fatal_errorv("Invalid elem_size operand in {0}", *call);
  }
  m_elem_size = elem_size.get_int().getLimitedValue();

  auto num_elem = builder->eval_operand(m_args.num_elem);
  if (!num_elem.is_int()) {
    report_fatal_errorv("Invalid num_elem operand in {0}", *call);
  }
  m_num_elem = num_elem.get_int().getLimitedValue();
}

void channel_info::set_sideband_size(size_t sideband_size)
{
  LLVM_DEBUG(
    dbgs() << "Channel " << m_channel_index
           << ": Setting sideband size to " << sideband_size << "\n";
  );

  m_sideband_size = sideband_size;
}

void channel_info::set_sideband_signals_size(size_t sideband_signals)
{
  LLVM_DEBUG(
    dbgs() << "Channel " << m_channel_index
           << ": Setting sideband signals to " << sideband_signals << "\n";
  );
  m_sideband_signals_size = sideband_signals;
}

void channel_info::set_writer(context_index_t context_index,
                              port_index_t port_index)
{
  LLVM_DEBUG(
    dbgs() << "Channel " << m_channel_index
           << ": Setting writer to context " << context_index
           << ", port " << port_index << "\n";
  );

  assert(port_index != port_index_none);
  if (m_writer_port_index != port_index_none)
    report_fatal_errorv("Multiple writers of channel '{0}'.",
                        m_args.name);
  m_writer_context_index = context_index;
  m_writer_port_index = port_index;
}

void channel_info::set_reader(context_index_t context_index,
                              port_index_t port_index)
{
  LLVM_DEBUG(
    dbgs() << "Channel " << m_channel_index
           << ": Setting reader to context " << context_index
           << ", port " << port_index << "\n";
  );

  assert(port_index != port_index_none);
  if (m_reader_port_index != port_index_none)
    report_fatal_errorv("Multiple readers of channel '{0}'.",
                        m_args.name);
  m_reader_context_index = context_index;
  m_reader_port_index = port_index;
}

void channel_info::set_export_type(nanotube_channel_type_t type,
                                   nanotube_channel_flags_t flags)
{
  LLVM_DEBUG(
    dbgs() << "Channel " << m_channel_index
           << ": Setting export type " << type
           << " with flags " << flags << "\n";
  );

  if ((flags & NANOTUBE_CHANNEL_WRITE) != 0) {
    if (m_write_export_type != NANOTUBE_CHANNEL_TYPE_NONE)
      report_fatal_errorv("Channel '{0}' has already been exported"
                          " for writing.", m_args.name);
    m_write_export_type = type;
  }

  if ((flags & NANOTUBE_CHANNEL_READ) != 0) {
    if (m_read_export_type != NANOTUBE_CHANNEL_TYPE_NONE)
      report_fatal_errorv("Channel '{0}' has already been exported"
                          " for reading.", m_args.name);
    m_read_export_type = type;
  }
}

///////////////////////////////////////////////////////////////////////////

thread_info::thread_info(CallBase *call,
                         thread_create_args &args,
                         context_index_t context_index):
    m_call(call),
    m_args(args),
    m_context_index(context_index)
{
}

///////////////////////////////////////////////////////////////////////////

map_info::map_info(map_index_t idx, CallBase *call,
                   setup_func_builder* builder):
    m_idx(idx),
    m_creator(call),
    m_args(call),
    m_ctx_idx(context_index_none)
{
  LLVM_DEBUG(
    dbgs() << "Creating map " << *call
           << " at idx " << idx
           << " user id: " << m_args.id
           << " type: " << m_args.type
           << " key_sz: " << *m_args.key_sz
           << " value_sz: " << *m_args.value_sz << '\n';
  );
}

///////////////////////////////////////////////////////////////////////////

kernel_info::kernel_info(kernel_index_t idx, CallBase *call,
                         setup_func_builder* builder):
    m_idx(idx),
    m_creator(call),
    m_args(call)
{
  LLVM_DEBUG(
    dbgs() << "Adding kernel " << m_args.name
           << " using function " << m_args.kernel->getName()
           << " with call " << *call
           << " at idx " << idx << '\n';
  );
}

///////////////////////////////////////////////////////////////////////////
llvm::raw_ostream &llvm::operator<<(llvm::raw_ostream& os,
                                    const nanotube::setup_value &val)
{
  switch (val.get_kind()) {
  default:
  case setup_value::KIND_UNKNOWN:
    os << "unknown";
    break;
  case setup_value::KIND_UNDEFINED:
    os << "undefined";
    break;
  case setup_value::KIND_INT: {
    setup_int_value_t int_val = val.get_int();
    os << formatv("int{0}({1})", int_val.getBitWidth(), int_val);
    break;
  }
  case setup_value::KIND_PTR:
    os << "ptr(" << formatv("{0}", val.get_ptr()) << ")";
    break;
  case setup_value::KIND_MEMSET:
    os << "memset(" << int(val.get_memset()) << ")";
    break;
  case setup_value::KIND_CHANNEL:
    os << "channel(" << int(val.get_channel()) << ")";
    break;
  case setup_value::KIND_CONTEXT:
    os << "context(" << int(val.get_context()) << ")";
    break;
  case setup_value::KIND_MAP:
    os << "map(" << int(val.get_map()) << ")";
    break;
  }

  return os;
}

///////////////////////////////////////////////////////////////////////////

Function &setup_func::get_setup_func(Module &m)
{
  if (opt_nanotube_setup.getValue() == "")
    report_fatal_error("No setup function specified."
                       "  Use -nanotube-setup.");

  ValueSymbolTable &mod_syms = m.getValueSymbolTable();
  std::string &setup_name = opt_nanotube_setup.getValue();
  Value *func_value = mod_syms.lookup(setup_name);

  if (func_value == NULL)
    report_fatal_error("Failed to find the Nanotube setup function '" +
                       setup_name + "'.");

  Function *func = dyn_cast<Function>(func_value);
  if (func == NULL)
    report_fatal_error("The Nanotube setup '" + setup_name +
                       "' is not a function.");

  if (func->empty())
    report_fatal_error("The Nanotube setup function has no definition.");

  return *func;
}

setup_func::setup_func(Module &m, setup_tracer *tracer, bool strict):
  m_func(get_setup_func(m)),
  m_data_layout(&(m_func.getParent()->getDataLayout()))
{
  m_max_ptr_bits = 8 * m_data_layout->getMaxPointerSize();
  assert(m_max_ptr_bits <= setup_ptr_value_num_bits);
  auto builder = setup_func_builder(*this, tracer, strict);
  builder.build();
}

context_info &
setup_func::get_context_info(context_index_t context_index)
{
  assert(context_index < m_context_infos.size());
  return m_context_infos[context_index];
}

channel_info &
setup_func::get_channel_info(channel_index_t channel_index)
{
  assert(channel_index < m_channel_infos.size());
  return m_channel_infos[channel_index];
}

void setup_func::print_memory_contents(raw_os_ostream &output)
{
  for (auto &mem: m_alloc_map) {
    output << formatv("Memory allocation {0}-{1}: {2}\n", mem.first,
                      mem.second.end, *mem.second.info);
  }

  for (auto &region: m_region_map) {
    output << formatv("Write {0}-{1} offset {2} value {3}\n",
                      region.first, region.second.end,
                      region.second.write_offset,
                      region.second.write_value);
  }
}

llvm::iterator_range<setup_alloc_iterator> setup_func::allocs() const
{
  return llvm::make_range(
    setup_alloc_iterator(m_alloc_map.begin()),
    setup_alloc_iterator(m_alloc_map.end())
  );
}

setup_alloc_id_t
setup_func::find_alloc_of_var(GlobalVariable *global_var)
{
  // Find the existing allocation or create a new one.
  auto ins = m_gv_allocs.emplace(global_var, 0);
  auto it = ins.first;

  // If the element could not be inserted then it must already be in
  // the map.  Simply return the previously allocated ID.
  if (!ins.second) {
    return it->second;
  }

  // The element was inserted into the map.  Allocate some memory and
  // store it in the inserted element.
  Type *ty = global_var->getValueType();
  LLVM_DEBUG(dbgs() << formatv("Variable {0} has type {1}.\n", *global_var, *ty););
  APInt size = APInt(m_max_ptr_bits, m_data_layout->getTypeStoreSize(ty));
  auto result = do_alloc(size, global_var);
  it->second = result;

  // Initialise the memory if necessary.
  if (global_var->hasInitializer()) {
    setup_ptr_value_t ptr = get_alloc_base(result);
    do_store(ptr, global_var->getInitializer());
  }

  return result;
}

setup_alloc_id_t
setup_func::get_alloc_base_of_ptr(setup_ptr_value_t ptr) const
{
  auto it = find_alloc(ptr);
  if (it == m_alloc_map.end())
    return setup_alloc_id_t(0);
  return it->first;
}

setup_ptr_value_t setup_func::get_alloc_end_of_ptr(setup_ptr_value_t ptr) const
{
  auto it = find_alloc(ptr);
  if (it == m_alloc_map.end())
    return setup_ptr_value_t(0);
  return it->second.end;
}

setup_func_memory_iterator
setup_func::memory_at(setup_ptr_value_t ptr) const
{
  return setup_func_memory_iterator(this, ptr);
}

std::pair<bool, uint8_t> setup_func::try_extract_byte(
  const setup_value &value, setup_ptr_value_t offset)
{
  // Try to load a byte from an integer.
  if (value.is_int()) {
    setup_int_value_t int_val = value.get_int();

    unsigned num_bits = int_val.getBitWidth();
    unsigned num_bytes = (num_bits+7)/8;
    unsigned lsb;
    if (m_data_layout->isLittleEndian()) {
      lsb = offset*8;
    } else {
      lsb = ((num_bytes-1) - offset)*8;
    }
    assert(lsb < num_bits);
    unsigned width = std::min(8u, num_bits-lsb);
    uint8_t result = int_val.extractBits(width, lsb).getLimitedValue();

    LLVM_DEBUG(dbgs() << formatv("    Value {0} byte {1} is {2}\n",
                                 value, offset, result););

    return std::make_pair(true, result);
  }

  // Try to load a byte from a memset.
  if (value.is_memset()) {
    uint8_t result = value.get_memset();

    LLVM_DEBUG(dbgs() << formatv("  Value {0} byte {1} is {2}\n",
                                 value, offset, result););

    return std::make_pair(true, result);
  }

  // An unsupported value.
  return std::make_pair(false, 0);
}


bool setup_func::try_load_bytes(uint8_t *buffer, setup_ptr_value_t ptr,
                                setup_ptr_value_t size)
{
  auto it = memory_at(ptr);
  for (setup_ptr_value_t index=0; index<size; ++index, ++it) {
    auto res = try_extract_byte(it->write_value, it->write_offset);
    if (!res.first)
      return false;
    buffer[index] = res.second;
  }

  return true;
}

setup_value setup_func::load_integer(IntegerType *ty,
                                     setup_ptr_value_t ptr)
{
  // Collect the bytes into an array.  If any byte cannot be
  // determined, return an unknown value.
  SmallVector<uint8_t, 64> bytes;

  auto type_size = m_data_layout->getTypeStoreSize(ty);
  bytes.resize(type_size, 0);

  if (!try_load_bytes(&(bytes[0]), ptr, type_size))
    return setup_value::unknown();

  // Convert the bytes into an integer.
  unsigned num_bits = ty->getBitWidth();
  unsigned num_bytes = (num_bits+7)/8;
  assert(num_bytes <= type_size);
  bool is_little_endian = m_data_layout->isLittleEndian();
  setup_int_value_t result = APInt(num_bits, 0);
  for (setup_ptr_value_t index=0; index<num_bytes; ++index) {
    unsigned lsb;
    if (is_little_endian) {
      lsb = index*8;
    } else {
      lsb = ((num_bytes-1) - index)*8;
    }
    assert(lsb < num_bits);
    unsigned width = std::min(8u, num_bits-lsb);
    result.insertBits(APInt(width, bytes[index]), lsb);
  }

  return setup_value::of_int(result);
}

setup_value setup_func::load_pointer(PointerType *ty,
                                     setup_ptr_value_t ptr)
{
  auto it = memory_at(ptr);
  auto type_size = m_data_layout->getTypeStoreSize(ty);
  auto result = it->write_value;

  LLVM_DEBUG(dbgs() <<
             formatv("    Checking pointer consistency at {0} value {1}.\n",
                     ptr, result););

  // Validate all the bytes.
  for (setup_ptr_value_t offset=0; offset<type_size; ++offset, ++it) {
    if (it->write_value != result ||
        it->write_offset != offset) {
      LLVM_DEBUG(dbgs() <<
                 formatv("    Byte {0} is inconsistent: value {1}, offset {2}\n",
                         offset, it->write_value, it->write_offset););

      return setup_value::unknown();
    }

  }

  LLVM_DEBUG(dbgs() << "    All bytes are consistent.\n";);

  return result;
}

setup_value setup_func::load_value(Type *ty, setup_ptr_value_t ptr)
{
  // Load an integer value if it is an integer type.
  if (ty->isIntegerTy())
    return load_integer(cast<IntegerType>(ty), ptr);

  // Load an pointer value if it is an pointer type.
  if (ty->isPointerTy())
    return load_pointer(cast<PointerType>(ty), ptr);

  LLVM_DEBUG(dbgv("    Value is unknown type.\n"););
  return setup_value::unknown();
}

Value *setup_func::convert_value(Type *ty, const setup_value &value)
{
  LLVM_DEBUG(dbgs() << formatv("Converting {0} to {1}\n", value, *ty));

  // Try to convert an integer.
  if (ty->isIntegerTy()) {
    if (!value.is_int()) {
      LLVM_DEBUG(dbgv("  Value is not an integer.\n"););
      return nullptr;
    }

    setup_int_value_t int_val = value.get_int();
    return ConstantInt::get(ty, int_val);
  }

  // Try to convert a pointer.
  if (ty->isPointerTy()) {
    if (!value.is_ptr()) {
      LLVM_DEBUG(dbgs() << "Value is not a pointer.\n";);
      return nullptr;
    }

    setup_ptr_value_t ptr_val = value.get_ptr();
    auto it = find_alloc(ptr_val);
    if (it == m_alloc_map.end()) {
      LLVM_DEBUG(dbgs() << "No allocation found.\n";);
      return nullptr;
    }

    GlobalVariable *var = dyn_cast<GlobalVariable>(it->second.info);
    if (var == nullptr) {
      LLVM_DEBUG(dbgs() << "Not a global variable.\n";);
      return nullptr;
    }

    // Find the relevant types.
    LLVMContext &ctxt = ty->getContext();
    Type *i8_ty = IntegerType::get(ctxt, 8);
    unsigned address_space = var->getAddressSpace();
    Type *i8ptr_ty = PointerType::get(i8_ty, address_space);

    // Create a constant expression which describes the pointer.  The
    // general form is:
    //
    //   bitcast(get_element_ptr(bitcast(ptr)))
    //
    // The get_element_ptr and inner bitcast are omitted if the offset
    // is zero.  The bitcasts are omitted if they have no effect.
    Constant *ptr = var;
    auto offset = ptr_val - it->first;
    if (offset != 0) {
      if (var->getType() != i8ptr_ty)
        ptr = ConstantExpr::getBitCast(ptr, i8ptr_ty);
      Type *ptr_int_ty = IntegerType::get(ctxt, m_max_ptr_bits);
      Constant *gep_offset = ConstantInt::get(ptr_int_ty, offset);
      ptr = ConstantExpr::getInBoundsGetElementPtr(i8_ty, ptr, gep_offset);
    }

    // Cast the result if necessary.
    if (ptr->getType() != ty)
      ptr = ConstantExpr::getBitCast(ptr, ty);

    return ptr;
  }

  // Unsupported type.
  LLVM_DEBUG(dbgv("  Unsupported type.\n"););
  return nullptr;
}

setup_func::alloc_map_t::const_iterator
setup_func::find_alloc(setup_ptr_value_t ptr) const
{
  setup_alloc_id_t alloc_id = ptr;

  // Find the first allocation after the one containing the pointer.
  // Fail if there is no allocation before the one found.
  auto it = m_alloc_map.upper_bound(alloc_id);
  if (it == m_alloc_map.begin())
    return m_alloc_map.end();

  // Find the only allocation which could contain the pointer.
  --it;

  // Fail if the allocation doesn't contain the pointer.
  if (ptr < it->first || ptr > it->second.end)
    return m_alloc_map.end();

  // Return the iterator.
  return it;
}

setup_alloc_id_t setup_func::do_alloc(APInt size, Value *info)
{
  // Find the end of the last allocation.
  setup_ptr_value_t last_alloc_end = 0;
  auto it = m_alloc_map.end();
  if (it != m_alloc_map.begin()) {
    --it;
    last_alloc_end = it->second.end;
  }

  setup_ptr_value_t alloc_base = last_alloc_end + 1;
  setup_ptr_value_t alloc_end = alloc_base + size.getZExtValue();

  if (alloc_end <= last_alloc_end) {
    report_fatal_errorv("Failed to allocate {0} bytes for {1}.",
                        size, *info);
  }

  LLVM_DEBUG(
    dbgs() << formatv("do_alloc: size={0} base={1}, end={2}\n",
                      size, alloc_base, alloc_end);
  );

  // Add the allocation.
  auto ins = m_alloc_map.emplace(alloc_base, alloc_contents_t());
  assert(ins.second);
  it = ins.first;

  // Initialize the other members.
  it->second.end = alloc_end;
  it->second.info = info;

  return alloc_base;
}

void setup_func::do_memcpy(setup_ptr_value_t dest, setup_ptr_value_t src,
                           setup_ptr_value_t size)
{
  // Do nothing for an empty copy.
  if (size == 0)
    return;

  // Erase any previous stores which overlap the destination region.
  erase_region(dest, size);

  // Make sure the source region is within a single allocation.  This
  // has already been checked for the destination region.
  auto src_alloc_it = m_alloc_map.upper_bound(src);
  if (src_alloc_it == m_alloc_map.begin()) {
    report_fatal_errorv("Invalid memcpy from {0}.", src);
  }

  --src_alloc_it;
  setup_ptr_value_t max_size = src_alloc_it->second.end - src;
  if (src < src_alloc_it->first || size > max_size) {
    report_fatal_errorv("Memcpy from {0} size {1} extends beyond"
                        " allocation {2}..{3}.", src, size,
                        src_alloc_it->first, src_alloc_it->second.end);
  }

  // Find the first region to copy.
  auto src_it = m_region_map.upper_bound(src);
  if (src_it != m_region_map.begin()) {
    auto prev_it = src_it;
    --prev_it;
    if (prev_it->second.end > src)
      src_it = prev_it;
  }

  // Copy the regions.
  setup_ptr_value_t src_end = src + size;
  setup_ptr_value_t dest_offset = dest - src;
  while (src_it != m_region_map.end() && src_it->first < src_end) {
    LLVM_DEBUG(
      dbgv("  Copying: base={0}, end={1},"
           " write_offset={2}, write_value={3}\n",
           src_it->first, src_it->second.end,
           src_it->second.write_offset,
           src_it->second.write_value);
    );

    // Work out some relevant pointers and offsets.
    setup_ptr_value_t reg_base = std::max(src, src_it->first);
    setup_ptr_value_t reg_end = std::min(src_end, src_it->second.end);
    setup_ptr_value_t reg_offset = reg_base - src_it->first;
    setup_ptr_value_t dest_base = reg_base + dest_offset;
    setup_ptr_value_t dest_end = reg_end + dest_offset;

    // Insert a copy of the region.
    auto ins = m_region_map.emplace(dest_base, src_it->second);
    assert(ins.second);
    auto ins_elem = &(ins.first->second);
    ins_elem->end = dest_end;
    ins_elem->write_offset = src_it->second.write_offset + reg_offset;

    LLVM_DEBUG(
      dbgv("  Insert: base={0}, end={1}, write_offset={2}\n",
           dest_base, ins_elem->end,
           ins_elem->write_offset);
    );

    // Move on to the next element.
    ++src_it;
  }
}

void setup_func::do_memset(setup_ptr_value_t ptr, setup_ptr_value_t size,
                           uint8_t value)
{
  do_store(ptr, size, setup_value::of_memset(value));
}

void setup_func::erase_region(setup_ptr_value_t ptr, setup_ptr_value_t size)
{
  // Find the allocation after the one which might contain the pointer.
  auto alloc_it = m_alloc_map.upper_bound(ptr);
  if (alloc_it == m_alloc_map.begin()) {
    report_fatal_errorv("Invalid store to {0}.", ptr);
  }

  // Find the allocation which might contain the pointer.
  --alloc_it;

  // Check that the allocation does contain the pointer.
  setup_ptr_value_t max_size = alloc_it->second.end - ptr;
  if (ptr < alloc_it->first || size > max_size) {
    report_fatal_errorv("Store to {0} size {1} extends beyond"
                        " allocation {2}..{3}.", ptr, size,
                        alloc_it->first, alloc_it->second.end);
  }

  LLVM_DEBUG(
    dbgs() << formatv("  alloc: base={0}, end={1}\n", alloc_it->first,
                      alloc_it->second.end);
  );

  // When erasing a region which may overlap with a region from
  // one or more previous stores, we need to:
  //
  //   1. Split any previous region which contains the erased region.
  //
  //   2. Remove any previous region which is contained in the erased
  //      region.
  //
  //   3. Truncate any other previous region which overlaps the erased
  //      region.
  //
  // The approach taken here is to work from the end of the erased
  // region in three stages:
  //
  // Stage 1: Find a region which contains the last byte of the erased
  // region and the byte after.  Split the previous region by adding a
  // region which starts from the end of the erased region and then
  // truncating the old region.
  //
  // Stage 2: Remove any region which is contained within the erased
  // region.  Repeat until all these regions have gone.
  //
  // Stage 3: Shrink any region which extends into the erased region.
  // This region cannot extend past the end of the erased region
  // because it would have been split in stage 1 if it did.

  // Find the insertion point.
  setup_ptr_value_t erase_end = ptr + size;
  auto ub_it = m_region_map.lower_bound(erase_end);

  // Split a region which extends beyond the end of the erased region
  // if there is one.
  if (ub_it != m_region_map.begin()) {
    auto it = ub_it;
    --it;
    // Only split it if it overlaps with the last byte.
    if (it->first < erase_end && it->second.end > erase_end) {
      LLVM_DEBUG(
        dbgs() << formatv("  Split: base={0}, end={1}, write_offset={2}\n",
                          it->first, it->second.end,
                          it->second.write_offset);
      );

      // Create the region which extends above the erased region.
      auto ins = m_region_map.emplace(erase_end, it->second);
      assert(ins.second);
      ub_it = ins.first;
      ub_it->second.write_offset += (erase_end - it->first);

      // Shrink the existing region.
      it->second.end = erase_end;

      LLVM_DEBUG(
        dbgs() << formatv("    0: base={0}, end={1}, write_offset={2}\n",
                          it->first, it->second.end,
                          it->second.write_offset);
        dbgs() << formatv("    1: base={0}, end={1}, write_offset={2}\n",
                          ub_it->first, ub_it->second.end,
                          ub_it->second.write_offset);
      );
    }
  }

  // Remove any regions which are completely within the erased region.
  while (ub_it != m_region_map.begin()) {
    auto it = ub_it;
    --it;

    // Stop if the start of the region is before the erased region.
    if (it->first < ptr)
      break;

    LLVM_DEBUG(
      dbgs() << formatv("  Remove: base={0}, end={1}, write_offset={2}\n",
                        it->first, it->second.end,
                        it->second.write_offset);
    );

    // Erase the element and repeat.
    m_region_map.erase(it);
  }

  // Shrink a region which extends into the erased region, if any.
  if (ub_it != m_region_map.begin()) {
    auto it = ub_it;
    --it;

    // Only shrink it if overlaps with the erased region.
    if (it->second.end > ptr) {
      LLVM_DEBUG(
        dbgs() << formatv("  Shrink: base={0}, end={1}, write_offset={2}\n",
                          it->first, it->second.end,
                          it->second.write_offset);
      );
      it->second.end = ptr;
      LLVM_DEBUG(
        dbgs() << formatv("    0: base={0}, end={1}, write_offset={2}\n",
                          it->first, it->second.end,
                          it->second.write_offset);
      );
    }
  }
}

void setup_func::do_store(setup_ptr_value_t ptr, setup_ptr_value_t size,
                          setup_value value)
{
  LLVM_DEBUG(
    dbgs() << formatv("do_store: addr={0}, size={1}, value={2}\n",
                      ptr, size, value);
  );

  // Ignore an empty store.  For example, memset with length zero.
  if (size == 0)
    return;

  erase_region(ptr, size);

  // Insert the new region.
  auto ins = m_region_map.emplace(ptr, memory_region_t());
  assert(ins.second);
  auto ins_elem = &(ins.first->second);
  ins_elem->end = ptr + size;
  ins_elem->write_offset = 0;
  ins_elem->write_value = value;

  LLVM_DEBUG(
    dbgs() << formatv("  Insert: base={0}, end={1}, write_offset={2}\n",
                      ptr, ins_elem->end,
                      ins_elem->write_offset);
  );
}

void setup_func::do_store(setup_ptr_value_t pointer, Constant *constant)
{
  // Use a stack to handle nested values.
  SmallVector<std::pair<setup_ptr_value_t,Constant *>, 64> stack;
  stack.emplace_back(pointer, constant);

  // Process all entries on the stack.
  while (stack.size() != 0) {
    // Get the entry from the top of the stack.
    pointer = stack.back().first;
    constant = stack.back().second;
    stack.pop_back();

    auto type_size = m_data_layout->getTypeStoreSize(constant->getType());

    // Handle constant ints.
    auto *constant_int = dyn_cast<ConstantInt>(constant);
    if (constant_int != nullptr) {
      auto &int_val = constant_int->getValue();
      do_store(pointer, type_size, setup_value::of_int(int_val));
      continue;
    }

    auto data_seq = dyn_cast<ConstantDataSequential>(constant);
    if (data_seq != nullptr) {
      unsigned num_elem = data_seq->getNumElements();
      uint64_t stride = data_seq->getElementByteSize();
      for (unsigned i=0; i<num_elem; i++) {
        stack.emplace_back(pointer, data_seq->getElementAsConstant(i));
        pointer += stride;
      }
      continue;
    }

    // Handle NULL pointers.
    auto *constant_null = dyn_cast<ConstantPointerNull>(constant);
    if (constant_null != nullptr) {
      do_memset(pointer, type_size, 0);
      continue;
    }

    // Handle constant zero.
    auto *constant_zero = dyn_cast<ConstantAggregateZero>(constant);
    if (constant_zero != nullptr) {
      do_memset(pointer, type_size, 0);
      continue;
    }

    report_fatal_errorv("Unsupported constant for store: {0}.",
                        *constant);
  }
}

///////////////////////////////////////////////////////////////////////////

setup_func_memory_iterator::setup_func_memory_iterator(
  const setup_func *func, setup_ptr_value_t ptr):
  m_setup_func(func),
  m_ptr(ptr)
{
  // Find the entry which contains the pointer, if any.
  m_iter = m_setup_func->m_region_map.upper_bound(ptr);
  if (m_iter != m_setup_func->m_region_map.begin())
    --m_iter;

  update_value();
}

setup_func_memory_iterator &setup_func_memory_iterator::operator++()
{
  // Increment the pointer.
  ++m_ptr;

  // Do nothing else if the iterator is already at the end.
  if (m_iter == m_setup_func->m_region_map.end()) {
    LLVM_DEBUG(dbgv("Incremented iterator at {0} is still out of bounds.\n",
                    m_ptr););
    return *this;
  }

  // Advance to the next region if necessary.
  if (m_ptr >= m_iter->second.end) {
    LLVM_DEBUG(dbgv("Incremented iterator at {0} moves to the next region.\n",
                    m_ptr););
    ++m_iter;
  }

  // Update the value.
  update_value();

  // Return a reference to this iterator.
  return *this;
}

void setup_func_memory_iterator::update_value()
{
  // Check whether the entry contains the pointer.
  if (m_iter != m_setup_func->m_region_map.end()) {
    if (m_ptr >= m_iter->first && m_ptr < m_iter->second.end) {
      m_value.write_value = m_iter->second.write_value;
      m_value.write_offset = ( (m_ptr - m_iter->first) +
                               m_iter->second.write_offset );
      LLVM_DEBUG(dbgv("Iterator at {0} is in [{1}..{2}]\n", m_ptr,
                      m_iter->first, m_iter->second.end););
      return;
    }
    LLVM_DEBUG(dbgv("Iterator at {0} is outside [{1}..{2}]\n", m_ptr,
                    m_iter->first, m_iter->second.end););
  } else {
    LLVM_DEBUG(dbgv("Iterator at {0} is beyond all writes.\n", m_ptr););
  }

  m_value.write_value = setup_value::unknown();
  m_value.write_offset = 0;
}

///////////////////////////////////////////////////////////////////////////
