/*******************************************************/
/*! \file test_timers.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief Unit tests for Nanotube thread timers.
**   \date 2020-09-14
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
#include <iomanip>
#include <iostream>

extern "C" {
#include <pthread.h>
#include <unistd.h>
}

static const long ns_per_s = 1000*1000*1000;
static const long test_timeout = ns_per_s/2;

pthread_mutex_t mutex_1;
pthread_cond_t cond_1;

#define STATE_INIT -1
#define STATE_END -2
int state = STATE_INIT;

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
  int rc;

  // Start the test.
  set_state(0);

  std::cout << "func_1: Started.\n";

  // Wait for 100 ms.
  timespec start_ts;
  rc = clock_gettime(CLOCK_REALTIME, &start_ts);
  if (rc != 0)
    fatal_error("clock_gettime (start_ts)", rc);

  nanotube_thread::time_point_t timer;
  nanotube_thread::init_timer(timer, boost::posix_time::millisec(100));
  assert(!nanotube_thread::check_timer(timer));
  nanotube_thread_wait();
  assert(nanotube_thread::check_timer(timer));

  timespec end_ts;
  rc = clock_gettime(CLOCK_REALTIME, &end_ts);
  if (rc != 0)
    fatal_error("clock_gettime (end_ts)", rc);

  std::cout << "func_1: Woken.\n";

  // Check the time spent asleep.
  timespec delta_ts = {0};
  delta_ts.tv_sec = end_ts.tv_sec - start_ts.tv_sec;
  if (end_ts.tv_nsec >= start_ts.tv_nsec) {
    delta_ts.tv_nsec = end_ts.tv_nsec - start_ts.tv_nsec;
  } else {
    delta_ts.tv_nsec = ns_per_s + end_ts.tv_nsec - start_ts.tv_nsec;
    delta_ts.tv_sec -= 1;
  }

  if (delta_ts.tv_sec != 0 ||
      delta_ts.tv_nsec < 100*1000*1000 ||
      delta_ts.tv_nsec > 150*1000*1000) {
    std::cout << "func_1: Failed sleep delay: "
              << delta_ts.tv_sec << "."
              << std::setw(9) << std::setfill('0')
              << delta_ts.tv_nsec
              << std::setw(0) << std::setfill(' ')
              << "\n";
    exit(1);
  }

  // Make sure nanotube_thread_wait does not return immediately.
  std::cout << "func_1: Sleeping again.\n";
  set_state(1);
  nanotube_thread_wait();
  assert(state == 2);

  std::cout << "func_1: Finished.\n";

  // Indicate that the test is complete.
  set_state(STATE_END);

  while (true) {
    nanotube_thread_wait();
  }
}

void func_2(nanotube_context_t *context, void *arg)
{
  int rc;

  // Wait for the first thread to finish its sleep.
  wait_state(1);
  std::cout << "func_2: Started.\n";

  // Give the first thread time to go to sleep.
  rc = usleep(1000);
  if (rc != 0)
    fatal_error("usleep", errno);

  // Wake up the thread.
  std::cout << "func_2: Wake up!\n";
  set_state(2);
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

  while (state != STATE_END) {
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
