/**************************************************************************\
*//*! \file test.hpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Definitions to make test writing easier.
**   \date  2020-09-29
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#ifndef TEST_HPP
#define TEST_HPP

#include <cstdint>
#include <string>

extern bool test_failed;

/*! The test case number selected by the user.
 *
 * Defaults to -1 if no number is specified.
 */
extern int test_case;

/*! A count of the -v options on the command line. */
extern int test_verbose;

/*! Set the random number seed to a specific value. 
 *
 * This can be called before test_init to set the random number seed.
 * If the seed is not set explicitly then it will take a different
 * value on different runs.
 *
 * \param seed The random number seed to use.
 */
void test_set_rand_seed(uint32_t seed);

/*! Set the random number seed to a specific value. 
 *
 * This can be called before test_init to set the random number seed.
 * If the seed is not set explicitly then it will take a different
 * value on different runs.
 *
 * \param seed_str The random number seed to use.
 */
void test_set_rand_seed_str(const char *seed_str);

/*! Initialise the test harness.
 *
 * This function parses the command line and initialises the random
 * number generator.
 *
 * \param argc The number of command line parameters.
 * \param argv An array of command line parameters.
 */
void test_init(int argc, char *argv[]);

/*! Report the end of the test.
 *
 * \returns Zero on success and one on failure.
 *
 * This function is called when all the test code has been run.  The
 * return value is typically used as the return value from main(). */
int test_fini();

/*! Dump the contents of a buffer to stdout.
 *
 * \param name The name of the buffer, or nullptr for none.
 * \param buffer The start of the buffer.
 * \param size The size of the buffer in bytes.
 */
void test_dump_buffer(const char *name, const uint8_t *buffer, size_t size);

/*! Indicate that a test has failed.
 *
 * This function simply sets test_failed.  It is always called when a
 * test fails, so it is a useful place to put a breakpoint.
 */
void test_failure();

/*! Indicate that an assertion has failed.
 *
 * \param file The source file containing the assertion.
 * \param line The line number of the assertion.
 * \param x The value of the left hand side expression.
 * \param y The value of the right hand side expression.
 * \param x_str The left hand side expression as a string.
 * \param y_str The right hand side expression as a string.
 */
void assert_eq_fail(const std::string &file, int line,
                    uint32_t x, uint32_t y,
                    const std::string &x_str, const std::string y_str);

/*! Assert that two expressions are equal.
 *
 * \param X The left hand side expression.
 * \param Y The right hand side expression.
 *
 * The expressions must not have side-effects.
 */
#define assert_eq(X, Y)                                         \
  do {                                                          \
    if ((X) != (Y))                                             \
      assert_eq_fail(__FILE__, __LINE__, (X), (Y), #X, #Y);     \
  } while(0)

/*! Assert that two arrays are equal.
 *
 * \param file The source file containing the assertion.
 * \param line The line number of the assertion.
 * \param x A pointer to the left hand side array.
 * \param y A pointer to the right hand side array.
 * \param length The number of bytes to compare.
 * \param x_str The left hand side expression as a string.
 * \param y_str The right hand side expression as a string.
 *
 * The expressions must not have side-effects.
 */
void assert_array_eq_int(const std::string &file, int line,
                         const uint8_t *x, const uint8_t *y,
                         size_t length,
                         const std::string &x_str,
                         const std::string &y_str);

/*! Assert that two arrays are equal.
 *
 * \param X The left hand side array.
 * \param Y The right hand side array.
 * \param N The number of bytes to comapare.
 *
 * The parameters must not have side-effects.
 */
#define assert_array_eq(X, Y, N) \
  assert_array_eq_int(__FILE__, __LINE__, X, Y, N, #X, #Y)

#endif // TEST_HPP
