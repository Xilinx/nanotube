;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; The aim of this test is to check that the select instruction is
; handled correctly for map pointers.  It performs two map lookups and
; perfoms a select operation on them.  It then performs a null check
; on the resulting pointer and performs loads and stores if it is not
; null.

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque
%struct.nanotube_map = type opaque

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
  %entry_0 = call i8* @nanotube_map_lookup(%struct.nanotube_context* %nt_ctx, i16 0, i8* bitcast([4 x i8]* @key to i8*), i64 4, i64 8)
  %succ_0 = icmp ne i8* %entry_0, null
  %entry_1 = call i8* @nanotube_map_lookup(%struct.nanotube_context* %nt_ctx, i16 1, i8* bitcast([4 x i8]* @key to i8*), i64 4, i64 8)
  %ptr = select i1 %succ_0, i8* %entry_0, i8* %entry_1
  %succ = icmp ne i8* %ptr, null
  br i1 %succ, label %edit, label %done

edit:
  %old_val = load i8, i8* %ptr
  %new_val = add i8 %old_val, 1
  store i8 %new_val, i8* %ptr
  br label %done

done:
  ret i32 0
}
