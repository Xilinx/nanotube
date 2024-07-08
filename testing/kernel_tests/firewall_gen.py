#! /usr/bin/env python3
###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################

# This script generates some of the test files for the firewall test,
# to make it easier to keep the different files in step.  Simply run
# it with no arguments to regenerate the files.

from scapy.config import Conf
from scapy.themes import BlackAndWhite
from scapy.packet import raw
from scapy.layers.l2 import Ether
from scapy.layers.inet import IP,ICMP,TCP,UDP
from scapy.layers.inet6 import IPv6

import sys

Conf.color_theme = BlackAndWhite()

file_prefix = "testing/kernel_tests/golden/test_firewall"

copyright = """
###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################

""".lstrip()

print("Generating firewall packets")

def hex2ipv4(s):
    blist = [int(x,16) for x in s.split()]
    assert(len(blist)==4)
    return (s, ".".join(str(x) for x in blist))

def hex2ipv6(s):
    blist = [int(x,16) for x in s.split()]
    assert(len(blist)==16)
    return (s, ":".join("%02x%02x" % (blist[i],blist[i+1])
                        for i in range(0,len(blist),2)))

def hex2port(s):
    blist = [int(x,16) for x in s.split()]
    assert(len(blist)==2)
    return (s, blist[0]*256 + blist[1])

def hex2bytes(s):
    blist = [int(x,16) for x in s.split()]
    return bytes(blist)

PAD4="00 00 00 00"

ETHER_SRC="ee:28:06:55:6d:8f"
ETHER_DST="00:05:e3:c1:c4:0a"

BLOCKED_V4=hex2ipv4("f2 b5 dc 8a")
ALLOWED_V4_A=hex2ipv4("f6 b5 dc 8a")
ALLOWED_V4_B=hex2ipv4("f2 b5 dc 9a")
SRC_PORT_V4=hex2port("df eb")

BLOCKED_V6=hex2ipv6("72 58 70 7f  61 a5 42 fd  22 42 dd 01  96 5d b7 fc")
ALLOWED_V6_A=hex2ipv6("7a 58 70 7f  61 a5 42 fd  22 42 dd 01  96 5d b7 fc")
ALLOWED_V6_B=hex2ipv6("72 58 70 7f  61 a5 42 fd  22 42 dd 01  96 5d b7 bc")
SRC_PORT_V6=hex2port("c6 50")

ICMP_ADDR_V4=hex2ipv4("35 66 37 65")

TCP_ADDR_V4=hex2ipv4("08 a0 5e 05")
TCP_PORT_V4=hex2port("c2 7e")

UDP_ADDR_V4=hex2ipv4("5e c2 3f 38")
UDP_PORT_V4=hex2port("fa f4")

ICMP_ADDR_V6=hex2ipv6("85 bb 11 21 5f fe 5a cb 01 e8 98 ed f4 1b cf 9e")

TCP_ADDR_V6=hex2ipv6("b2 65 a5 27 14 b9 6e 18 82 90 a3 56 56 a8 46 50")
TCP_PORT_V6=hex2port("5a 7f")

UDP_ADDR_V6=hex2ipv6("22 67 14 a5 e5 3f 76 42 65 7c 54 5f 4f 08 83 91")
UDP_PORT_V6=hex2port("70 c5")

PAYLOAD=raw(hex2bytes("1e f2 b9 75 70 db 9c 9a 10 b4 f8 78 fc 16 6b 84"))

packets=[]

ALLOWED_V4_B = ALLOWED_V4_A
ALLOWED_V6_B = ALLOWED_V6_A

packets.append(("Non-IP", 0x1,
                Ether(src=ETHER_SRC, dst=ETHER_DST, type=0x0806)/
                raw([])))

packets.append(("Malformed Ethernet", 0x0,
                bytes(packets[-1][2])[:-1]))

packets.append(("Empty ICMPv4", 0x5,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=ALLOWED_V4_A[1], dst=ICMP_ADDR_V4[1], proto=1)))

packets.append(("Malformed IPv4", 0x0,
                Ether(bytes(packets[-1][2])[:-1])))

packets.append(("Empty TCPv4", 0x6,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=ALLOWED_V4_B[1], dst=TCP_ADDR_V4[1])/
                TCP(sport=SRC_PORT_V4[1], dport=TCP_PORT_V4[1])))

packets.append(("Malformed TCPv4", 0x0,
                Ether(bytes(packets[-1][2])[:-1])))

packets.append(("Empty UDPv4", 0x7,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=ALLOWED_V4_B[1], dst=UDP_ADDR_V4[1])/
                UDP(sport=SRC_PORT_V4[1], dport=UDP_PORT_V4[1])))

packets.append(("Malformed UDPv4", 0x0,
                Ether(bytes(packets[-1][2])[:-1])))

packets.append(("Empty ICMPv6", 0x8,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=ALLOWED_V6_A[1], dst=ICMP_ADDR_V6[1], nh=0x3a)))

packets.append(("Malformed IPv6", 0x0,
                Ether(bytes(packets[-1][2])[:-1])))

packets.append(("Empty TCPv6", 0x9,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=ALLOWED_V6_B[1], dst=TCP_ADDR_V6[1])/
                TCP(sport=SRC_PORT_V6[1], dport=TCP_PORT_V6[1])))

packets.append(("Malformed TCPv6", 0x0,
                Ether(bytes(packets[-1][2])[:-1])))

packets.append(("Empty UDPv6", 0xa,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=ALLOWED_V6_B[1], dst=UDP_ADDR_V6[1])/
                UDP(sport=SRC_PORT_V6[1], dport=UDP_PORT_V6[1])))

packets.append(("Malformed UDPv6", 0x0,
                Ether(bytes(packets[-1][2])[:-1])))

packets.append(("ICMPv4, Blocked, Known", 0x3,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=BLOCKED_V4[1], dst=ICMP_ADDR_V4[1])/
                ICMP()/
                PAYLOAD))

packets.append(("ICMPv4, Allowed, Known", 0x5,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=ALLOWED_V4_A[1], dst=ICMP_ADDR_V4[1])/
                ICMP()/
                PAYLOAD))

packets.append(("TCPv4, Blocked, Known", 0x3,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=BLOCKED_V4[1], dst=TCP_ADDR_V4[1])/
                TCP(sport=SRC_PORT_V4[1], dport=TCP_PORT_V4[1])/
                PAYLOAD))

packets.append(("TCPv4, Allowed, Known", 0x6,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=ALLOWED_V4_B[1], dst=TCP_ADDR_V4[1])/
                TCP(sport=SRC_PORT_V4[1], dport=TCP_PORT_V4[1])/
                PAYLOAD))

packets.append(("UDPv4, Blocked, Known", 0x3,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=BLOCKED_V4[1], dst=UDP_ADDR_V4[1])/
                UDP(sport=SRC_PORT_V4[1], dport=UDP_PORT_V4[1])/
                PAYLOAD))

packets.append(("UDPv4, Allowed, Known", 0x7,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=ALLOWED_V4_A[1], dst=UDP_ADDR_V4[1])/
                UDP(sport=SRC_PORT_V4[1], dport=UDP_PORT_V4[1])/
                PAYLOAD))

packets.append(("ICMPv6, Blocked, Known", 0x4,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=BLOCKED_V6[1], dst=ICMP_ADDR_V6[1], nh=0x3a)/
                PAYLOAD))

packets.append(("ICMPv6, Allowed, Known", 0x8,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=ALLOWED_V6_A[1], dst=ICMP_ADDR_V6[1], nh=0x3a)/
                PAYLOAD))

packets.append(("TCPv6, Blocked, Known", 0x4,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=BLOCKED_V6[1], dst=TCP_ADDR_V6[1])/
                TCP(sport=SRC_PORT_V6[1], dport=TCP_PORT_V6[1])/
                PAYLOAD))

packets.append(("TCPv6, Allowed, Known", 0x9,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=ALLOWED_V6_B[1], dst=TCP_ADDR_V6[1])/
                TCP(sport=SRC_PORT_V6[1], dport=TCP_PORT_V6[1])/
                PAYLOAD))

packets.append(("UDPv6, Blocked, Known", 0x4,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=BLOCKED_V6[1], dst=UDP_ADDR_V6[1])/
                UDP(sport=SRC_PORT_V6[1], dport=UDP_PORT_V6[1])/
                PAYLOAD))

packets.append(("UDPv6, Allowed, Known", 0xa,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=ALLOWED_V6_A[1], dst=UDP_ADDR_V6[1])/
                UDP(sport=SRC_PORT_V6[1], dport=UDP_PORT_V6[1])/
                PAYLOAD))

packets.append(("ICMPv4, Blocked, Unknown", 0x3,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=BLOCKED_V4[1], dst=TCP_ADDR_V4[1])/
                ICMP()/
                PAYLOAD))

packets.append(("ICMPv4, Allowed, Unknown", 0x2,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=ALLOWED_V4_A[1], dst=TCP_ADDR_V4[1])/
                ICMP()/
                PAYLOAD))

packets.append(("TCPv4, Blocked, Unknown", 0x3,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=BLOCKED_V4[1], dst=UDP_ADDR_V4[1])/
                TCP(sport=SRC_PORT_V4[1], dport=TCP_PORT_V4[1])/
                PAYLOAD))

packets.append(("TCPv4, Allowed, Unknown", 0x2,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=ALLOWED_V4_B[1], dst=UDP_ADDR_V4[1])/
                TCP(sport=SRC_PORT_V4[1], dport=TCP_PORT_V4[1])/
                PAYLOAD))

packets.append(("UDPv4, Blocked, Unknown", 0x3,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=BLOCKED_V4[1], dst=ICMP_ADDR_V4[1])/
                UDP(sport=SRC_PORT_V4[1], dport=UDP_PORT_V4[1])/
                PAYLOAD))

packets.append(("UDPv4, Allowed, Unknown", 0x2,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IP(src=ALLOWED_V4_B[1], dst=ICMP_ADDR_V4[1])/
                UDP(sport=SRC_PORT_V4[1], dport=UDP_PORT_V4[1])/
                PAYLOAD))

packets.append(("ICMPv6, Blocked, Unknown", 0x4,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=BLOCKED_V6[1], dst=UDP_ADDR_V6[1], nh=0x3a)/
                PAYLOAD))

packets.append(("ICMPv6, Allowed, Unknown", 0x2,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=ALLOWED_V6_A[1], dst=UDP_ADDR_V6[1], nh=0x3a)/
                PAYLOAD))

packets.append(("TCPv6, Blocked, Unknown", 0x4,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=BLOCKED_V6[1], dst=ICMP_ADDR_V6[1])/
                TCP(sport=SRC_PORT_V6[1], dport=TCP_PORT_V6[1])/
                PAYLOAD))

packets.append(("TCPv6, Allowed, Unknown", 0x2,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=ALLOWED_V6_B[1], dst=ICMP_ADDR_V6[1])/
                TCP(sport=SRC_PORT_V6[1], dport=TCP_PORT_V6[1])/
                PAYLOAD))

packets.append(("UDPv6, Blocked, Unknown", 0x4,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=BLOCKED_V6[1], dst=TCP_ADDR_V6[1])/
                UDP(sport=SRC_PORT_V6[1], dport=UDP_PORT_V6[1])/
                PAYLOAD))

packets.append(("UDPv6, Allowed, Unknown", 0x2,
                Ether(src=ETHER_SRC, dst=ETHER_DST)/
                IPv6(src=ALLOWED_V6_B[1], dst=TCP_ADDR_V6[1])/
                UDP(sport=SRC_PORT_V6[1], dport=UDP_PORT_V6[1])/
                PAYLOAD))

f = open(file_prefix+".packets.IN", "w")
f.write(copyright)

for pn,(desc,ctr,pkt) in enumerate(packets):
    desc = "%s, counter 0x%x." % (desc,ctr)
    b = bytearray(bytes(pkt))
    b[0] = pn
    f.write("# %s\n" % (desc,))
    f.write("# %r\n" % (pkt,))
    for i in range(0, len(b), 16):
        frag_len = min(16,len(b)-i)
        frag = b[i:i+frag_len]
        s = " ".join("%02x" % x for x in frag)
        f.write("%04x  %s\n" % (i, s))
    f.write("\n")

f.close()

f = open(file_prefix+".maps.OUT", "w")
f.write(copyright[:-1])

f.write("# Testing kernel ip_firewall\n")

config_maps="""
nanotube_map: 0 0 17 4
key: 72 58 70 7f 61 a5 42 fd 22 42 dd 01 96 5d b7 fc
     06
value: 04 00 00 00
key: f2 b5 dc 8a 00 00 00 00 00 00 00 00 00 00 00 00
     04
value: 03 00 00 00
end

nanotube_map: 1 0 20 4
key: 22 67 14 a5 e5 3f 76 42 65 7c 54 5f 4f 08 83 91
     06 11 70 c5
value: 0a 00 00 00
key: b2 65 a5 27 14 b9 6e 18 82 90 a3 56 56 a8 46 50
     06 06 5a 7f
value: 09 00 00 00
key: 85 bb 11 21 5f fe 5a cb 01 e8 98 ed f4 1b cf 9e
     06 3a 00 00
value: 08 00 00 00
key: 08 a0 5e 05 00 00 00 00 00 00 00 00 00 00 00 00
     04 06 c2 7e
value: 06 00 00 00
key: 5e c2 3f 38 00 00 00 00 00 00 00 00 00 00 00 00
     04 11 fa f4
value: 07 00 00 00
key: 35 66 37 65 00 00 00 00 00 00 00 00 00 00 00 00
     04 01 00 00
value: 05 00 00 00
end

""".lstrip()

counters = [(0,0) for i in range(32)]
for pn,(desc,ctr,pkt) in enumerate(packets):
    # Update the counter.
    old = counters[ctr]
    pkt_len = len(bytes(pkt))
    counters[ctr] = (old[0]+1, old[1]+pkt_len)

    f.write("# Maps after packet %d\n" % pn)
    f.write(config_maps)
    f.write("nanotube_map: 2 2 4 16\n")
    for i in range(len(counters)):
        f.write("key: %02x 00 00 00\n" % i)
        vals = counters[i]
        s0 = " ".join("%02x" % ((vals[0]>>j)&0xff)
                      for j in range(0,64,8))
        s1 = " ".join("%02x" % ((vals[1]>>j)&0xff)
                      for j in range(0,64,8))
        f.write("value: %s %s\n" % (s0, s1))
    f.write("end\n\n")

f.close()

sys.exit(0)
