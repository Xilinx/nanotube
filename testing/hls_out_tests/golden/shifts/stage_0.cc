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
  uint8_t v0[65];
#pragma HLS array_partition variable=v0 complete
  ap_uint<32> v1;
  ap_uint<1> v2;
  ap_uint<8> v3;
  ap_uint<8> v4;
  ap_uint<8> v5;
  ap_uint<8> v6;
  ap_uint<8> v7;
  ap_uint<8> v8;
  ap_uint<8> v9;
  ap_uint<8> v10;
  ap_uint<8> v11;
  ap_uint<32> v12;
  ap_uint<8> v13;
  ap_uint<32> v14;
  ap_uint<32> v15;
  ap_uint<8> v16;
  ap_uint<8> v17;
  ap_uint<8> v18;

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
  v3 = (ap_uint<8>(v0[0]) << 0);
  v4 = v3 << ap_uint<8>(3);
  v5 = (ap_uint<8>((v0+1)[0]) << 0);
  v6 = v4 ^ v5;
  (v0+1)[0] = (v6 >> 0);
  v7 = (ap_uint<8>((v0+2)[0]) << 0);
  v8 = v7 >> ap_uint<8>(3);
  v9 = (ap_uint<8>((v0+3)[0]) << 0);
  v10 = v9 ^ v8;
  (v0+3)[0] = (v10 >> 0);
  v11 = (ap_uint<8>((v0+4)[0]) << 0);
  v12 = ap_int<8>(v11);
  v13 = (ap_uint<8>((v0+6)[0]) << 0);
  v14 = ap_uint<8>(v13);
  v15 = ap_int<32>(v12) >> ap_int<32>(v14);
  v16 = (ap_uint<8>((v0+5)[0]) << 0);
  v17 = ap_uint<32>(v15);
  v18 = v16 ^ v17;
  (v0+5)[0] = (v18 >> 0);
  nanotube_memcpy(port1_data.data, v0, 65);
  port1.write(port1_data);
  goto L2;

L2:
  return;
}
