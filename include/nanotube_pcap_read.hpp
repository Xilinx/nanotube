/*******************************************************/
/*! \file nanotube_pcap_read.hpp
** \author Stephan Diestelhorst <stephand@amd.com>
**         Neil Turton <neilt@amd.com>
**  \brief Reading Nanotube packets from pcap files.
**   \date 2020-08-26
*//******************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_PCAP_READ_HPP
#define NANOTUBE_PCAP_READ_HPP

#include <pcap/pcap.h>

#include "nanotube_api.h"
#include "nanotube_packet.hpp"

#include <iostream>

/*!
** Read packets from a pcap file using libpcap.
*/
class nanotube_pcap_read {
public:
  /*!
  ** Creates a new pcap reader with the specified file.
  **
  ** \param file Full name of the file where packets are read from
  */
  nanotube_pcap_read(const char *file);

  ~nanotube_pcap_read();

  /*!
  ** Reads the next available packet from the file.
  **
  ** \return a unique_ptr to the next packet read from the file, which
  ** is empty if no packet could be read.
  */
  nanotube_packet_ptr_t read_next(void);

  pcap_t* get_pcap() const
  {
    return pcap;
  }
private:
  pcap_t *pcap;
};

#endif // NANOTUBE_PCAP_READ_HPP
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
