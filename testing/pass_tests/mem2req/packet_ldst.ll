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
  %hdr_end = getelementptr inbounds i8, i8* %pkt_begin, i64 8
  %icmp = icmp ugt i8* %hdr_end, %pkt_end
  br i1 %icmp, label %done, label %edit

edit:
  %field_i8 = getelementptr inbounds i8, i8* %pkt_begin, i64 4
  %field_i32 = bitcast i8* %field_i8 to i32*
  %old_val = load i32, i32* %field_i32
  %new_val = add i32 %old_val, 1
  store i32 %new_val, i32* %field_i32
  br label %done

done:
  %retval = phi i32 [ 0, %edit ], [ 1, %entry ]
  ret i32 %retval
}
