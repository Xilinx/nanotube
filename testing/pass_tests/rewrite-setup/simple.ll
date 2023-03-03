; *************************************************************************
; *! \file simple.ll
; \author  Neil Turton <neilt@amd.com>
;  \brief  A simple test for the rewrite-setup pass.
;   \date  2021-12-09
; *************************************************************************
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

source_filename = "testing/pass_tests/rewrite-setup/simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_channel = type opaque

declare i8* @nanotube_malloc(i64)
declare %struct.nanotube_channel* @nanotube_channel_create(i8*, i64, i64)
declare void @nanotube_channel_export(%struct.nanotube_channel*, i32, i32)
declare %struct.nanotube_context* @nanotube_context_create()
declare void @nanotube_context_add_channel(%struct.nanotube_context*, i32, %struct.nanotube_channel*, i32)
declare void @nanotube_thread_create(%struct.nanotube_context*, i8*, void (%struct.nanotube_context*, i8*)*, i8*, i64) 
declare void @nanotube_thread_wait()

;declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #1
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1) #1

@chan.str = private constant [8 x i8] c"channel\00"
@logic.str = private constant [6 x i8] c"logic\00"
;@init_data = private constant [2 x i8] [i8 1, i8 2]

define void @logic_0(%struct.nanotube_context* %ctxt, i8* %data) {
entry:
  call void @nanotube_thread_wait()
  ret void
}

define void @nanotube_setup() {
entry:
  %alloca.0 = alloca i32, i32 4, align 4
  store i32 5678, i32* %alloca.0
  br label %loop

loop:
  %count = phi i32 [0, %entry], [%inc, %loop]
  %inc = add i32 %count, 1

  %mem = call i8* @nanotube_malloc(i64 8)

  %mem_0.i32 = bitcast i8* %mem to i32*
  store i32 1234, i32* %mem_0.i32

  %mem_4.i8 = getelementptr inbounds i8, i8* %mem, i32 4
  call void @llvm.memset.p0i8.i64(i8* %mem_4.i8, i8 85, i64 4, i1 0)

  %chan = call %struct.nanotube_channel* @nanotube_channel_create(i8* bitcast ([8 x i8]* @chan.str to i8*), i64 8, i64 8)
  call void @nanotube_channel_export(%struct.nanotube_channel* %chan, i32 1, i32 2)
  %ctxt = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %ctxt, i32 0, %struct.nanotube_channel* %chan, i32 1)
  call void @nanotube_thread_create(%struct.nanotube_context* %ctxt, i8* bitcast ([6 x i8]* @logic.str to i8*), void (%struct.nanotube_context*, i8*)* @logic_0, i8* %mem, i64 8)

  %cont = icmp ult i32 %count, 7
  br i1 %cont, label %loop, label %exit

exit:
  ret void
}

attributes #1 = { argmemonly nounwind }
