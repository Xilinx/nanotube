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

static uint8_t s0[1] = {
  0x0a,
};
static uint8_t s1[2] = {
  0x14, 0x1e,
};
static uint8_t s2[8] = {
  0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static uint8_t s3[16] = {
  0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

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
  ap_uint<64> v10;
  ap_uint<64> v11;
  ap_uint<64> v12;
  ap_uint<8> v13;
  ap_uint<8> v14;
  ap_uint<64> v15;
  ap_uint<64> v16;
  ap_uint<64> v17;
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
#pragma HLS array_partition variable=s0 complete
#pragma HLS array_partition variable=s1 complete
#pragma HLS array_partition variable=s2 complete
#pragma HLS array_partition variable=s3 complete

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
  v4 = (ap_uint<8>(s0[0]) << 0);
  v5 = v4 + v3;
  s0[0] = (v5 >> 0);
  v0[0] = (v5 >> 0);
  v6 = (ap_uint<8>((v0+1)[0]) << 0);
  v7 = (ap_uint<8>((s1+1)[0]) << 0);
  v8 = v7 + v6;
  (s1+1)[0] = (v8 >> 0);
  (v0+1)[0] = (v8 >> 0);
  v9 = (ap_uint<8>((v0+2)[0]) << 0);
  v10 = ap_uint<8>(v9);
  v11 = (ap_uint<64>(s2[0]) << 0) |
    (ap_uint<64>(s2[1]) << 8) |
    (ap_uint<64>(s2[2]) << 16) |
    (ap_uint<64>(s2[3]) << 24) |
    (ap_uint<64>(s2[4]) << 32) |
    (ap_uint<64>(s2[5]) << 40) |
    (ap_uint<64>(s2[6]) << 48) |
    (ap_uint<64>(s2[7]) << 56);
  v12 = v11 + v10;
  s2[0] = (v12 >> 0);
  s2[1] = (v12 >> 8);
  s2[2] = (v12 >> 16);
  s2[3] = (v12 >> 24);
  s2[4] = (v12 >> 32);
  s2[5] = (v12 >> 40);
  s2[6] = (v12 >> 48);
  s2[7] = (v12 >> 56);
  v13 = ap_uint<64>(v12);
  (v0+2)[0] = (v13 >> 0);
  v14 = (ap_uint<8>((v0+3)[0]) << 0);
  v15 = ap_uint<8>(v14);
  v16 = (ap_uint<64>((s3+8)[0]) << 0) |
    (ap_uint<64>((s3+8)[1]) << 8) |
    (ap_uint<64>((s3+8)[2]) << 16) |
    (ap_uint<64>((s3+8)[3]) << 24) |
    (ap_uint<64>((s3+8)[4]) << 32) |
    (ap_uint<64>((s3+8)[5]) << 40) |
    (ap_uint<64>((s3+8)[6]) << 48) |
    (ap_uint<64>((s3+8)[7]) << 56);
  v17 = v16 + v15;
  (s3+8)[0] = (v17 >> 0);
  (s3+8)[1] = (v17 >> 8);
  (s3+8)[2] = (v17 >> 16);
  (s3+8)[3] = (v17 >> 24);
  (s3+8)[4] = (v17 >> 32);
  (s3+8)[5] = (v17 >> 40);
  (s3+8)[6] = (v17 >> 48);
  (s3+8)[7] = (v17 >> 56);
  v18 = ap_uint<64>(v17);
  (v0+3)[0] = (v18 >> 0);
  nanotube_memcpy(port1_data.data, v0, 65);
  port1.write(port1_data);
  goto L2;

L2:
  return;
}
