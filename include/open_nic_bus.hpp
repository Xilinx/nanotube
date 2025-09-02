/**************************************************************************\
*//*! \file open_nic_bus.hpp
** \author  Simone Mannarino 
**  \brief  OpenNIC 250 MHz bus support.
**   \date  2025-06-18
*//*
\**************************************************************************/

#ifndef OPEN_NIC_BUS_HPP
#define OPEN_NIC_BUS_HPP

#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>

namespace open_nic
{
  typedef unsigned char byte_t;

  /* OPEN_NIC_DATA AXI-S is 512 bits, so 64 bytes */
  static const int log_data_bytes = 6;
  static const int data_bytes = (1<<log_data_bytes);
  /* OPEN_NIC uses 48 bits for TUSER, so 6 bytes */
  static const int sideband_bytes = 6;
  // Tkeep is 64 bits, so 8 bytes
  // Tstrb is 64 bits, so 8 bytes
  // Tlast is 1 bit, so 1 byte
  static const int sideband_signals_bytes = 17;
  static const int total_bytes = data_bytes+sideband_bytes+sideband_signals_bytes;

  static inline unsigned data_offset(unsigned index) { return 0+index; }
  static inline unsigned sideband_offset(unsigned index) { return data_bytes+index; }
  static inline unsigned sideband_signals_offset(unsigned index) { return data_bytes+sideband_bytes+index; }

  /* Metadata format is as follows:
   * TUSER 64 bits
   * [0 - 15] TUSER_SIZE - size of the packet 
   * [16 - 31] TUSER_SRC - source of the packet
   * [32 - 47] TUSER_DST - destination of the packet
   */

  static const unsigned TUSER_SIZE_BYTE_INDEX = 0;
  static const unsigned TUSER_SIZE_BIT_MASK = 0xFFFF;
  static const unsigned TUSER_SRC_BYTE_INDEX = 2;
  static const unsigned TUSER_SRC_BIT_MASK = 0xFFFF;
  static const unsigned TUSER_DST_BYTE_INDEX = 4;
  static const unsigned TUSER_DST_BIT_MASK = 0xFFFF;

  static const unsigned TKEEP_BYTE_INDEX = 0;
  static const unsigned long long TKEEP_BIT_MASK = 0xFFFFFFFFFFFFFFFF;
  static const unsigned TLAST_BYTE_INDEX = sideband_signals_bytes-1;
  static const unsigned TLAST_BIT_MASK = 0x1;

// Get functions

  static inline unsigned short int get_sideband_tuser_size(const byte_t *sideband) {
    return *((unsigned short int*)&sideband[TUSER_SIZE_BYTE_INDEX]) & TUSER_SIZE_BIT_MASK;
  }

  static inline unsigned short int get_sideband_tuser_src(const byte_t *sideband) {
    return *((unsigned short int*)&sideband[TUSER_SRC_BYTE_INDEX]) & TUSER_SRC_BIT_MASK;
  }

  static inline unsigned short int get_sideband_tuser_dst(const byte_t *sideband) {
    return *((unsigned short int*)&sideband[TUSER_DST_BYTE_INDEX]) & TUSER_DST_BIT_MASK;
  }

  static inline unsigned long long get_sideband_signal_tkeep(const byte_t *sideband_signal) {
    return *((unsigned long long*)&sideband_signal[TKEEP_BYTE_INDEX]) & TKEEP_BIT_MASK;
  }

  static inline unsigned short int get_sideband_signal_tlast(const byte_t *sideband_signal) {
    return *((unsigned short int*)&sideband_signal[TLAST_BYTE_INDEX]) & TLAST_BIT_MASK;
  }

// Set functions

  static inline void set_tuser_size(byte_t *sideband, unsigned short int tuser_size) {
    *((unsigned short int*)&sideband[TUSER_SIZE_BYTE_INDEX]) = tuser_size & TUSER_SIZE_BIT_MASK;
  }

  static inline void set_tuser_src(byte_t *sideband, unsigned short int tuser_src) {
    *((unsigned short int*)&sideband[TUSER_SRC_BYTE_INDEX]) = tuser_src & TUSER_SRC_BIT_MASK;
  }

  static inline void set_tuser_dst(byte_t *sideband, unsigned short int tuser_dst) {
    *((unsigned short int*)&sideband[TUSER_DST_BYTE_INDEX]) = tuser_dst & TUSER_DST_BIT_MASK;
  }

  static inline void set_tkeep(byte_t *sideband_signal, unsigned short int tkeep_bits_enabled) {
    // Create a bitmask with `tkeep` bits set to 1
    unsigned long long value = (tkeep_bits_enabled < 64 ) ? ((1ULL << tkeep_bits_enabled) - 1) : ~0ULL;

    // Copy the value to the sideband signal at the correct position
    memcpy(&sideband_signal[TKEEP_BYTE_INDEX], &value, sizeof(value));
  }

  static inline void set_tlast(byte_t *sideband_signal, unsigned short int tlast) {
    unsigned short int value = tlast & TLAST_BIT_MASK;
    memcpy(&sideband_signal[TLAST_BYTE_INDEX], &value, sizeof(value));
  }

// Set raw function

  static inline void set_tuser_raw(byte_t *sideband, 
                                 unsigned const int tuser_size, 
                                 unsigned short int tuser_src, 
                                 unsigned short int tuser_dst) {
    // Clear sideband to avoid any data carry-over
    memset(sideband, 0, sideband_bytes);
    
    // Set each part with its corresponding mask and index
    set_tuser_size(sideband, tuser_size);
    set_tuser_src(sideband, tuser_src);
    set_tuser_dst(sideband, tuser_dst);
  }

  /* Define the structure for a word of 512 bits (64 bytes), sideband data, tkeep and tlast signals */
  struct word {
    byte_t bytes[total_bytes];

    /* Data manipulation methods */
    byte_t &data_ref(unsigned index = 0) {
        return bytes[data_offset(index)];
    }
    const byte_t &data_ref(unsigned index = 0) const {
        return bytes[data_offset(index)];
    }
    byte_t *data_ptr(unsigned index = 0) {
        return &(bytes[data_offset(index)]);
    }
    const byte_t *data_ptr(unsigned index = 0) const {
        return &(bytes[data_offset(index)]);
    }

    // Accessor functions
    int get_tuser_size() const {
        return open_nic::get_sideband_tuser_size(bytes + sideband_offset(0));
    }

    int get_tuser_src() const {
        return open_nic::get_sideband_tuser_src(bytes + sideband_offset(2));
    }

    int get_tuser_dst() const {
        return open_nic::get_sideband_tuser_dst(bytes + sideband_offset(4));
    }

    //Mutator functions
    void set_tuser_size(unsigned short int size) {
        open_nic::set_tuser_size(bytes + sideband_offset(0), size);
    }

    void set_tuser_src(unsigned short int src) {
        open_nic::set_tuser_src(bytes + sideband_offset(2), src);
    }

    void set_tuser_dst(unsigned short int dst) {
        open_nic::set_tuser_dst(bytes + sideband_offset(4), dst);
    }

    byte_t &tuser() {
      return bytes[sideband_offset(0)];
    }
    byte_t &tkeep() {
      return bytes[sideband_offset(0)+sideband_bytes];
    }
    byte_t &tlast() {
      return bytes[total_bytes-1];
    }

    unsigned long long get_tkeep() const {
      unsigned long long keep;
        memcpy(&keep, bytes + sideband_offset(0) + sideband_bytes, sizeof(keep));
        return keep;
    }

    // Accessor for tlast
    bool get_tlast() const {
        return bytes[total_bytes - 1] != 0; // Assuming last byte is treated as a boolean
    }

    /* Sets first N bits of TKEEP to 1, the rest to 0 */
    void set_tkeep(int N) {
        // Create a mask for the first N bits set to 1
        unsigned long long value = (N < 64 ) ? ((1ULL << N) - 1) : ~0ULL;

        // Write the mask to the appropriate location in TKEEP
        memcpy(&tkeep(), &value, sizeof(value)); // Copy the mask (8 bytes) into tkeep
    }

    void set_tlast(bool last) {
      tlast() = last;
    }
  };

  static inline std::ostream &operator <<(std::ostream &o, const word &w)
  {
    const int width = 16;

    int num_bytes = ( data_bytes );
    assert (num_bytes >= 0 && num_bytes <= data_bytes);

    int col = 0;
    for (int i=0; i<num_bytes; i++) {
      if (col >= width) {
        col = 0;
      }

      if (col == 0) {
        o << (i==0 ? "" : "\n")
          << std::hex << std::setw(4) << std::setfill('0')
          << i
          << std::dec << std::setw(0) << std::setfill(' ')
          << ":";
      }

      o << ((col>0 && col%4==0) ? " : " : " ")
        << std::hex << std::setw(2) << std::setfill('0')
        << unsigned(w.data_ref(i))
        << std::dec << std::setw(0) << std::setfill(' ');

      col++;
    }

    o << "\n\n";

    return o;
  }
};

#endif // OPEN_NIC_BUS_HPP