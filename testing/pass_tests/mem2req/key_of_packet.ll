;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; The aim of this test is to perform a map lookup with a key which is
; located in the packet.  The pass should read the key from the packet
; and perform the lookup using the copy on the stack.

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque
%struct.nanotube_map = type opaque

@name = private unnamed_addr constant [5 x i8] c"prog\00", align 1

declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr
declare dso_local %struct.nanotube_map* @nanotube_map_create(i16 zeroext, i32, i64, i64) local_unnamed_addr
declare dso_local void @nanotube_context_add_map(%struct.nanotube_context*, %struct.nanotube_map*) local_unnamed_addr
declare void @nanotube_add_plain_packet_kernel(i8*, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32)

declare dso_local i8* @nanotube_packet_data(%struct.nanotube_packet*) local_unnamed_addr
declare dso_local i8* @nanotube_packet_end(%struct.nanotube_packet*) local_unnamed_addr
declare dso_local i8* @nanotube_map_lookup(%struct.nanotube_context*, i16 zeroext, i8*, i64, i64) local_unnamed_addr

define void @nanotube_setup() {
  %ctxt = tail call %struct.nanotube_context* @nanotube_context_create()
  %map_0 = tail call %struct.nanotube_map* @nanotube_map_create(i16 zeroext 0, i32 0, i64 4, i64 8)
  tail call void @nanotube_context_add_map(%struct.nanotube_context* %ctxt, %struct.nanotube_map* %map_0)
  call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @name, i32 0, i32 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* @prog, i32 0, i32 1)
  ret void
}

define private i32 @prog(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) local_unnamed_addr {
  %pkt_begin = tail call i8* @nanotube_packet_data(%struct.nanotube_packet* %packet)
  %pkt_end = tail call i8* @nanotube_packet_end(%struct.nanotube_packet* %packet)
  %hdr_end = getelementptr inbounds i8, i8* %pkt_begin, i64 8
  %icmp = icmp ugt i8* %hdr_end, %pkt_end
  br i1 %icmp, label %done, label %lookup

lookup:
  %entry_0 = call i8* @nanotube_map_lookup(%struct.nanotube_context* %nt_ctx, i16 0, i8* %pkt_begin, i64 4, i64 8)
  %succ = icmp ne i8* %entry_0, null
  br i1 %succ, label %edit, label %done

edit:
  %old_val = load i8, i8* %entry_0
  %new_val = add i8 %old_val, 1
  store i8 %new_val, i8* %entry_0
  br label %done

done:
  ret i32 0
}
