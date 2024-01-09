;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; A test to make sure the memcpy optimiziations are performed
; correctly.  The test function prepares some pointers to packet data,
; some pointers to map entries and uses some pointers to static
; buffers.  It then decides which case to perform and jumps to a basic
; block which performs a memcpy between two of the pointers.

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque
%struct.nanotube_map = type opaque

@name = private unnamed_addr constant [5 x i8] c"prog\00", align 1
@key = private unnamed_addr constant [4 x i8] c"\01\02\03\04", align 1
@buf_0 = internal unnamed_addr global [4 x i8] c"\00\00\00\00"
@buf_1 = internal unnamed_addr global [4 x i8] c"\00\00\00\00"

declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) local_unnamed_addr
declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr
declare dso_local i8* @nanotube_packet_data(%struct.nanotube_packet*) local_unnamed_addr
declare dso_local i8* @nanotube_packet_end(%struct.nanotube_packet*) local_unnamed_addr
declare dso_local %struct.nanotube_map* @nanotube_map_create(i16 zeroext, i32, i64, i64) local_unnamed_addr
declare dso_local void @nanotube_context_add_map(%struct.nanotube_context*, %struct.nanotube_map*) local_unnamed_addr
declare void @nanotube_add_plain_packet_kernel(i8*, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32)
declare dso_local i8* @nanotube_map_lookup(%struct.nanotube_context*, i16 zeroext, i8*, i64, i64) local_unnamed_addr

define void @nanotube_setup() {
  %ctxt = tail call %struct.nanotube_context* @nanotube_context_create()
  %map_0 = tail call %struct.nanotube_map* @nanotube_map_create(i16 zeroext 0, i32 0, i64 4, i64 8)
  tail call void @nanotube_context_add_map(%struct.nanotube_context* %ctxt, %struct.nanotube_map* %map_0)
  %map_1 = tail call %struct.nanotube_map* @nanotube_map_create(i16 zeroext 1, i32 0, i64 4, i64 8)
  tail call void @nanotube_context_add_map(%struct.nanotube_context* %ctxt, %struct.nanotube_map* %map_1)
  call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @name, i32 0, i32 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* @prog, i32 0, i32 1)
  ret void
}

define private i32 @prog(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) local_unnamed_addr {
entry:
  ; Get two pointers to packet pointers and make sure the packet is large enough.
  %pkt_begin = tail call i8* @nanotube_packet_data(%struct.nanotube_packet* %packet)
  %pkt_end = tail call i8* @nanotube_packet_end(%struct.nanotube_packet* %packet)
  %pkt_0 = getelementptr inbounds i8, i8* %pkt_begin, i64 4
  %pkt_1 = getelementptr inbounds i8, i8* %pkt_begin, i64 8
  %hdr_end = getelementptr inbounds i8, i8* %pkt_begin, i64 12
  %pkt_okay = icmp ugt i8* %hdr_end, %pkt_end

  ; Get two pointers to map entries and make sure they are not null.
  %entry_0 = call i8* @nanotube_map_lookup(%struct.nanotube_context* %nt_ctx, i16 0, i8* bitcast([4 x i8]* @key to i8*), i64 4, i64 8)
  %entry_0_okay = icmp ne i8* %entry_0, null

  %entry_1 = call i8* @nanotube_map_lookup(%struct.nanotube_context* %nt_ctx, i16 1, i8* bitcast([4 x i8]* @key to i8*), i64 4, i64 8)
  %entry_1_okay = icmp ne i8* %entry_1, null

  %entries_okay = and i1 %entry_0_okay, %entry_1_okay
  %all_okay = and i1 %entries_okay, %pkt_okay

  br i1 %all_okay, label %edit, label %done

edit:
  ; Choose a case and jump to the relevant basic block.
  %case = load i8, i8* %pkt_begin
  switch i8 %case, label %case_mem2mem [
    i8 1, label %case_mem2pkt
    i8 2, label %case_mem2map
    i8 3, label %case_pkt2mem
    i8 4, label %case_pkt2pkt
    i8 5, label %case_pkt2map
    i8 6, label %case_map2mem
    i8 7, label %case_map2pkt
    i8 8, label %case_map2map
  ]

case_mem2mem:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* bitcast ([4 x i8]* @buf_1 to i8*), i8* bitcast ([4 x i8]* @buf_0 to i8*), i64 4, i1 false)
  br label %done

case_mem2pkt:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %pkt_0, i8* bitcast ([4 x i8]* @buf_1 to i8*), i64 4, i1 false)
  br label %done

case_mem2map:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %entry_0, i8* bitcast ([4 x i8]* @buf_1 to i8*), i64 4, i1 false)
  br label %done

case_pkt2mem:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* bitcast ([4 x i8]* @buf_0 to i8*), i8* %pkt_1, i64 4, i1 false)
  br label %done

case_pkt2pkt:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %pkt_1, i8* %pkt_0, i64 4, i1 false)
  br label %done

case_pkt2map:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %entry_0, i8* %pkt_1, i64 4, i1 false)
  br label %done

case_map2mem:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* bitcast ([4 x i8]* @buf_0 to i8*), i8* %entry_1, i64 4, i1 false)
  br label %done

case_map2pkt:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %pkt_0, i8* %entry_1, i64 4, i1 false)
  br label %done

case_map2map:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %entry_1, i8* %entry_0, i64 4, i1 false)
  br label %done

done:
  ret i32 0
}
