;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/mem2req/packet_ldst.ll"
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
  %_buffer = alloca i32
  %_buffer.bc = bitcast i32* %_buffer to i8*
  %old_val_buffer = alloca i32
  %old_val_buffer.bc = bitcast i32* %old_val_buffer to i8*
  %0 = call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 32767)
  %icmp = icmp ugt i64 8, %0
  br i1 %icmp, label %done, label %edit

edit:                                             ; preds = %entry
  %1 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %old_val_buffer.bc, i64 4, i64 4)
  %old_val = load i32, i32* %old_val_buffer
  %new_val = add i32 %old_val, 1
  store i32 %new_val, i32* %_buffer
  %2 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %_buffer.bc, i64 4, i64 4)
  br label %done

done:                                             ; preds = %edit, %entry
  %retval = phi i32 [ 0, %edit ], [ 1, %entry ]
  ret i32 %retval
}

declare i64 @nanotube_packet_bounded_length(%struct.nanotube_packet*, i64)

declare i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64)

declare i64 @nanotube_packet_write(%struct.nanotube_packet*, i8*, i64, i64)
