/*******************************************************/
/*! \file nanotube_map.cpp
** \author Stephan Diestelhorst <stephand@amd.com>
**  \brief Implementations of Nanotube maps.
**   \date 2020-03-03
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include <assert.h>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <unordered_map>

#include "nanotube_api.h"
#include "nanotube_context.hpp"
#include "nanotube_map.hpp"
#include "nanotube_private.hpp"
#include "processing_system.hpp"

///////////////////////////////////////////////////////////////////////////

/*!
** The nanotube_hash_map class provides a hash-map with pointerised keys
** and values of flexible length.
**
** This class wraps around a std::unordered_map and provides hash functions
** and comparators for keys and values.  Per map, the user can specify the
** key and value sizes in bytes; the hash and equals functions will take
** those into accounts.
**/
struct nanotube_hash_map: public nanotube_map {

  /*!
  ** Construct a hash_map with user-defined key and value sizes.
  ** \param key_sz   Size of the key in bytes.
  ** \param value_sz Size of the value in bytes.
  **/
  nanotube_hash_map(nanotube_map_id_t id, size_t key_sz, size_t value_sz) :
    nanotube_map(id, key_sz, value_sz, 0, NANOTUBE_MAP_TYPE_HASH, 0),
    hash(key_sz),
    equals(key_sz),
    map(10, hash, equals) {}

  /*!
  ** Lookup a key with specified length in the map.
  ** \param key        Pointer to the key data.
  ** \return Pointer to the value, nullptr if not found.
  **/
  uint8_t* lookup(const uint8_t* key) override {
    if( key == nullptr )
      return nullptr;
    auto it = map.find(key_wrap(key));
    if( it == map.end() )
      return nullptr;
    return it->second.value;
  }

  /*!
  ** Insert an empty key into the map.
  **
  ** Note that this function creates a copy of the provided key, rather
  ** than using the provided pointer.  That means that the caller is free
  ** to use their input array for something else, and the stored key will
  ** not change.
  **/
  uint8_t* insert_empty(const uint8_t* key) override {
    auto value_size = value_sz;

    if( key == nullptr )
      return nullptr;

    auto it = map.find(key_wrap(key));
    if( it != map.end() )
      return nullptr;

    /* Allocate a new key and value for our storage */
    uint8_t* new_key   = new uint8_t[key_sz + value_size];
    uint8_t* new_value = new_key + key_sz;
    memcpy(new_key, key, key_sz);
    memset(new_value, 0, value_size);
    //std::cout << "Insert key: " << (void*)new_key << " value: "
    //     << (void*) new_value << '\n';
    map.emplace(new_key, new_value);
    return new_value;
  }

  /*!
  ** Remove a key-value entry from the map.
  **
  ** \param key Key that should get removed.
  ** \return was the entry found (and removed)?
  **/
  bool remove(const uint8_t* key) override {
    if( key == nullptr )
      return false;

    auto it = map.find(key_wrap(key));
    if( it == map.end() )
      return false;

    /* we created a copy of the key and value and store pointers to those
    ** in the hash map -> free the memory, key allocates for the whole
    ** thing */
    const uint8_t *storage = it->first.key;
    map.erase(it);
    delete storage;
    return true;
  }

  struct key_wrap {
    const uint8_t* key;
    key_wrap(const uint8_t* k) : key(k) {
      //std::cout << "New key: " << (void*) k << '\n';
    }
  };

  struct value_wrap {
    uint8_t* value;
    value_wrap(uint8_t* v) : value(v) {
      //std::cout << "New value: " << (void*)v << '\n';
    }
  };

  /*!
  ** Hash function for key_wrap.
  **
  ** Provide a hash function for a key_wrap of the size specified at the
  ** map's / key_hash's instantiation time.  The key_hash stores the size
  ** of the key, so it does not have to be stored per key_wrap.
  **/
  struct key_hash {
    size_t key_size;
    std::size_t operator() (const key_wrap& kw) const noexcept {
      std::size_t h = 0;
      for( size_t i = 0; i < key_size; ++i ) {
        std::size_t d = kw.key[i];
        /* According to the spec, the hash function does not have to be
         * cryptographically secure.  I think I got that one right ;) */
        h = (h << 13) | (h >> 51); /* rol13 */
        h ^= d | (d << 17);
      }
      return h;
    }
    key_hash(size_t ks) : key_size(ks) {}
  };

  struct key_equals {
    size_t key_size;
    bool operator() (const key_wrap& m, const key_wrap& n) const noexcept {
      // std::cout << (void*)k1.key << " " << (void*)k2.key << " "
      //      << key_size << '\n';
      return memcmp(m.key, n.key, key_size) == 0;
    }
    key_equals(size_t ks) : key_size(ks) {}
  };

  /*!
  ** Print the entries of this map to os.
  **/
  std::ostream& print_entries(std::ostream &os) const override {
    auto key_size   = key_sz;
    auto value_size = value_sz;

    os << std::hex << std::setfill('0');
    assert(key_size > 0);
    assert(value_size > 0);
    for( auto &e : map ) {
      os << "key: ";
      for( size_t i = 0; i < key_size ; ++i ) {
        os << std::setw(2) << (unsigned)e.first.key[i];

        /* Let's do some overly complex whitespace handling ;) */
        bool wrap = (i % 16 == 15);
        bool end  = (i == key_size - 1);

        if( !end ) {
          if( wrap )
            os << "\n     ";
          else
            os << " ";
        }
      }
      os << '\n';
      os << "value: ";
      for( size_t i = 0; i < value_size; ++i ) {
        os << std::setw(2) << (unsigned)e.second.value[i];

        bool wrap = (i % 16 == 15);
        bool end  = (i == value_size - 1);

        if( !end ) {
          if( wrap )
            os << "\n       ";
          else
            os << " ";
        }
      }
      os << '\n';
    }
    os << std::dec;
    return os;
  }

  key_hash   hash;
  key_equals equals;
  std::unordered_map<key_wrap, value_wrap, key_hash, key_equals> map;

};

///////////////////////////////////////////////////////////////////////////

/*!
** The nanotube_array_map class provides an array-map with pointerised keys
** and values of flexible length.
**
** This class wraps around a std::vector.  The elements are indexed by
**/
class nanotube_array_map: public nanotube_map
{
public:
  /*!
  ** Construct an array_map with user-defined key and value sizes.
  ** \param key_sz   Size of the key in bytes.
  ** \param value_sz Size of the value in bytes.
  **/
  nanotube_array_map(nanotube_map_id_t id, size_t key_sz,
                     size_t value_sz, size_t max_entries) :
    nanotube_map(id, key_sz, value_sz, max_entries,
                 NANOTUBE_MAP_TYPE_ARRAY_LE, 0),
    m_contents(value_sz*max_entries, 0)
  {
  }

  /*!
  ** Lookup a key with specified length in the map.
  ** \param key        Pointer to the key data.
  ** \return Pointer to the value, nullptr if not found.
  **/
  uint8_t* lookup(const uint8_t* key) override {
    if( key == nullptr )
      return nullptr;

    size_t index = 0;
    for (size_t i=key_sz; i!=0; ) {
      --i;
      if (index > max_entries>>8)
        return nullptr;
      index = (index<<8) | key[i];
    }
    if (index >= max_entries)
      return nullptr;

    return &(m_contents[index*value_sz]);
  }

  /*!
  ** Insert an empty key into the map.
  **
  ** Note that this function creates a copy of the provided key, rather
  ** than using the provided pointer.  That means that the caller is free
  ** to use their input array for something else, and the stored key will
  ** not change.
  **/
  uint8_t* insert_empty(const uint8_t* key) override {
    std::cerr << "ERROR: Attempt to insert an element into an"
              << " array map " << unsigned(id) << "\n";
    exit(1);
  }

  /*!
  ** Remove a key-value entry from the map.
  **
  ** \param key Key that should get removed.
  ** \return was the entry found (and removed)?
  **/
  bool remove(const uint8_t* key) override {
    std::cerr << "ERROR: Attempt to remove an entry from an"
              << " array map " << unsigned(id) << "\n";
    exit(1);
  }

  /*!
  ** Print the entries of this map to os.
  **/
  std::ostream& print_entries(std::ostream &os) const override {
    auto key_size   = key_sz;
    auto value_size = value_sz;

    os << std::hex << std::setfill('0');
    for( size_t index=0; index<max_entries; ++index ) {
      os << "key: ";
      size_t key = index;
      for( size_t i = 0; i < key_size ; ++i ) {
        os << std::setw(2) << (key & 0xff);
        key >>= 8;

        /* Let's do some overly complex whitespace handling ;) */
        bool wrap = (i % 16 == 15);
        bool end  = (i == key_size - 1);

        if( !end ) {
          if( wrap )
            os << "\n     ";
          else
            os << " ";
        }
      }
      os << '\n';

      const uint8_t *value = &(m_contents[index*value_size]);
      os << "value: ";
      for( size_t i = 0; i < value_size; ++i ) {
        os << std::setw(2) << unsigned(value[i]);

        bool wrap = (i % 16 == 15);
        bool end  = (i == value_size - 1);

        if( !end ) {
          if( wrap )
            os << "\n       ";
          else
            os << " ";
        }
      }
      os << '\n';
    }
    os << std::dec;
    return os;
  }

  std::vector<uint8_t> m_contents;
};

///////////////////////////////////////////////////////////////////////////

nanotube_map_t*
nanotube_map_create(nanotube_map_id_t id, enum map_type_t type,
                    size_t key_sz, size_t value_sz) {
  nanotube_map* map = NULL;
  switch( type ) {
  case NANOTUBE_MAP_TYPE_HASH:
    map = new nanotube_hash_map(id, key_sz, value_sz);
    break;
  case NANOTUBE_MAP_TYPE_ARRAY_LE:
    map = new nanotube_array_map(id, key_sz, value_sz,
                                 nanotube_array_map_capacity);
    break;
  default:
    assert(false && "Unknown map type!");
  }

  auto* sys = &processing_system::get_current();
  if( sys != nullptr )
    sys->add_map(std::unique_ptr<nanotube_map>(map));
  return map;
}

void nanotube_map_destroy(nanotube_map* map) {
  delete map;
}


size_t nanotube_map_op(nanotube_context_t* ctx, nanotube_map_id_t id,
                       enum map_access_t type, const uint8_t* key,
                       size_t key_length, const uint8_t* data_in,
                       uint8_t* data_out, const uint8_t* mask,
                       size_t offset, size_t data_length) {
  /* Short circuit no-ops, as they may have random other parameters */
  if( type == NANOTUBE_MAP_NOP )
    return 0;

  auto* map = ctx->get_map(id);
  if( map == nullptr )
    return 0;

  if (key_length != map->key_sz)
    return 0;

  //auto key_size   = map->key_sz;
  auto value_size = map->value_sz;

  /* When we can, overwrite the output */
  if( data_out != nullptr)
    memset(data_out, 0, data_length);

  if( key == nullptr )
    return 0;

  uint8_t* value = map->lookup(key);

  /* Read a found value for all operations. (NANO-274) */
  int len = 0;
  if( value != nullptr && data_out != nullptr ) {
    len = data_length;
    /* Allow reads with bigger buffers / requests, for now */
    if( offset + len > value_size )
      len = value_size - offset;
    if( len > 0 )
      memcpy(data_out, value + offset, len);
    else
      len = 0;
  }

  switch( type ) {
  case NANOTUBE_MAP_READ:
    return len;

  case NANOTUBE_MAP_WRITE:
  case NANOTUBE_MAP_INSERT:
  case NANOTUBE_MAP_UPDATE:
    if( offset + data_length > value_size )
      return 0;
    if( data_in == nullptr || mask == nullptr )
      return 0;

    /* Handle specialisations for key present / not present */
    /* UPDATE needs the entry present */
    if( value == nullptr && type == NANOTUBE_MAP_UPDATE )
      return 0;
    /* key is already present, INSERT does not overwrite */
    if( value != nullptr && type == NANOTUBE_MAP_INSERT)
        return 0;
    /* all other cases, create a new entry if needed */
    if( value == nullptr ) {
      value = map->insert_empty(key);
      if( value == nullptr )
        return 0;
    }

    assert(value);

    for( size_t i = 0; i < data_length; ++i ) {
      if( mask[i / 8] & (1u << (i % 8)) )
        value[offset + i] = data_in[i];
    }
    return data_length;

  case NANOTUBE_MAP_REMOVE: {
    if( value == nullptr )
      return 0;
    bool removed = map->remove(key);
    return removed ? SIZE_MAX : 0;
  }
  default:
    assert(false && "Unknown map op!");
  }
}

uint8_t* nanotube_map_lookup(nanotube_context_t* ctx, nanotube_map_id_t id,
                             const uint8_t* key, size_t key_length,
                             size_t data_length) {
  auto* map = ctx->get_map(id);
  /* Error checking => bail with nullptr in all cases */
  if( map == nullptr )
    return nullptr;
  if( key == nullptr )
    return nullptr;
  if( map->key_sz != key_length )
    return nullptr;
  if( map->value_sz != data_length )
    return nullptr;

  /* And allow direct access to the value */
  return map->lookup(key);
}

std::ostream& operator<<(std::ostream& os, nanotube_map* map) {
  return map->print(os);
}

void nanotube_map_print(nanotube_map* map) {
  std::cout << map;
}

nanotube_map_id_t nanotube_map_get_id(nanotube_map* map) {
  if( map == nullptr )
    return NANOTUBE_MAP_ILLEGAL;
  return map->id;
}

std::istream& operator>>(std::istream &is, enum map_type_t &t) {
  int i;
  is >> i;
  t = (enum map_type_t)i;
  return is;
}

nanotube_map* nanotube_map_create_from_stdin(void) {
  return nanotube_map_read_from(std::cin);
}
nanotube_map* nanotube_map_read_from(std::istream& is) {
  std::string word;
  nanotube_map* map = nullptr;

  /* Allow comments starting with # before each map definition and skip
   * them here. */
  const auto all = std::numeric_limits<std::streamsize>::max();
  while(is.good()) {
    is >> word;
    if( word[0] != '#' )
      break;
    is.ignore(all, '\n');
  }

  /* Abort if there is no nanotube_map definition */
  if( word.compare("nanotube_map:") != 0 ) {
    is.setstate(is.failbit);
    return map;
  }

  nanotube_map_id_t id;
  map_type_t        type;
  size_t            key_sz, value_sz;

  is >> id >> type >> key_sz >> value_sz;
  if (!is.good())
    return map;


  auto* sys = &processing_system::get_current();
  if( sys == nullptr ) {
    /* For unit tests, simply create new maps */
    //std::cerr << "TEST MODE: Creating new map ID " << id << '\n';
    map = nanotube_map_create(id, type, key_sz, value_sz);
  } else {
    auto* sys_maps = &sys->maps();
    auto it = sys_maps->find(id);

    if( it == sys_maps->end() ) {
      //std::cerr << "Creating new map ID " << id << '\n';
      map = nanotube_map_create(id, type, key_sz, value_sz);
    } else {
      //std::cerr << "Reusing found map " << id << '\n';
      map = it->second.get();
    }
  }

  uint8_t *key   = new uint8_t[key_sz];
  uint8_t *value = new uint8_t[value_sz];
  unsigned u;

  while( is.good() ) {
    /* Read the key bytes. */
    is >> word;
    /* "end" ends the map description */
    if( word.compare("end") == 0 )
      break;

    if( word.compare("key:") != 0 ) {
      is.setstate(is.failbit);
      break;
    }
    for( size_t i = 0; i < key_sz && is.good(); ++i ) {
      is >> std::hex >> u;
      assert(u <= 255);
      key[i] = (uint8_t)u;
    }

    /* Read the value bytes. */
    is >> word;
    if( word.compare("value:") != 0 )
      is.setstate(is.failbit);

    for( size_t i = 0; i < value_sz && is.good(); ++i ) {
      is >> std::hex >> u;
      assert(u <= 255);
      value[i] = (uint8_t)u;
    }

    /* Insert into map */
    uint8_t *v;
    if (map->type == NANOTUBE_MAP_TYPE_ARRAY_LE) {
      // Array maps to not allow insert, so write instead.
      v = map->lookup(key);
    } else {
      v = map->insert_empty(key);
    }
    assert(v != nullptr);
    memcpy(v, value, value_sz);
  }
  is >> std::dec;
  delete[] key;
  delete[] value;
  return map;
}

///////////////////////////////////////////////////////////////////////////
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
