###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################
import re
import sys

###########################################################################

def dump_packet(fh, prefix, pkt):
  for i in range(0, len(pkt)-1, 16):
    end = min(i+16, len(pkt))
    data_str = " ".join("%02x" % b for b in pkt[i:end])
    fh.write("%s%04x %s\n" % (prefix, i, data_str))

_timeout_re = re.compile(r'((?:\d+|\d+\.\d*|\.\d+)(?:[eE][-+]\d+)?)'
                         r'([mun]?s)')

_time_unit_map = { "s": 1e0, "ms": 1e-3, "us": 1e-6, "ns": 1e-9 }

###########################################################################

def parse_time(time_str):
  if time_str == None:
    return None

  m = _timeout_re.match(time_str)
  if not m:
    sys.stderr.write("%s: Invalid time specification %r.\n" %
                     (sys.argv[0], time_str))
    sys.exit(1)

  timeout_val = float(m.group(1))
  return timeout_val * _time_unit_map[m.group(2)]

###########################################################################
