/**************************************************************************\
*//*! \file test.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Definitions to make test writing easier.
**   \date  2020-09-29
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "test.hpp"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>

bool test_failed = false;
int test_case = -1;
int test_verbose = 0;

static unsigned long test_rand_seed = 0xfee1900d;
static bool test_rand_seed_set = false;

void test_set_rand_seed(uint32_t seed)
{
  test_rand_seed = seed;
  test_rand_seed_set = true;
}

void test_set_rand_seed_str(const char *seed_str)
{
  char *end = nullptr;
  uint32_t seed = strtoul(seed_str, &end, 0);

  if (seed_str[0] == 0 || end[0] != 0) {
    std::cerr << "Invalid random seed '" << seed_str << "'.\n";
    exit(1);
  }
  test_set_rand_seed(seed);
}

void test_init(int argc, char *argv[])
{
  optind = 1;
  while (true) {
    int opt = getopt(argc, argv, ":c:s:v");
    if (opt == -1)
      break;

    switch (opt) {
    case 'c': {
      char *end = nullptr;
      test_case = strtol(optarg, &end, 0);
      if (optarg[0] == 0 || end[0] != 0) {
        std::cerr << argv[0] << ": Argument '"
                  << optarg
                  << "' to '-c' is invalid.\n";
        exit(1);
      }
      break;
    }

    case 's': {
      test_set_rand_seed_str(optarg);
      break;
    }

    case 'v':
      test_verbose++;
      break;

    case ':':
      std::cerr << argv[0] << ": Option '"
                << argv[optind-1] << "' requires an argument.\n";
      exit(1);
      break;

    case '?':
    default:
      std::cerr << argv[0] << ": Unknown option '"
                << argv[optind-1] << "'.\n";
      exit(1);
    }
  }

  if (!test_rand_seed_set) {
    struct timeval tv;
    int rc = gettimeofday(&tv, nullptr);
    if (rc != 0) {
      int err = errno;
      std::cerr << argv[0] << ": Call to gettimeofday failed: "
                << strerror(err) << "\n";
      exit(1);
    }
    test_rand_seed = tv.tv_sec ^ tv.tv_usec;
  }
  if (test_verbose) {
    std::cout << "Random seed is 0x" << std::hex << test_rand_seed
              << std::dec << "\n";
  }
  srand(test_rand_seed);
}

int test_fini()
{
  if (test_failed) {
    std::cout << "TEST FAILED!\n"
              << "Random seed was " << test_rand_seed << "\n";
    return 1;
  }

  std::cout << "Test passed.\n";
  return 0;
}

void test_dump_buffer(const char *name, const uint8_t *buffer, size_t size)
{
  size_t width = 8;
  if (name != nullptr)
    std::cout << "Dumping buffer " << name << "\n";
  std::cout << std::hex << std::setfill('0');
  for (size_t i = 0; i < size; i += width) {
    std::cout << "  0x" << std::setw(8) << i << std::setw(0) << ':';
    for (size_t j = i; j < i+width && j < size; j++) {
      std::cout << " 0x" << std::setw(2) << uint32_t(buffer[j])
                << std::setw(0);
    }
    std::cout << '\n';
  }
  std::cout << std::dec << std::setfill(' ');
}

void test_failure()
{
  test_failed = true;
}

void assert_eq_fail(const std::string &file, int line,
                    uint32_t x, uint32_t y,
                    const std::string &x_str, const std::string y_str)
{
  std::cout << file << ':' << line << ": Mismatch between "
            << x_str << '=' << x << " and "
            << y_str << '=' << y << ".\n";
  test_failure();
}

void assert_array_eq_int(const std::string &file, int line,
                         const uint8_t *x, const uint8_t *y,
                         size_t length,
                         const std::string &x_str,
                         const std::string &y_str)
{
  if (memcmp(x, y, length) == 0)
    return;

  test_failure();
  std::cout << file << ':' << line
            << ": Mismatch between (" << x_str << ") and ("
            << y_str << ") length " << length << ".\n";
  for (size_t offset = 0; offset < length; offset++) {
    if (x[offset] != y[offset])
      std::cout << "  Offset " << std::setw(2) << offset
                << std::setw(0) << ": 0x"
                << std::setfill('0')
                << std::setw(2) << std::hex << int(x[offset])
                << std::setw(0) << " != 0x"
                << std::setw(2) << std::hex << int(y[offset])
                << std::setw(0) << std::setfill(' ') << std::dec
                << ".\n";
  }
}

