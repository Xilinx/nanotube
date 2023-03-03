;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/rewrite-setup/simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_channel = type opaque
%struct.nanotube_context = type opaque

@chan.str = private constant [8 x i8] c"channel\00"
@logic.str = private constant [6 x i8] c"logic\00"

declare i8* @nanotube_malloc(i64)

declare %struct.nanotube_channel* @nanotube_channel_create(i8*, i64, i64)

declare void @nanotube_channel_export(%struct.nanotube_channel*, i32, i32)

declare %struct.nanotube_context* @nanotube_context_create()

declare void @nanotube_context_add_channel(%struct.nanotube_context*, i32, %struct.nanotube_channel*, i32)

declare void @nanotube_thread_create(%struct.nanotube_context*, i8*, void (%struct.nanotube_context*, i8*)*, i8*, i64)

declare void @nanotube_thread_wait()

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1) #0

define void @logic_0(%struct.nanotube_context* %ctxt, i8* %data) {
entry:
  call void @nanotube_thread_wait()
  ret void
}

define void @nanotube_setup() {
entry:
  %alloca.0 = alloca i32, i32 4, align 4
  store i32 5678, i32* %alloca.0
  %mem = call i8* @nanotube_malloc(i64 8)
  %0 = bitcast i8* %mem to i32*
  store i32 1234, i32* %0
  %1 = getelementptr i8, i8* %mem, i64 4
  call void @llvm.memset.p0i8.i64(i8* %1, i8 85, i64 4, i1 false)
  %chan = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @chan.str, i32 0, i32 0), i64 8, i64 8)
  call void @nanotube_channel_export(%struct.nanotube_channel* %chan, i32 1, i32 2)
  %ctxt = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %ctxt, i32 0, %struct.nanotube_channel* %chan, i32 1)
  call void @nanotube_thread_create(%struct.nanotube_context* %ctxt, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @logic.str, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @logic_0, i8* %mem, i64 8)
  %mem1 = call i8* @nanotube_malloc(i64 8)
  %2 = bitcast i8* %mem1 to i32*
  store i32 1234, i32* %2
  %3 = getelementptr i8, i8* %mem1, i64 4
  call void @llvm.memset.p0i8.i64(i8* %3, i8 85, i64 4, i1 false)
  %chan2 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @chan.str, i32 0, i32 0), i64 8, i64 8)
  call void @nanotube_channel_export(%struct.nanotube_channel* %chan2, i32 1, i32 2)
  %ctxt3 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %ctxt3, i32 0, %struct.nanotube_channel* %chan2, i32 1)
  call void @nanotube_thread_create(%struct.nanotube_context* %ctxt3, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @logic.str, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @logic_0, i8* %mem1, i64 8)
  %mem4 = call i8* @nanotube_malloc(i64 8)
  %4 = bitcast i8* %mem4 to i32*
  store i32 1234, i32* %4
  %5 = getelementptr i8, i8* %mem4, i64 4
  call void @llvm.memset.p0i8.i64(i8* %5, i8 85, i64 4, i1 false)
  %chan5 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @chan.str, i32 0, i32 0), i64 8, i64 8)
  call void @nanotube_channel_export(%struct.nanotube_channel* %chan5, i32 1, i32 2)
  %ctxt6 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %ctxt6, i32 0, %struct.nanotube_channel* %chan5, i32 1)
  call void @nanotube_thread_create(%struct.nanotube_context* %ctxt6, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @logic.str, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @logic_0, i8* %mem4, i64 8)
  %mem7 = call i8* @nanotube_malloc(i64 8)
  %6 = bitcast i8* %mem7 to i32*
  store i32 1234, i32* %6
  %7 = getelementptr i8, i8* %mem7, i64 4
  call void @llvm.memset.p0i8.i64(i8* %7, i8 85, i64 4, i1 false)
  %chan8 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @chan.str, i32 0, i32 0), i64 8, i64 8)
  call void @nanotube_channel_export(%struct.nanotube_channel* %chan8, i32 1, i32 2)
  %ctxt9 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %ctxt9, i32 0, %struct.nanotube_channel* %chan8, i32 1)
  call void @nanotube_thread_create(%struct.nanotube_context* %ctxt9, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @logic.str, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @logic_0, i8* %mem7, i64 8)
  %mem10 = call i8* @nanotube_malloc(i64 8)
  %8 = bitcast i8* %mem10 to i32*
  store i32 1234, i32* %8
  %9 = getelementptr i8, i8* %mem10, i64 4
  call void @llvm.memset.p0i8.i64(i8* %9, i8 85, i64 4, i1 false)
  %chan11 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @chan.str, i32 0, i32 0), i64 8, i64 8)
  call void @nanotube_channel_export(%struct.nanotube_channel* %chan11, i32 1, i32 2)
  %ctxt12 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %ctxt12, i32 0, %struct.nanotube_channel* %chan11, i32 1)
  call void @nanotube_thread_create(%struct.nanotube_context* %ctxt12, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @logic.str, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @logic_0, i8* %mem10, i64 8)
  %mem13 = call i8* @nanotube_malloc(i64 8)
  %10 = bitcast i8* %mem13 to i32*
  store i32 1234, i32* %10
  %11 = getelementptr i8, i8* %mem13, i64 4
  call void @llvm.memset.p0i8.i64(i8* %11, i8 85, i64 4, i1 false)
  %chan14 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @chan.str, i32 0, i32 0), i64 8, i64 8)
  call void @nanotube_channel_export(%struct.nanotube_channel* %chan14, i32 1, i32 2)
  %ctxt15 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %ctxt15, i32 0, %struct.nanotube_channel* %chan14, i32 1)
  call void @nanotube_thread_create(%struct.nanotube_context* %ctxt15, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @logic.str, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @logic_0, i8* %mem13, i64 8)
  %mem16 = call i8* @nanotube_malloc(i64 8)
  %12 = bitcast i8* %mem16 to i32*
  store i32 1234, i32* %12
  %13 = getelementptr i8, i8* %mem16, i64 4
  call void @llvm.memset.p0i8.i64(i8* %13, i8 85, i64 4, i1 false)
  %chan17 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @chan.str, i32 0, i32 0), i64 8, i64 8)
  call void @nanotube_channel_export(%struct.nanotube_channel* %chan17, i32 1, i32 2)
  %ctxt18 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %ctxt18, i32 0, %struct.nanotube_channel* %chan17, i32 1)
  call void @nanotube_thread_create(%struct.nanotube_context* %ctxt18, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @logic.str, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @logic_0, i8* %mem16, i64 8)
  %mem19 = call i8* @nanotube_malloc(i64 8)
  %14 = bitcast i8* %mem19 to i32*
  store i32 1234, i32* %14
  %15 = getelementptr i8, i8* %mem19, i64 4
  call void @llvm.memset.p0i8.i64(i8* %15, i8 85, i64 4, i1 false)
  %chan20 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @chan.str, i32 0, i32 0), i64 8, i64 8)
  call void @nanotube_channel_export(%struct.nanotube_channel* %chan20, i32 1, i32 2)
  %ctxt21 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %ctxt21, i32 0, %struct.nanotube_channel* %chan20, i32 1)
  call void @nanotube_thread_create(%struct.nanotube_context* %ctxt21, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @logic.str, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @logic_0, i8* %mem19, i64 8)
  ret void
}

attributes #0 = { argmemonly nounwind }
