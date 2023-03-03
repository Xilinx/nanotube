/**************************************************************************\
*//*! \file nanotube_thread.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Implementation of Nanotube threads.
**   \date  2020-09-03
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_thread.hpp"

#include "nanotube_context.hpp"
#include "processing_system.hpp"

#include <atomic>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/thread/xtime.hpp>

///////////////////////////////////////////////////////////////////////////

static thread_local nanotube_thread *s_current_thread = nullptr;

///////////////////////////////////////////////////////////////////////////

static void fatal_error(const std::string &message, int rc)
{
  std::cerr << "ERROR: " << message << ": "
            << strerror(rc) << " (rc=" << rc << ").\n";
  exit(1);
}

///////////////////////////////////////////////////////////////////////////

std::mutex nanotube_stderr_guard::s_stderr_mutex;

///////////////////////////////////////////////////////////////////////////

nanotube_thread_idle_waiter::nanotube_thread_idle_waiter():
  m_busy_count(0),
  m_waiter(s_current_thread)
{
  assert(m_waiter != nullptr);
  assert(m_waiter->m_is_main_thread);
}

nanotube_thread_idle_waiter::~nanotube_thread_idle_waiter()
{
  // Disconnect the waiter from all the threads.
  for (auto thread: m_threads) {
    thread->unset_idle_waiter(*this);
  }
  assert(m_busy_count.load() == 0);
}

void nanotube_thread_idle_waiter::monitor_thread(nanotube_thread &thread)
{
  assert(!thread.m_is_main_thread);
  thread.set_idle_waiter(*this);
  m_threads.push_back(&thread);
}

bool nanotube_thread_idle_waiter::is_idle()
{
  size_t busy_count = m_busy_count.load();
  return (busy_count == 0);
}

void nanotube_thread_idle_waiter::inc_busy_count()
{
  size_t old_val = m_busy_count.fetch_add(+1);
  assert(old_val+1 > 0);
}

void nanotube_thread_idle_waiter::dec_busy_count()
{
  size_t old_val = m_busy_count.fetch_add(-1);
  assert(old_val != 0);
  if (old_val == 1)
    m_waiter->wake();
}

///////////////////////////////////////////////////////////////////////////

nanotube_thread::nanotube_thread(nanotube_context_t* context):
  m_context(context),
  m_is_main_thread(true),
  m_name("main"),
  m_func(nullptr),
  m_thread_state(THREAD_STATE_RUNNING),
  m_wake_state(WAKE_STATE_RUNNING),
  m_thread_id(pthread_self()),
  m_current_time_valid(false),
  m_wake_time_valid(false),
  m_idle_waiter(nullptr)
{
  assert(s_current_thread == nullptr);
  s_current_thread = this;
  init();
}

nanotube_thread::nanotube_thread(nanotube_context_t* context,
                                 const char* name,
                                 nanotube_thread_func_t* func,
                                 const void* data,
                                 size_t data_size):
  m_context(context),
  m_is_main_thread(false),
  m_name(name),
  m_func(func),
  m_user_data(((const uint8_t*)data),
              ((const uint8_t*)data) + data_size),
  m_thread_state(THREAD_STATE_INIT),
  m_wake_state(WAKE_STATE_RUNNING),
  m_current_time_valid(false),
  m_wake_time_valid(false),
  m_idle_waiter(nullptr)
{
  init();
}

void nanotube_thread::init()
{
  int rc = pthread_mutex_init(&m_mutex, nullptr);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_init", rc);

  rc = pthread_cond_init(&m_cond, nullptr);
  if (rc != 0)
    fatal_error("Error calling pthread_cond_init", rc);

  m_context->bind_thread(this);
}

nanotube_thread::~nanotube_thread()
{
  // Make sure there is no waiter.
  assert(m_idle_waiter == nullptr);

  // Make sure the thread is stopped.
  stop();

  // Clear the thread pointer if this is the main thread.  Other
  // threads will do this when a stop request is received.
  if (m_is_main_thread) {
    assert(s_current_thread == this);
    s_current_thread = nullptr;
  }

  // Unbind the thread from the context.
  m_context->unbind_thread(this);

  // Destroy the pthread resources.
  int rc = pthread_cond_destroy(&m_cond);
  if (rc != 0)
    fatal_error("Error calling pthread_cond_destroy", rc);

  rc = pthread_mutex_destroy(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_destroy", rc);
}

void nanotube_thread::destory(std::unique_ptr<nanotube_thread> &&thread)
{
  thread.reset();
}

bool nanotube_thread::is_current()
{
  return this == s_current_thread;
}

bool nanotube_thread::is_stopped()
{
  auto state = m_thread_state.load();
  return (state == THREAD_STATE_INIT);
}

void nanotube_thread::init_current_time()
{
  typedef boost::posix_time::microsec_clock clock_t;
  m_current_time = clock_t::universal_time();
  m_current_time_valid = true;
}

nanotube_thread::time_point_t nanotube_thread::get_current_time()
{
  assert(s_current_thread != nullptr);
  if (!s_current_thread->m_current_time_valid)
    s_current_thread->init_current_time();

  return s_current_thread->m_current_time;
}

void nanotube_thread::init_timer(time_point_t &timer,
                                 const duration_t &duration)
{
  // Make sure we read the current time.
  assert(s_current_thread != nullptr);
  if (!s_current_thread->m_current_time_valid)
    s_current_thread->init_current_time();

  // Add the duration to the current time.
  timer = s_current_thread->m_current_time + duration;
}

bool nanotube_thread::check_timer(const time_point_t &timer)
{
  // Make sure we read the current time.
  assert(s_current_thread != nullptr);
  if (!s_current_thread->m_current_time_valid)
    s_current_thread->init_current_time();

  // If the timer has fired, return true.
  if (timer <= s_current_thread->m_current_time)
    return true;

  // Make sure the wake time has been set.
  if (!s_current_thread->m_wake_time_valid ||
      s_current_thread->m_wake_time > timer) {
    s_current_thread->m_wake_time = timer;
    s_current_thread->m_wake_time_valid = true;
  }

  // Indicate that the timer has not fired.
  return false;
}

void *nanotube_thread::enter_thread(void *arg)
{
  nanotube_thread *thread = (nanotube_thread*)arg;

  // Set the thread ID.
  s_current_thread = thread;

  // Call the thread function repeately.
  while (true)
    thread->m_func(thread->m_context, &thread->m_user_data[0]);
}

void nanotube_thread::start()
{
  // This should be called from the main thread to avoid race
  // conditions on m_thread_state.
  assert(s_current_thread->m_is_main_thread);

  // Make sure there is no waiter.
  assert(m_idle_waiter == nullptr);

  // Ignore start requests on the main thread.
  if (m_is_main_thread)
    return;

  // Start the thread under the mutex so that nanotube_thread::kill()
  // knows whether m_thread_id is valid.
  int rc = pthread_mutex_lock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_lock", rc);

  // Create the thread.
  assert(m_thread_state.load() == THREAD_STATE_INIT);
  m_thread_state.store(THREAD_STATE_RUNNING);
  rc = pthread_create(&m_thread_id, nullptr, enter_thread, (void*)this);
  if (rc != 0)
    fatal_error("Error calling pthread_create", rc);

  rc = pthread_mutex_unlock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_unlock", rc);
}

void nanotube_thread::check_stop()
{
  thread_state_t thread_state = m_thread_state.load();
  if (thread_state == THREAD_STATE_RUNNING)
    return;

  assert(thread_state == THREAD_STATE_STOP_REQ);
  m_thread_state.store(THREAD_STATE_STOPPED);

  // Clear the thread ID.
  assert(s_current_thread == this);
  s_current_thread = nullptr;

  // Exit the thread.
  pthread_exit(nullptr);
}

void nanotube_thread::stop()
{
  // This should be called from the main thread to avoid race
  // conditions on m_thread_state.
  assert(s_current_thread->m_is_main_thread);

  // Make sure there is no waiter.
  assert(m_idle_waiter == nullptr);

  // The main thread ignores stop requests.
  if (m_is_main_thread)
    return;

  // Do not stop a stopped thread.
  thread_state_t thread_state = m_thread_state.load();
  if (thread_state == THREAD_STATE_INIT)
    return;

  // Transition to the STOP_REQ state under the mutex so that
  // nanotube_thread::kill knows whether m_thread_id is valid.
  int rc = pthread_mutex_lock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_lock", rc);

  // Indicate a stop request.
  assert(thread_state == THREAD_STATE_RUNNING);
  m_thread_state.store(THREAD_STATE_STOP_REQ);

  rc = pthread_mutex_unlock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_unlock", rc);

  // Wake the thread to make sure it notices the stop request.
  wake();

  // Wait for the thread to exit.
  rc = pthread_join(m_thread_id, nullptr);
  if (rc != 0)
    fatal_error("Error calling pthread_join", rc);

  // Return the thread to the init state.
  assert(m_thread_state.load() == THREAD_STATE_STOPPED);
  m_thread_state.store(THREAD_STATE_INIT);
}

void nanotube_thread::check_current()
{
  assert(s_current_thread != nullptr);
  assert(s_current_thread == this);
  check_stop();
}

void nanotube_thread::sleep()
{
  // Check for a stop request.
  check_stop();

  // Quick exit if possible.
  wake_state_t wake_state = WAKE_STATE_WAKE;
  if (m_wake_state.compare_exchange_weak(wake_state, WAKE_STATE_RUNNING))
    return;

  // Hold the mutex while going to sleep to break race conditions.
  int rc = pthread_mutex_lock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_lock", rc);

  // Try to transition from running to sleeping.  This should only
  // fail if another thread has called wake(), causing a transition
  // from running to wake.
  wake_state = WAKE_STATE_RUNNING;
  if (m_wake_state.compare_exchange_strong(wake_state, WAKE_STATE_SLEEPING)) {
    // Notify the waiter that the thread has entered the sleeping state.
    if (m_idle_waiter != nullptr)
      m_idle_waiter->dec_busy_count();

    // Call either pthread_cond_wait or pthread_cond_timedwait
    // depending on whether there is a wake time.
    if (m_wake_time_valid) {
      boost::xtime xt = boost::get_xtime(m_wake_time);
      timespec ts = { .tv_sec = xt.sec, .tv_nsec = xt.nsec };
      rc = pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
      if (rc != 0 && rc != ETIMEDOUT)
        fatal_error("Error calling pthread_cond_timedwait", rc);

      // Cancel the wake time.  The caller should poll the timer
      // again.
      m_wake_time_valid = false;

    } else {
      rc = pthread_cond_wait(&m_cond, &m_mutex);
      if (rc != 0)
        fatal_error("Error calling pthread_cond_wait", rc);
    }

    // Time has passed, so invalidate the current time.
    m_current_time_valid = false;
  }

  // Reset the state to running.
  wake_state_t old_state = m_wake_state.exchange(WAKE_STATE_RUNNING);

  // Notify the waiter that the thread has exited the SLEEPING state
  // if this call made the transition.
  if (old_state == WAKE_STATE_SLEEPING && m_idle_waiter != nullptr)
    m_idle_waiter->inc_busy_count();

  // Drop the mutex now that the thread has woken.
  rc = pthread_mutex_unlock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_unlock", rc);

  // Check for a stop request again since one may have arrived while
  // the thread was sleeping.
  check_stop();
}

void nanotube_thread::wake()
{
  // Do nothing if the thread has already been woken.
  if (m_wake_state.load() == WAKE_STATE_WAKE)
    return;

  // If the thread is running, we can just transition to the wake
  // state.
  wake_state_t wake_state = WAKE_STATE_RUNNING;
  if (m_wake_state.compare_exchange_strong(wake_state, WAKE_STATE_WAKE))
    return;

  // Do nothing if another thread woke the target thread at the same
  // time.
  if (wake_state == WAKE_STATE_WAKE)
    return;

  // We need to wake a sleeping thread.  Acquire the mutex to break
  // race conditions.
  int rc = pthread_mutex_lock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_lock", rc);

  wake_state = m_wake_state.load();
  switch (wake_state) {
  case WAKE_STATE_RUNNING:
    // Since we acquired the mutex, the following transition will only
    // fail if some other thread also woke the target thread.
    if (!m_wake_state.compare_exchange_strong(wake_state, WAKE_STATE_WAKE))
      assert(wake_state == WAKE_STATE_WAKE);
    break;

  default:
  case WAKE_STATE_SLEEPING:
    // All transitions out of the sleeping state must have the mutex
    // held so there is no race condition here.
    m_wake_state.store(WAKE_STATE_WAKE);

    rc = pthread_cond_signal(&m_cond);
    if (rc != 0)
      fatal_error("Error calling pthread_cond_signal", rc);

    // Notify the waiter that this thread exited the SLEEPING state.
    if (m_idle_waiter != nullptr)
      m_idle_waiter->inc_busy_count();

    break;

  case WAKE_STATE_WAKE:
    // Another thread woke the target thread, so do nothing.
    break;
  }

  rc = pthread_mutex_unlock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_unlock", rc);
}

void nanotube_thread::kill(int sig)
{
  int rc = pthread_mutex_lock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_lock", rc);

  // Send the signal if the thread ID is valid.
  auto state = m_thread_state.load();
  if (state == THREAD_STATE_RUNNING) {
    rc = pthread_kill(m_thread_id, sig);
    if (rc != 0)
      fatal_error("Error calling pthread_kill", rc);
  }

  rc = pthread_mutex_unlock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_unlock", rc);
}

void
nanotube_thread::set_idle_waiter(nanotube_thread_idle_waiter &waiter)
{
  // Acquire the mutex to update m_idle_waiter.
  int rc = pthread_mutex_lock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_lock", rc);

  assert(m_idle_waiter == nullptr);
  m_idle_waiter = &waiter;

  if (m_wake_state.load() != WAKE_STATE_SLEEPING)
    waiter.inc_busy_count();

  // Acquire the mutex to update m_idle_waiter.
  rc = pthread_mutex_unlock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_unlock", rc);
}

void
nanotube_thread::unset_idle_waiter(nanotube_thread_idle_waiter &waiter)
{
  // Acquire the mutex to update m_idle_waiter.
  int rc = pthread_mutex_lock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_lock", rc);

  assert(m_idle_waiter == &waiter);
  m_idle_waiter = nullptr;

  if (m_wake_state != WAKE_STATE_SLEEPING)
    waiter.dec_busy_count();

  // Acquire the mutex to update m_idle_waiter.
  rc = pthread_mutex_unlock(&m_mutex);
  if (rc != 0)
    fatal_error("Error calling pthread_mutex_unlock", rc);
}

///////////////////////////////////////////////////////////////////////////

void nanotube_thread_create(nanotube_context_t* context,
                            const char* name,
                            nanotube_thread_func_t* func,
                            const void* data,
                            size_t data_size)
{
  std::unique_ptr<nanotube_thread> thread(
    new nanotube_thread(context, name, func, data, data_size));

  processing_system &ps = processing_system::get_current();
  ps.add_thread(std::move(thread));
}

void nanotube_thread_wait()
{
  assert(s_current_thread != nullptr);
  s_current_thread->sleep();
}

///////////////////////////////////////////////////////////////////////////
