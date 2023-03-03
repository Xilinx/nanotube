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
  hls::stream<bytes<36> > &port0,
  hls::stream<bytes<1> > &port1)
{
  bytes<36> port0_data;
  bytes<1> port1_data;
  uint8_t v0[36]__attribute__((aligned(4)));
#pragma HLS array_partition variable=v0 complete
  uint8_t v1[1];
#pragma HLS array_partition variable=v1 complete
  ap_uint<32> v2;
  ap_uint<1> v3;
  ap_uint<32> v4;
  ap_uint<32> v5;
  ap_uint<64> v6;
  uint8_t *v7;
  ap_uint<16> v8;
  ap_uint<8> v9;

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

  v2 = port0.read_nb(port0_data);
  nanotube_memcpy(v0, port0_data.data, 36);
  v3 = v2 == ap_uint<32>(0);
  if ( v3 ) {
    goto L0;
  } else {
    goto L1;
  }

L0:
  goto L2;

L1:
  v4 = (ap_uint<32>(v0[0]) << 0) |
    (ap_uint<32>(v0[1]) << 8) |
    (ap_uint<32>(v0[2]) << 16) |
    (ap_uint<32>(v0[3]) << 24);
  v5 = v4 & ap_uint<32>(15);
  v6 = ap_uint<32>(v5);
  v7 = (v0 + 4 + (2 * v6));
  v8 = (ap_uint<16>(v7[0]) << 0) |
    (ap_uint<16>(v7[1]) << 8);
  v9 = ap_uint<16>(v8);
  v1[0] = (v9 >> 0);
  nanotube_memcpy(port1_data.data, v1, 1);
  port1.write(port1_data);
  goto L2;

L2:
  return;
}
