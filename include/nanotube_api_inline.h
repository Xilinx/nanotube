/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

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
//#define MAGIC_INLINE static inline
#define MAGIC_INLINE __attribute__((always_inline)) inline static

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
                         size_t data_length) {
  return nanotube_map_op(ctx, map, NANOTUBE_MAP_READ, key, key_length,
                         NULL /* data_in */, data_out,
                         NULL /* enables */, offset, data_length);
}

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
                         size_t data_length) {
  size_t mask_size = (data_length + 7) / 8;
  uint8_t* all_one = (uint8_t*)alloca(mask_size);
  memset(all_one, 0xff, mask_size);
  return nanotube_map_op(ctx, map, NANOTUBE_MAP_WRITE, key, key_length,
                         data_in, NULL /* data_out */,
                         all_one,  offset, data_length);
}

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
                                 size_t data_length) {
    return nanotube_map_op(ctx, map, NANOTUBE_MAP_WRITE, key, key_length,
                           data_in, NULL /* data_out */,
                           enables,  offset, data_length);
}

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
                                  size_t data_length) {
    return nanotube_map_op(ctx, map, NANOTUBE_MAP_INSERT, key, key_length,
                           data_in, NULL /* data_out */,
                           enables,  offset, data_length);
}

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
                                  size_t data_length) {
    return nanotube_map_op(ctx, map, NANOTUBE_MAP_UPDATE, key, key_length,
                           data_in, NULL /* data_out */,
                           enables,  offset, data_length);
}

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
                           const uint8_t* key, size_t key_length) {
    return nanotube_map_op(ctx, map, NANOTUBE_MAP_REMOVE, key, key_length,
                           NULL /* data_in */, NULL /* data_out */,
                           NULL /* enables */,  0 /* offset */,
                           0 /* data_length */);
}
