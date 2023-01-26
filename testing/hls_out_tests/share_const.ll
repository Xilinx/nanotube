;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; //! \file   share_const.ll
; // \author  Neil Turton <neilt@amd.com>
; //  \brief  A HLS output pass error test for sharing constants.
; //   \date  2020-08-08
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; In this test, functions logic_0 and logic_1 both read
; variable @s8.  The HLS pass should not report an error.

source_filename = "testing/hls_out_tests/share_const.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%"struct.simple_bus::word" = type { [65 x i8] }
%struct.nanotube_channel = type opaque

@s8 = internal unnamed_addr global i8 0, align 1
@packets_in.str = private unnamed_addr constant [11 x i8] c"packets_in\00", align 1
@packets_through.str = private unnamed_addr constant [16 x i8] c"packets_through\00", align 1
@packets_out.str = private unnamed_addr constant [12 x i8] c"packets_out\00", align 1
@stage_0.str = private unnamed_addr constant [8 x i8] c"stage_0\00", align 1
@stage_1.str = private unnamed_addr constant [8 x i8] c"stage_1\00", align 1

declare dso_local i32 @nanotube_channel_try_read(%struct.nanotube_context*, i32, i8*, i64) local_unnamed_addr #0
declare dso_local void @nanotube_thread_wait() local_unnamed_addr #0
declare dso_local void @nanotube_channel_write(%struct.nanotube_context*, i32, i8*, i64) local_unnamed_addr #0
declare dso_local %struct.nanotube_channel* @nanotube_channel_create(i8*, i64, i64) local_unnamed_addr #0
declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr #0
declare dso_local void @nanotube_context_add_channel(%struct.nanotube_context*, i32, %struct.nanotube_channel*, i32) local_unnamed_addr #0
declare dso_local void @nanotube_thread_create(%struct.nanotube_context*, i8*, void (%struct.nanotube_context*, i8*)*, i8*, i64) local_unnamed_addr #0

; Function Attrs: uwtable
define dso_local void @logic_0(%struct.nanotube_context* %context, i8* nocapture readnone %arg) #1 {
entry:
  %word = alloca %"struct.simple_bus::word", align 1
  %data = getelementptr inbounds %"struct.simple_bus::word", %"struct.simple_bus::word"* %word, i64 0, i32 0, i64 0
  %succ.int = call i32 @nanotube_channel_try_read(%struct.nanotube_context* %context, i32 0, i8* nonnull %data, i64 65)
  %fail.bool = icmp eq i32 %succ.int, 0
  br i1 %fail.bool, label %read_fail, label %read_succ

read_fail:                                          ; preds = %entry
  call void @nanotube_thread_wait()
  br label %cleanup

read_succ:                                           ; preds = %entry
  %data.val = load i8, i8* %data, align 1
  %s8.val = load i8, i8* @s8, align 1
  %add = add i8 %s8.val, %data.val
  store i8 %add, i8* %data, align 1
  call void @nanotube_channel_write(%struct.nanotube_context* %context, i32 1, i8* nonnull %data, i64 65)
  br label %cleanup

cleanup:                                          ; preds = %read_succ, %read_fail
  ret void
}

; Function Attrs: uwtable
define dso_local void @logic_1(%struct.nanotube_context* %context, i8* nocapture readnone %arg) #1 {
entry:
  %word = alloca %"struct.simple_bus::word", align 1
  %data = getelementptr inbounds %"struct.simple_bus::word", %"struct.simple_bus::word"* %word, i64 0, i32 0, i64 0
  %succ.int = call i32 @nanotube_channel_try_read(%struct.nanotube_context* %context, i32 1, i8* nonnull %data, i64 65)
  %fail.bool = icmp eq i32 %succ.int, 0
  br i1 %fail.bool, label %read_fail, label %read_succ

read_fail:                                          ; preds = %entry
  call void @nanotube_thread_wait()
  br label %cleanup

read_succ:                                           ; preds = %entry
  %data.val = load i8, i8* %data, align 1
  %s8.val = load i8, i8* @s8, align 1
  %add = add i8 %s8.val, %data.val
  store i8 %add, i8* %data, align 1
  call void @nanotube_channel_write(%struct.nanotube_context* %context, i32 2, i8* nonnull %data, i64 65)
  br label %cleanup

cleanup:                                          ; preds = %read_succ, %read_fail
  ret void
}

; Function Attrs: uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #1 {
entry:
  %channel_0 = tail call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @packets_in.str, i64 0, i64 0), i64 65, i64 16)
  %channel_1 = tail call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([16 x i8], [16 x i8]* @packets_through.str, i64 0, i64 0), i64 65, i64 16)
  %channel_2 = tail call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @packets_out.str, i64 0, i64 0), i64 65, i64 16)
  %context_0 = tail call %struct.nanotube_context* @nanotube_context_create()
  tail call void @nanotube_context_add_channel(%struct.nanotube_context* %context_0, i32 0, %struct.nanotube_channel* %channel_0, i32 1)
  tail call void @nanotube_context_add_channel(%struct.nanotube_context* %context_0, i32 1, %struct.nanotube_channel* %channel_1, i32 2)
  tail call void @nanotube_thread_create(%struct.nanotube_context* %context_0, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @stage_0.str, i64 0, i64 0), void (%struct.nanotube_context*, i8*)* nonnull @logic_0, i8* null, i64 0)
  %context_1 = tail call %struct.nanotube_context* @nanotube_context_create()
  tail call void @nanotube_context_add_channel(%struct.nanotube_context* %context_1, i32 1, %struct.nanotube_channel* %channel_1, i32 1)
  tail call void @nanotube_context_add_channel(%struct.nanotube_context* %context_1, i32 2, %struct.nanotube_channel* %channel_2, i32 2)
  tail call void @nanotube_thread_create(%struct.nanotube_context* %context_1, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @stage_1.str, i64 0, i64 0), void (%struct.nanotube_context*, i8*)* nonnull @logic_1, i8* null, i64 0)
  ret void
}

attributes #0 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
