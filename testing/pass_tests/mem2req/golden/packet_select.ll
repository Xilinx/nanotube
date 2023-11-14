;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/mem2req/packet_select.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque

@name = private unnamed_addr constant [5 x i8] c"prog\00", align 1

declare void @nanotube_add_plain_packet_kernel(i8*, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32)

declare dso_local i8* @nanotube_packet_data(%struct.nanotube_packet*) local_unnamed_addr

declare dso_local i8* @nanotube_packet_end(%struct.nanotube_packet*) local_unnamed_addr

define void @nanotube_setup() {
  call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @name, i32 0, i32 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* @prog, i32 0, i32 1)
  ret void
}

define private i32 @prog(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) local_unnamed_addr {
entry:
  %_buffer = alloca i8
  %byte_0_buffer = alloca i8
  %0 = call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 32767)
  %1 = icmp ugt i64 3, %0
  br i1 %1, label %done, label %edit

edit:                                             ; preds = %entry
  %2 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %byte_0_buffer, i64 0, i64 1)
  %3 = load i8, i8* %byte_0_buffer
  %sel = icmp ne i8 %3, 42
  %ptr1 = select i1 %sel, i64 1, i64 2
  store i8 0, i8* %_buffer
  %4 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %_buffer, i64 %ptr1, i64 1)
  br label %done

done:                                             ; preds = %edit, %entry
  %retval = phi i32 [ 0, %edit ], [ 1, %entry ]
  ret i32 %retval
}

declare i64 @nanotube_packet_bounded_length(%struct.nanotube_packet*, i64)

declare i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64)

declare i64 @nanotube_packet_write(%struct.nanotube_packet*, i8*, i64, i64)
