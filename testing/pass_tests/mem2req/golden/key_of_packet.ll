;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/mem2req/key_of_packet.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_map = type opaque
%struct.nanotube_packet = type opaque

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
  %entry_0__mask = alloca i8, i64 1, align 1
  %entry_0_buffer_copy = alloca i8, i64 4, align 1
  %entry_0_buf = alloca i8, i64 8, align 1
  %entry_0_buffer = alloca i8, i64 4, align 1
  %1 = call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 32767, i32 1)
  %icmp = icmp ugt i64 8, %1
  br i1 %icmp, label %done, label %lookup

lookup:                                           ; preds = %0
  %pkt_begin__rd = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %entry_0_buffer, i64 0, i64 4)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %entry_0_buffer_copy, i8* %entry_0_buffer, i64 4, i1 false)
  %key_check = call i64 @nanotube_map_read(%struct.nanotube_context* %nt_ctx, i16 0, i8* %entry_0_buffer_copy, i64 4, i8* %entry_0_buf, i64 0, i64 8)
  %succ = icmp ne i64 %key_check, 0
  br i1 %succ, label %edit, label %done

edit:                                             ; preds = %lookup
  %old_val = load i8, i8* %entry_0_buf, align 1
  %new_val = add i8 %old_val, 1
  store i8 %new_val, i8* %entry_0_buf, align 1
  call void @nanotube_init_mask(i8* %entry_0__mask, i64 1, i64 0, i64 1)
  %entry_0__wr = call i64 @nanotube_map_update_masked(%struct.nanotube_context* %nt_ctx, i16 0, i8* %entry_0_buffer_copy, i64 4, i8* %entry_0_buf, i8* %entry_0__mask, i64 0, i64 8)
  br label %done

done:                                             ; preds = %edit, %lookup, %0
  ret i32 0
}

; Function Attrs: inaccessiblememonly
declare i64 @nanotube_packet_bounded_length(%struct.nanotube_packet*, i64, i32) #0

; Function Attrs: inaccessiblemem_or_argmemonly
declare i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64) #1

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1 immarg) #2

; Function Attrs: inaccessiblemem_or_argmemonly
declare i64 @nanotube_map_read(%struct.nanotube_context*, i16, i8*, i64, i8*, i64, i64) #1

declare void @nanotube_init_mask(i8*, i64, i64, i64)

declare i64 @nanotube_map_update_masked(%struct.nanotube_context*, i16, i8*, i64, i8*, i8*, i64, i64)

attributes #0 = { inaccessiblememonly }
attributes #1 = { inaccessiblemem_or_argmemonly }
attributes #2 = { argmemonly nofree nosync nounwind willreturn }
