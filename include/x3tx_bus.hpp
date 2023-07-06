/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef X3TX_BUS_HPP
#define X3TX_BUS_HPP

#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>

namespace x3tx_bus
{
  typedef unsigned char byte_t;

  /* ULP_NIC_TX_DATA AXI-S is 32 bits, so 4 bytes */
  static const int log_data_bytes = 2;
  static const int data_bytes = (1<<log_data_bytes);
  /* ULP_NIC_TX_DATA uses 64 bits for TUSER, so 8 bytes */
  static const int sideband_bytes = 8;
  //This is really ((data_bytes / 8) * 2 + 1) but with data_bytes just 4, we
  //round up to one byte for TKEEP, one for TSTRB, and one for TLAST.
  static const int sideband_signals_bytes = 3;
  static const int total_bytes = data_bytes+sideband_bytes+sideband_signals_bytes;

  static inline unsigned data_offset(unsigned index) { return 0+index; }
  static inline unsigned sideband_offset(unsigned index) { return data_bytes+index; }

  /* Metadata format is as follows:
   * TUSER 64 bits
   * [0] DATA_SOP - set on first valid data word
   * [1] DATA_EOP - set on last valid data word
   * [2:9] DATA_EOP_PTR - indicates which bytes are valid when DATA_EOP set
   * [10:17] DATA_PARITY - reserved
   * [18] META_SOP - set when start of packet metadata (from MAC) is valid, must be aligned with DATA_SOP
   * [19] META_EOP - set when end of packet metadata, can come after DATA_EOP
   * [20-27] PACKET_SEQUENCE_ID - Packet sequence identifier, valid with META_SOP
   * [28-31] PACKET_QUEUE_ID - Packet queue identifier, valid with META_SOP
   * [32-35] - reserved
   * [36-49] PACKET_LENGTH - length in bytes, valid with META_SOP
   * [50-51] - reserved
   * [52] TX_TIMESTAMP_REQUEST - asserted to request a timestamp from MAC, valid with META_SOP
   * [53] BAD_PACKET - 0 == good, 1 == bad, used to terminate packet towards the MAC, must be asserted with META_EOP, DATA_EOP and TLAST
   * [54] ULP_PACKET - set for a packet that was generated in ULP rather than host, valid with META_SOP
   * [55-63] - reserved
   */

  static const unsigned DATA_SOP_BYTE_INDEX = 0;
  static const unsigned DATA_SOP_BIT_MASK = (1 << 0);
  static const unsigned DATA_EOP_BYTE_INDEX = 0;
  static const unsigned DATA_EOP_BIT_MASK = (1 << 1);
  static const unsigned META_SOP_BYTE_INDEX = 2;
  static const unsigned META_SOP_BIT_MASK = (1 << 2);
  static const unsigned META_EOP_BYTE_INDEX = 2;
  static const unsigned META_EOP_BIT_MASK = (1 << 3);
  static const unsigned DATA_EOP_VALID_BYTE_INDEX = 0;
  static const unsigned DATA_EOP_VALID_BIT_MASK = 0x3c; //Just 4 bits used for 10G data path
  static const unsigned DATA_EOP_VALID_OFFSET = 2;


  static inline bool get_sideband_data_sop(const byte_t* sideband) {
    return (sideband[DATA_SOP_BYTE_INDEX] & DATA_SOP_BIT_MASK) != 0;
  }
  static inline bool get_sideband_meta_sop(const byte_t* sideband) {
    return (sideband[META_SOP_BYTE_INDEX] & META_SOP_BIT_MASK) != 0;
  }
  static inline bool get_sideband_data_eop(const byte_t* sideband) {
    return (sideband[DATA_EOP_BYTE_INDEX] & DATA_EOP_BIT_MASK) != 0;
  }
  static inline bool get_sideband_meta_eop(const byte_t* sideband) {
    return (sideband[META_EOP_BYTE_INDEX] & META_EOP_BIT_MASK) != 0;
  }
  static inline int get_sideband_data_eop_valid(const byte_t* sideband) {
    return (sideband[DATA_EOP_VALID_BYTE_INDEX] & DATA_EOP_VALID_BIT_MASK) >> DATA_EOP_VALID_OFFSET;
  }
  // Same as get_sideband_data_eop_valid but converts the valid mask to 
  // the number of bytes that are valid
  static inline int get_sideband_data_eop_valid_nbytes(const byte_t *sideband) {
    int valid = (sideband[DATA_EOP_VALID_BYTE_INDEX] & DATA_EOP_VALID_BIT_MASK) >> DATA_EOP_VALID_OFFSET;
    int num_bytes = 0;
    switch (valid) {
    case 1:
      num_bytes = 1;
      break;
    case 3:
      num_bytes = 2;
      break;
    case 7:
      num_bytes = 3;
      break;
    case 15:
      num_bytes = 4;
      break;
    default:
      assert(false);
    }
    return num_bytes;
  }
  /* NB this is incomplete, exists mainly to support test harness,
   * and zeros other fields.  Extend as necessary */
  static inline void set_sideband_raw(byte_t* sideband,
                                      int data_sop, int data_eop, int n_valid,
                                      int meta_sop, int meta_eop) {
      int valid_bits = 0;
      assert(n_valid <= 4);
      valid_bits = ((1 << n_valid) - 1) << DATA_EOP_VALID_OFFSET;
      memset(sideband, 0, sideband_bytes);
      assert(DATA_SOP_BYTE_INDEX == DATA_EOP_BYTE_INDEX);
      assert(DATA_SOP_BYTE_INDEX == DATA_EOP_VALID_BYTE_INDEX);
      sideband[DATA_SOP_BYTE_INDEX] |=
        (data_sop ? DATA_SOP_BIT_MASK : 0) |
        (data_eop ? DATA_EOP_BIT_MASK : 0) |
        (data_eop ? valid_bits : 0);
      assert(META_SOP_BYTE_INDEX == META_EOP_BYTE_INDEX);
      sideband[META_SOP_BYTE_INDEX] |=
        (meta_sop ? META_SOP_BIT_MASK : 0) |
        (meta_eop ? META_EOP_BIT_MASK : 0);
  }

  struct word {
    byte_t bytes[total_bytes];

    byte_t& data_ref(unsigned index=0) {
      return bytes[data_offset(index)];
    }
    const byte_t& data_ref(unsigned index=0) const {
      return bytes[data_offset(index)];
    }
    byte_t* data_ptr(unsigned index=0) {
      return &(bytes[data_offset(index)]);
    }
    const byte_t* data_ptr(unsigned index=0) const {
      return &(bytes[data_offset(index)]);
    }

    bool get_data_sop() const {
      return (get_sideband_data_sop(bytes + sideband_offset(0)));
    }
    bool get_meta_sop() const {
      return (get_sideband_meta_sop(bytes + sideband_offset(0)));
    }
    bool get_data_eop() const {
      return (get_sideband_data_eop(bytes + sideband_offset(0)));
    }
    bool get_meta_eop() const {
      return (get_sideband_meta_eop(bytes + sideband_offset(0)));
    }
    int get_data_eop_valid() const {
      return (get_sideband_data_eop_valid(bytes + sideband_offset(0)));
    }

    void set_sideband(int data_sop, int data_eop, int n_valid,
                      int meta_sop, int meta_eop) {
      return set_sideband_raw(bytes + sideband_offset(0),
                              data_sop, data_eop, n_valid,
                              meta_sop, meta_eop);
    }

    byte_t& tuser() {
      return bytes[sideband_offset(0)];
    }
    byte_t& tkeep() {
      return bytes[sideband_offset(0)+sideband_bytes];
    }
    byte_t& tstrb() {
      return bytes[sideband_offset(0)+sideband_bytes+((sideband_signals_bytes-1)/2)];
    }
    byte_t& tlast() {
      return bytes[total_bytes-1];
    }

    /* Sets first N bits of TKEEP to 1, the rest to 0 */
    void set_tkeep(int N) {
      //uint8_t assumes data bytes<=8
      uint8_t mask = (1ull << N) - 1;
      memcpy(&tkeep(), &mask, 1 /* assumes x3tx_bus::data_bytes <= 8 */);
    }

    /* Sets first N bits of TKEEP to 1, the rest to 0 */
    void set_tstrb(int N) {
      //uint8_t assumes data bytes<=8
      uint8_t mask = (1ull << N) - 1;
      memcpy(&tstrb(), &mask, 1 /* assumes x3tx_bus::data_bytes <= 8 */);
    }

    void set_tlast(bool last) {
      tlast() = last;
    }
  };


  static inline std::ostream &operator <<(std::ostream& o, const word& w)
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

#endif // X3TX_BUS_HPP
