/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_packet_taps.h"
#include "simple_bus.hpp"
#include "nanotube_packet_taps_sb.h"
#include "ap_int.h"
#include "test.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

extern "C" {
#include <unistd.h>
}

#define DEBUG_RESIZE_TAP 0

/*************************************************************************/

// The edit tap needs to copy each packet from the input stream to the
// output stream, but delete some bytes and insert others from the
// request.
//
// Bytes from before the write offset from the request need to be
// passed through untouched.  They will always be a contiguous block
// of bytes at the start of the word.
//
// Next will be the inserted bytes from the request.  These need to be
// rotated by the write offset modulo the word size.  They are either
// all inserted or not inserted.
//
// Finally, the input bytes from after the deleted bytes will be
// output.  These bytes will be rotated by the difference between the
// number of inserted bytes and the number of deleted bytes.
//
// The tap is split into two parts.  The ingress part examines the
// request and ingress packet words to determine how the output stream
// will be composed.  The reason for this is to provide decoupling in
// the case where the egress needs to generate two output words for a
// single input word.

/*************************************************************************/

void packet_resize_ingress(
  std::deque<nanotube_tap_packet_resize_cword_t> &cword_strm,
  std::deque<simple_bus::word> &packet_data_strm,
  std::deque<nanotube_tap_packet_resize_req_t> &resize_req_strm,
  std::deque<simple_bus::word> &packet_in_strm,
  nanotube_tap_offset_t *new_packet_length)
{
  nanotube_tap_packet_resize_ingress_state_t state;
  nanotube_tap_packet_resize_ingress_state_init(&state);

  bool packet_done_out = true;
  while (!packet_in_strm.empty()) {
    nanotube_tap_packet_resize_cword_t cword_out;
    simple_bus::word packet_data_out;
    nanotube_tap_packet_resize_req_t resize_req_in;
    simple_bus::word packet_data_in;
    nanotube_tap_offset_t packet_length_out;

    if (packet_done_out) {
      resize_req_in = resize_req_strm.front();
      resize_req_strm.pop_front();
    }
    packet_data_in = packet_in_strm.front();
    packet_in_strm.pop_front();

    packet_data_out = packet_data_in;

    nanotube_tap_packet_resize_ingress_sb(&packet_done_out,
                                          &cword_out,
                                          &packet_length_out,
                                          &state,
                                          &resize_req_in,
                                          &packet_data_in);

    cword_strm.push_back(cword_out);
    packet_data_strm.push_back(packet_data_out);

    if(packet_done_out)
      *new_packet_length = packet_length_out;
  }
}

void packet_resize_egress(std::deque<simple_bus::word> &packet_out_strm,
                          std::deque<nanotube_tap_packet_resize_cword_t> &cword_strm,
                          std::deque<simple_bus::word> &packet_data_strm,
                          nanotube_tap_offset_t new_packet_length)
{
  nanotube_tap_packet_resize_egress_state_t state;
  simple_bus::word state_packet_word;
  nanotube_tap_packet_resize_cword_t cword;
  bool input_done_out = true;

  nanotube_tap_packet_resize_egress_state_init(&state);

  while(!packet_data_strm.empty() || !input_done_out) {
    bool packet_valid_out;
    simple_bus::word packet_word_out;
    simple_bus::word packet_word_in;

    if (input_done_out) {
#if DEBUG_RESIZE_TAP
      std::cout << "resize_egress: Reading inputs.\n";
#endif
      cword = cword_strm.front();
      cword_strm.pop_front();
      packet_word_in = packet_data_strm.front();
      packet_data_strm.pop_front();
    }

    bool packet_done_out;

    nanotube_tap_packet_resize_egress_sb(
      &input_done_out, &packet_done_out,
      &packet_valid_out, &packet_word_out,
      &state, &state_packet_word,
      &cword, &packet_word_in,
      new_packet_length);

    if (packet_valid_out) {
      packet_out_strm.push_back(packet_word_out);
    }
  }
}

void packet_resize_tap(std::deque<simple_bus::word> &packet_out_strm,
                       std::deque<nanotube_tap_packet_resize_req_t> &resize_req_strm,
                       std::deque<simple_bus::word> &packet_in_strm)
{
  std::deque<nanotube_tap_packet_resize_cword_t> cword_strm;
  std::deque<simple_bus::word> packet_data_strm;
  nanotube_tap_offset_t new_packet_length;

  packet_resize_ingress(cword_strm,
                      packet_data_strm,
                      resize_req_strm,
                      packet_in_strm,
                      &new_packet_length);

  packet_resize_egress(packet_out_strm,
                     cword_strm,
                     packet_data_strm,
                     new_packet_length);
}

/*************************************************************************/

#if RAND_MAX < 65536*256
#error "The value of RAND_MAX is too small."
#endif

class packet_resize_test {
  std::deque<nanotube_tap_packet_resize_req_t>  m_dut_resize_req_strm;
  std::deque<simple_bus::word>     m_dut_packet_in_strm;
  std::deque<simple_bus::word>     m_dut_packet_out_strm;
  std::list<simple_bus::word>       m_exp_packet_out;
  bool m_verbose;
  int m_selected_case;
  int m_case;

  nanotube_tap_packet_resize_req_t m_resize_req;

  simple_bus::word m_in_word;
  nanotube_tap_offset_t m_in_word_fill;

  simple_bus::word *m_out_word;
  nanotube_tap_offset_t m_out_word_fill;

public:
  packet_resize_test():
    m_dut_resize_req_strm(),
    m_dut_packet_in_strm(),
    m_dut_packet_out_strm(),
    m_verbose(false), m_selected_case(-1), m_case(0) {}
  void set_verbose() { m_verbose = true; }
  void set_case(int i) { m_selected_case = i; }

  void dump_word(const std::string &msg, simple_bus::word *word);

  void start_in_packet();
  void next_in_word();
  void add_in_byte(uint8_t val);
  void end_in_packet(bool err);

  void start_out_packet();
  void next_out_word();
  void add_out_byte(uint8_t val);
  void end_out_packet(bool err);

  void generate_common_byte();
  void generate_insert_byte();
  void generate_input_byte();
  void end_req();

  void add_case(nanotube_tap_offset_t packet_length,
                nanotube_tap_offset_t edit_offset,
                nanotube_tap_offset_t delete_length,
                nanotube_tap_offset_t insert_length,
                int err=-1);

  void pass_packets();
  void compare();
};

void packet_resize_test::dump_word(const std::string &msg,
                                   simple_bus::word *word)
{
  std::stringstream ss;
  ss << msg
     << " eop=" << int(word->get_eop())
     << " err=" << int(word->get_err())
     << " empty=" << int(word->get_empty())
     << (word->get_empty() >= 10 ? "" : " ")
     << " data:";

  for(nanotube_tap_offset_t i=0; i<simple_bus::data_bytes; i++)
    ss << " " << std::setw(2) << std::setfill('0') << std::hex
       << unsigned(word->data_ptr()[i]);

  ss << "\n";

  std::cout << ss.str();
}

void packet_resize_test::start_in_packet()
{
  m_in_word_fill = 0;
}

void packet_resize_test::next_in_word()
{
  m_in_word.set_control(false, false, 0);
  m_dut_packet_in_strm.push_back(m_in_word);

  if (m_verbose)
    dump_word("Input word(M): ", &m_in_word);

  m_in_word_fill = 0;
}

void packet_resize_test::add_in_byte(uint8_t val)
{
  if (m_in_word_fill >= simple_bus::data_bytes)
    next_in_word();

  m_in_word.data_ptr()[m_in_word_fill] = val;
  m_in_word_fill++;
}

void packet_resize_test::end_in_packet(bool err)
{
  assert(m_in_word_fill != 0);
  simple_bus::empty_t empty = simple_bus::data_bytes - m_in_word_fill;
  m_in_word.set_control(true, err, empty);
  m_dut_packet_in_strm.push_back(m_in_word);

  if (m_verbose)
    dump_word("Input word(E): ", &m_in_word);
}

void packet_resize_test::start_out_packet()
{
  m_exp_packet_out.push_back(simple_bus::word());
  m_out_word = &(m_exp_packet_out.back());
  m_out_word_fill = 0;
}

void packet_resize_test::next_out_word()
{
  m_out_word->set_control(false, false, 0);

  if (m_verbose)
    dump_word("Output word(M):", m_out_word);

  m_exp_packet_out.push_back(simple_bus::word());
  m_out_word = &(m_exp_packet_out.back());
  m_out_word_fill = 0;
}

void packet_resize_test::add_out_byte(uint8_t val)
{
  if (m_out_word_fill >= simple_bus::data_bytes)
    next_out_word();

  m_out_word->data_ptr()[m_out_word_fill] = val;
  m_out_word_fill++;
}

void packet_resize_test::end_out_packet(bool err)
{
  assert(m_out_word_fill != 0);

  simple_bus::empty_t empty = simple_bus::data_bytes - m_out_word_fill;
  m_out_word->set_control(true, err, empty);

  if (m_verbose)
    dump_word("Output word(E):", m_out_word);

  m_out_word = NULL;
}

void packet_resize_test::generate_common_byte()
{
  uint8_t val = rand() & 0xff;
  add_in_byte(val);
  add_out_byte(val);
}

void packet_resize_test::generate_insert_byte()
{
  m_resize_req.insert_length++;
  add_out_byte(0);
}

void packet_resize_test::generate_input_byte()
{
  uint8_t val = rand() & 0xff;
  add_in_byte(val);
}

void packet_resize_test::end_req()
{
  m_dut_resize_req_strm.push_back(m_resize_req);
}

void packet_resize_test::add_case(nanotube_tap_offset_t packet_length,
                                nanotube_tap_offset_t write_offset,
                                nanotube_tap_offset_t delete_length,
                                nanotube_tap_offset_t insert_length,
                                int err)
{
  int case_num = m_case++;
  if (m_selected_case != -1 && m_selected_case != case_num) {
    return;
  }

  if (err < 0)
    err = (rand() & 1);

  if (m_verbose)
    std::cout << "Adding case " << case_num
              << " packet_length=" << packet_length
              << " write_offset=" << write_offset
              << " delete_length=" << delete_length
              << " insert_length=" << insert_length
              << " err=" << err
              << "\n";

  /* Create the resize request */
  m_resize_req.write_offset = write_offset;
  m_resize_req.delete_length = delete_length;
  m_resize_req.insert_length = 0; // Updated during insert.

  /* Create the first input packet word. */
  start_in_packet();

  /* Create the first output packet word. */
  start_out_packet();

  /* Handle the data before the resize request. */
  while(packet_length > 0 && write_offset > 0) {
    generate_common_byte();
    packet_length--;
    write_offset--;
  }

  /* Handle the inserted data. */
  if (write_offset == 0) {
    while(insert_length > 0) {
      generate_insert_byte();
      insert_length--;
    }
  }

  /* Handle the deleted data. */
  while(packet_length > 0 && delete_length > 0) {
    generate_input_byte();
    packet_length--;
    delete_length--;
  }

  /* Handle the remaining input data. */
  while(packet_length > 0) {
    generate_common_byte();
    packet_length--;
  }

  end_in_packet(err);

  end_out_packet(err);

  end_req();
}

void packet_resize_test::pass_packets()
{
  while(!m_dut_resize_req_strm.empty() ||
        !m_dut_packet_in_strm.empty()) {
    packet_resize_tap(m_dut_packet_out_strm,
                    m_dut_resize_req_strm,
                    m_dut_packet_in_strm);
  }
}

void packet_resize_test::compare()
{
  unsigned word_index = 0;
  while(!m_dut_packet_out_strm.empty() &&
        !m_exp_packet_out.empty()) {
    simple_bus::word &dut_word = m_dut_packet_out_strm.front();
    simple_bus::word &exp_word = m_exp_packet_out.front();

    if (m_verbose) {
      dump_word("DUT word:", &dut_word);
      dump_word("EXP word:", &exp_word);
    }

    if (dut_word.get_eop() != exp_word.get_eop()) {
      if (dut_word.get_eop())
        std::cerr << "Received unexpected EOP at word " << word_index << "\n";
      else
        std::cerr << "Did not receive EOP at word " << word_index << "\n";
      test_failed = true;
    }

    if (exp_word.get_eop() && dut_word.get_eop()) {
      if (dut_word.get_err() != exp_word.get_err()) {
        if (dut_word.get_err())
          std::cerr << "Received unexpected ERR at word " << word_index << "\n";
        else
          std::cerr << "Did not receive ERR at word " << word_index << "\n";
        test_failed = true;
      }

      if (dut_word.get_empty() != exp_word.get_empty()) {
        std::cerr << "Received word " << word_index
                  << " had " << dut_word.get_empty()
                  << " empty bytes but expected "
                  << exp_word.get_empty() << "\n";
        test_failed = true;
      }
    }

    simple_bus::empty_t empty =
      ( exp_word.get_eop() ? exp_word.get_empty() : 0 );
    nanotube_tap_offset_t word_fill = ( simple_bus::data_bytes - empty );
    for(nanotube_tap_offset_t i=0; i<word_fill; i++) {
      if (dut_word.data_ptr()[i] != exp_word.data_ptr()[i]) {
        std::cerr << "Received word " << word_index
                  << " had data mismatch at byte " << i << "\n";
        std::stringstream ss;
        ss << "  Expected word:";
        for(i=0; i<word_fill; i++)
          ss << " " << std::setw(2) << std::setfill('0') << std::hex
             << unsigned(exp_word.data_ptr()[i]);
        ss << "\n";
        ss << "  Received word:";
        for(i=0; i<word_fill; i++)
          ss << " " << std::setw(2) << std::setfill('0') << std::hex
             << unsigned(dut_word.data_ptr()[i]);
        ss << "\n";
        std::cerr << ss.str();
        test_failed = true;
        break;
      }
    }
    m_dut_packet_out_strm.pop_front();
    m_exp_packet_out.pop_front();
    word_index++;
  }

  if (!m_dut_packet_out_strm.empty()) {
    std::cerr << "Received " << m_dut_packet_out_strm.size()
              << " too many word.\n";
    test_failed = true;
  }

  if (!m_exp_packet_out.empty()) {
    std::cerr << "Received " << m_exp_packet_out.size()
              << " too few word.\n";
    test_failed = true;
  }
}

int main(int argc, char **argv)
{
  packet_resize_test t;

  while(true) {
    int opt = getopt(argc, argv, "c:s:v");
    if (opt == -1)
      break;
    switch(opt) {
    case 'c':
      {
        char *end;
        unsigned long n = strtoul(optarg, &end, 0);
        if (*optarg == 0 || *end != 0) {
          std::cerr << argv[0] << ": Failed to parse integer '"
                    << optarg << "'\n";
          return 1;
        }
        t.set_case(n);
      }

    case 's':
      test_set_rand_seed_str(optarg);
      break;

    case 'v':
      t.set_verbose();
      break;

    default:
      std::cerr << "Usage: " << argv[0] << "[-v]\n";
      return 1;
    }
  }

  test_init(1, argv);

  // Corner test cases:
  //   - Zero length insert/delete at start of packet.
  t.add_case(60, 0, 0, 0, false);
  //   - Zero length insert/delete mid-packet.
  t.add_case(60, 17, 0, 0, false);

  //   - Insert some bytes at start of packet without overflow.
  t.add_case(60, 0, 0, 3);
  //   - Insert some bytes at start of packet with overflow.
  t.add_case(60, 0, 0, 11);
  //   - Insert some bytes mid-word without overflow.
  t.add_case(60, 17, 0, 3);
  //   - Insert some bytes mid-word packet with overflow.
  t.add_case(60, 17, 0, 11);
  //   - Insert some bytes to end of word without overflow.
  t.add_case(60, 60, 0, 4);
  //   - Insert some bytes to end of word with overflow.
  t.add_case(60, 58, 0, 6);
  //   - Insert some bytes, crossing a word boundary.
  t.add_case(60, 58, 0, 11);
  //   - Insert some bytes mid-packet.
  t.add_case(240, 93, 0, 11);
  //   - Insert some bytes mid-packet, crossing a word boundary.
  t.add_case(240, 123, 0, 11);

  //   - Delete some bytes at start of packet without underflow.
  t.add_case(63, 0, 3, 0);
  //   - Delete some bytes at start of packet with underflow.
  t.add_case(71, 0, 11, 0);
  //   - Delete some bytes mid-word without underflow.
  t.add_case(63, 17, 0, 3);
  //   - Delete some bytes mid-word with underflow.
  t.add_case(71, 17, 11, 0);
  //   - Delete some bytes from end of word without underflow.
  t.add_case(64, 60, 4, 0);
  //   - Delete some bytes from end of word with underflow.
  t.add_case(66, 58, 6, 0);
  //   - Delete some bytes, crossing a word.
  t.add_case(71, 58, 11, 0);
  //   - Delete bytes past EOP.
  t.add_case(63, 58, 11, 0);
  //   - Delete some bytes mid-packet.
  t.add_case(240, 93, 11, 0);
  //   - Delete some bytes mid-packet, crossing a word boundary.
  t.add_case(240, 123, 11, 0);

  //   - Overwrite bytes at start of packet.
  t.add_case(60, 0, 13, 13);
  //   - Overwrite bytes mid-word.
  t.add_case(60, 22, 13, 13);
  //   - Overwrite bytes to end of word at EOP.
  t.add_case(64, 51, 13, 13);
  //   - Overwrite bytes to end of word, not at EOP.
  t.add_case(73, 51, 13, 13);
  //   - Overwrite bytes crossing word boundary at EOP.
  t.add_case(73, 58, 13, 13);
  //   - Overwrite bytes crossing word boundary, not at EOP.
  t.add_case(73, 56, 13, 13);
  //   - Overwrite some bytes mid-packet.
  t.add_case(240, 93, 11, 11);
  //   - Overwrite some bytes mid-packet, crossing a word boundary.
  t.add_case(240, 123, 11, 11);

  // Random test cases.
  for(unsigned i=0; i<64; i++) {
    nanotube_tap_offset_t packet_length;
    nanotube_tap_offset_t edit_offset;
    nanotube_tap_offset_t delete_length = (rand() % 65);
    nanotube_tap_offset_t insert_length = (rand() % 65);
    unsigned r = rand();
    if (r & 1) {
      packet_length = (rand() % (1514-60)) + 60;
      edit_offset = (rand() % (1514-1)) + 1;
    } else {
      nanotube_tap_offset_t base = (rand() % (1514-64-60)) + 60;
      nanotube_tap_offset_t other = base + (rand() % 64);
      if (r & 2) {
        packet_length = base;
        edit_offset = other;
      } else {
        packet_length = other;
        edit_offset = base;
      }
    }
    t.add_case(packet_length, edit_offset, delete_length, insert_length);
  }

  t.pass_packets();
  t.compare();

  return test_fini();
}
