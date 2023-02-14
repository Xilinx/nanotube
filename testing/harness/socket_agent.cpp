/*******************************************************/
/*! \file socket_agent.cpp
** \author Neil Turton <neilt@amd.com>
**  \brief A test agent to transport packets over a socket.
**   \date 2022-10-24
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

// The socket agent
// ================
//
// The socket_agent class provides interactive access to the testbench
// via a socket using the QEMU socket protocol.  This makes it
// possible to send packets into and receive packets from the Nanotube
// kernel being tested.
//
// The QEMU socket protocol encodes packets in a very simple way.
// Each packet is prefixed with a header containing a 4-byte packet
// length in network byte order.
//
// Lifetimes
// ---------
//
// Shared pointers are used to avoid dangling references.  This means
// that object destruction only occurs after all references have gone
// away.  Short-lived objects are used for the completion of async
// operations.  The objects hold references to the required long-lived
// objects.
//
// The start_test and end_test callbacks are used to start and
// stop IO operations.  Canceling an IO operation doesn't wait until
// the callback has completed, so mutexes and flags are used to make
// sure a callback has no effect.
//
// For simplicity there are only two mutexes per socket_agent, one for
// sending and one for receiving.  These are stored in the impl class.
// The receive mutex is held while processing received data, which may
// involve the framework calling our receive_packet callback.  The
// send mutex is only held within the receive_packet callback.  Some
// members are protected by both mutexes which means that readers need
// only hold one of the mutexes, but writers need to hold both.

#include "socket_agent.hpp"

#include "nanotube_packet.hpp"
#include "test_harness.hpp"

#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/write.hpp"

extern "C" {
#include <arpa/inet.h>
}

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <unordered_set>
#include <utility>

namespace asio = boost::asio;
using boost::asio::ip::tcp;

typedef std::mutex mutex;
typedef std::lock_guard<mutex> lock_guard;

static const std::size_t BUFFER_SIZE = 1024;
static const std::size_t PREFIX_LENGTH = sizeof(uint32_t);
static const std::size_t MTU = 16384;

#define SOCKET_DEBUG 0

///////////////////////////////////////////////////////////////////////////

// Examples:
//   packet,listen,tx:12345
//   capsule,connect:localhost:12345

static const std::string F_CONNECT = "connect";
static const std::string F_LISTEN  = "listen";
static const std::string F_RX      = "rx";
static const std::string F_TX      = "tx";
static const std::string F_PACKET  = "packet";
static const std::string F_CAPSULE = "capsule";

static const std::map<std::string, std::string> flag_map = {
  { F_CONNECT,     "Connect to the remote socket." },
  { F_LISTEN,      "Listen for incoming connections." },
  { F_RX,          "Use the socket for RX." },
  { F_TX,          "Use the socket for TX." },
  { F_PACKET,      "Transfer packets on the socket." },
  { F_CAPSULE,     "Transfer capsules on the socket." },
};

///////////////////////////////////////////////////////////////////////////

namespace {
  // A structured representation of the agent parameters.
  class param_strings;

  // An async receive operation.
  class async_recv;

  // A socket connection.
  class connection;

  // An async accept operation.
  class async_accept;

  // The socket agent implementation.
  typedef socket_agent::impl impl;

  // A shared pointer to the agent implementation.
  typedef std::shared_ptr<impl> impl_ptr_t;

  // A shared pointer to a connection.
  typedef std::shared_ptr<connection> connection_ptr_t;
}

///////////////////////////////////////////////////////////////////////////

static void fatal_error()
{
 std::cerr << "Socket parameter format: <flags>:[<hostname>:]<port>\n"
            << "Flags:\n";
  auto mask = std::ios::left|std::ios::right|std::ios::internal;
  auto old_flags = std::cerr.setf(std::ios::left, mask);
  for (auto it=flag_map.begin(); it != flag_map.end(); it++) {
    std::cerr << "  " << std::setw(10) << it->first
              << std::setw(0) << it->second << '\n';

  }
  std::cerr.flags(old_flags);
  exit(1);
}

///////////////////////////////////////////////////////////////////////////

namespace {
  class param_strings {
  public:
    typedef std::unordered_set<std::string> str_set_t;

    param_strings(const std::string &s);

    // The substring before the first colon.
    std::string m_flags_str;

    // The substring between the first and last colons, if any.
    std::string m_host_str;

    // The substring after the last colon.
    std::string m_port_str;

    // The flags which have been seen. 
    str_set_t m_flag_set;

  private:
    void split_params(const std::string &s);
    void split_flags();
    void check_flags();
  };
}

///////////////////////////////////////////////////////////////////////////

namespace {
  class async_recv {
  public:
    explicit async_recv(impl_ptr_t impl,
                        connection_ptr_t conn);

    void operator() (const boost::system::error_code &error,
                     std::size_t bytes_read);

    impl_ptr_t m_impl;
    connection_ptr_t m_connection;
  };
}

///////////////////////////////////////////////////////////////////////////

namespace {
  class connection {
  public:
    explicit connection(tcp::socket &&socket);

    ~connection();

    // Start processing packets.
    void start_rs_locked(const impl_ptr_t *impl,
                         const connection_ptr_t *self);

    // Stop processing packets.
    void stop_rs_locked();

    // Start an async receive operation.
    void start_receive_r_locked(impl_ptr_t impl,
                                connection_ptr_t self);

    // Finish an async receive operation.
    void finish_receive(async_recv &&recv_op,
                        const boost::system::error_code *error,
                        std::size_t bytes_read);

    // Process data which has arrived.  Returns true on success.
    bool process_rx_data_r_locked(impl *the_impl,
                                  std::size_t bytes_read);

    // Send a packet.
    void send_packet_s_locked(const nanotube_packet_t *packet);

  private:
    // A flag indicating whether the connection is processing packets.
    // Changes only occur with both m_recv_mutex and m_send_mutex
    // held.
    bool m_active;

    // The connected socket.  Receive operations are protected by
    // m_recv_mutex.  Send operations are protected by m_send_mutex.
    tcp::socket m_socket;

    // The RX buffer.  Protected by m_recv_mutex.
    char m_rx_buffer[BUFFER_SIZE];

    // The Nanotube packet which is being received.  Protected by
    // m_recv_mutex.
    nanotube_packet_t m_rx_packet;

    // The offset into the current packet being received, including
    // the length prefix.  Protected by m_recv_mutex.
    std::size_t m_rx_offset;
  };
}

///////////////////////////////////////////////////////////////////////////
namespace {
  class async_accept {
  public:
    explicit async_accept(impl_ptr_t impl_ptr);

    void operator() (const boost::system::error_code &error);

    impl_ptr_t m_impl;
  };
}

///////////////////////////////////////////////////////////////////////////

class socket_agent::impl {
public:
  impl(socket_agent* agent, const std::string &params);
  ~impl();

public:
  // The receive mutex.  Held for the duration of a receive
  // operation, which may involve send operations.  May not be
  // acquired with the send mutex held.
  mutex m_recv_mutex;

  // The send mutex.  Held during the receive_packet callback which
  // sends data to the socket.  May be acquired while holding
  // m_recv_mutex.
  mutex m_send_mutex;

private:
  // A flag indicating whether the agent is processing connections and
  // packets.
  bool m_active;

  // The test agent.
  socket_agent *m_agent;

  // The test harness.
  test_harness *m_harness;

  // The current packet kernel.
  packet_kernel *m_kernel;

  // True if the socket is listening.
  bool m_is_listening;

  // The async IO acceptor for use when listening.
  tcp::acceptor m_acceptor;

  // The socket to accept new connections.
  tcp::socket m_new_socket;

  // The open connections.  Protected by both mutexes.
  typedef std::unordered_set<connection_ptr_t> connections_t;
  connections_t m_connections;

public:
  // Start processing.
  void start(const impl_ptr_t *self);

  // Stop processing.
  void stop();

  // Test a packet kernel.
  void test_kernel(packet_kernel *kernel);

  // Send a packet to the sockets.
  void send_packet(const nanotube_packet_t *packet);

  // Set the current packet kernel.
  void set_kernel(packet_kernel *kernel);

  // Start an accept operation.
  void start_accept_rs_locked(const impl_ptr_t *self);

  // Finish an accept operation.
  void finish_accept(const impl_ptr_t &self,
                     const boost::system::error_code *error);

  // Add a new connection.
  void add_connection_rs_locked(const impl_ptr_t *self,
                                tcp::socket socket);

  // Remove a connection.
  void remove_connection_r_locked(const connection_ptr_t *conn);

  // Process a received packet.
  void process_rx_packet_r_locked(nanotube_packet_t *packet);
};


///////////////////////////////////////////////////////////////////////////

param_strings::param_strings(const std::string &s)
{
  split_params(s);
  split_flags();
  check_flags();
}

void param_strings::split_params(const std::string &params)
{
  // Find the substring containing flags.
  auto flags_end = params.find(':');
  if (flags_end == std::string::npos) {
    std::cerr << "Error: Missing ':' in socket parameters.\n";
    fatal_error();
  }

  // Find the substring containing the port.
  auto port_begin = params.rfind(':');
  assert(port_begin != std::string::npos);

  // Split the string into substrings.
  m_flags_str = params.substr(0, flags_end);
  auto host_len = port_begin-flags_end-1;
  m_host_str = (flags_end == port_begin ? ""
                : params.substr(flags_end+1, host_len));
  m_port_str = params.substr(port_begin+1);

#if SOCKET_DEBUG
  std::cerr << "Flags: " << m_flags_str << "\n";
  std::cerr << "Hostname: " << m_host_str << "\n";
  std::cerr << "Port: " << m_port_str << "\n";
#endif
}

void param_strings::split_flags()
{
  // Collect the flags.
  std::string::size_type pos = 0;
  while (true) {
    auto old_pos = pos;
    pos = m_flags_str.find(',', pos);

    auto len = (pos == std::string::npos ? pos : pos-old_pos);
    std::string flag_str = m_flags_str.substr(old_pos, len);

    if (flag_str.length() != 0) {
      auto it = flag_map.find(flag_str);
      if (it == flag_map.end()) {
        std::cerr << "Error: Invalid socket flag '" << flag_str << "'.\n";
        fatal_error();
      }
      m_flag_set.insert(std::move(flag_str));
    }

    if (pos == std::string::npos)
      break;
    ++pos;
  }
}

void param_strings::check_flags()
{
  if (m_flag_set.count(F_LISTEN) != 0 &&
      m_flag_set.count(F_CONNECT) != 0) {
    std::cerr << "Error: Socket cannot use both listen and connect.\n";
    fatal_error();
  }

  if (m_flag_set.count(F_LISTEN) == 0 &&
      m_flag_set.count(F_CONNECT) == 0) {
    m_flag_set.insert(F_LISTEN);
  }

  if (m_flag_set.count(F_CONNECT) != 0) {
    std::cerr << "Error: Connecting sockets are not yet supported.\n";
    fatal_error();
  }

  if (m_flag_set.count(F_TX) == 0 &&
      m_flag_set.count(F_RX) == 0) {
    m_flag_set.insert(F_TX);
    m_flag_set.insert(F_RX);
  }

  if (m_flag_set.count(F_PACKET) != 0 &&
      m_flag_set.count(F_CAPSULE) != 0) {
    std::cerr << "Error: Socket cannot use both packet and capsule.\n";
    fatal_error();
  }

  if (m_flag_set.count(F_CAPSULE) != 0) {
    std::cerr << "Error: Capsule sockets are not yet supported.\n";
  }

  if (m_flag_set.count(F_PACKET) == 0 &&
      m_flag_set.count(F_CAPSULE) == 0) {
    m_flag_set.insert(F_PACKET);
  }

#if SOCKET_DEBUG
  std::cerr << "Final flags:";
  for (auto s: m_flag_set) {
    std::cerr << " " << s;
  }
  std::cerr << "\n";
#endif
}

///////////////////////////////////////////////////////////////////////////

async_recv::async_recv(impl_ptr_t the_impl,
                       connection_ptr_t conn):
  m_impl(std::move(the_impl)),
  m_connection(std::move(conn))
{
}

void async_recv::operator() (const boost::system::error_code& error,
                             std::size_t bytes_read)
{
  m_connection->finish_receive(std::move(*this), &error, bytes_read);
}

///////////////////////////////////////////////////////////////////////////

connection::connection(tcp::socket &&socket):
  m_active(false),
  m_socket(std::move(socket)),
  m_rx_offset(0)
{
#if SOCKET_DEBUG
  std::cerr << "socket_agent: Created connection at " << this << "\n";
#endif
}

connection::~connection()
{
#if SOCKET_DEBUG
  std::cerr << "socket_agent: Destroyed connection at " << this << "\n";
#endif
}

void connection::start_rs_locked(const impl_ptr_t *impl,
                                 const connection_ptr_t *self)
{
  m_active = true;
  start_receive_r_locked(*impl, *self);
}

void connection::stop_rs_locked()
{
  m_active = false;
  m_socket.cancel();
}

void connection::start_receive_r_locked(impl_ptr_t impl,
                                        connection_ptr_t self)
{
  async_recv op(impl, self);

  if (m_rx_offset < PREFIX_LENGTH) {
    char *base = m_rx_buffer + m_rx_offset;
    std::size_t len = BUFFER_SIZE - m_rx_offset;
#if SOCKET_DEBUG
    std::cerr << "socket_agent: Performing receive: buffer("
              << m_rx_offset << ", " << len << ")\n";
#endif
    m_socket.async_receive(asio::buffer(base, len), op);

  } else {
    std::size_t data_offset = m_rx_offset - PREFIX_LENGTH;

    auto sec = NANOTUBE_SECTION_WHOLE;
    auto sec_len = m_rx_packet.size(sec);
    uint8_t *base = m_rx_packet.begin(sec) + data_offset;
    std::size_t len = sec_len - data_offset;

    std::array<asio::mutable_buffer,2> buffers = {
      asio::mutable_buffer(base, len),
      asio::mutable_buffer(m_rx_buffer, BUFFER_SIZE),
    };
#if SOCKET_DEBUG
    std::cerr << "socket_agent: Performing receive: packet("
              << data_offset << ", " << len << "), buffer(0, "
              << BUFFER_SIZE << ")\n";
#endif
    m_socket.async_receive(buffers, op);
  }
}

void connection::finish_receive(async_recv &&recv_op,
                                const boost::system::error_code *error,
                                std::size_t bytes_read)
{
  lock_guard rg(recv_op.m_impl->m_recv_mutex);

  if (!*error) {
#if SOCKET_DEBUG
    std::cerr << "socket_agent: Received " << bytes_read
              << " byte(s) at offset " << m_rx_offset << "\n";
#endif
    bool succ = process_rx_data_r_locked(recv_op.m_impl.get(), bytes_read);
    if (succ) {
      start_receive_r_locked(std::move(recv_op.m_impl),
                             std::move(recv_op.m_connection));
      return;
    }

  } else if (*error == asio::error::operation_aborted) {
#if SOCKET_DEBUG
    std::cerr << "socket_agent: Receive aborted.\n";
#endif

  } else if (*error == asio::error::eof) {
#if SOCKET_DEBUG
    std::cerr << "socket_agent: Receive EOF.\n";
#endif

  } else {
    std::cerr << "socket_agent: Receive error: " << *error << "\n";
  }

  recv_op.m_impl->remove_connection_r_locked(&(recv_op.m_connection));
}

bool connection::process_rx_data_r_locked(impl *the_impl,
                                          std::size_t num_bytes)
{
  // Process data received into m_rx_packet if any.
  if (m_rx_offset >= PREFIX_LENGTH) {
    std::size_t packet_offset = m_rx_offset - PREFIX_LENGTH;
    auto sec = NANOTUBE_SECTION_WHOLE;
    std::size_t packet_length = m_rx_packet.size(sec);
    std::size_t remaining = packet_length - packet_offset;

    // Stop if the packet is still incomplete.
    if (num_bytes < remaining) {
#if SOCKET_DEBUG
      std::cerr << "socket_agent: Have only " << num_bytes << " of "
                << packet_length << " byte(s) at packet offset "
                << packet_offset << "\n";
#endif
      m_rx_offset += num_bytes;
      return true;
    }

    // Send the packet to the kernel.
    the_impl->process_rx_packet_r_locked(&m_rx_packet);

    // Prepare to process data in the RX buffer.
    num_bytes -= remaining;
    m_rx_offset = 0;
  }

  // Move back to the start of the packet for the case where there were
  // bytes in the buffer when the receive was invoked.
  num_bytes += m_rx_offset;
  m_rx_offset = 0;

  // Process data in the RX buffer.
  while (num_bytes >= PREFIX_LENGTH) {
    union {
      uint32_t val;
      char bytes[PREFIX_LENGTH];
    } length_buffer;

#if SOCKET_DEBUG
    std::cerr << "socket_agent: Have " << num_bytes
              << " byte(s) at buffer offset " << m_rx_offset << "\n";
#endif

    // Read the packet length.
    memcpy(length_buffer.bytes, m_rx_buffer+m_rx_offset,
           PREFIX_LENGTH);
    std::size_t pkt_len = ntohl(length_buffer.val);
    m_rx_offset += PREFIX_LENGTH;
    num_bytes -= PREFIX_LENGTH;

    // Check the MTU.
    if (pkt_len > MTU) {
      std::cerr << "socket_agent: Received packet length "
                << pkt_len << " exceeds the MTU "
                << MTU << ".\n";
      return false;
    }

    // Resize the packet.
    auto sec = NANOTUBE_SECTION_WHOLE;
    m_rx_packet.resize(sec, pkt_len);

    // Copy data into the packet.
    std::size_t frag_len = std::min(pkt_len, num_bytes);
    memcpy(m_rx_packet.begin(sec), m_rx_buffer+m_rx_offset,
           frag_len);
#if SOCKET_DEBUG
     std::cerr << "socket_agent: Copied " << frag_len
               << " of " << pkt_len
               << " byte(s) from buffer offset " << m_rx_offset
               << ".\n";
#endif
    m_rx_offset += frag_len;
    num_bytes -= frag_len;

    // Stop processing if the packet is incomplete.
    if (frag_len < pkt_len) {
#if SOCKET_DEBUG
      std::cerr << "socket_agent: Packet has " << frag_len
                << " byte(s) of " << pkt_len << ".\n";
#endif
      m_rx_offset = PREFIX_LENGTH + frag_len;
      return true;
    }

    // Send the packet to the Nanotube kernel.
    the_impl->process_rx_packet_r_locked(&m_rx_packet);
  }

  // Move the remaining data to the start of the RX buffer.
  memmove(m_rx_buffer, m_rx_buffer+m_rx_offset, num_bytes);
#if SOCKET_DEBUG
  std::cerr << "socket_agent: Moved " << num_bytes
            << " byte(s) from RX buffer offset " << m_rx_offset << ".\n";
#endif
  m_rx_offset = num_bytes;
  return true;
}

void connection::send_packet_s_locked(const nanotube_packet_t *packet)
{
  union {
    uint32_t val;
    char bytes[PREFIX_LENGTH];
  } length_buffer;

  // Fill the length buffer.
  auto sec = NANOTUBE_SECTION_WHOLE;
  uint32_t length = packet->size(sec);
  length_buffer.val = htonl(length);

#if SOCKET_DEBUG
  std::cerr << "socket_agent: Sending packet length " << length << "\n";
#endif

  std::array<asio::const_buffer,2> buffers = {
    asio::const_buffer(length_buffer.bytes, PREFIX_LENGTH),
    asio::const_buffer(packet->begin(sec), length),
  };
  asio::write(m_socket, buffers);
}

///////////////////////////////////////////////////////////////////////////

async_accept::async_accept(impl_ptr_t impl_ptr):
  m_impl(std::move(impl_ptr))
{
}

void async_accept::operator() (const boost::system::error_code &error)
{
  m_impl->finish_accept(m_impl, &error);
}

///////////////////////////////////////////////////////////////////////////

impl::impl(socket_agent* agent,
           const std::string &params):
  m_active(false),
  m_agent(agent),
  m_harness(agent->get_harness()),
  m_kernel(nullptr),
  m_is_listening(false),
  m_acceptor(*(m_harness->get_asio_context())),
  m_new_socket(*(m_harness->get_asio_context()))
{
  auto p = param_strings(params);

  if (p.m_flag_set.count(F_LISTEN) != 0)
    m_is_listening = true;

  // Resolve the hostname and port number.
  tcp::resolver resolver(*(m_harness->get_asio_context()));
  tcp::resolver::query q(p.m_host_str, p.m_port_str);
  auto eps = resolver.resolve(q);

  if (m_is_listening) {
    m_acceptor.open(eps->endpoint().protocol());
    m_acceptor.set_option(tcp::acceptor::reuse_address(true));
    m_acceptor.bind(eps->endpoint());
    m_acceptor.listen();
  }
}

impl::~impl()
{
}

void
impl::start(const impl_ptr_t *self)
{
  lock_guard rg(m_recv_mutex);
  lock_guard sg(m_send_mutex);
#if SOCKET_DEBUG
  std::cerr << "socket_agent: Starting.\n";
#endif
  m_active = true;
  if (m_is_listening) {
    start_accept_rs_locked(std::move(self));
  }
}

void
impl::stop()
{
  lock_guard rg(m_recv_mutex);
  lock_guard sg(m_send_mutex);
#if SOCKET_DEBUG
  std::cerr << "socket_agent: Stopping.\n";
#endif
  m_active = false;
  m_acceptor.cancel();
  for (auto &conn: m_connections) {
    conn->stop_rs_locked();
  }
  m_connections.clear();
}

void
impl::test_kernel(packet_kernel *kernel)
{
  // Set the current kernel.
  set_kernel(kernel);

  while (m_harness->get_quit_flag() == false) {
    if (!kernel->poll())
      nanotube_thread_wait();
  }

  // Clear the current kernel.
  set_kernel(nullptr);
}

void
impl::send_packet(const nanotube_packet_t *packet)
{
  lock_guard sg(m_send_mutex);
#if SOCKET_DEBUG
  auto sec = NANOTUBE_SECTION_WHOLE;
  std::cerr << "socket_agent: Got packet length "
            << packet->size(sec) << "\n";
#endif
  for (auto &conn: m_connections) {
    conn->send_packet_s_locked(packet);
  }
}

void
impl::set_kernel(packet_kernel *kernel)
{
  lock_guard rg(m_recv_mutex);
  m_kernel = kernel;
}

void
impl::start_accept_rs_locked(const impl_ptr_t *self)
{
#if SOCKET_DEBUG
  std::cerr << "socket_agent: Accepting.\n";
#endif
  async_accept op(*self);
  m_acceptor.async_accept(m_new_socket, op);
}

void
impl::finish_accept(const impl_ptr_t &self,
                    const boost::system::error_code *error)
{
  if (!*error) {
    lock_guard rg(m_recv_mutex);
    lock_guard sg(m_send_mutex);
#if SOCKET_DEBUG
    std::cerr << "socket_agent: Accept successful!\n";
#endif
    if (!m_active)
      return;

    // Add the new connection.
    add_connection_rs_locked(&self, std::move(m_new_socket));

    // Start accepting another connection.
    start_accept_rs_locked(&self);

  } else if (*error == asio::error::operation_aborted) {
#if SOCKET_DEBUG
    std::cerr << "socket_agent: Accept aborted.\n";
#endif

  } else {
    std::cerr << "socket_agent: Accept error: " << *error << "\n";
  }
}

void impl::add_connection_rs_locked(const impl_ptr_t *self,
                                    tcp::socket socket)
{
  connection_ptr_t conn(new connection(std::move(socket)));
  conn->start_rs_locked(self, &conn);
  m_connections.insert(std::move(conn));
 assert(m_active);
}

void impl::remove_connection_r_locked(const connection_ptr_t *conn)
{
  lock_guard sg(m_send_mutex);

  auto n = m_connections.erase(*conn);
#if SOCKET_DEBUG
  std::cerr << "socket_agent: Removed connection at "
            << conn->get() << ", n=" << n << "\n";
#endif
  assert(n == 1);
}

void impl::process_rx_packet_r_locked(nanotube_packet_t *packet)
{
#if SOCKET_DEBUG
  auto sec = NANOTUBE_SECTION_WHOLE;
      std::cerr << "socket_agent: Assembled packet with length "
                << packet->size(sec) << ".\n";
#endif
  packet->set_port(0);
  m_kernel->process(packet);
}

///////////////////////////////////////////////////////////////////////////

socket_agent::socket_agent(test_harness* harness,
                           const std::string &params):
  test_agent(harness)
{
  m_impl.reset(new impl(this, params));
}

socket_agent::~socket_agent()
{
}

void socket_agent::start_test()
{
  m_impl->start(&m_impl);
}

void socket_agent::test_kernel(packet_kernel* kernel)
{
  // Allow the RX thread to send packets to the kernel.
  kernel->set_sender(get_harness()->get_io_thread_context());

  m_impl->test_kernel(kernel);

  // Allow the main thread to send packets to the kernel.
  processing_system *ps = get_harness()->get_system();
  kernel->set_sender(ps->get_main_context());
}

void socket_agent::receive_packet(nanotube_packet_t* packet,
                                  unsigned packet_index)
{
  m_impl->send_packet(packet);
}

void socket_agent::end_test()
{
  m_impl->stop();
}

void socket_agent::handle_quit()
{
  // Wake the main thread to make sure it notices that the quit flag
  // has been set.
  test_harness *th = get_harness();
  processing_system *ps = th->get_system();
  nanotube_thread *main_thread = ps->get_main_thread();
  main_thread->wake();
}

///////////////////////////////////////////////////////////////////////////
