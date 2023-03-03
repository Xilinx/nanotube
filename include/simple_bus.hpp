/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef SIMPLE_BUS_HPP
#define SIMPLE_BUS_HPP

#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>

namespace simple_bus
{
  typedef unsigned char byte_t;
  typedef unsigned char wsize_t;
  typedef unsigned char empty_t;

  static const byte_t control_eop = 0x80;
  static const byte_t control_err = 0x40;
  static const byte_t control_empty_lbn = 0;
  static const byte_t control_empty_width = 6;
  static const byte_t control_empty_mask =
    (byte_t(2) << (control_empty_width-1)) - 1;
  static const byte_t control_empty_smask =
    (control_empty_mask << control_empty_lbn);

  static const int log_data_bytes = 6;
  static const int data_bytes = (1<<log_data_bytes);
  static const int sideband_signals_bytes = 0;
#if 1 //control byte inline with data 
  static const int sideband_bytes = 0;
  static const int total_bytes = data_bytes+1+sideband_signals_bytes;
  static inline unsigned sideband_signals_offset() { return data_bytes+1; }
#else //control byte as sideband
  static const int sideband_bytes = 1;
  static const int total_bytes = data_bytes+sideband_bytes+sideband_signals_bytes;
  static inline unsigned sideband_signals_offset() { return data_bytes+sideband_bytes; }
#endif
  static inline unsigned data_offset(unsigned index) { return 0+index; }
  static inline unsigned control_offset() { return data_bytes; }

  static inline bool get_control_eop(byte_t control) {
    return (control & control_eop) != 0;
  }
  static inline bool get_control_err(byte_t control) {
      return (control & control_err) != 0;
  }
  static inline empty_t get_control_empty(byte_t control) {
    return ( (control & control_empty_smask) >> control_empty_lbn );
  }
  static inline byte_t control_value(bool eop, bool err, empty_t empty) {
    return ( (eop ? control_eop : 0) |
             (err ? control_err : 0) |
             ( (empty & control_empty_mask) << control_empty_lbn ) );
  }

  struct header {
    byte_t port;
  };

  struct word {
    byte_t bytes[total_bytes];

    byte_t &control() {
      return bytes[control_offset()];
    }
    const byte_t &control() const {
      return bytes[control_offset()];
    }
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

    bool get_eop() const {
      return get_control_eop(control());
    }
    bool get_err() const {
      return get_control_err(control());
    }
    empty_t get_empty() const {
      return get_control_empty(control());
    }
    void set_control(bool eop, bool err, empty_t empty) {
      control() = control_value(eop, err, empty);
    }
    
    byte_t &tkeep() {
      return bytes[sideband_signals_offset()];
    }
    byte_t &tstrb() {
      return bytes[sideband_signals_offset()+((sideband_signals_bytes-1)/2)];
    }
    byte_t &tlast() {
      return bytes[total_bytes-1];
    }

    /* Sets first N bits of TKEEP to 1, the rest to 0 */
    void set_tkeep(int N) {
      //uint64_t assumes 64 data bytes 
      uint64_t mask = (~0ull << (simple_bus::data_bytes - N));
      memcpy(&tkeep(), &mask, simple_bus::data_bytes / 8);
    }

    /* Sets first N bits of TKEEP to 1, the rest to 0 */
    void set_tstrb(int N) {
      //uint64_t assumes 64 data bytes 
      uint64_t mask = (~0ull << (simple_bus::data_bytes - N));
      memcpy(&tstrb(), &mask, simple_bus::data_bytes / 8);
    }
    void set_tlast(bool last) {
      tlast() = last;
    }
  };

  static inline std::ostream &operator <<(std::ostream &o, const word &w)
  {
    const int width = 16;

    int num_bytes = ( simple_bus::data_bytes -
                      (w.get_eop() ? w.get_empty() : 0) );
    assert (num_bytes >= 0 && num_bytes <= simple_bus::data_bytes);

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

    // Fill in empty columns.
    while (col<width) {
      o << ((col>0 && col%4==0) ? "     " : "   ");
      col++;
    }

    // Output the control info.
    if (w.get_eop()) {
      o << "  " << (w.get_err() ? "Err" : "EOP")
        << " empty=" << unsigned(w.get_empty());
    }
    o << "\n\n";

    return o;
  }
};

#endif // SIMPLE_BUS_HPP
