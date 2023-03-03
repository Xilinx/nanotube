/**************************************************************************\
*//*! \file nanotube_pcap_dump.hpp
** \author  Stephan Diestelhorst <stephand@amd.com>
**          Neil Turton <neilt@amd.com>
**  \brief  Dumping Nanotube packets to pcap files.
**   \date  2020-08-26
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PCAP_DUMP_HPP
#define NANOTUBE_PCAP_DUMP_HPP

#include "nanotube_api.h"

#include <cstddef>
#include <pcap/pcap.h>

class nanotube_pcap_read;

/*!
** Dump Nanotube packets to a file with libpcap.
*/
class nanotube_pcap_dump {
public:
  /*!
  ** Create a new pcap packet dumper.
  **
  ** \param file          Full filename where the packets should be dumped to.
  ** \param with_metadata Prefix each packet with a PPI header containing its
  **                      metadata. Will generate a pcap file with PPI link type.
  */
  nanotube_pcap_dump(const char *file, bool with_metadata);

  ~nanotube_pcap_dump();

  /*!
  ** Write out a packet through the pcap packet dumper.
  **
  ** NB: We currently do not keep track of timestamps and also do not
  ** support partially read / captured packets.  Therefore, timestamps will
  ** be lost, and all packets will be full dumps.
  **
  ** \param p The packet that will be written.
  */
  void write(nanotube_packet_t *p);

private:
  pcap_dumper_t *dumper;
  bool m_with_metadata;
};

#endif // NANOTUBE_PCAP_DUMP_HPP
