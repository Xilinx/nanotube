///////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////
#ifndef STAGES_HH
#define STAGES_HH

#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include <byteswap.h>
#include <cstddef>
#include <cstdint>

static inline void nanotube_memcpy(void *dest, const void *src, size_t n)
{
#pragma HLS inline
  for(size_t i=0; i<n; i++)
    ((char*)dest)[i] = ((const char*)src)[i];
}

static inline int
nanotube_memcmp(const void *src1, const void *src2, size_t n)
{
#pragma HLS inline
  const char* p1 = (const char*)src1;
  const char* p2 = (const char*)src2;
  for (size_t i=0; i<n; i++) {
    if (p1[i] != p2[i])
      return (p1[i] < p2[i] ? -1 : 1);
  }
  return 0;
}

template<int N> struct bytes {
  uint8_t data[N];
};

void stage_0(
  hls::stream<bytes<65> > &port0,
  hls::stream<bytes<65> > &port1);

void stage_1(
  hls::stream<bytes<65> > &port0,
  hls::stream<bytes<65> > &port1);

#endif // STAGES_HH
