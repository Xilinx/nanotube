;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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

define dso_local i32 @prog(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) local_unnamed_addr {
entry:
  %pkt_begin = tail call i8* @nanotube_packet_data(%struct.nanotube_packet* %packet)
  %pkt_end = tail call i8* @nanotube_packet_end(%struct.nanotube_packet* %packet)
  %len_end = getelementptr inbounds i8, i8* %pkt_begin, i64 1
  %icmp_len = icmp ugt i8* %len_end, %pkt_end
  br i1 %icmp_len, label %done, label %check_len

check_len:
  %len.i8 = load i8, i8* %pkt_begin
  %len = zext i8 %len.i8 to i64
  %data_begin = getelementptr inbounds i8, i8* %pkt_begin, i64 %len
  %data_end = getelementptr inbounds i8, i8* %data_begin, i32 1
  %icmp_data = icmp ugt i8* %data_end, %pkt_end
  br i1 %icmp_data, label %done, label %edit

edit:
  store i8 42, i8* %data_begin
  br label %done

done:
  %retval = phi i32 [ 1, %entry ], [ 1, %check_len ], [ 0, %edit ]
  ret i32 %retval
}
