/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef SOFTHUB_BUS_HPP
#define SOFTHUB_BUS_HPP

#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>

namespace softhub_bus
{
  typedef unsigned char byte_t;
  typedef unsigned char empty_t;

  /* AXI-S is 512 bits, so 64 bytes */
  static const int log_data_bytes = 6;
  static const int data_bytes = (1<<log_data_bytes);
  static const int sideband_bytes = 0;
  static const int sideband_signals_bytes = (data_bytes / 8) * 2 + 1;
  static const int total_bytes = data_bytes+sideband_bytes+sideband_signals_bytes;

  static inline unsigned data_offset(unsigned index) { return 0+index; }
 
 /* Layout of bitfields is compiler dependent, but this is the header structure */
 /*
  struct net_pkt_capsule {
    ef100_cap_hdr ch;
    ef100_cap_netpkt_hdr np;
    ef100_cap_netpkt_h2c_dma h2c_dma; // etc.
    octet_t extended_meta[NP_EXTENDED_META*64];
    octet_t frame_data[];
    // followed by ef100_cap_payload_check if CH_CPC_PRESENT.
  };
  struct ef100_cap_hdr {
    unsigned int ch_is_net_pkt : 1;
    unsigned int ch_vc : 6;
    unsigned int ch_route : 16;
    unsigned int ch_cpc_present : 1;
    unsigned int ch_length : 14;
  };
  */
  static const uint64_t CH_IS_NET_PKT_MASK = 0x1;
  static const uint64_t CH_VC_MASK = 0x7E;
  static const uint64_t CH_ROUTE_MASK_0 = 0x80;
  static const uint64_t CH_ROUTE_MASK_1 = 0xFF;
  static const uint64_t CH_ROUTE_MASK_2 = 0x7F;
  /* CH_ROUTE_MASK_0 | CH_ROUTE_MASK_1 << 8 | CH_ROUTE_MASK_2 << 16 */
  static const uint64_t CH_ROUTE_MASK = 0x7FFF80;
  static const uint64_t CH_CPC_PRESENT_MASK = 0x800000;
  static const uint64_t CH_LENGTH_MASK_0 = 0xFF;
  static const uint64_t CH_LENGTH_MASK_1 = 0x3F;
  /* (CH_LENGTH_MASK_0 | CH_LENGTH_MASK_1 << 8) << CH_LENGTH_OFFSET */
  static const uint64_t CH_LENGTH_MASK = 0x3FFF000000;
  static const unsigned CH_IS_NET_PKT_OFFSET = 0;
  static const unsigned CH_VC_OFFSET = 1;
  static const unsigned CH_ROUTE_OFFSET = 7;
  static const unsigned CH_CPC_PRESENT_OFFSET = 23;
  static const unsigned CH_LENGTH_OFFSET = 24;

  /* Streaming sub-system capsule header */
  struct header {
    byte_t hdr_data[28]; // struct net_pkt_capsule
  };

  /* Callers of get/set_ch_route_raw use these to know how which bytes of a packet 
   * need to be passed. */
  static const unsigned CH_ROUTE_RAW_PKT_OFFSET = 0;
  static const unsigned CH_ROUTE_RAW_PKT_LENGTH = 3;
  
  static inline uint16_t get_ch_route_raw(const byte_t *header) {
    uint16_t ch_route;
    ch_route = ((uint16_t)(header[0] & CH_ROUTE_MASK_0) >> CH_ROUTE_OFFSET) | 
               ((uint16_t)(header[1] & CH_ROUTE_MASK_1) << (8-CH_ROUTE_OFFSET)) | 
               ((uint16_t)(header[2] & CH_ROUTE_MASK_2) << (16-CH_ROUTE_OFFSET));
    return ch_route;
  }
  static inline void set_ch_route_raw(byte_t *header, uint16_t port) {
    header[0] = (header[0] & ~CH_ROUTE_MASK_0) | (byte_t)((port << CH_ROUTE_OFFSET) & CH_ROUTE_MASK_0);
    header[1] = (header[1] & ~CH_ROUTE_MASK_1) | (byte_t)((port >> (8-CH_ROUTE_OFFSET)) & CH_ROUTE_MASK_1);
    header[2] = (header[2] & ~CH_ROUTE_MASK_2) | (byte_t)((port >> (16-CH_ROUTE_OFFSET)) & CH_ROUTE_MASK_2);
  }

  static inline uint16_t get_ch_length_raw(const byte_t *header) {
    uint16_t ch_length;
    ch_length = ((uint16_t)(header[3] & CH_LENGTH_MASK_0)) |
      ((uint16_t)(header[4] & CH_LENGTH_MASK_1) << 8);
    return ch_length;
  }

  static inline void set_ch_length_raw(byte_t *header, uint16_t length) {
    header[3] = (header[3] & ~CH_LENGTH_MASK_0) | (byte_t)(length & CH_LENGTH_MASK_0);
    header[4] = (header[4] & ~CH_LENGTH_MASK_1) | (byte_t)((length >> 8) & CH_LENGTH_MASK_1);
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

    /* Assumes there is a capsule header at start of the word */
    uint16_t get_ch_route() const {
      return get_ch_route_raw(bytes);
    }
    /* Assumes there is a capsule header at start of the word */
    void set_ch_route(uint16_t port) {
      return set_ch_route_raw(bytes, port);
    }
    /* Assumes there is a capsule header at start of the word */
    uint16_t get_ch_length() const {
      return get_ch_length_raw(bytes);
    }
    /* Assumes there is a capsule header at start of the word */
    void set_ch_length(uint16_t length) {
      set_ch_length_raw(bytes, length);
    }

    byte_t &tkeep() {
      return bytes[data_bytes+sideband_bytes];
    }
    byte_t &tstrb() {
      return bytes[data_bytes+sideband_bytes+((sideband_signals_bytes-1)/2)];
    }
    byte_t &tlast() {
      return bytes[total_bytes-1];
    }

    /* Sets first N bits of TKEEP to 1, the rest to 0 */
    void set_tkeep(int N) {
      //uint64_t assumes 64 data bytes 
      uint64_t mask = (1ull << N) - 1;
      memcpy(&tkeep(), &mask, softhub_bus::data_bytes / 8);
    }

    /* Sets first N bits of TKEEP to 1, the rest to 0 */
    void set_tstrb(int N) {
      //uint64_t assumes 64 data bytes 
      uint64_t mask = (1ull << N) - 1;
      memcpy(&tstrb(), &mask, softhub_bus::data_bytes / 8);
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

#endif // SOFTHUB_BUS_HPP
