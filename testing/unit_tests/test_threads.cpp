/*******************************************************/
/*! \file test_threads.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief Unit tests for Nanotube threads implementations.
**   \date 2020-09-10
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/
#include "nanotube_context.hpp"
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
int state = -1;

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

void func_1(nanotube_context_t *context, void *arg)
{
  assert(strcmp((char*)arg, "data_1") == 0);
  thread_ptr_1->check_current();

  std::cout << "func_1: Started.\n";

  assert(state == 0);

  // Indicate that this thread has started.
  set_state(1);

  // Wait for the second thread to wake this thread.
  nanotube_thread_wait();

  std::cout << "func_1: Woken (1).\n";

  // Make sure the second thread has woken this thread.
  assert(state == 2);

  // Indicate that this thread has woken.
  set_state(3);

  // Wait for the second thread to set a pending wake.
  wait_state(4);

  std::cout << "func_1: Sleeping again.\n";

  // This should return immediately since the wait is pending.
  nanotube_thread_wait();

  std::cout << "func_1: Woken (2).\n";

  // Indicate that the thread is about to go to sleep again.
  set_state(5);

  // Go to sleep again.
  nanotube_thread_wait();

  // Make sure the second thread has woken this thread.
  assert(state == 6);

  std::cout << "func_1: Woken (3).\n";

  // Indicate that the test is complete.
  set_state(10);

  while (true) {
    nanotube_thread_wait();
  }
}

void func_2(nanotube_context_t *context, void *arg)
{
  int rc;

  assert(strcmp((char*)arg, "data_2") == 0);
  thread_ptr_2->check_current();
  assert(state != -1);

  // Wait for the first thread to start.
  wait_state(1);

  std::cout << "func_2: Started.\n";

  // Give the first thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  std::cout << "func_2: Wake up! (1)\n";

  // Make sure the first thread has not woken up.
  assert(state == 1);

  // Indicate that the first thread can wake up.
  set_state(2);

  // Wake the first thread.
  thread_ptr_1->wake();

  // Wait for the first thread to wake up.
  wait_state(3);

  std::cout << "func_2: Wake up! (2)\n";

  // Wake the first thread while it is not sleeping.
  thread_ptr_1->wake();

  // Allow the first thread to go to sleep.
  set_state(4);

  // Wait for the first thread to go to sleep again.
  wait_state(5);

  std::cout << "func_2: Delaying...\n";

  // Give the first thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  std::cout << "func_2: Wake up! (3)\n";

  // Indicate that the first thread is allowed to wake up.
  set_state(6);

  // Wake the first thread again.
  thread_ptr_1->wake();

  while (true) {
    nanotube_thread_wait();
  }
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

  nanotube_context main_context;
  nanotube_thread main_thread(&main_context);

  char data_1[] = "data_1";
  nanotube_context context_1;
  nanotube_thread thread_1(&context_1, "thread_1", &func_1, data_1,
                           sizeof(data_1));
  thread_ptr_1 = &thread_1;

  char data_2[] = "data_2";
  nanotube_context context_2;
  nanotube_thread thread_2(&context_2, "thread_2", &func_2, data_2,
                           sizeof(data_2));
  thread_ptr_2 = &thread_2;

  // Make sure the threads do not start before the start request.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Allow the threads to start.
  state = 0;

  // Start the threads.
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

  while (state != 10) {
    rc = pthread_cond_timedwait(&cond_1, &mutex_1, &ts);

    if (rc == ETIMEDOUT) {
      fprintf(stderr, "Test timed out.\n");
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
