; *************************************************************************
; *! \file memcpy.ll
; \author  Neil Turton <neilt@amd.com>
;  \brief  A test for memcpy support in the rewrite-setup pass.
;   \date  2021-12-15
; *************************************************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

source_filename = "testing/pass_tests/rewrite-setup/simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i8* @nanotube_malloc(i64)
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #1

@init_data = private constant [3 x i16] [i16 512, i16 256, i16 1024]

define void @nanotube_setup() {
entry:
  %mem_0 = call i8* @nanotube_malloc(i64 6)
  %mem_0.1 = getelementptr i8, i8* %mem_0, i64 1
  %mem_0.1.i32 = bitcast i8* %mem_0.1 to i32*
  %mem_0.i16 = bitcast i8* %mem_0 to i16*
  %mem_0.i16.1 = getelementptr i16, i16* %mem_0.i16, i64 1
  %mem_0.i16.2 = getelementptr i16, i16* %mem_0.i16, i64 2
  store i16 5, i16* %mem_0.i16
  store i16 6, i16* %mem_0.i16.1
  store i16 7, i16* %mem_0.i16.2

  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %mem_0.1, i8* getelementptr(i8, i8* bitcast ([3 x i16]* @init_data to i8*), i64 1), i64 4, i1 0)

  %val.0 = load i32, i32* %mem_0.1.i32

  %mem_1 = call i8* @nanotube_malloc(i64 4)
  %mem_1.i32 = bitcast i8* %mem_1 to i32*
  store i32 %val.0, i32* %mem_1.i32
  ret void
}

attributes #1 = { argmemonly nounwind }
