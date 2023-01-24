#ifndef NANOTUBE_API_H
#define NANOTUBE_API_H
/*******************************************************/
/*! \file nanotube_api.h
** \author Neil Turton, Stephan Diestelhorst <stephand@amd.com>
**  \brief Interfaces for Nanotube packets and maps.
**   \date 2020-02-10
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <alloca.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>


/**
** Note: There are various levels of interfaces that the Nanotube API
** provides.  These are there to aid with code written at various
** abstraction levels and its translation.
**
** Level 1 - Pointer-based interface
**
**   This level provides a very high-level abstraction for the programmer.
**   Both packets and map entries are handled as single memory blocks, and
**   are modified throgh loads and stores on that memory.  The application
**   does not have to worry about packetisation of packet data, or explicit
**   read / write interfaces.
**
**  Provided functions are XXX.  Sizes are XXX.
**
** Level 2 - Explicit packet-level interface
**
**   This interface provides explicit read / write functions to access data
**   in packets and associated with specific map keys.  Note that packet
**   data is still flat, and not split into chunks / words.
**   The programmer has to (1) explicitly mark accesses as packet / map
**   accesses, whether they read / write, which offsets, sizes, and
**   crucially, tie the map look-up key to the actual access; going from a
**   "lookup, then access" style interface to an access-key interface that
**   directly ties the key to the access operation.
**
**   XXX: One potential intermediate step could be to introduce level 1.5
**   for map accesses where the lookup returns a handle that is then fed to
**   an explicit access function later.
**
** Level 3 - Explicit word-level interface
**
**   This interface reflects on the fact that packets will be split into
**   words and special care needs to be taken to ensure that accesses
**   happen to the right word of the packet.
**/

#ifdef __cplusplus
extern "C" {
#endif

/******************** Return codes ********************/

/*! Kernels can return one of these to indicate the fate of the packet */
typedef enum {
  NANOTUBE_PACKET_PASS = 0x0,
  NANOTUBE_PACKET_DROP = 0x1,
} nanotube_kernel_rc_t;

/******************** Types ********************/

typedef uint32_t nanotube_channel_id_t;

typedef uint8_t nanotube_packet_port_t;
/*! A special port number for control packets. */
static const nanotube_packet_port_t NANOTUBE_PORT_CONTROL = 0xfe;

typedef uint16_t nanotube_map_id_t;
static const nanotube_map_id_t MAP_ID_NONE = (nanotube_map_id_t)-1;

typedef struct nanotube_channel nanotube_channel_t;
typedef struct nanotube_context nanotube_context_t;
typedef struct nanotube_map     nanotube_map_t;
typedef struct nanotube_packet  nanotube_packet_t;

typedef uint32_t nanotube_channel_flags_t;
static const nanotube_channel_flags_t NANOTUBE_CHANNEL_READ  = (1<<0);
static const nanotube_channel_flags_t NANOTUBE_CHANNEL_WRITE = (1<<1);

typedef enum {
  /*! Indicates no type.  This is not usually valid. */
  NANOTUBE_CHANNEL_TYPE_NONE = 0,
  /*! Indicates a simple packet interface. */
  NANOTUBE_CHANNEL_TYPE_SIMPLE_PACKET = 1,
  /*! Indicates a softhub packet interface. */
  NANOTUBE_CHANNEL_TYPE_SOFTHUB_PACKET = 2,
  /*! Indicates a X3RX packet interface */
  NANOTUBE_CHANNEL_TYPE_X3RX_PACKET = 3,
} nanotube_channel_type_t;

/* The following attributes control how the channel is represented by
 * an underlying AXIS interface
 * If neither attribute is specified (i.e. both default 0) the whole
 * bus word will be encapsulated as data bytes on the AXIS interface.
 * 
 * If SIDEBAND_BYTES is defined, additional bytes are provided (e.g. 
 * for metadata) using TUSER in the AXIS interface.
 * 
 * If SIDEBAND_SIGNALS is defined, the AXIS strb/keep/last signals are
 * provided.  The number of bytes required for these is:
 *   - strb: axis_data_width / 8
 *   - keep: axis_data_width / 8
 *   - last: 1 (technically only 1 bit is required, but a whole byte 
 *              is used in the internal flat representation)
 * 
 * We convert the structured AXIS definition into a flat byte array 
 * internally with the following layout:
 * 0..(data_width - 1)  : data bytes
 * +..sideband_bytes    : TUSER sideband bytes
 * +..sideband_signals  : TKEEP/TSTRB/TLAST sideband signals
 *
 * Within the sideband_signals section of the flat byte array they are
 * stored in order: 
 * (data_width/8) bytes TKEEP
 * (data_width/8) bytes TSTRB
 * 1 byte TLAST
 * 
 * The total element size is then:
 * data_width + sideband_bytes + sideband_signals 
 *  = data_width + sideband_bytes + (data_width/8)*2 + 1
 */
typedef enum {
  /* Indicates the number of bytes to provision to store sideband data,
   * in addition to the standard data bytes */
  NANOTUBE_CHANNEL_ATTR_SIDEBAND_BYTES = 0,
  /* Indicates the number of bytes required to support sideband signals,
   * in addition to the standard data bytes.
   * In AXIS terms these are strb/keep/last and their size is a function
   * of the data width of the axis bus */
  NANOTUBE_CHANNEL_ATTR_SIDEBAND_SIGNALS = 1,
} nanotube_channel_attr_id_t;

/******************** The setup function ********************/

/*!
** A function called to create the user design.
**
** This function is defined by the user and called by the Nanotube
** library.  It should create all the resources needed to implement
** the user's design.
*/
void nanotube_setup(void);

/******************** Memory allocation ********************/

/*!
** Allocate memory during setup.
**
** This function allocates memory which can be used by threads and
** kernels.  It should only be called from the setup function, either
** directly or indirectly.  The returned memory has the specified size
** and will be initialised to zero.  It will be valid until all
** threads and kernels have stopped running.  The memory is freed
** automatically when the system is destroyed.  If allocation fails
** then a message is displayed and the program exits.
**
** \param size The number of bytes to allocate.
** \returns a pointer to the allocated memory.
*/
void *nanotube_malloc(size_t size);

/******************** Contexts ********************/

/*!
** Create a Nanotube context for holding thread resources.  Each
** thread must use a different context.
**
** \returns A new context.
**/
nanotube_context_t *nanotube_context_create(void);

void
nanotube_context_add_channel(nanotube_context_t* context,
                             nanotube_channel_id_t channel_id,
                             nanotube_channel_t* channel,
                             nanotube_channel_flags_t flags);


/* Let's keep the map ID inside the map and have it with map_create. */
void
nanotube_context_add_map(nanotube_context_t* context,
                         /*nanotube_map_id_t   map_id,*/
                         nanotube_map_t*     map);

/******************** Channels ********************/

/*!
** Create a Nanotube channel for communication between threads.
**
** A channel acts as a FIFO and can be used to communicate between
** Nanotube threads or within a thread.  The elements in the channel
** must all be the same size.
**
** \param name A descriptive name for the channel.
** \param elem_size The size of each element in the channel.
** \param num_elem The minimum number of elements which can fit in the
**   channel.
** \return A pointer to the new channel.
**/
nanotube_channel_t* nanotube_channel_create(const char* name,
                                            size_t elem_size,
                                            size_t num_elem);


/*!
** Set attributes on a channel
**
** \param channel The channel to modify the attributes of
** \param attr_id The attribute to set
** \param attr_val The value of the attribute
**/
int32_t 
nanotube_channel_set_attr(nanotube_channel_t *channel,
                          nanotube_channel_attr_id_t attr_id,
                          int32_t attr_val);

/*!
** Make a channel available to the external environment.
**
** This function exports a channel externally and specifies how the
** channel will be used.
**
** \param channel  The channel to export.
** \param type     Indicates the format of channel words.
** \param flags    Indicates the type of the external accesses.
**/
void
nanotube_channel_export(nanotube_channel_t *channel,
                        nanotube_channel_type_t type,
                        nanotube_channel_flags_t flags);


/*!
** Blocking read of an element from a Nanotube channel.
**
** Waits until an element is available for reading from the channel
** and then reads it into the buffer provided.
**
** \param context A context which holds the channel.
** \param data The buffer to be written with the element.
** \param data_size The size of the buffer in bytes.
**/
void nanotube_channel_read(nanotube_context_t* context,
                           nanotube_channel_id_t channel_id,
                           void* data,
                           size_t data_size);

/*!
** Non-blocking read of an element from a Nanotube channel.
**
** Reads an element from a Nanotube channel if there is one available.
** If no element is available then the thread is marked as the reader
** of the channel.  This means that a wake-up will be send to the
** thread when an element becomes available.  The wake-up will wake
** the next call to nanotube_thread_wait and will only be when the
** thread wakes up from a blocking call.  If no element is available
** then the buffer is cleared to zero.
**
** \param context A context which holds the channel.
** \param channel_id The ID of the channel within the context.
** \param data The buffer to be written with the element.
** \param data_size The size of the buffer in bytes.
**
** \return zero if there were no elements available and a non-zero
** value if an element was read.
**/
int nanotube_channel_try_read(nanotube_context_t* context,
                              nanotube_channel_id_t channel_id,
                              void* data,
			      size_t data_size);

/*!
** Blocking write of an element to a Nanotube channel.
**
** Waits until there is enough space in the channel and then writes
** the element from the buffer to the channel.
**
** \param context A context which holds the channel.
** \param channel_id The ID of the channel within the context.
** \param data The buffer to be written with the element.
** \param data_size The size of the buffer in bytes.
**/
void nanotube_channel_write(nanotube_context_t* context,
                            nanotube_channel_id_t channel_id,
                            const void* data,
                            size_t data_size);

/*!
** Determine whether there is space to write elements to the channel.
**
** This function indicates whether there is enough space to write an
** element to the channel.  If there is not enough space then the
** current thread is marked as the current writer of the channel.
** This means that a wake-up will be sent to the thread when space
** becomes available.
**
** \param context A context which holds the channel.
** \param channel_id The ID of the channel within the context.
** \returns Zero if there is not enough space and a non-zero value
** otherwise.
**/
int nanotube_channel_has_space(nanotube_context_t* context,
                               nanotube_channel_id_t channel_id);


/******************** Packets ********************/

typedef uint16_t nanotube_packet_size_t;
//XXX: Convert interfaces below!

/*!
** Get access to the payload data of a packet.
** \param packet Input packet
** \return Pointer to the packet payload data
**/
extern
uint8_t* nanotube_packet_data(nanotube_packet_t* packet);

extern
uint8_t* nanotube_packet_end(nanotube_packet_t* packet);

/*!
** Get access to the metadata of a packet. The end of the metadata
** is implicitly the start of the payload data.
** TODO refactor this into nanotube_packet_data with a section argument.
** \param packet Input packet
** \return Pointer to the packet metadata
**/
extern
uint8_t* nanotube_packet_meta(nanotube_packet_t* packet);


/*!
** Get a bounded length of the specified packet.
** \param packet     The input packet
** \param max_length The maximum length supported by the caller
** \return MIN(packet length, max_length)
**/
extern
size_t nanotube_packet_bounded_length(nanotube_packet_t* packet,
                                      size_t max_length);

/*!
** Get the destination port of the specified packet.
** \param packet  The packet to examine
** \return        The destination port of the packet.
**/
extern nanotube_packet_port_t
nanotube_packet_get_port(nanotube_packet_t* packet);

/*!
** Set the destination port of the specified packet.
** \param packet  The packet to modify
** \param port    The new destination port
**/
extern void
nanotube_packet_set_port(nanotube_packet_t* packet,
                         nanotube_packet_port_t port);

/*!
** Read from a Nanotube packet payload.
** \param packet   The packet to read from
** \param data_out Caller-supplied buffer to store the data in
** \param offset   Offset in bytes in the packet to read from
** \param length   Number of bytes to read. Note: buffer needs to be at
**                 least this large!
** \return Number of bytes read
**/
extern
size_t nanotube_packet_read(nanotube_packet_t* packet, uint8_t* data_out,
                            size_t offset, size_t length);

/*!
** Plain write to a Nanotube packet.
** \param packet  The packet to write to
** \param data_in Data to be written (including data that is not written
**                due to mask!)
** \param offset  Offset in bytes in the packet to perform the write
** \param length  Total size of data in data_in (including data that will
**                not be written due to mask.)
** \return Number of bytes processed
**/
extern
size_t nanotube_packet_write(nanotube_packet_t* packet,
                             const uint8_t* data_in, size_t offset,
                             size_t length);
/*!
** Write to a Nanotube packet with a write-enable mask.
** \param packet  The packet to write to
** \param data_in Data to be written (including data that is not written
**                due to mask!)
** \param mask    Byte enables: each set bit in mask enables the
**                corresponding byte to be written. The offsets are
**                relative to data_in.
** \param offset  Offset in bytes in the packet to perform the write
** \param length  Total size of data in data_in (including data that will
**                not be written due to mask.)
** \return Number of bytes processed
**/
extern
size_t nanotube_packet_write_masked(nanotube_packet_t* packet,
                                    const uint8_t* data_in,
                                    const uint8_t* mask,
                                    size_t offset, size_t length);

/*!
** Add or remove bytes in a packet's payload at a given offset.
** Applications must get new data pointers after calling this.
** \param packet   The packet to adjust.
** \param offset   The offset at which to perform the adjustment.
** \param adjust   The number of bytes added (positive) or removed
**                 (negative) from packet.
** \return         Non-zero if the adjust operation was successful, zero
**                 otherwise.
**/
extern
int nanotube_packet_resize(nanotube_packet_t* packet,
                           size_t offset, int32_t adjust);

/*!
** Add or remove bytes in a packet's metadata at a given offset.
** Applications must get new data pointers after calling this.
** \param packet   The packet to adjust.
** \param offset   The offset at which to perform the adjustment.
** \param adjust   The number of bytes added (positive) or removed
**                 (negative) from packet.
** \return         Non-zero if the adjust operation was successful, zero
**                 otherwise.
**/
extern
int32_t nanotube_packet_meta_resize(nanotube_packet_t *packet,
                                    size_t offset, int32_t adjust);

/*! Indicates the class of a capsule. */
typedef enum {
  /*! The capsule should be passed through. */
  NANOTUBE_CAPSULE_CLASS_PASS_THROUGH,
  /*! The capsule is a network packet. */
  NANOTUBE_CAPSULE_CLASS_NETWORK,
  /*! The capsule is a control capsule. */
  NANOTUBE_CAPSULE_CLASS_CONTROL,
} nanotube_capsule_class_t;

/*! Determine the class of a capsule. */
extern nanotube_capsule_class_t
nanotube_capsule_classify(nanotube_packet_t* packet);


/******************** Maps ********************/
enum map_type_t {
  NANOTUBE_MAP_TYPE_ILLEGAL = -1,
  NANOTUBE_MAP_TYPE_HASH,
  NANOTUBE_MAP_TYPE_LRU_HASH,   /* Unimplemented */
  NANOTUBE_MAP_TYPE_ARRAY_LE,
};

static const uint32_t nanotube_array_map_capacity = 32;

/*!
** Create a new map with a specific map-ID, and key / value sizes.
** \param id       Numerical ID of the map.
** \param type     Requested type of the map
** \param key_sz   Size of the keys in bytes.
** \param value_sz Size of the stored values in bytes.
**/
nanotube_map_t* nanotube_map_create(nanotube_map_id_t id,
                                    enum map_type_t type, size_t key_sz,
                                    size_t value_sz);

/*!
** Destroys the specified map.
** \param id       Map that should be destroyed.
**/
void nanotube_map_destroy(nanotube_map_t* map);

/*!
** Get the ID of a Nanotube map.
** \param map     Map of which to get the ID>
** \return        ID of the map.
**/
nanotube_map_id_t nanotube_map_get_id(nanotube_map_t* map);
static const
nanotube_map_id_t NANOTUBE_MAP_ILLEGAL = (nanotube_map_id_t)(-1);

/* Level 1 interface */

/*!
 * Lookup in a map and return a pointer to the matching entry.
 * \param XXX
 */
uint8_t* nanotube_map_lookup(nanotube_context_t* ctx,
                             nanotube_map_id_t map, const uint8_t* key,
                             size_t key_length, size_t data_length);

/** Level 2 & 3 Interface **/
enum map_access_t {
  NANOTUBE_MAP_READ,
  NANOTUBE_MAP_INSERT,      //< Entry must not exist and will be created
  NANOTUBE_MAP_UPDATE,      //< Entry must exist and will be updated
  NANOTUBE_MAP_WRITE,       //< Either add or overwrite entry
  NANOTUBE_MAP_REMOVE,      //< Remove an entry from the map
  NANOTUBE_MAP_NOP,
};

/*!
** Perform an operation (map_access_t) on the specified map.
** \param map_id     Map-ID of the map to work on
** \param type       Type of the access
** \param key        Key of the entry that will be accessed
** \param key_length Size of the key
** \param data_in    Data to be written (including data that is not written
**                   due to mask!)
** \param data_out   Buffer for data read by reads (caller allocated!)
** \param mask       Byte enables: each set bit in mask enables the
**                   corresponding byte to be written.  The offsets are
**                   relative to data__in.
** \param offset     Offset in bytes in the map value to access
** \param length     Total size of access (including data that will
**                   not be written due to mask.)
** \return Number of bytes processed
**/
extern
size_t nanotube_map_op(nanotube_context_t* ctx, nanotube_map_id_t map,
                       enum map_access_t type, const uint8_t* key,
                       size_t key_length, const uint8_t* data_in,
                       uint8_t* data_out, const uint8_t* mask,
                       size_t offset, size_t data_length);


/*!
** Note that the inlining treatment between Clang and GCC is very
** different.  GCC will inline the small stubs below and then *not* create
** full functions for these even without the static keyword.  Clag, on the
** other hand treats the "inline" keyword only as a hint for the inliner,
** and incase it does not inline (because we may want to compile the packet
** kernels with say -O0), it will expect a real definition of the function.
** Clang also ignores any sort of always_inline hint... *grrrr*
**
** Therefore in Clang:
** * use -finline-hint-functions
** * use static inline for the function (which should be safe for GCC,
**   too!)
**/
#ifndef DONT_INLINE_NANOTUBE_FUNCTIONS
//#define MAGIC_INLINE static inline
#define MAGIC_INLINE __attribute__((always_inline)) inline static
#else
#define MAGIC_INLINE
#endif

/*!
** Perform a map read operation, i.e., read the value belonging to key.
** \param map_id     Map-ID of the map to work on
** \param key        Key of the entry that will be accessed
** \param key_length Size of the key
** \param data_out   Buffer for data read by reads (caller allocated!)
** \param offset     Offset in bytes in the map value to access
** \param length     Total size of read
** \return Number of bytes read
**/
MAGIC_INLINE
size_t nanotube_map_read(nanotube_context_t* ctx, nanotube_map_id_t map,
                         const uint8_t* key, size_t key_length,
                         uint8_t* data_out, size_t offset,
                         size_t data_length);
/*!
** Perform a map write operation, i.e., read the value belonging to key.
** \param map_id     Map-ID of the map to work on
** \param key        Key of the entry that will be accessed
** \param key_length Size of the key
** \param data_in    Data to be written
** \param offset     Offset in bytes in the map value to access
** \param length     Total size of write
** \return Number of bytes processed
**/
MAGIC_INLINE
size_t nanotube_map_write(nanotube_context_t* ctx, nanotube_map_id_t map,
                         const uint8_t* key, size_t key_length,
                         const uint8_t* data_in, size_t offset,
                         size_t data_length);


/*!
** Write to the value associated with the specific key.
** \param map_id     Map-ID of the map to work on
** \param key        Key of the entry that will be accessed
** \param key_length Size of the key
** \param data_in    Data to be written (including data that is not written
**                   due to mask!)
** \param mask       Byte enables: each set bit in mask enables the
**                   corresponding byte to be written.  The offsets are
**                   relative to data__in.
** \param offset     Offset in bytes in the map value to access
** \param length     Total size of access (including data that will
**                   not be written due to mask.)
** \return Number of bytes processed
**/
MAGIC_INLINE
size_t nanotube_map_write_masked(nanotube_context_t* ctx,
                                 nanotube_map_id_t map, const uint8_t* key,
                                 size_t key_length, const uint8_t* data_in,
                                 const uint8_t* enables, size_t offset,
                                 size_t data_length);

/*!
** Insert a new entry with key and value to the map.
**
** If the entry is already present, nothing will get written.
** \param map_id     Map-ID of the map to work on
** \param key        Key of the entry that will be accessed
** \param key_length Size of the key
** \param data_in    Data to be written (including data that is not written
**                   due to mask!)
** \param mask       Byte enables: each set bit in mask enables the
**                   corresponding byte to be written.  The offsets are
**                   relative to data__in.
** \param offset     Offset in bytes in the map value to access
** \param length     Total size of access (including data that will
**                   not be written due to mask.)
** \return Number of bytes processed
**/
MAGIC_INLINE
size_t nanotube_map_insert_masked(nanotube_context_t* ctx,
                                  nanotube_map_id_t map, const uint8_t* key,
                                  size_t key_length, const uint8_t* data_in,
                                  const uint8_t* enables, size_t offset,
                                  size_t data_length);
/*!
** Update the value of an already-present entry in the map.
**
** If the entry is not present, it will not be added.
** \param map_id     Map-ID of the map to work on
** \param key        Key of the entry that will be accessed
** \param key_length Size of the key
** \param data_in    Data to be written (including data that is not written
**                   due to mask!)
** \param mask       Byte enables: each set bit in mask enables the
**                   corresponding byte to be written.  The offsets are
**                   relative to data__in.
** \param offset     Offset in bytes in the map value to access
** \param length     Total size of access (including data that will
**                   not be written due to mask.)
** \return Number of bytes processed
**/
MAGIC_INLINE
size_t nanotube_map_update_masked(nanotube_context_t* ctx,
                                  nanotube_map_id_t map, const uint8_t* key,
                                  size_t key_length, const uint8_t* data_in,
                                  const uint8_t* enables, size_t offset,
                                  size_t data_length);
/*!
** Remove an entry from the map.
**
** \param map_id     Map-ID of the map to work on
** \param key        Key of the entry that will be accessed
** \param key_length Size of the key
** \return Non-zero if the entry was present before removal.
**/
MAGIC_INLINE
size_t nanotube_map_remove(nanotube_context_t* ctx, nanotube_map_id_t map,
                           const uint8_t* key, size_t key_length);

/*!
** Process a map control capsule.
**
** \param ctxt        The Nanotube context.
** \param map_id      The map ID of the map to access.
** \param capsule     The buffer holding the capsule.
** \param key_len     The length of the map key.
** \param value_len   The length of the map value.
**/
void nanotube_map_process_capsule(nanotube_context_t *ctxt,
                                  nanotube_map_id_t map_id,
                                  uint8_t *capsule,
                                  size_t key_len,
                                  size_t value_len);

/*!
** Print out the map specified with map_id to stdout.
**
** \param map_id  Map-ID of the map that should be printed.
**/
extern
void nanotube_map_print(nanotube_map_t *map);

/*!
** Reads a map with content from stdin.
**
** The map is read in the same format as print above, created, and filled
** with content.  The map will then be available to read from / write to
** with the provideed map-id.
**
** \return Map-ID of the map that was read.
**/
extern
nanotube_map_t* nanotube_map_create_from_stdin(void);

/******************** Threads ********************/
typedef void nanotube_thread_func_t(nanotube_context_t* context,
                                    void* data);

/*!
** Create a Nanotube thread.
**
** The data block provided will be copied to a newly allocated region
** of memory. Then a new thread will be created which will call the
** specified function with a single argument pointing to the copied
** data block.
**
** \param context The context for the thread.
** \param name A descriptive name for the thread.
** \param func The function to call in the thread.
** \param data A pointer to the data to be copied.
** \param data_size The number of bytes to copy.
**/
void nanotube_thread_create(nanotube_context_t* context,
                            const char* name,
                            nanotube_thread_func_t* func,
                            const void* data,
                            size_t data_size);

/*!
** Suspend the current thread until more work can be done.
**
** Calling this function will suspend the current thread unless or
** until it has a wake-up.  Various non-blocking functions will cause
** a wake-up to be sent to the calling thread if the non-blocking
** function fails and then an event happens which would allow it to
** succeed.
**/
void nanotube_thread_wait(void);

/******************** Packet Kernel Interface ********************/

typedef struct nanotube_context nanotube_ccontext_t;

typedef nanotube_kernel_rc_t nanotube_kernel_t(nanotube_context_t *context,
                                               nanotube_packet_t *packet);
/*!
** Add a packet kernel to Nanotube.
**
** Plain packet kernels invoke the kernel function once for every packet.
**
** Capsule kernels are like packet kernels, except they inline the bus
** meta-data into the packet data and thus turning it into a capsule.
**
** Specify which (packet/capsule) using the capsule argument.  For capsule
** kernels the format of the capsule is indicated by bus_type
** \param name     The name of the kernel.
** \param kernel   Pointer to the kernel function.
** \param bus_type Format of the capsule to use
** \param capsules Set true for capsule kernel
**/
void nanotube_add_plain_packet_kernel(const char* name,
                                      nanotube_kernel_t* kernel,
                                      int bus_type,
                                      int capsules);

/******************** Time ********************/

/*!
** Returns the time in nanoseconds since boot.
**
** \return Nanoseconds since boot.
**/
uint64_t nanotube_get_time_ns(void);

/******************** Debug tracing ********************/

/*!
** Report a value to the debug log.
**
** \param id    An identifier for the value.
** \param value The value to log.
**/
void nanotube_debug_trace(uint64_t id, uint64_t value);

/*!
** Report the contents of a buffer to the debug log.
**
** \param id     An identifier for the value.
** \param buffer The buffer to report.
** \param size   The number of bytes in the buffer.
**/
void nanotube_trace_buffer(uint64_t id, uint8_t *buffer,
                           uint64_t size);


/******************** Utility functions ********************/
void
nanotube_merge_data_mask(uint8_t* inout_data, uint8_t* inout_mask,
                         const uint8_t* in_data, const uint8_t* in_mask,
                         size_t offset, size_t data_length);
/******************** Inline functions  ********************/

#ifndef DONT_INLINE_NANOTUBE_FUNCTIONS
#include "nanotube_api_inline.h"
#endif

#ifdef __cplusplus
} // extern "C"
#endif //__cplusplus

#endif // NANOTUBE_API_H
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
