///////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////
// Stage 0
// Thread name:    stage_0
// Thread function logic_0
//   Port 0 reads  channel 0
//   Port 1 writes channel 1
#include "ap_int.h"
#include "hls_stream.h"
#include "stages.hh"
#include <cassert>
#include <cstdint>
#include <cstring>

void stage_0(
  hls::stream<bytes<65> > &port0,
  hls::stream<bytes<65> > &port1)
{
  bytes<65> port0_data;
  bytes<65> port1_data;
  uint8_t v0[65]__attribute__((aligned(4)));
#pragma HLS array_partition variable=v0 complete
  ap_uint<32> v1;
  ap_uint<1> v2;
  ap_uint<32> v3;
  ap_uint<32> v4;
  ap_uint<16> v5;
  ap_uint<16> v6;

#pragma HLS pipeline II=1
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS interface axis port=port0
#if defined(NANOTUBE_USING_VIVADO_HLS)
#pragma HLS data_pack variable=port0
#else // defined(NANOTUBE_USING_VIVADO_HLS)
#pragma HLS aggregate variable=port0
#pragma HLS aggregate variable=port0_data
#endif // defined(NANOTUBE_USING_VIVADO_HLS)
#pragma HLS interface axis port=port1
#if defined(NANOTUBE_USING_VIVADO_HLS)
#pragma HLS data_pack variable=port1
#else // defined(NANOTUBE_USING_VIVADO_HLS)
#pragma HLS aggregate variable=port1
#pragma HLS aggregate variable=port1_data
#endif // defined(NANOTUBE_USING_VIVADO_HLS)

  v1 = port0.read_nb(port0_data);
  nanotube_memcpy(v0, port0_data.data, 65);
  v2 = v1 == ap_uint<32>(0);
  if ( v2 ) {
    goto L0;
  } else {
    goto L1;
  }

L0:
  goto L2;

L1:
  v3 = (ap_uint<32>(v0[0]) << 0) |
    (ap_uint<32>(v0[1]) << 8) |
    (ap_uint<32>(v0[2]) << 16) |
    (ap_uint<32>(v0[3]) << 24);
  v4 = ( ( (v3 << 24) & (ap_int<32>(0xff) << 24) ) |
    ( (v3 << 8) & (ap_int<32>(0xff) << 16) ) |
    ( (v3 >> 8) & (ap_int<32>(0xff) << 8) ) |
    ( (v3 >> 24) & (ap_int<32>(0xff) << 0) ) );
  v0[0] = (v4 >> 0);
  v0[1] = (v4 >> 8);
  v0[2] = (v4 >> 16);
  v0[3] = (v4 >> 24);
  v5 = (ap_uint<16>((v0+4)[0]) << 0) |
    (ap_uint<16>((v0+4)[1]) << 8);
  v6 = ( ( (v5 << 8) & (ap_int<16>(0xff) << 8) ) |
    ( (v5 >> 8) & (ap_int<16>(0xff) << 0) ) );
  (v0+4)[0] = (v6 >> 0);
  (v0+4)[1] = (v6 >> 8);
  nanotube_memcpy(port1_data.data, v0, 65);
  port1.write(port1_data);
  goto L2;

L2:
  return;
}
