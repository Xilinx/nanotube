;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; This test performs two lookups on the same map, peforms a store
; through one pointer and then performs a load and a store through the
; other pointer.  The current implementation of mem2req is expected to
; report an error in this case because it doesn't do either a static
; or dynamic check to determine whether the keys are different.  We
; might add support for this case in the future, in which case the
; test expectation can be changed.

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque
%struct.nanotube_map = type opaque

@name = private unnamed_addr constant [5 x i8] c"prog\00", align 1
@key_0 = private unnamed_addr constant [4 x i8] c"\01\02\03\04", align 1
@key_1 = private unnamed_addr constant [4 x i8] c"\05\06\07\08", align 1

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
  %entry_0 = call i8* @nanotube_map_lookup(%struct.nanotube_context* %nt_ctx, i16 0, i8* bitcast([4 x i8]* @key_0 to i8*), i64 4, i64 8)
  %succ_0 = icmp ne i8* %entry_0, null
  %entry_1 = call i8* @nanotube_map_lookup(%struct.nanotube_context* %nt_ctx, i16 0, i8* bitcast([4 x i8]* @key_1 to i8*), i64 4, i64 8)
  %succ_1 = icmp ne i8* %entry_1, null
  %succ = and i1 %succ_0, %succ_1
  br i1 %succ, label %edit, label %done

edit:
  store i8 0, i8* %entry_0
  %old_val = load i8, i8* %entry_1
  %new_val = add i8 %old_val, 1
  store i8 %new_val, i8* %entry_1
  br label %done

done:
  ret i32 0
}
