;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'testing/pass_tests/mem2req/map_select.ll'
source_filename = "testing/pass_tests/mem2req/map_select.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_map = type opaque
%struct.nanotube_packet = type opaque

@name = private unnamed_addr constant [5 x i8] c"prog\00", align 1
@key = private unnamed_addr constant [4 x i8] c"\01\02\03\04", align 1

declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr

declare dso_local %struct.nanotube_map* @nanotube_map_create(i16 zeroext, i32, i64, i64) local_unnamed_addr

declare dso_local void @nanotube_context_add_map(%struct.nanotube_context*, %struct.nanotube_map*) local_unnamed_addr

declare void @nanotube_add_plain_packet_kernel(i8*, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32)

declare dso_local i8* @nanotube_map_lookup(%struct.nanotube_context*, i16 zeroext, i8*, i64, i64) local_unnamed_addr

define void @nanotube_setup() {
  %call = tail call %struct.nanotube_context* @nanotube_context_create()
  %map_0 = tail call %struct.nanotube_map* @nanotube_map_create(i16 zeroext 0, i32 0, i64 4, i64 8)
  tail call void @nanotube_context_add_map(%struct.nanotube_context* %call, %struct.nanotube_map* %map_0)
  %map_1 = tail call %struct.nanotube_map* @nanotube_map_create(i16 zeroext 1, i32 0, i64 4, i64 8)
  tail call void @nanotube_context_add_map(%struct.nanotube_context* %call, %struct.nanotube_map* %map_1)
  call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @name, i32 0, i32 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* @prog, i32 0, i32 1)
  ret void
}

define private i32 @prog(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) local_unnamed_addr {
entry:
  %ptr_mask = alloca i8, i64 1
  %_copy1 = alloca i8, i64 4
  %entry_1_buf = alloca i8, i64 8
  %_copy = alloca i8, i64 4
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %_copy, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @key, i32 0, i32 0), i64 4, i1 false)
  %entry_0_buf = alloca i8, i64 8
  %key_check = call i64 @nanotube_map_read(%struct.nanotube_context* %nt_ctx, i16 0, i8* %_copy, i64 4, i8* %entry_0_buf, i64 0, i64 8)
  %succ_0 = icmp ne i64 %key_check, 0
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %_copy1, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @key, i32 0, i32 0), i64 4, i1 false)
  %key_check2 = call i64 @nanotube_map_read(%struct.nanotube_context* %nt_ctx, i16 1, i8* %_copy1, i64 4, i8* %entry_1_buf, i64 0, i64 8)
  %ptr3 = select i1 %succ_0, i64 0, i64 0
  %0 = select i1 %succ_0, i8* %entry_0_buf, i8* %entry_1_buf
  %1 = select i1 %succ_0, i8* %_copy, i8* %_copy1
  %2 = select i1 %succ_0, i16 0, i16 1
  %3 = select i1 %succ_0, i64 %key_check, i64 %key_check2
  %succ = icmp ne i64 %3, 0
  br i1 %succ, label %edit, label %done

edit:                                             ; preds = %entry
  %ptr_gep = getelementptr i8, i8* %0, i64 %ptr3
  %old_val = load i8, i8* %ptr_gep
  %new_val = add i8 %old_val, 1
  %ptr_gep4 = getelementptr i8, i8* %0, i64 %ptr3
  store i8 %new_val, i8* %ptr_gep4
  call void @nanotube_init_mask(i8* %ptr_mask, i64 1, i64 %ptr3, i64 1)
  %ptr_wr = call i64 @nanotube_map_update_masked(%struct.nanotube_context* %nt_ctx, i16 %2, i8* %1, i64 4, i8* %0, i8* %ptr_mask, i64 0, i64 8)
  br label %done

done:                                             ; preds = %edit, %entry
  ret i32 0
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #0

; Function Attrs: inaccessiblemem_or_argmemonly
declare i64 @nanotube_map_read(%struct.nanotube_context*, i16, i8*, i64, i8*, i64, i64) #1

declare void @nanotube_init_mask(i8*, i64, i64, i64)

declare i64 @nanotube_map_update_masked(%struct.nanotube_context*, i16, i8*, i64, i8*, i8*, i64, i64)

attributes #0 = { argmemonly nounwind }
attributes #1 = { inaccessiblemem_or_argmemonly }
