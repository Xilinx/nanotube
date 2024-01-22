;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/mem2req/map_phi_simple.ll"
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
  %call1 = tail call %struct.nanotube_map* @nanotube_map_create(i16 zeroext 0, i32 0, i64 4, i64 8)
  tail call void @nanotube_context_add_map(%struct.nanotube_context* %call, %struct.nanotube_map* %call1)
  call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @name, i32 0, i32 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* @prog, i32 0, i32 1)
  ret void
}

define private i32 @prog(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) local_unnamed_addr {
entry:
  %ptr_mask = alloca i8, i64 1
  %_copy = alloca i8, i64 4
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %_copy, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @key, i32 0, i32 0), i64 4, i1 false)
  %entry_0_buf = alloca i8, i64 8
  %key_check = call i64 @nanotube_map_read(%struct.nanotube_context* %nt_ctx, i16 0, i8* %_copy, i64 4, i8* %entry_0_buf, i64 0, i64 8)
  %succ = icmp ne i64 %key_check, 0
  br i1 %succ, label %lookup_okay, label %done

lookup_okay:                                      ; preds = %entry
  %byte_0 = load i8, i8* %entry_0_buf
  %cond = icmp ne i8 %byte_0, 0
  br i1 %cond, label %ne_0, label %edit

ne_0:                                             ; preds = %lookup_okay
  br label %edit

edit:                                             ; preds = %ne_0, %lookup_okay
  %0 = phi i64 [ 1, %lookup_okay ], [ 2, %ne_0 ]
  %ptr_gep = getelementptr i8, i8* %entry_0_buf, i64 %0
  store i8 0, i8* %ptr_gep
  call void @nanotube_init_mask(i8* %ptr_mask, i64 1, i64 %0, i64 1)
  %ptr_wr = call i64 @nanotube_map_update_masked(%struct.nanotube_context* %nt_ctx, i16 0, i8* %_copy, i64 4, i8* %entry_0_buf, i8* %ptr_mask, i64 0, i64 8)
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
