/*******************************************************/
/*! \file test_channels.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief Unit tests for Nanotube channels implementation.
**   \date 2020-09-10
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_context.hpp"
#include "nanotube_channel.hpp"
#include "nanotube_thread.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>

extern "C" {
#include <pthread.h>
#include <unistd.h>
}

static const long ns_per_s = 1000*1000*1000;
static const long test_timeout = ns_per_s/2;

pthread_mutex_t mutex_1;
pthread_cond_t cond_1;
int state = 0;

int elem_size = 8;
int channel_capacity = 64;
int sideband_size = 0;
int sideband_signals = 0;
nanotube_channel *channel_ptr_1;
nanotube_channel *channel_ptr_2;
nanotube_thread *thread_ptr_1;
nanotube_thread *thread_ptr_2;

void fatal_error(const std::string &message, int rc)
{
  fprintf(stderr, "%s: %s (rc=%d)\n", message.c_str(),
          strerror(rc), rc);
  exit(1);
}

void set_state(int s)
{
  int rc;

  rc = pthread_mutex_lock(&mutex_1);
  if (rc != 0)
    fatal_error("pthread_mutex_lock failed", rc);

  state = s;
  rc = pthread_cond_broadcast(&cond_1);
  if (rc != 0)
    fatal_error("pthread_cond_broadcast", rc);

  rc = pthread_mutex_unlock(&mutex_1);
  if (rc != 0)
    fatal_error("pthread_mutex_lock failed", rc);
}

void wait_state(int s)
{
  int rc;

  rc = pthread_mutex_lock(&mutex_1);
  if (rc != 0)
    fatal_error("pthread_mutex_lock failed", rc);

  while (state != s) {
    rc = pthread_cond_wait(&cond_1, &mutex_1);
    if (rc != 0)
      fatal_error("pthread_cond_wait", rc);
  }

  rc = pthread_mutex_unlock(&mutex_1);
  if (rc != 0)
    fatal_error("pthread_mutex_lock failed", rc);
}

void check_read_ptr(nanotube_channel *channel, size_t index)
{
  size_t read_ptr = channel->get_read_ptr();
  size_t exp_ptr = (index % channel_capacity) * elem_size;
  if (read_ptr != exp_ptr) {
    std::cout << "Unexpected read pointer: "
              << read_ptr << " != " << exp_ptr << "\n";
    exit(1);
  }
}

void check_write_ptr(nanotube_channel *channel, size_t index)
{
  size_t write_ptr = channel->get_write_ptr();
  size_t exp_ptr = (index % channel_capacity) * elem_size;
  if (write_ptr != exp_ptr) {
    std::cout << "Unexpected write pointer: "
              << write_ptr << " != " << exp_ptr << "\n";
    exit(1);
  }
}

void set_elem(uint8_t *buffer, uint8_t base)
{
  base ^= (base << 4) ^ 0x55;
  for (int i=0; i<elem_size; i++)
    buffer[i] = base + i;
}

void check_elem(uint8_t *buffer, uint8_t base)
{
  base ^= (base << 4) ^ 0x55;
  for (int i=0; i<elem_size; i++)
    assert(buffer[i] == base + i);
}

void check_zero(uint8_t *buffer)
{
  for (int i=0; i<elem_size; i++)
    assert(buffer[i] == 0);
}

void func_1(nanotube_context_t *context, void *arg)
{
  uint8_t buffer[elem_size];
  bool succ;
  int rc;

  set_elem(buffer, 0xff);

  std::cout << "func_1: Case 1: try_read returns false.\n";
  succ = nanotube_channel_try_read(context, 0, buffer, sizeof(buffer));
  assert(!succ);
  check_read_ptr(channel_ptr_1, 0);
  check_zero(buffer);

  // Make sure the writer can wake us.
  std::cout << "func_1: Sleeping.\n";
  set_state(1);
  nanotube_thread_wait();
  assert(state == 2);

  // Make sure waking is one-shot.
  std::cout << "func_1: Sleeping.\n";
  set_state(3);
  nanotube_thread_wait();
  assert(state == 4);

  std::cout << "func_1: Case 2: try_read returns true.\n";
  succ = nanotube_channel_try_read(context, 0, buffer, sizeof(buffer));
  assert(succ);
  check_read_ptr(channel_ptr_1, 1);
  check_elem(buffer, 0);

  // Make sure a write does not wake the reader.
  std::cout << "func_1: Sleeping.\n";
  set_state(10);
  nanotube_thread_wait();
  assert(state == 11);

  std::cout << "func_1: Case 3: has_space returns true.\n";
  set_state(20);

  // Make sure a read does not wake the writer.
  wait_state(21);
  std::cout << "func_1: Reading.\n";
  succ = nanotube_channel_try_read(context, 0, buffer, sizeof(buffer));
  assert(succ);
  check_read_ptr(channel_ptr_1, 2);
  check_elem(buffer, 1);

  // Give the second thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Wake the second thread.
  set_state(22);
  thread_ptr_2->wake();
  wait_state(23);

  std::cout << "func_1: Case 4: try_write returns true.\n";
  set_state(30);

  // Make sure a read does not wake the writer.
  wait_state(31);
  std::cout << "func_1: Reading.\n";
  succ = nanotube_channel_try_read(context, 0, buffer, sizeof(buffer));
  assert(succ);
  check_read_ptr(channel_ptr_1, 3);
  check_elem(buffer, 2);

  // Give the second thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Wake the second thread.
  set_state(32);
  thread_ptr_2->wake();
  wait_state(33);

  std::cout << "func_1: Case 5: has_space returns false.\n";
  set_state(40);

  // Make sure a read wakes the writer.
  wait_state(41);

  // Give the second thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Read an element to wake the writer.
  set_state(42);
  succ = nanotube_channel_try_read(context, 0, buffer, sizeof(buffer));
  assert(succ);
  check_read_ptr(channel_ptr_1, 4);
  check_elem(buffer, 3);

  // Make sure a second read does not wake the writer.
  wait_state(43);
  succ = nanotube_channel_try_read(context, 0, buffer, sizeof(buffer));
  assert(succ);
  check_read_ptr(channel_ptr_1, 5);
  check_elem(buffer, 4);

  // Give the second thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Wake the second thread.
  set_state(44);
  thread_ptr_2->wake();
  wait_state(45);

  std::cout << "func_1: Case 6: try_write returns false.\n";
  set_state(50);

  // Make sure a read wakes the writer.
  wait_state(51);

  // Give the second thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Read an element to wake the writer.
  set_state(52);
  succ = nanotube_channel_try_read(context, 0, buffer, sizeof(buffer));
  assert(succ);
  check_read_ptr(channel_ptr_1, 6);
  check_elem(buffer, 5);

  // Make sure a second read does not wake the writer.
  wait_state(53);
  succ = nanotube_channel_try_read(context, 0, buffer, sizeof(buffer));
  assert(succ);
  check_read_ptr(channel_ptr_1, 7);
  check_elem(buffer, 6);

  // Give the second thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Wake the second thread.
  set_state(54);
  thread_ptr_2->wake();
  wait_state(55);

  set_state(-2);

  while (true)
    nanotube_thread_wait();
}

void func_2(nanotube_context_t *context, void *arg)
{
  uint8_t buffer[elem_size];
  bool succ;
  int rc;

  // Wait for the first thread to go to sleep.
  wait_state(1);
  std::cout << "func_2: Started.\n";

  // Give the first thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Wake up the first thread.
  std::cout << "func_2: Write. (1)\n";
  set_elem(buffer, 0);
  set_state(2);
  succ = channel_ptr_1->try_write(buffer, sizeof(buffer));
  assert(succ);
  check_write_ptr(channel_ptr_1, 1);

  // Wait for the first thread to wake up.
  wait_state(3);

  // Make sure waking is one-shot.
  std::cout << "func_2: Write. (2)\n";
  set_elem(buffer, 1);
  succ = channel_ptr_1->try_write(buffer, sizeof(buffer));
  assert(succ);
  check_write_ptr(channel_ptr_1, 2);

  // Give the first thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Wake up the first thread.
  std::cout << "func_2: Wake. (2)\n";
  set_state(4);
  thread_ptr_1->wake();

  // Wait for the first thread to wake up.
  wait_state(10);

  // Make sure a write does not wake the reader.
  std::cout << "func_2: Write. (3)\n";
  set_elem(buffer, 2);
  channel_ptr_1->try_write(buffer, sizeof(buffer));
  check_write_ptr(channel_ptr_1, 3);

  // Give the first thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Wake up the first thread.
  std::cout << "func_2: Wake. (3)\n";
  set_state(11);
  thread_ptr_1->wake();

  // Case 3: has_space returns true.
  wait_state(20);

  succ = nanotube_channel_has_space(context, 1);
  assert(succ);

  // Make sure a read does not wake the writer.
  set_state(21);
  nanotube_thread_wait();
  assert(state == 22);
  set_state(23);

  // Case 4: try_write returns true.
  wait_state(30);
  set_elem(buffer, 3);
  succ = channel_ptr_1->try_write(buffer, sizeof(buffer));
  assert(succ);
  check_write_ptr(channel_ptr_1, 4);

  // Make sure a read does not wake the writer.
  set_state(31);
  nanotube_thread_wait();
  assert(state == 32);
  set_state(33);

  // Case 5: has_space returns false.
  wait_state(40);

  // Fill the channel.
  for (int i=1; i<channel_capacity; i++) {
    set_elem(buffer, 3+i);
    succ = channel_ptr_1->try_write(buffer, sizeof(buffer));
    assert(succ);
  }
  check_read_ptr(channel_ptr_1, 3);
  check_write_ptr(channel_ptr_1, 3);

  // Call has_space.
  succ = nanotube_channel_has_space(context, 1);
  assert(!succ);

  // Make sure a read wakes the writer.
  set_state(41);
  nanotube_thread_wait();
  assert(state == 42);

  // Make sure a second read does not wake the writer.
  set_state(43);
  nanotube_thread_wait();
  assert(state == 44);

  set_state(45);

  // Case 6: try_write returns false.\n";
  wait_state(50);

  // Fill the channel.
  for (int i=0; i<2; i++) {
    set_elem(buffer, 3+i);
    succ = channel_ptr_1->try_write(buffer, sizeof(buffer));
    assert(succ);
  }

  // Try an extra write.
  set_elem(buffer, 5);
  succ = channel_ptr_1->try_write(buffer, sizeof(buffer));
  assert(!succ);
  
  // Make sure a read wakes the writer.
  set_state(51);
  nanotube_thread_wait();
  assert(state == 52);

  // Make sure a second read does not wake the writer.
  set_state(53);
  nanotube_thread_wait();
  assert(state == 54);

  set_state(55);

  while (true)
    nanotube_thread_wait();
}

int main()
{
  int rc;

  rc = pthread_mutex_init(&mutex_1, nullptr);
  if (rc != 0)
    fatal_error("pthread_mutex_init failed", rc);

  rc = pthread_cond_init(&cond_1, nullptr);
  if (rc != 0)
    fatal_error("pthread_cond_init failed", rc);

  nanotube_channel channel_1("channel_1", elem_size, channel_capacity);
  channel_ptr_1 = &channel_1;
  nanotube_channel channel_2("channel_2", elem_size, channel_capacity);
  channel_ptr_2 = &channel_2;

  if (sideband_size != 0) {
    channel_1.set_sideband_size(sideband_size);
    channel_2.set_sideband_size(sideband_size);
  }

  if (sideband_signals) {
    channel_1.set_sideband_signals_size(sideband_signals);
    channel_2.set_sideband_signals_size(sideband_signals);
  }

  nanotube_context main_context;
  nanotube_thread main_thread(&main_context);

  char data_1[] = "data_1";
  nanotube_context context_1;
  context_1.add_channel(0, &channel_1, NANOTUBE_CHANNEL_READ);
  context_1.add_channel(5, &channel_2, NANOTUBE_CHANNEL_WRITE);
  nanotube_thread thread_1(&context_1, "thread_1", &func_1, data_1,
                           sizeof(data_1));
  thread_ptr_1 = &thread_1;

  char data_2[] = "data_2";
  nanotube_context context_2;
  context_2.add_channel(0, &channel_2, NANOTUBE_CHANNEL_READ);
  context_2.add_channel(1, &channel_1, NANOTUBE_CHANNEL_WRITE);
  nanotube_thread thread_2(&context_2, "thread_2", &func_2, data_2,
                           sizeof(data_2));
  thread_ptr_2 = &thread_2;

  thread_1.start();
  thread_2.start();

  struct timespec ts;
  rc = clock_gettime(CLOCK_REALTIME, &ts);
  if (ts.tv_nsec < ns_per_s - test_timeout) {
    ts.tv_nsec += test_timeout;
  } else {
    ts.tv_sec += 1;
    ts.tv_nsec -= (ns_per_s - test_timeout);
  }

  rc = pthread_mutex_lock(&mutex_1);
  if (rc != 0)
    fatal_error("pthread_mutex_lock failed", rc);

  while (state != -2) {
    rc = pthread_cond_timedwait(&cond_1, &mutex_1, &ts);

    if (rc == ETIMEDOUT) {
      fprintf(stderr, "Test timed out in state %d.\n", state);
      exit(1);
    }

    if (rc != 0)
      fatal_error("pthread_cond_timedwait", rc);
  }

  rc = pthread_mutex_unlock(&mutex_1);
  if (rc != 0)
    fatal_error("pthread_mutex_lock failed", rc);

  thread_1.stop();
  thread_2.stop();

  printf("Test complete!\n");
  
  return 0;
}
