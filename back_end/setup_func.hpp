/**************************************************************************\
*//*! \file setup_func.hpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Nanotube setup function handling.
**   \date  2020-08-18
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_SETUP_FUNC_HPP
#define NANOTUBE_SETUP_FUNC_HPP

#include "Intrinsics.h"
#include "llvm_common.h"
#include "llvm_insns.h"
#include "llvm_pass.h"

#include <cstdint>

namespace nanotube
{
class setup_func;
class setup_func_builder;
class setup_func_memory_iterator;
class thread_create_args;

static const int channel_index_bits = 32;
typedef uint32_t channel_index_t;
static const auto channel_index_none = channel_index_t(-1);

static const int context_index_bits = 32;
typedef uint32_t context_index_t;
static const auto context_index_none = context_index_t(-1);

typedef uint32_t port_index_t;
static const auto port_index_none = port_index_t(-1);

typedef uint32_t thread_id_t;
static const auto thread_id_none = thread_id_t(-1);

typedef uint32_t map_index_t;
static const auto map_index_none = map_index_t(-1);
static const auto map_index_bits = sizeof(map_index_t)*8;

typedef uint32_t kernel_index_t;
static const auto kernel_index_none = kernel_index_t(-1);
static const auto kernel_index_bits = sizeof(kernel_index_t)*8;

///////////////////////////////////////////////////////////////////////////

// A port of a processing stage.
class stage_port
{
public:
  stage_port(channel_index_t channel_index, bool is_read);

  channel_index_t channel_index() const { return m_channel_index; }
  bool is_read() const { return m_is_read; }

private:
  // The connected channel.
  channel_index_t m_channel_index;

  // Whether this is a read port.
  bool m_is_read;
};

///////////////////////////////////////////////////////////////////////////

// A thread context.
class context_info
{
public:
  typedef SmallVector<stage_port, 8> port_vec_t;

  explicit context_info(context_index_t context_index);

  thread_id_t get_thread_id() const { return m_thread_id; }
  void set_thread_id(thread_id_t thread_id);

  // Get a reference to the port vector.
  const port_vec_t &ports() const { return m_ports; }

  // Process a call to nanotube_context_add_channel.
  void add_channel(setup_func &setup, const Instruction &insn,
                   nanotube_channel_id_t channel_id,
                   channel_index_t channel_index,
                   nanotube_channel_flags_t flags);

  // Add a port.
  port_index_t add_port(channel_index_t channel_index,
                        nanotube_channel_id_t channel_id, bool is_read,
                        const Instruction &insn);

  // Get the port ID given the channel ID and direction.
  port_index_t get_port_index(nanotube_channel_id_t channel_id,
                              bool is_read) const;

  // Get information about a port.
  stage_port &get_port(port_index_t id);

  // Add a map
  void add_map(setup_func &setup, const Instruction &call,
               map_index_t map_idx);

  map_index_t map_user_id_to_idx(nanotube_map_id_t uid) const;

private:
  // The specifier for a port.
  typedef std::pair<nanotube_channel_id_t, bool> port_spec_t;

  // The identifier of this context.
  context_index_t m_context_index;

  // The associated thread.
  thread_id_t m_thread_id;

  // The channel ports added to the context.
  port_vec_t m_ports;

  // A mapping from local channel ID and direction to the port ID.
  std::map<port_spec_t, port_index_t> m_port_map;

  // The maps added to this context
  std::map<nanotube_map_id_t, map_index_t> m_map_map;

};

///////////////////////////////////////////////////////////////////////////

// A channel created in a setup function.
class channel_info
{
public:
  channel_info(channel_index_t channel_index, const CallBase *call,
               setup_func_builder *builder);

  void set_writer(context_index_t context_index, port_index_t port_index);
  void set_reader(context_index_t context_index, port_index_t port_index);
  void set_export_type(nanotube_channel_type_t type,
                       nanotube_channel_flags_t flags);
  void set_sideband_size(size_t sideband_size);
  void set_sideband_signals_size(size_t sideband_signals);

  bool has_writer() const { return m_writer_port_index != port_index_none; }
  bool has_reader() const { return m_reader_port_index != port_index_none; }

  const std::string &get_name() const { return m_args.name; }
  uint32_t get_elem_size_bits() const { return 32; }
  uint32_t get_elem_size() const { return m_elem_size; }
  uint32_t get_num_elem_bits() const { return 32; }
  uint32_t get_num_elem() const { return m_num_elem; }
  uint32_t get_sideband_size_bits() const { return 32; }
  uint32_t get_sideband_size() const { return m_sideband_size; }
  uint32_t get_sideband_signals_size_bits() const { return 32; }
  uint32_t get_sideband_signals_size() const { return m_sideband_signals_size; }

  nanotube_channel_type_t get_write_export_type() const {
    return m_write_export_type;
  }
  nanotube_channel_type_t get_read_export_type() const {
    return m_read_export_type;
  }
  context_index_t get_reader_context() const { return m_reader_context_index; }
  context_index_t get_writer_context() const { return m_writer_context_index; }
  port_index_t get_reader_port() const { return m_reader_port_index; }
  port_index_t get_writer_port() const { return m_writer_port_index; }

private:
  // The index of the channel in the array of channel_infos.
  channel_index_t m_channel_index;

  // The instruction which creates the channel.
  const CallBase *m_creator;

  // The decoded arguments to the channel creation call.
  channel_create_args m_args;

  // The element size.
  uint32_t m_elem_size;

  // The number of elements.
  uint32_t m_num_elem;

  // The number of bytes in each element that are sideband
  uint32_t m_sideband_size;

  // The number of bytes in each element that are sideband signals
  uint32_t m_sideband_signals_size;

  // The context of the port feeding data into the channel.
  context_index_t m_writer_context_index;

  // The port feeding data into the channel.
  port_index_t m_writer_port_index;

  // The write export type.
  nanotube_channel_type_t m_write_export_type;

  // The context of the port taking data out of the channel.
  context_index_t m_reader_context_index;

  // The port taking data out of the channel.
  port_index_t m_reader_port_index;

  // The read export type.
  nanotube_channel_type_t m_read_export_type;
};

///////////////////////////////////////////////////////////////////////////

// Information about a thread.
struct thread_info
{
  thread_info(CallBase *call,
              thread_create_args &args,
              context_index_t context_index);

  CallBase *call() const { return m_call; }
  const thread_create_args &args() const { return m_args; }
  context_index_t context_index() const { return m_context_index; }

private:
  // The call instruction which created the thread.
  CallBase *m_call;

  // Information about the function and channels.
  thread_create_args m_args;

  // The associated context.
  context_index_t m_context_index;
};

///////////////////////////////////////////////////////////////////////////

// Information about a map
struct map_info
{
  map_info(map_index_t idx, CallBase *call, setup_func_builder* builder);
  map_index_t index() const {return m_idx;}
  CallBase *creator() const {return m_creator;}
  const map_create_args &args() const {return m_args;}
  context_index_t context_index() const {return m_ctx_idx;}

private:
  map_index_t m_idx;         /* index into stage_func::map_infos */
  CallBase*   m_creator;     /* call to map_create that created this map */
  map_create_args m_args;    /* id, type, key & value size */
  context_index_t m_ctx_idx; /* context this map is associated with */
};

///////////////////////////////////////////////////////////////////////////

// Kernel information
struct kernel_info
{
  kernel_info(kernel_index_t idx, CallBase* call, setup_func_builder* builder);
  kernel_index_t index() const {return m_idx;}
  CallBase *creator() const {return m_creator;}
  const add_plain_kernel_args &args() const {return m_args;}
private:
  kernel_index_t        m_idx;
  CallBase*             m_creator;
  add_plain_kernel_args m_args;
};

///////////////////////////////////////////////////////////////////////////


// Integers are represented using APInt.
typedef APInt setup_int_value_t;

// Pointers are represented by the address.
typedef uint64_t setup_ptr_value_t;
static const int setup_ptr_value_num_bits = 64;

// A memset initialised region is represented by the integer fill
// value.
typedef uint8_t setup_memset_value_t;

// An allocation is identified by its base pointer.
typedef setup_ptr_value_t setup_alloc_id_t;

class setup_value {
public:
  // Indicates what kind a value is.
  typedef enum {
    // A well defined value which is too complicated to express.
    KIND_UNKNOWN,
    // An undefined value.  Can be assumed to be equal to any value.
    KIND_UNDEFINED,
    // An integer value.
    KIND_INT,
    // A pointer value.
    KIND_PTR,
    // A repeating byte.
    KIND_MEMSET,
    // A channel pointer.
    KIND_CHANNEL,
    // A context pointer.
    KIND_CONTEXT,
    // A map
    KIND_MAP,
  } kind_t;

  explicit setup_value(): m_kind(KIND_UNKNOWN) {}

  static setup_value unknown() {
    return setup_value(KIND_UNKNOWN, APInt(8, 0));
  }

  static setup_value undefined() {
    return setup_value(KIND_UNDEFINED, APInt(8, 0));
  }

  static setup_value of_int(setup_int_value_t value) {
    return setup_value(KIND_INT, value);
  }

  static setup_value of_ptr(setup_ptr_value_t value) {
    return setup_value(KIND_PTR, APInt(setup_ptr_value_num_bits, value));
  }

  static setup_value of_memset(setup_memset_value_t value) {
    return setup_value(KIND_MEMSET, APInt(8, value));
  }

  static setup_value of_channel(channel_index_t channel) {
    return setup_value(KIND_CHANNEL, APInt(channel_index_bits, channel));
  }

  static setup_value of_context(context_index_t context) {
    return setup_value(KIND_CONTEXT, APInt(context_index_bits, context));
  }

  static setup_value of_map(map_index_t idx) {
    return setup_value(KIND_MAP, APInt(map_index_bits, idx));
  }

  bool operator==(const setup_value &other) const {
    return ( m_kind == other.m_kind && m_value == other.m_value );
  }
  bool operator!=(const setup_value &other) const {
    return !(*this == other);
  }

  kind_t get_kind() const { return m_kind; }

  void set_unknown() { m_kind = KIND_UNKNOWN; }
  bool is_unknown() const { return m_kind == KIND_UNKNOWN; }

  void set_undefined() { m_kind = KIND_UNDEFINED; }
  bool is_undefined() const { return m_kind == KIND_UNDEFINED; }

  void set_int(setup_int_value_t value) {
    m_kind = KIND_INT;
    m_value = value;
  }
  bool is_int() const { return m_kind == KIND_INT; }
  setup_int_value_t get_int() const { return m_value; }

  void set_ptr(setup_ptr_value_t value) {
    m_kind = KIND_PTR;
    m_value = value;
  }
  bool is_ptr() const { return m_kind == KIND_PTR; }
  setup_ptr_value_t get_ptr() const { return m_value.getZExtValue(); }

  void set_memset(setup_memset_value_t value) {
    m_kind = KIND_MEMSET;
    m_value = APInt(8, value, false);
  }
  bool is_memset() const { return m_kind == KIND_MEMSET; }
  setup_memset_value_t get_memset() const {
    return m_value.getLimitedValue();
  }

  bool is_channel() const { return m_kind == KIND_CHANNEL; }
  channel_index_t get_channel() const {
    return m_value.getLimitedValue();
  }

  bool is_context() const { return m_kind == KIND_CONTEXT; }
  context_index_t get_context() const {
    return m_value.getLimitedValue();
  }

  bool is_map() const { return m_kind == KIND_MAP; }
  map_index_t get_map() const {
    return m_value.getLimitedValue();
  }

private:
  setup_value(kind_t kind, APInt value): m_kind(kind), m_value(value) {}

  kind_t m_kind;
  APInt m_value;
};

///////////////////////////////////////////////////////////////////////////

class setup_tracer
{
public:
  virtual void process_alloca(
    setup_func_builder *builder,
    AllocaInst *insn,
    setup_value num_elem,
    setup_alloc_id_t alloc_id);

  virtual void process_channel_create(
    setup_func_builder *builder,
    CallBase *insn,
    channel_info *info,
    channel_index_t result) = 0;

  virtual void process_channel_set_attr(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value index,
    nanotube_channel_attr_id_t attr_id,
    int32_t attr_val) = 0;

  virtual void process_channel_export(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value index,
    nanotube_channel_type_t type,
    nanotube_channel_flags_t flags) = 0;

  virtual void process_context_create(
    setup_func_builder *builder,
    CallBase *insn,
    context_info *info,
    context_index_t result) = 0;

  virtual void process_context_add_channel(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value context_index,
    setup_value channel_id,
    setup_value channel_index,
    nanotube_channel_flags_t flags) = 0;

  virtual void process_malloc(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value size_val,
    setup_alloc_id_t alloc_id) = 0;

  virtual void process_memcpy(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value dest,
    setup_value src,
    setup_value size) = 0;

  virtual void process_memset(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value dest,
    setup_value val,
    setup_value size) = 0;

  virtual void process_store(
    setup_func_builder *builder,
    StoreInst *insn,
    setup_value pointer,
    setup_value data) = 0;

  virtual void process_thread_create(
    setup_func_builder *builder,
    CallBase *insn,
    setup_value context_index,
    thread_info *info) = 0;

  virtual void process_map_create(
    setup_func_builder *builder,
    CallBase *insn,
    map_index_t map_index,
    map_info *info) = 0;

  virtual void process_context_add_map(
    setup_func_builder *builder,
    CallBase *insn,
    context_index_t ctx_index,
    map_index_t map_index) = 0;

  virtual void process_add_plain_kernel(
    setup_func_builder *builder,
    CallBase *insn,
    kernel_index_t kernel_index,
    kernel_info *info) = 0;
};

///////////////////////////////////////////////////////////////////////////

class setup_alloc_iterator;

// The setup function.
class setup_func
{
public:
  // The type of the channel info vector.
  typedef SmallVector<channel_info, 8> channel_info_vec_t;

  // The type of the context info vector.
  typedef SmallVector<context_info, 8> context_info_vec_t;

  // The type of the thread info vector.
  typedef SmallVector<thread_info, 8> thread_info_vec_t;

  typedef SmallVector<map_info, 8> map_info_vec_t;
  typedef SmallVector<kernel_info, 8> kernel_info_vec_t;

  // Find the setup function.
  static Function &get_setup_func(Module &m);

  // Construct a setup function.
  //
  // \param func    The LLVM function.
  // \param tracer  The tracer that gets invoked when parsing the setup
  //                function.
  // \param strict  Should this parser complain about calls which should not be
  //                present after linking and inlining the taps?
  setup_func(Module &m, setup_tracer *tracer=nullptr, bool strict = true);

  // Get the LLVM function.
  Function &get_function() { return m_func; }

  // Get the context info vector.
  const context_info_vec_t &contexts() const {
    return m_context_infos;
  }

  // Get information about a context.
  context_info &get_context_info(context_index_t context_index);

  // Get the channel info vector.
  const channel_info_vec_t &channels() const {
    return m_channel_infos;
  }

  // Get information about a channel.
  channel_info &get_channel_info(channel_index_t channel_index);

  const thread_info_vec_t &threads() const {
    return m_thread_infos;
  }

  // The information about a thread.
  thread_info &get_thread_info(thread_id_t thread_id) {
    return m_thread_infos[thread_id];
  }

  const map_info_vec_t &maps() const {
    return m_map_infos;
  }

  const map_info & get_map_info(map_index_t map_idx) {
    return m_map_infos[map_idx];
  }

  const map_info &get_map_info(context_index_t ctx_id, nanotube_map_id_t uid) {
    return get_map_info(m_context_infos[ctx_id].map_user_id_to_idx(uid));
  }

  const kernel_info_vec_t &kernels() const {
    return m_kernel_infos;
  }

  const kernel_info & get_kernel_info(kernel_index_t kernel_idx) {
    return m_kernel_infos[kernel_idx];
  }

  // Iterate over the allocactions.
  llvm::iterator_range<setup_alloc_iterator> allocs() const;

  // Print the memory contents.
  void print_memory_contents(raw_os_ostream &output);

  // Return the null allocation.
  setup_alloc_id_t get_null_alloc() const {
    return setup_alloc_id_t(0);
  }

  // Find the allocation for a global variable, create one if
  // necessary.
  setup_alloc_id_t find_alloc_of_var(GlobalVariable *global_var);

  // Find the allocation for a pointer.
  setup_alloc_id_t find_alloc_of_ptr(setup_ptr_value_t ptr) const {
    return setup_alloc_id_t(get_alloc_base_of_ptr(ptr));
  }

  // Find the base address of the allocation which contains the
  // pointer.
  setup_ptr_value_t get_alloc_base_of_ptr(setup_ptr_value_t ptr) const;

  // Find the end address of the allocation which contains the
  // pointer.
  setup_ptr_value_t get_alloc_end_of_ptr(setup_ptr_value_t ptr) const;

  // Find the base address of an allocation.
  setup_ptr_value_t get_alloc_base(setup_alloc_id_t alloc) const {
    return alloc;
  }
  // Find the end address of an allocation.
  setup_ptr_value_t get_alloc_end(setup_alloc_id_t alloc) const {
    return get_alloc_end_of_ptr(alloc);
  }

  // Get an iterator to the memory at a pointer.
  setup_func_memory_iterator memory_at(setup_ptr_value_t ptr) const;

  // Try to extract the byte at the specified offset from value.
  std::pair<bool, uint8_t> try_extract_byte(const setup_value &value,
                                            setup_ptr_value_t offset);

  // Try to load size bytes from ptr into buffer.
  bool try_load_bytes(uint8_t *buffer, setup_ptr_value_t ptr,
                      setup_ptr_value_t size);

  // Load an integer of type ty from ptr.  Returns unknown on
  // failure.
  setup_value load_integer(IntegerType *ty, setup_ptr_value_t ptr);

  // Load an pointer of type ty from ptr.  Returns unknown on
  // failure.
  setup_value load_pointer(PointerType *ty, setup_ptr_value_t ptr);

  // Load a value of type ty from ptr.  Returns unknown on failure.
  setup_value load_value(Type *ty, setup_ptr_value_t ptr);

  // Convert a setup value to an LLVM value.  Returns nullptr if the
  // value cannot be converted to the requested type.
  Value *convert_value(Type *ty, const setup_value &value);

private:
  friend class setup_func_builder;
  friend class setup_func_memory_iterator;
  friend class setup_alloc_iterator;

  typedef struct {
    // The address just past the end of the region.
    setup_ptr_value_t end;

    // The offset into the write.  A single write can be broken up
    // into multiple regions in which case the offset indicates the
    // difference between the start of the write and the start of the
    // region.
    setup_ptr_value_t write_offset;

    // The value which was stored.  This member also indicate what
    // kind of value was stored.
    setup_value write_value;
  } memory_region_t;

  typedef std::map<setup_ptr_value_t, memory_region_t> region_map_t;

  // The contents of all the memory allocations.
  typedef struct {
    setup_ptr_value_t end;
    Value *info;
  } alloc_contents_t;

  typedef std::map<setup_alloc_id_t, alloc_contents_t> alloc_map_t;

  alloc_map_t::const_iterator find_alloc(setup_ptr_value_t ptr) const;

public:
  // Perform a memory allocation.  This is called when an alloca,
  // malloc, thread_create or global variable is encountered.
  setup_alloc_id_t do_alloc(APInt size, Value *info);

private:
  // Perform a memcpy operation between regions of memory.
  void do_memcpy(setup_ptr_value_t dest, setup_ptr_value_t src,
                 setup_ptr_value_t size);

  // Perform a memset operation on a region of memory.
  void do_memset(setup_ptr_value_t ptr, setup_ptr_value_t size,
                 uint8_t value);

  // Erase a region of memory.
  void erase_region(setup_ptr_value_t ptr, setup_ptr_value_t size);

  // Perform a store operation on a region of memory.
  void do_store(setup_ptr_value_t ptr, setup_ptr_value_t size,
                setup_value value);

  // Perform a store operation of an LLVM constant to memory.
  void do_store(setup_ptr_value_t ptr, Constant *cst);

  // The LLVM function.
  Function &m_func;

  // The DataLayout.
  const DataLayout *m_data_layout;

  // The maximum number of bits required for a pointer.
  unsigned m_max_ptr_bits;

  // The contexts.
  context_info_vec_t m_context_infos;

  // The channels.
  channel_info_vec_t m_channel_infos;

  // The threads.
  thread_info_vec_t m_thread_infos;

  map_info_vec_t m_map_infos;

  kernel_info_vec_t m_kernel_infos;

  // The contents of memory after the setup function has run.
  region_map_t m_region_map;

  // The memory which is allocated after the setup function has run.
  alloc_map_t m_alloc_map;

  // The allocation ID for each global variable.
  std::map<const GlobalVariable *, setup_alloc_id_t> m_gv_allocs;
};

///////////////////////////////////////////////////////////////////////////

class setup_alloc_iterator
{
public:
  struct entry {
    setup_alloc_id_t id;
    setup_ptr_value_t base;
    setup_ptr_value_t end;
  };
  entry operator*() {
    return entry { m_it->first, m_it->first, m_it->second.end };
  }
  setup_alloc_iterator &operator++() {
    ++m_it;
    return *this;
  }
  bool operator!=(const setup_alloc_iterator &other) const {
    return m_it != other.m_it;
  }

private:
  friend class setup_func;

  setup_alloc_iterator(setup_func::alloc_map_t::const_iterator it):
    m_it(it) {}
  setup_func::alloc_map_t::const_iterator m_it;
};

///////////////////////////////////////////////////////////////////////////

class setup_func_memory_iterator
{
public:
  struct value_type {
    setup_value write_value;
    setup_ptr_value_t write_offset;
  };

  setup_func_memory_iterator(const setup_func *func,
                             setup_ptr_value_t ptr);

  bool operator==(const setup_func_memory_iterator &other) const {
    return (m_ptr == other.m_ptr &&
            m_setup_func == other.m_setup_func);
  }
  bool operator!=(const setup_func_memory_iterator &other) const {
    return !(*this == other);
  }

  setup_func_memory_iterator &operator++();
  const value_type &operator*() { return m_value; }
  const value_type *operator->() { return &m_value; }

private:
  const setup_func *m_setup_func;
  setup_ptr_value_t m_ptr;
  setup_func::region_map_t::const_iterator m_iter;
  value_type m_value;

  void update_value();
};

///////////////////////////////////////////////////////////////////////////

}

namespace llvm {
llvm::raw_ostream &operator<<(llvm::raw_ostream& os,
                              const nanotube::setup_value &val);
}

///////////////////////////////////////////////////////////////////////////

#endif // NANOTUBE_SETUP_FUNC_HPP
