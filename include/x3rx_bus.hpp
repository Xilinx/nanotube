/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef X3RX_BUS_HPP
#define X3RX_BUS_HPP

#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>

namespace x3rx_bus
{
  typedef unsigned char byte_t;

  /* ULP_NIC_RX_DATA AXI-S is 32 bits, so 4 bytes */
  static const int log_data_bytes = 2;
  static const int data_bytes = (1<<log_data_bytes);
  /* ULP_NIC_RX_DATA uses 128 bits for TUSER, so 16 bytes */
  static const int sideband_bytes = 16;
  //This is really ((data_bytes / 8) * 2 + 1) but with data_bytes just 4, we
  //round up to one byte for TKEEP, one for TSTRB, and one for TLAST.
  static const int sideband_signals_bytes = 3;
  static const int total_bytes = data_bytes+sideband_bytes+sideband_signals_bytes;

  static inline unsigned data_offset(unsigned index) { return 0+index; }
  static inline unsigned sideband_offset(unsigned index) { return data_bytes+index; }

  struct header {
    /* Track the destination port */
    byte_t port;
  };

  /* Metadata format is as follows:
   * TUSER 128 bits
   * [0] DATA_SOP - set on first valid data word
   * [1] DATA_EOP - set on last valid data word
   * [2:9] DATA_EOP_PTR - indicates which bytes are valid when DATA_EOP set
   * [10:17] DATA_PARITY - reserved
   * [18] META_SOP - set when start of packet metadata (from MAC) is valid, must be aligned with DATA_SOP
   * [19] META_EOP - set when end of packet metadata, can come after DATA_EOP
   * [20-99] RX_TIMESTAMP
   * [100] RX_TIMESTAMP_VALID - RX_TIMESTAMP is valid, must be aligned with META_SOP
   * [101] BAD_PACKET - Must be aligned with META_EOP
   * [102-109] ULP_METADATA - For ULP to pass to host
   * [110] QUEUE_VALID - Must be aligned with META_SOP
   * [111-115] QUEUE - Queue to deliver to, when QUEUE_VALID asserted
   * [116-127] - reserved
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
  static const unsigned ULP_META_BYTE_INDEX = 12; //Just first 8 bits that fit in byte 12
  static const unsigned ULP_META_BIT_MASK = 0xff;
  static const unsigned ULP_META_OFFSET = 0;


  static inline bool get_sideband_data_sop(const byte_t *sideband) {
    return (sideband[DATA_SOP_BYTE_INDEX] & DATA_SOP_BIT_MASK) != 0;
  }
  static inline bool get_sideband_meta_sop(const byte_t *sideband) {
    return (sideband[META_SOP_BYTE_INDEX] & META_SOP_BIT_MASK) != 0;
  }
  static inline bool get_sideband_data_eop(const byte_t *sideband) {
    return (sideband[DATA_EOP_BYTE_INDEX] & DATA_EOP_BIT_MASK) != 0;
  }
  static inline bool get_sideband_meta_eop(const byte_t *sideband) {
    return (sideband[META_EOP_BYTE_INDEX] & META_EOP_BIT_MASK) != 0;
  }
  static inline int get_sideband_data_eop_valid(const byte_t *sideband) {
    return (sideband[DATA_EOP_VALID_BYTE_INDEX] & DATA_EOP_VALID_BIT_MASK) >> DATA_EOP_VALID_OFFSET;
  }
  static inline int get_sideband_ulp_meta(const byte_t *sideband) {
    return (sideband[ULP_META_BYTE_INDEX] & ULP_META_BIT_MASK) >> ULP_META_OFFSET;
  }

  static inline void set_sideband_ulp_meta(byte_t *sideband, int ulp_meta) {
    sideband[ULP_META_BYTE_INDEX] |= (ulp_meta & ULP_META_BIT_MASK) << ULP_META_OFFSET;
  }

  /* NB this is incomplete, exists mainly to support test harness,
   * and zeros other fields.  Extend as necessary */
  static inline void set_sideband_raw(byte_t *sideband,
                                      int data_sop, int data_eop, int n_valid,
                                      int meta_sop, int meta_eop, int ulp_meta) {
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
      sideband[ULP_META_BYTE_INDEX] |=
        (ulp_meta & ULP_META_BIT_MASK) << ULP_META_OFFSET;
  }

  struct word {
    byte_t bytes[total_bytes];

    byte_t &data_ref(unsigned index=0) {
      return bytes[data_offset(index)];
    }
    const byte_t &data_ref(unsigned index=0) const {
      return bytes[data_offset(index)];
    }
    byte_t *data_ptr(unsigned index=0) {
      return &(bytes[data_offset(index)]);
    }
    const byte_t *data_ptr(unsigned index=0) const {
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
    int get_ulp_meta() const {
      return (get_sideband_ulp_meta(bytes + sideband_offset(0)));
    }

    void set_sideband(int data_sop, int data_eop, int n_valid,
                      int meta_sop, int meta_eop, int ulp_meta) {
      return set_sideband_raw(bytes + sideband_offset(0),
                              data_sop, data_eop, n_valid,
                              meta_sop, meta_eop, ulp_meta);
    }

    byte_t &tuser() {
      return bytes[sideband_offset(0)];
    }
    byte_t &tkeep() {
      return bytes[sideband_offset(0)+sideband_bytes];
    }
    byte_t &tstrb() {
      return bytes[sideband_offset(0)+sideband_bytes+((sideband_signals_bytes-1)/2)];
    }
    byte_t &tlast() {
      return bytes[total_bytes-1];
    }

    /* Sets first N bits of TKEEP to 1, the rest to 0 */
    void set_tkeep(int N) {
      //uint8_t assumes data bytes<=8
      uint8_t mask = (1ull << N) - 1;
      memcpy(&tkeep(), &mask, 1 /* assumes x3rx_bus::data_bytes <= 8 */);
    }

    /* Sets first N bits of TKEEP to 1, the rest to 0 */
    void set_tstrb(int N) {
      //uint8_t assumes data bytes<=8
      uint8_t mask = (1ull << N) - 1;
      memcpy(&tstrb(), &mask, 1 /* assumes x3rx_bus::data_bytes <= 8 */);
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

#endif // X3RX_BUS_HPP
