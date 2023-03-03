;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/rewrite-setup/simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@init_data = private constant [3 x i16] [i16 512, i16 256, i16 1024]

declare i8* @nanotube_malloc(i64)

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #0

define void @nanotube_setup() {
entry:
  %mem_0 = call i8* @nanotube_malloc(i64 6)
  %0 = bitcast i8* %mem_0 to i16*
  store i16 5, i16* %0
  %1 = getelementptr i8, i8* %mem_0, i64 2
  %2 = bitcast i8* %1 to i16*
  store i16 6, i16* %2
  %3 = getelementptr i8, i8* %mem_0, i64 4
  %4 = bitcast i8* %3 to i16*
  store i16 7, i16* %4
  %5 = getelementptr i8, i8* %mem_0, i64 1
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %5, i8* getelementptr (i8, i8* bitcast ([3 x i16]* @init_data to i8*), i64 1), i64 4, i1 false)
  %mem_1 = call i8* @nanotube_malloc(i64 4)
  %6 = bitcast i8* %mem_1 to i32*
  store i32 65538, i32* %6
  ret void
}

attributes #0 = { argmemonly nounwind }
