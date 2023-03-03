/**************************************************************************\
*//*! \file test_tap_map_cam.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  A simple test for the CAM map tap.
**   \date  2021-06-07
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_map_taps.h"
#include "processing_system.hpp"
#include "test.hpp"

#include <map>
#include <vector>
#include <iostream>
#include <iomanip>

///////////////////////////////////////////////////////////////////////////

class map_test
{
public:
  map_test();
  void run_all();

private:
  typedef std::vector<uint8_t> byte_vec_t;
  typedef std::map<byte_vec_t, byte_vec_t> shadow_map_t;

  void init_map(nanotube_map_width_t key_length,
                nanotube_map_width_t data_length,
                nanotube_map_depth_t capacity);
  void comment(const std::string &s);
  void gen_key(int key_id);
  void gen_data();
  void invoke(enum map_access_t access);
  void check_data_zero();
  void check_data(const byte_vec_t &exp_data);
  void insert(int key_id, bool exp_succ);
  void update(int key_id, bool exp_succ);
  void write(int key_id, bool exp_succ);
  void remove(int key_id, bool exp_succ);
  void read(int key_id, bool exp_succ);
  void verify_all();

  int m_key_length;
  int m_data_length;
  int m_capacity;
  uint8_t *m_map_state;
  shadow_map_t m_shadow_map;
  byte_vec_t m_initial_key;
  byte_vec_t m_zero_data;
  byte_vec_t m_key_in;
  byte_vec_t m_data_in;
  byte_vec_t m_data_out;
  nanotube_map_result_t m_result_out;
};

map_test::map_test():
  m_key_length(0),
  m_data_length(0),
  m_capacity(0),
  m_map_state(nullptr)
{
}

///////////////////////////////////////////////////////////////////////////

void map_test::run_all()
{
  init_map(4, 16, 4);

  comment("Read in an empty map");
  read(0, false);

  comment("Insert a new entry");
  insert(0, true);

  comment("Read hit single");
  read(0, true);

  comment("Read miss single");
  read(1, false);

  comment("Insert an existing entry + read");
  insert(0, false);
  read(0, true);

  comment("Write a new entry + read");
  write(1, true);
  read(1, true);

  comment("Write an existing entry + read");
  write(0, true);
  read(0, true);

  comment("Update a missing entry + read");
  update(2, false);
  read(2, false);

  comment("Update an existing entry + read");
  update(1, true);
  read(1, true);

  comment("Fill the table");
  insert(2, true);
  insert(3, true);

  comment("Verify the contents");
  verify_all();

  comment("Insert new entry overflow + read");
  insert(4, false);
  read(4, false);

  comment("Verify the contents");
  verify_all();

  comment("Remove an existing entry + read");
  remove(2, true);
  read(2, false);

  comment("Remove a missing entry + read");
  remove(4, false);
  read(4, false);

  comment("Verify the contents");
  verify_all();
}

///////////////////////////////////////////////////////////////////////////

void map_test::init_map(nanotube_map_width_t key_length,
                        nanotube_map_width_t data_length,
                        nanotube_map_depth_t capacity)
{
  m_key_length = key_length;
  m_data_length = data_length;
  m_capacity = capacity;
  m_map_state = nanotube_tap_map_core_alloc(
        NANOTUBE_MAP_TYPE_HASH,
        key_length,
        data_length,
        capacity);
  m_shadow_map.clear();
  m_initial_key.resize(m_key_length);
  m_zero_data.resize(m_data_length, 0);
  m_key_in.resize(m_key_length);
  m_data_in.resize(m_data_length);
  m_data_out.resize(m_data_length);

  for (int i=0; i<m_key_length; i++) {
    m_initial_key.at(i) = (rand() & 0xff);
  }
}

void map_test::comment(const std::string &s)
{
  if (test_verbose) {
    std::cout << "\n# " << s << "\n";
  }
}

void map_test::gen_key(int key_id)
{
  for (int i=0; i<m_key_length; i++) {
    m_key_in.at(i) = m_initial_key.at(i) ^ uint8_t(key_id);
    key_id ^= (key_id >> 8);
  }
}

void map_test::gen_data()
{
  for (int i=0; i<m_data_length; i++) {
    m_data_in.at(i) = (rand() & 0xff);
  }
}

void map_test::invoke(enum map_access_t access)
{
  if (test_verbose) {
    std::cout << "Invoking ";
    switch (access) {
    case NANOTUBE_MAP_READ:   std::cout << "READ\n"; break;
    case NANOTUBE_MAP_INSERT: std::cout << "INSERT\n"; break;
    case NANOTUBE_MAP_UPDATE: std::cout << "UPDATE\n"; break;
    case NANOTUBE_MAP_WRITE:  std::cout << "WRITE\n"; break;
    case NANOTUBE_MAP_REMOVE: std::cout << "REMOVE\n"; break;
    case NANOTUBE_MAP_NOP:    std::cout << "NOP\n"; break;
    }
    std::cout << "  Key:     " << std::hex << std::setfill('0');
    for (int i=0; i<m_key_length; i++) {
      std::cout << " 0x" << std::setw(2) << int(m_key_in.at(i));
    }
    std::cout << '\n';
    std::cout << "  Data in: ";
    for (int i=0; i<m_data_length; i++) {
      std::cout << " 0x" << std::setw(2) << int(m_data_in.at(i));
    }
    std::cout << '\n';
  }

  nanotube_tap_map_core(
    NANOTUBE_MAP_TYPE_HASH,
    m_key_length,
    m_data_length,
    m_capacity,
    &(m_data_out[0]),
    &m_result_out,
    m_map_state,
    &(m_key_in[0]),
    &(m_data_in[0]),
    access);

  if (test_verbose) {
    std::cout << "  Result:   ";
    switch (m_result_out) {
    case NANOTUBE_MAP_RESULT_ABSENT:   std::cout << "ABSENT\n"; break;
    case NANOTUBE_MAP_RESULT_INSERTED: std::cout << "INSERTED\n"; break;
    case NANOTUBE_MAP_RESULT_REMOVED:  std::cout << "REMOVED\n"; break;
    case NANOTUBE_MAP_RESULT_PRESENT:  std::cout << "PRESENT\n"; break;
    }
    std::cout << "  Data out:";
    for (int i=0; i<m_data_length; i++) {
      std::cout << " 0x" << std::setw(2) << int(m_data_out.at(i));
    }
    std::cout << '\n' << std::dec << std::setfill(' ');
  }
}

void map_test::check_data_zero()
{
  assert_array_eq(&(m_data_out[0]), &(m_zero_data[0]), m_data_length);
}

void map_test::check_data(const byte_vec_t &exp_data)
{
  assert_array_eq(&(m_data_out[0]), &(exp_data[0]), m_data_length);
}

void map_test::insert(int key_id, bool exp_succ)
{
  gen_key(key_id);
  gen_data();
  invoke(NANOTUBE_MAP_INSERT);

  auto it = m_shadow_map.find(m_key_in);
  if (it != m_shadow_map.end()) {
    assert_eq(exp_succ, false);
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_PRESENT);
    check_data(it->second);

  } else if (exp_succ) {
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_INSERTED);
    check_data_zero();
    m_shadow_map.emplace(m_key_in, m_data_in);

  } else {
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_ABSENT);
    check_data_zero();
  }
}

void map_test::update(int key_id, bool exp_succ)
{
  gen_key(key_id);
  gen_data();
  invoke(NANOTUBE_MAP_UPDATE);

  auto it = m_shadow_map.find(m_key_in);
  if (it != m_shadow_map.end()) {
    assert_eq(exp_succ, true);
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_PRESENT);
    check_data(it->second);
    it->second = m_data_in;

  } else {
    assert_eq(exp_succ, false);
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_ABSENT);
    check_data_zero();
  }
}

void map_test::write(int key_id, bool exp_succ)
{
  gen_key(key_id);
  gen_data();
  invoke(NANOTUBE_MAP_WRITE);

  auto it = m_shadow_map.find(m_key_in);
  if (it != m_shadow_map.end()) {
    assert_eq(exp_succ, true);
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_PRESENT);
    check_data(it->second);
    it->second = m_data_in;

  } else if (exp_succ) {
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_INSERTED);
    check_data_zero();
    m_shadow_map.emplace(m_key_in, m_data_in);

  } else {
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_ABSENT);
    check_data_zero();
  }
}

void map_test::remove(int key_id, bool exp_succ)
{
  gen_key(key_id);
  gen_data();
  invoke(NANOTUBE_MAP_REMOVE);

  auto it = m_shadow_map.find(m_key_in);
  if (it != m_shadow_map.end()) {
    assert_eq(exp_succ, true);
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_REMOVED);
    check_data(it->second);
    m_shadow_map.erase(it);

  } else {
    assert_eq(exp_succ, false);
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_ABSENT);
    check_data_zero();
  }
}

void map_test::read(int key_id, bool exp_succ)
{
  gen_key(key_id);
  gen_data();
  invoke(NANOTUBE_MAP_READ);

  auto it = m_shadow_map.find(m_key_in);
  if (it != m_shadow_map.end()) {
    assert_eq(exp_succ, true);
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_PRESENT);
    check_data(it->second);

  } else {
    assert_eq(exp_succ, false);
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_ABSENT);
    check_data_zero();
  }
}

void map_test::verify_all()
{
  for (auto it=m_shadow_map.begin(); it!=m_shadow_map.end(); it++) {
    m_key_in = it->first;
    gen_data();
    invoke(NANOTUBE_MAP_READ);
    assert_eq(m_result_out, NANOTUBE_MAP_RESULT_PRESENT);
    check_data(it->second);
  }
}

///////////////////////////////////////////////////////////////////////////

void nanotube_setup()
{
  map_test t;
  t.run_all();
}

int main(int argc, char *argv[])
{
  test_init(argc, argv);
  dummy_ps_client psc;
  auto ps = processing_system::attach(psc);
  processing_system::detach(ps);
  return test_fini();
}

///////////////////////////////////////////////////////////////////////////
