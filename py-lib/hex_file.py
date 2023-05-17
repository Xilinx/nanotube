###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################

import re

timestamp_re = re.compile(r'\d+-\d+-\d+ ')
data_re = re.compile(r'([0-9a-fA-F]+)((?:\s+[0-9a-fA-F]+)*)\Z')
blank_re = re.compile(r'\s*(#|\Z)')

class HexReader:
  """Provides an iterator over the packets in a list of files.

  The constructor accepts a list of filenames as an argument.  The
  iter_packets method returns an iterator over the packets.  Each
  packet is represented as a bytearray containing the bytes of the
  packet.
  """

  def __init__(self, files, verbosity=0):
    self.__buffer = bytearray()
    self.__files = files
    self.__verbosity = verbosity

  def iter_packets(self):
    for f_name in self.__files:
      for p in self.__iter_file(f_name):
        if self.__verbosity >= 1:
          dump_packet(sys.stderr, f_name+": ", p)
        yield p

  def __iter_file(self, f_name):
    fh = open(f_name, "r")
    for l_num, line in enumerate(fh):
      line = line.rstrip("\r\n")
      if self.__verbosity >= 1:
        sys.stderr.write("%s read: %r\n" % (f_name, line))

      m = blank_re.match(line)
      if m is not None:
        continue

      m = timestamp_re.match(line)
      if m is not None:
        if len(self.__buffer) != 0:
          yield self.__buffer
          self.__buffer = bytearray()
        continue

      m = data_re.match(line)
      if m is not None:
        line_offset = int(m.group(1), 16)
        line_byte_str = m.group(2)
        if line_offset == 0 and len(self.__buffer) != 0:
          yield self.__buffer
          self.__buffer = bytearray()
        if line_offset != len(self.__buffer):
          sys.stderr.write("%s:%d: Expected offset 0x%x but got 0x%x.\n" %
                           (f_name, l_num, len(self.__buffer), line_offset))
          sys.exit(1)

        if line_byte_str != "":
          line_bytes = line_byte_str.split()
          self.__buffer.extend(int(b,16) for b in line_bytes)

    # Flush out the final packet.
    if len(self.__buffer) != 0:
      yield self.__buffer
      self.__buffer = bytearray()

