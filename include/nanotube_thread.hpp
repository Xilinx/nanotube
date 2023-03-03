/**************************************************************************\
*//*! \file nanotube_thread.hpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  Implementation of Nanotube threads.
**   \date  2020-09-03
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_THREAD_HPP
#define NANOTUBE_THREAD_HPP

#include "nanotube_api.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include <boost/date_time/posix_time/posix_time_types.hpp>

extern "C" {
#include <pthread.h>
}

///////////////////////////////////////////////////////////////////////////

/*! A guard class for locking stderr.
**
** This class can be used to lock stderr.  Create an instance of the
** class to hold the lock for the lifetime of the instance.  A static
** member can be used to get direct access to the mutex.
*/

class nanotube_stderr_guard
{
public:
  // Create the guard instance.
  nanotube_stderr_guard(): m_guard(s_stderr_mutex) {}

  // Get access to the mutex.
  static std::mutex &stderr_mutex() { return s_stderr_mutex; }

private:
  // The stderr mutex.
  static std::mutex s_stderr_mutex;

  // 
  std::lock_guard<std::mutex> m_guard;
};

///////////////////////////////////////////////////////////////////////////

class nanotube_thread;

/*! A class for waiting until threads are idle.
**
** To wait until all of a collection of threads are idle, create an
** object of this class, call the monitor_thread method for each
** thread and then poll is_idle() to determine whether all the threads
** are idle, calling nanotube_thread_wait() after is_idle() returns
** false to wait until the monitored threads become idle.
*/
class nanotube_thread_idle_waiter
{
public:
  /*! Construct an Nanotube thread idle waiter. */
  nanotube_thread_idle_waiter();

  /*! Destroy an Nanotube thread idle waiter. */
  ~nanotube_thread_idle_waiter();

  /*! Add a thread to be monitored. */
  void monitor_thread(nanotube_thread &thread);

  /*! Check whether all the monitored threads are idle. */
  bool is_idle();

private:
  friend class nanotube_thread;

  // Increment the busy count.  This is called when a monitored thread
  // is woken.
  void inc_busy_count();

  // Decrement the busy count.  This is called when a monitored thread
  // goes to sleep.
  void dec_busy_count();

  // The number of threads which are not sleeping.
  std::atomic<size_t> m_busy_count;

  // The thread which should be woken when all monitored threads are
  // idle.
  nanotube_thread *m_waiter;

  // The threads which are being monitored.
  std::vector<nanotube_thread *> m_threads;
};

class nanotube_thread
{
public:
  // Types related to clocks.
  typedef boost::posix_time::ptime time_point_t;
  typedef boost::posix_time::time_duration duration_t;

  /*! Contruct the main thread.
  //
  // The main thread is bound to the pthread which called this
  // constructor.
  //
  // \param context The context which holds resources for the thread.
  */
  nanotube_thread(nanotube_context_t *context);

  /*! Contruct a user thread.
  //
  // A user thread will execute a function which is supplied by the
  // caller, passing it a copy of the block of data which is provided
  // by the caller.
  //
  // \param context    The context which holds resources for the thread.
  // \param name       The name of the thread.
  // \param func       The function to execute in the thread.
  // \param data       The start of the data to copy.
  // \param data_size  The number of bytes of data to copy.
  */
  nanotube_thread(nanotube_context_t *context,
                  const char *name,
                  nanotube_thread_func_t *func,
                  const void *data,
                  size_t data_size);

  /*! Destroy the thread. */
  ~nanotube_thread();

  /*! Destroy a thread and release the memory. */
  static void destory(std::unique_ptr<nanotube_thread> &&thread);

  /*! Determine whether the thread is the current thread. */
  bool is_current();

  /*! Determine whether the thread is stopped. */
  bool is_stopped();

  /*! Get the current time.
  //
  // \returns the current time.
  */
  static time_point_t get_current_time();

  /*! Initialize a timer with the specified duration.
  //
  // This call should be called to initialise the timer before calling
  // check_timer to find out whether the timer has expired.
  //
  // \param timer     The timer to initialise.
  // \param duration  The duration of the timer.
  */
  static void init_timer(time_point_t &timer,
                         const duration_t &duration);

  /*! Check whether the specified time has been reached.
  //
  // \param timer     The timer to examine.
  //
  // \returns a flag indicating whether the timer has expired.
  */
  static bool check_timer(const time_point_t &timer);

  /*! Create the underlying pthread. */
  void start();

  /*! Stop a thread and return when complete.
  // 
  // This call causes the thread to exit when convenient and waits
  // until it has done so.  It must be called from a different thread.
  */
  void stop();

  /*! Make sure this thread object is the calling thread.
  //
  // This call checks whether this thread is the called thread.  If it
  // is not, it reports a fatal error.  It also checks whether a stop
  // request is pending and causes the thread to stop if there is.
  */
  void check_current();

  /*! Causes the thread to sleep until wake() is called.
  //
  // This method causes the current thread to suspend execution and
  // resume when the wake() method is called.  If the wake() method
  // has already been called since the previous call to sleep(), this
  // method returns without sleeping.  This method must be called from
  // the associated pthread.
  */
  void sleep();

  /*! Wake a thread from its current or next call to sleep().
  //
  // This method wakes up the thread.  If the thread is currently
  // sleeping in a call to sleep() then it is woken.  If it is not
  // sleeping then the next call to sleep() will return without
  // sleeping.  Usually called from a different thread, but .
  */
  void wake();

  /*! Send a signal to the thread if it has been started.
  //
  // \param sig The signal to send.
  */
  void kill(int sig);

  std::string get_name() { return m_name; }
private:
  friend class nanotube_thread_idle_waiter;

  /*! Create the mutex and condition variable. */
  void init();

  /*! Initialize m_current_time. */
  void init_current_time();

  /*! Call the underlying Nanotube thread function. */
  static void *enter_thread(void *thread);

  /*! Call pthread_exit if an exit request is pending.
  //
  // Called from the associated pthread.
  */
  void check_stop();

  /*! Set the idle waiter for the thread.
  //
  // This method is called from the monitor_thread method of an idle
  // waiter to start monitoring the thread.  There can only be one
  // idle waiter associated with a thread at any one time.
  */
  void set_idle_waiter(nanotube_thread_idle_waiter &waiter);

  /*! Unset the idle waiter for the thread.
  //
  // This method is called from the destructor of an idle waiter to
  // stop monitoring the thread.
  */
  void unset_idle_waiter(nanotube_thread_idle_waiter &waiter);

  typedef enum thread_state: uint32_t {
    THREAD_STATE_INIT,
    THREAD_STATE_RUNNING,
    THREAD_STATE_STOP_REQ,
    THREAD_STATE_STOPPED,
  } thread_state_t;

  // Wake state transitions are as follows:
  //   Running -> Sleeping: By the thread itself, with the mutex held.
  //   Running -> Wake: By another thread.
  //   Sleeping -> Running: By the thread itself, with the mutex held.
  //   Sleeping -> Wake: By another thread, with the mutex held, must wake.
  //   Wake -> Running: By the thread itself.
  //   Wake -> Sleeping: Invalid.
  typedef enum wake_state: uint32_t {
    WAKE_STATE_RUNNING,
    WAKE_STATE_SLEEPING,
    WAKE_STATE_WAKE,
  } wake_state_t;

  // The context which holds resources for the thread.
  nanotube_context_t *m_context;

  // A flag indicating that this is the main thread.  The main thread
  // inherits an already created pthread and does not have a thread
  // function.
  bool m_is_main_thread;

  // The name of the thread.
  std::string m_name;

  // The function to enter to run the thread.
  nanotube_thread_func_t *m_func;

  // Arbitrary data which is passed to the thread function.
  std::vector<char> m_user_data;

  // The state used for starting and stopping the thread.
  std::atomic<thread_state_t> m_thread_state;

  // The state used for sleeping and for waking the thread.
  std::atomic<wake_state_t> m_wake_state;

  // A mutex which protects m_wake_state.
  pthread_mutex_t m_mutex;

  // A condition variable to wake the thread.
  pthread_cond_t m_cond;

  // The pthread ID of the thread.
  pthread_t m_thread_id;

  // Whether m_current_time is valid.
  bool m_current_time_valid;

  // The current time.
  time_point_t m_current_time;

  // Whether m_timeout is valid.
  bool m_wake_time_valid;

  // The earliest time to wake the thread.
  time_point_t m_wake_time;

  // The idle waiter for the thread.  The thread will update the idle
  // waiter whenever it transitions into or out of the SLEEPING state.
  // This member is protected by m_mutex.
  nanotube_thread_idle_waiter *m_idle_waiter;
};

///////////////////////////////////////////////////////////////////////////

#endif // NANOTUBE_THREAD_HPP
