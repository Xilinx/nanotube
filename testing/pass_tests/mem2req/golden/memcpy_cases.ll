;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'testing/pass_tests/mem2req/memcpy_cases.ll'
source_filename = "testing/pass_tests/mem2req/memcpy_cases.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque
%struct.nanotube_map = type opaque

@name = private unnamed_addr constant [5 x i8] c"prog\00", align 1
@key = private unnamed_addr constant [4 x i8] c"\01\02\03\04", align 1
@buf_0 = internal unnamed_addr global [4 x i8] zeroinitializer
@buf_1 = internal unnamed_addr global [4 x i8] zeroinitializer

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) local_unnamed_addr #0

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
  %entry_0__mask4 = alloca i8, i64 1
  %_buffer = alloca i8, i64 4
  %entry_0__mask = alloca i8, i64 1
  %entry_1__mask = alloca i8, i64 1
  %case_buffer = alloca i8
  %_copy1 = alloca i8, i64 4
  %entry_1_buf = alloca i8, i64 8
  %_copy = alloca i8, i64 4
  %entry_0_buf = alloca i8, i64 8
  %0 = call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 32767)
  %pkt_okay = icmp ugt i64 12, %0
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %_copy, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @key, i32 0, i32 0), i64 4, i1 false)
  %key_check = call i64 @nanotube_map_read(%struct.nanotube_context* %nt_ctx, i16 0, i8* %_copy, i64 4, i8* %entry_0_buf, i64 0, i64 8)
  %entry_0_okay = icmp ne i64 %key_check, 0
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %_copy1, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @key, i32 0, i32 0), i64 4, i1 false)
  %key_check2 = call i64 @nanotube_map_read(%struct.nanotube_context* %nt_ctx, i16 1, i8* %_copy1, i64 4, i8* %entry_1_buf, i64 0, i64 8)
  %entry_1_okay = icmp ne i64 %key_check2, 0
  %entries_okay = and i1 %entry_0_okay, %entry_1_okay
  %all_okay = and i1 %entries_okay, %pkt_okay
  br i1 %all_okay, label %edit, label %done

edit:                                             ; preds = %entry
  %pkt_begin__rd = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %case_buffer, i64 0, i64 1)
  %case = load i8, i8* %case_buffer
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

case_mem2mem:                                     ; preds = %edit
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @buf_1, i32 0, i32 0), i8* getelementptr inbounds ([4 x i8], [4 x i8]* @buf_0, i32 0, i32 0), i64 4, i1 false)
  br label %done

case_mem2pkt:                                     ; preds = %edit
  %pkt_0_wr6 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @buf_1, i32 0, i32 0), i64 4, i64 4)
  br label %done

case_mem2map:                                     ; preds = %edit
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %entry_0_buf, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @buf_1, i32 0, i32 0), i64 4, i1 false)
  call void @nanotube_init_mask(i8* %entry_0__mask4, i64 1, i64 0, i64 4)
  %entry_0__wr5 = call i64 @nanotube_map_update_masked(%struct.nanotube_context* %nt_ctx, i16 0, i8* %_copy, i64 4, i8* %entry_0_buf, i8* %entry_0__mask4, i64 0, i64 8)
  br label %done

case_pkt2mem:                                     ; preds = %edit
  %pkt_1_rd3 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @buf_0, i32 0, i32 0), i64 8, i64 4)
  br label %done

case_pkt2pkt:                                     ; preds = %edit
  %pkt_0_rd = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer, i64 4, i64 4)
  %pkt_1_wr = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %_buffer, i64 8, i64 4)
  br label %done

case_pkt2map:                                     ; preds = %edit
  %pkt_1_rd = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %entry_0_buf, i64 8, i64 4)
  call void @nanotube_init_mask(i8* %entry_0__mask, i64 1, i64 0, i64 4)
  %entry_0__wr = call i64 @nanotube_map_update_masked(%struct.nanotube_context* %nt_ctx, i16 0, i8* %_copy, i64 4, i8* %entry_0_buf, i8* %entry_0__mask, i64 0, i64 8)
  br label %done

case_map2mem:                                     ; preds = %edit
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @buf_0, i32 0, i32 0), i8* %entry_1_buf, i64 4, i1 false)
  br label %done

case_map2pkt:                                     ; preds = %edit
  %pkt_0_wr = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %entry_1_buf, i64 4, i64 4)
  br label %done

case_map2map:                                     ; preds = %edit
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %entry_1_buf, i8* %entry_0_buf, i64 4, i1 false)
  call void @nanotube_init_mask(i8* %entry_1__mask, i64 1, i64 0, i64 4)
  %entry_1__wr = call i64 @nanotube_map_update_masked(%struct.nanotube_context* %nt_ctx, i16 1, i8* %_copy1, i64 4, i8* %entry_1_buf, i8* %entry_1__mask, i64 0, i64 8)
  br label %done

done:                                             ; preds = %case_map2map, %case_map2pkt, %case_map2mem, %case_pkt2map, %case_pkt2pkt, %case_pkt2mem, %case_mem2map, %case_mem2pkt, %case_mem2mem, %entry
  ret i32 0
}

; Function Attrs: inaccessiblememonly
declare i64 @nanotube_packet_bounded_length(%struct.nanotube_packet*, i64) #1

; Function Attrs: inaccessiblemem_or_argmemonly
declare i64 @nanotube_map_read(%struct.nanotube_context*, i16, i8*, i64, i8*, i64, i64) #2

; Function Attrs: inaccessiblemem_or_argmemonly
declare i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64) #2

declare void @nanotube_init_mask(i8*, i64, i64, i64)

declare i64 @nanotube_map_update_masked(%struct.nanotube_context*, i16, i8*, i64, i8*, i8*, i64, i64)

; Function Attrs: inaccessiblemem_or_argmemonly
declare i64 @nanotube_packet_write(%struct.nanotube_packet*, i8*, i64, i64) #2

attributes #0 = { argmemonly nounwind }
attributes #1 = { inaccessiblememonly }
attributes #2 = { inaccessiblemem_or_argmemonly }
