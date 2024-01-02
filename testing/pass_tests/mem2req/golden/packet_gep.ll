;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/mem2req/packet_gep.ll"
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
  %len.i8_buffer = alloca i8
  %0 = call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 32767)
  %icmp_len = icmp ugt i64 1, %0
  br i1 %icmp_len, label %done, label %check_len

check_len:                                        ; preds = %entry
  %pkt_begin__rd = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %len.i8_buffer, i64 0, i64 1)
  %len.i8 = load i8, i8* %len.i8_buffer
  %len = zext i8 %len.i8 to i64
  %data_begin.addb = add i64 0, %len
  %data_end.addb = add i64 %data_begin.addb, 1
  %icmp_data = icmp ugt i64 %data_end.addb, %0
  br i1 %icmp_data, label %done, label %edit

edit:                                             ; preds = %check_len
  store i8 42, i8* %_buffer
  %data_begin_wr = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %_buffer, i64 %data_begin.addb, i64 1)
  br label %done

done:                                             ; preds = %edit, %check_len, %entry
  %retval = phi i32 [ 1, %entry ], [ 1, %check_len ], [ 0, %edit ]
  ret i32 %retval
}

declare i64 @nanotube_packet_bounded_length(%struct.nanotube_packet*, i64)

declare i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64)

declare i64 @nanotube_packet_write(%struct.nanotube_packet*, i8*, i64, i64)
