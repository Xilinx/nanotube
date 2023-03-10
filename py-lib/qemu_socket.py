###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################
import socket
import struct

LISTEN = "listen"
CONNECT = "connect"

class QemuSocket:
  """A QEMU socket for transferring packets.

  The socket constructor has parameters which specify the hostname,
  port number and connection mode (listen/connect).  The actual
  hostname and port number may differ from the specifications, so
  there are methods to get those.

  There is a method to complete the connection.  The fileno method
  returns the file descriptor, allowing the socket to be used directly
  with a Selector.

  There are separate methods for sending and receiving data which will
  be called when the socket is ready for reading/writing.  The receive
  method will be able to return multiple packets.  The send method
  will indicate whether it is ready to accept the next packet.
  """

  def __init__(self, *, hostname, port, mode, mtu, timeout, verbosity):
    self.__mode = mode
    self.__mtu = mtu
    self.__timeout = timeout
    self.__verbosity = verbosity

    # A list of buffers to send.
    self.__send_buffers = []

    # The current offset into the first send buffer.
    self.__send_offset = 0

    # A buffer to receive bytes into.
    self.__recv_buffer = bytearray(4096)

    # The current packet being received, or None
    self.__recv_packet = None

    # The offset into the current packet being received, including the
    # QEMU socket header.
    self.__recv_offset = 0

    # A flag indicating whether the socket is connected.
    self.__connected = False

    family = socket.AF_INET
    proto = socket.IPPROTO_TCP
    addr = (hostname, port)

    # Create a socket.
    self.__socket = socket.socket(family=family, proto=proto)
    self.__socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    if mode == LISTEN:
      # Bind the socket and start listening.
      self.__socket.bind(addr)
      self.__socket.listen(1)

      # Capture the hostname and port produced by the bind operation.
      (hostname,port) = self.__socket.getsockname()

    else:
      assert(mode == CONNECT)

      # Allocate a port number if necessary.
      if port == 0:
        self.__socket.bind(addr)
        (hostname,port) = self.__socket.getsockname()
        self.__socket.close()
        self.__socket = socket.socket(family=family, proto=proto)
        self.__socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    self.__hostname = hostname
    self.__port = port
    self.__socket.setblocking(False)

  def get_hostname(self):
    return self.__hostname

  def get_port(self):
    return self.__port

  def try_connect(self):
    if self.__mode == CONNECT:
      addr = (self.__hostname, self.__port)
      try:
        self.__socket.connect(addr)
        self.__connected = True
        return True
      except BlockingIOError:
        return False
      except ConnectionRefusedError:
        return False
      except ConnectionResetError:
        return False

    else:
      try:
        conn, addr = self.__socket.accept()
        self.__socket = conn
        self.__connected = True
        return True

      except BlockingIOError:
        return False

      except TimeoutError:
        return False

  def fileno(self):
    return self.__socket.fileno()

  def send(self, packet=None):
    """Send some packet data.

    Either send a new packet or continue sending a previous packet.
    The first call to this method should pass a packet as an argument.
    Subsequent calls for the same packet should not provide an
    argument.  The method will return a flag indicating whether it is
    ready to accept the next packet.  If it returns false then the
    caller should keep calling the method with no argument when the
    socket is writable.  If it returns true then the caller may call
    the method again with another packet to send.
    """

    assert(self.__connected)

    # Handle a new packet, if any.
    if packet is not None:
      if self.__verbosity >= 1:
        sys.stderr.write("Sending packet length %r.\n" % (len(packet),))

      assert(len(self.__send_buffers) == 0)
      header = struct.pack("!I", len(packet))
      self.__send_buffers.append(header)
      self.__send_buffers.append(packet)
      self.__send_offset = 0

    else:
      if self.__verbosity >= 1:
        sys.stderr.write("Continuing send.\n")

    # Perform the send.
    assert(len(self.__send_buffers) != 0)
    view = memoryview(self.__send_buffers[0])[self.__send_offset:]
    buffers = [view] + self.__send_buffers[1:]

    num_bytes = self.__socket.sendmsg(buffers)
    if self.__verbosity >= 1:
      sys.stderr.write("Send returned %r.\n" % (num_bytes,))

    # Add the number of bytes sent previously.
    num_bytes += self.__send_offset

    # Remove fully sent buffers.
    while num_bytes >= len(self.__send_buffers[0]):
      if self.__verbosity >= 1:
        sys.stderr.write("Completed buffer length %r.\n" %
                         (len(self.__send_buffers[0]),))

      # Take account of this buffer being sent.
      num_bytes -= len(self.__send_buffers[0])

      # Move on to the next buffer if any.
      self.__send_offset = 0
      self.__send_buffers.pop(0)

      # Ready for the next packet?
      if len(self.__send_buffers) == 0:
        if self.__verbosity >= 1:
          sys.stderr.write("Ready to send next packet.\n")
        return True

    # There is a buffer which has not been fully sent.
    self.__send_offset = num_bytes
    if self.__verbosity >= 1:
      sys.stderr.write("Have sent %d bytes.\n" % (num_bytes,))
    return False

  def receive(self):
    """Receive some packet data.

    Called when data can be read from the socket.  Returns an
    interator over received packets.  Each packet is represented as a
    bytearray.  If the iterator yields None then EOF has been reached
    and the receive method should not be called again.
    """

    assert(self.__connected)

    # Receive some bytes.  The earliest bytes go into a partial packet
    # if there is one.  The remaining bytes go into the receive buffer.
    if self.__recv_packet is None:
      assert(self.__recv_buffer is not None)
      assert(self.__recv_offset < 4)
      view = memoryview(self.__recv_buffer)
      buffers = [view[self.__recv_offset:]]
    else:
      assert(self.__recv_offset >= 4)
      view = memoryview(self.__recv_packet)
      buffers = [view[self.__recv_offset - 4:], self.__recv_buffer]

    result = self.__socket.recvmsg_into(buffers)
    num_bytes = result[0]

    if self.__verbosity >= 1:
      if self.__recv_packet is None:
        sys.stderr.write("Receive at offset %d returned %r.\n" %
                         (self.__recv_offset, result))
      else:
        sys.stderr.write("Receive at offset %d of %d returned %r.\n" %
                         (self.__recv_offset,
                          4 + len(self.__recv_packet), result))

    # Handle EOF.
    if num_bytes == 0:
      yield None
      # Make sure the method is not called again.
      self.__recv_buffer = None
      self.__recv_packet = None
      return

    # Include any previous bytes.
    num_bytes += self.__recv_offset

    # Handle a partial packet if there is one.
    if self.__recv_packet is not None:
      pkt_total_len = 4 + len(self.__recv_packet)
      if num_bytes < pkt_total_len:
        self.__recv_offset = num_bytes
        return

      yield self.__recv_packet
      num_bytes -= pkt_total_len
      self.__recv_packet = None

    # Handle any completed packets in the buffer.
    buf = self.__recv_buffer
    offset = 0
    while num_bytes >= 4:
      # Process the header.
      pkt_len = struct.unpack("!I", buf[offset:offset+4])[0]
      pkt_total_len = 4 + pkt_len

      if pkt_len > self.__mtu:
          sys.stderr.write("%s: Packet length %d exceeds MTU %d.\n" %
                           (sys.argv[0], pkt_len, self.__mtu))
          sys.exit(1)

      # Stop if the packet is incomplete.
      if num_bytes < pkt_total_len:
        self.__recv_offset = num_bytes
        self.__recv_packet = bytearray(pkt_len)
        self.__recv_packet[:num_bytes-4] = buf[offset+4:offset+num_bytes]
        return

      # Process a complete packet.
      yield buf[offset+4:offset+pkt_total_len]
      num_bytes -= pkt_total_len
      offset += pkt_total_len

    # Move the received bytes to the start of the buffer.
    old_len = len(buf)
    buf[:num_bytes] = buf[offset:offset+num_bytes]
    assert(len(buf) == old_len)
    self.__recv_offset = num_bytes
    return

