;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/pipeline/simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_tap_packet_eop_state = type { i16, i16 }
%struct.nanotube_tap_packet_read_state = type { i16, i16, i16, i16, i8, i8 }
%struct.nanotube_tap_packet_write_state = type { i16, i16, i16, i16, i8, i8 }
%struct.nanotube_packet = type opaque
%struct.nanotube_channel = type opaque
%struct.nanotube_context = type opaque
%struct.nanotube_tap_packet_read_resp = type { i8, i16 }
%struct.nanotube_tap_packet_read_req = type { i8, i16, i16 }
%struct.nanotube_tap_packet_write_req = type { i8, i16, i16 }

@.str = private unnamed_addr constant [7 x i8] c"simple\00", align 1
@packet_eop_tap_state_stage_0 = private global %struct.nanotube_tap_packet_eop_state zeroinitializer
@sent_app_state_stage_0 = private global i1 false
@app_state_stage_1 = private global <{ [1 x i8] }> zeroinitializer
@have_app_state_stage_1 = private global i1 false
@packet_eop_tap_state_stage_1 = private global %struct.nanotube_tap_packet_eop_state zeroinitializer
@packet_read_data_stage_1 = private global [4 x i8] zeroinitializer
@packet_read_tap_state_stage_1 = private global %struct.nanotube_tap_packet_read_state zeroinitializer
@app_state_stage_2 = private global <{ [4 x i8], [1 x i8] }> zeroinitializer
@have_app_state_stage_2 = private global i1 false
@packet_eop_tap_state_stage_2 = private global %struct.nanotube_tap_packet_eop_state zeroinitializer
@packet_write_tap_state_stage_2 = private global %struct.nanotube_tap_packet_write_state zeroinitializer
@packet_eop_tap_state_stage_3 = private global %struct.nanotube_tap_packet_eop_state zeroinitializer
@0 = private unnamed_addr constant [15 x i8] c"packets_0_to_1\00", align 1
@1 = private unnamed_addr constant [15 x i8] c"packets_1_to_2\00", align 1
@2 = private unnamed_addr constant [15 x i8] c"packets_2_to_3\00", align 1
@3 = private unnamed_addr constant [12 x i8] c"packets_out\00", align 1
@4 = private unnamed_addr constant [13 x i8] c"state_0_to_1\00", align 1
@5 = private unnamed_addr constant [13 x i8] c"state_1_to_2\00", align 1
@6 = private unnamed_addr constant [8 x i8] c"stage_0\00", align 1
@7 = private unnamed_addr constant [8 x i8] c"stage_1\00", align 1
@8 = private unnamed_addr constant [8 x i8] c"stage_2\00", align 1
@9 = private unnamed_addr constant [8 x i8] c"stage_3\00", align 1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #0

declare dso_local i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64) local_unnamed_addr #1

declare dso_local i64 @nanotube_packet_write_masked(%struct.nanotube_packet*, i8*, i8*, i64, i64) local_unnamed_addr #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #0

; Function Attrs: uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #2 {
entry:
  %packet_in = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i64 0, i64 0), i64 65, i64 140)
  %packets_0_to_1 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([15 x i8], [15 x i8]* @0, i32 0, i32 0), i64 65, i64 140)
  %packets_1_to_2 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([15 x i8], [15 x i8]* @1, i32 0, i32 0), i64 65, i64 140)
  %packets_2_to_3 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([15 x i8], [15 x i8]* @2, i32 0, i32 0), i64 65, i64 140)
  %packets_out = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @3, i32 0, i32 0), i64 65, i64 140)
  call void @nanotube_channel_export(%struct.nanotube_channel* %packet_in, i32 1, i32 2)
  call void @nanotube_channel_export(%struct.nanotube_channel* %packets_out, i32 1, i32 1)
  %state_0_to_1 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([13 x i8], [13 x i8]* @4, i32 0, i32 0), i64 1, i64 10)
  %state_1_to_2 = call %struct.nanotube_channel* @nanotube_channel_create(i8* getelementptr inbounds ([13 x i8], [13 x i8]* @5, i32 0, i32 0), i64 5, i64 10)
  %context0 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context0, i32 0, %struct.nanotube_channel* %packet_in, i32 1)
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context0, i32 1, %struct.nanotube_channel* %packets_0_to_1, i32 2)
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context0, i32 3, %struct.nanotube_channel* %state_0_to_1, i32 2)
  %context1 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context1, i32 0, %struct.nanotube_channel* %packets_0_to_1, i32 1)
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context1, i32 1, %struct.nanotube_channel* %packets_1_to_2, i32 2)
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context1, i32 3, %struct.nanotube_channel* %state_1_to_2, i32 2)
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context1, i32 2, %struct.nanotube_channel* %state_0_to_1, i32 1)
  %context2 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context2, i32 0, %struct.nanotube_channel* %packets_1_to_2, i32 1)
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context2, i32 1, %struct.nanotube_channel* %packets_2_to_3, i32 2)
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context2, i32 2, %struct.nanotube_channel* %state_1_to_2, i32 1)
  %context3 = call %struct.nanotube_context* @nanotube_context_create()
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context3, i32 0, %struct.nanotube_channel* %packets_2_to_3, i32 1)
  call void @nanotube_context_add_channel(%struct.nanotube_context* %context3, i32 1, %struct.nanotube_channel* %packets_out, i32 2)
  call void @nanotube_thread_create(%struct.nanotube_context* %context0, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @6, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @simple_stage_0, i8* null, i64 0)
  call void @nanotube_thread_create(%struct.nanotube_context* %context1, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @7, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @simple_stage_1, i8* null, i64 0)
  call void @nanotube_thread_create(%struct.nanotube_context* %context2, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @8, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @simple_stage_2, i8* null, i64 0)
  call void @nanotube_thread_create(%struct.nanotube_context* %context3, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @9, i32 0, i32 0), void (%struct.nanotube_context*, i8*)* @simple_stage_3, i8* null, i64 0)
  ret void
}

declare dso_local void @nanotube_add_plain_packet_kernel(i8*, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32) local_unnamed_addr #1

declare void @nanotube_packet_drop(%struct.nanotube_packet*, i32)

define void @simple_stage_0(%struct.nanotube_context*, i8*) {
read_packet_word:
  %packet_word = alloca i8, i64 65
  %read_channel = call i32 @nanotube_channel_try_read(%struct.nanotube_context* %0, i32 0, i8* %packet_word, i64 65)
  %try_fail = icmp eq i32 %read_channel, 0
  br i1 %try_fail, label %thread_wait_exit, label %entry_post

thread_wait_exit:                                 ; preds = %read_packet_word
  call void @nanotube_thread_wait()
  ret void

entry_post:                                       ; preds = %read_packet_word
  %eop = call i1 @nanotube_tap_packet_is_eop_sb(i8* %packet_word, %struct.nanotube_tap_packet_eop_state* @packet_eop_tap_state_stage_0)
  br label %entry

entry:                                            ; preds = %entry_post
  %buffer_stage_0 = alloca [4 x i8], align 1, !nanotube.pipeline !2
  %mask_stage_0 = alloca i8, align 1
  %2 = getelementptr inbounds [4 x i8], [4 x i8]* %buffer_stage_0, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %2) #3
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %mask_stage_0) #3
  store i8 -1, i8* %mask_stage_0, align 1, !tbaa !3
  br label %stage_0_app_send_guard, !nanotube.pipeline !6

stage_0_app_send_guard:                           ; preds = %entry
  %stage_0sent_app_state = load i1, i1* @sent_app_state_stage_0
  %not_eop = xor i1 %eop, true
  store i1 %not_eop, i1* @sent_app_state_stage_0
  br i1 %stage_0sent_app_state, label %stage_0_epilogue, label %stage_0_app_epilogue

stage_0_app_epilogue:                             ; preds = %stage_0_app_send_guard
  %live_out_state = alloca <{ [1 x i8] }>
  %mask_stage_0_ptr = getelementptr <{ [1 x i8] }>, <{ [1 x i8] }>* %live_out_state, i32 0, i32 0
  %3 = bitcast [1 x i8]* %mask_stage_0_ptr to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %3, i8* %mask_stage_0, i64 1, i1 false)
  %4 = bitcast <{ [1 x i8] }>* %live_out_state to i8*
  call void @nanotube_channel_write(%struct.nanotube_context* %0, i32 3, i8* %4, i64 1)
  br label %stage_0_epilogue

stage_0_epilogue:                                 ; preds = %stage_0_app_epilogue, %stage_0_app_send_guard
  call void @nanotube_channel_write(%struct.nanotube_context* %0, i32 1, i8* %packet_word, i64 65)
  br label %exit

exit:                                             ; preds = %stage_0_epilogue
  ret void
}

define void @simple_stage_1(%struct.nanotube_context*, i8*) {
entry:
  %2 = load i1, i1* @have_app_state_stage_1
  %mask_stack_stage_1 = alloca i8
  %buffer_stage_1 = alloca [4 x i8], align 1
  %_stage_1 = getelementptr inbounds [4 x i8], [4 x i8]* %buffer_stage_1, i64 0, i64 0
  br i1 %2, label %read_packet_word, label %read_app_state

read_app_state:                                   ; preds = %entry
  %read_channel = call i32 @nanotube_channel_try_read(%struct.nanotube_context* %0, i32 2, i8* getelementptr inbounds (<{ [1 x i8] }>, <{ [1 x i8] }>* @app_state_stage_1, i32 0, i32 0, i32 0), i64 1)
  %try_fail = icmp eq i32 %read_channel, 0
  br i1 %try_fail, label %thread_wait_exit, label %read_app_state_post

thread_wait_exit:                                 ; preds = %read_packet_word, %read_app_state
  call void @nanotube_thread_wait()
  ret void

read_app_state_post:                              ; preds = %read_app_state
  store i1 true, i1* @have_app_state_stage_1
  br label %read_packet_word

read_packet_word:                                 ; preds = %read_app_state_post, %entry
  %packet_word = alloca i8, i64 65
  %read_channel1 = call i32 @nanotube_channel_try_read(%struct.nanotube_context* %0, i32 0, i8* %packet_word, i64 65)
  %try_fail2 = icmp eq i32 %read_channel1, 0
  br i1 %try_fail2, label %thread_wait_exit, label %entry_post_post

entry_post_post:                                  ; preds = %read_packet_word
  %eop = call i1 @nanotube_tap_packet_is_eop_sb(i8* %packet_word, %struct.nanotube_tap_packet_eop_state* @packet_eop_tap_state_stage_1)
  %in_packet = xor i1 %eop, true
  store i1 %in_packet, i1* @have_app_state_stage_1
  br label %unmarshal_stage_1

unmarshal_stage_1:                                ; preds = %entry_post_post
  %3 = bitcast [1 x i8]* getelementptr inbounds (<{ [1 x i8] }>, <{ [1 x i8] }>* @app_state_stage_1, i32 0, i32 0) to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %mask_stack_stage_1, i8* %3, i64 1, i1 false)
  br label %entry3.post.pre

entry3.post.pre:                                  ; preds = %unmarshal_stage_1
  %resp = alloca %struct.nanotube_tap_packet_read_resp, !nanotube.pipeline !2
  %req = alloca %struct.nanotube_tap_packet_read_req
  %req.valid.p = getelementptr inbounds %struct.nanotube_tap_packet_read_req, %struct.nanotube_tap_packet_read_req* %req, i32 0, i32 0
  %req.read_offset.p = getelementptr inbounds %struct.nanotube_tap_packet_read_req, %struct.nanotube_tap_packet_read_req* %req, i32 0, i32 1
  %req.read_length.p = getelementptr inbounds %struct.nanotube_tap_packet_read_req, %struct.nanotube_tap_packet_read_req* %req, i32 0, i32 2
  store i8 1, i8* %req.valid.p
  store i16 0, i16* %req.read_offset.p
  store i16 4, i16* %req.read_length.p
  call void @nanotube_tap_packet_read_sb(i16 4, i8 2, %struct.nanotube_tap_packet_read_resp* %resp, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @packet_read_data_stage_1, i32 0, i32 0), %struct.nanotube_tap_packet_read_state* @packet_read_tap_state_stage_1, i8* %packet_word, %struct.nanotube_tap_packet_read_req* %req)
  %resp.valid.p = getelementptr inbounds %struct.nanotube_tap_packet_read_resp, %struct.nanotube_tap_packet_read_resp* %resp, i32 0, i32 0
  %resp.valid.i8 = load i8, i8* %resp.valid.p
  %resp.valid = trunc i8 %resp.valid.i8 to i1
  br i1 %resp.valid, label %entry3.post, label %stage_1_epilogue

entry3.post:                                      ; preds = %entry3.post.pre
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %_stage_1, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @packet_read_data_stage_1, i32 0, i32 0), i64 4, i1 false)
  br label %stage_1_app_epilogue, !nanotube.pipeline !6

stage_1_app_epilogue:                             ; preds = %entry3.post
  %live_out_state = alloca <{ [4 x i8], [1 x i8] }>
  %buffer_stage_1_ptr = getelementptr <{ [4 x i8], [1 x i8] }>, <{ [4 x i8], [1 x i8] }>* %live_out_state, i32 0, i32 0
  %4 = bitcast [4 x i8]* %buffer_stage_1_ptr to i8*
  %5 = bitcast [4 x i8]* %buffer_stage_1 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %4, i8* %5, i64 4, i1 false)
  %mask_stack_stage_1_ptr = getelementptr <{ [4 x i8], [1 x i8] }>, <{ [4 x i8], [1 x i8] }>* %live_out_state, i32 0, i32 1
  %6 = bitcast [1 x i8]* %mask_stack_stage_1_ptr to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %6, i8* %mask_stack_stage_1, i64 1, i1 false)
  %7 = bitcast <{ [4 x i8], [1 x i8] }>* %live_out_state to i8*
  call void @nanotube_channel_write(%struct.nanotube_context* %0, i32 3, i8* %7, i64 5)
  br label %stage_1_epilogue

stage_1_epilogue:                                 ; preds = %entry3.post.pre, %stage_1_app_epilogue
  call void @nanotube_channel_write(%struct.nanotube_context* %0, i32 1, i8* %packet_word, i64 65)
  br label %exit

exit:                                             ; preds = %stage_1_epilogue
  ret void
}

define void @simple_stage_2(%struct.nanotube_context*, i8*) {
entry:
  %2 = load i1, i1* @have_app_state_stage_2
  %buffer_stack_stage_2 = alloca i8, i32 4
  %buffer_stage_2 = bitcast i8* %buffer_stack_stage_2 to [4 x i8]*
  %mask_stack_stage_2 = alloca i8
  %_stage_2 = getelementptr inbounds [4 x i8], [4 x i8]* %buffer_stage_2, i64 0, i64 0
  br i1 %2, label %read_packet_word, label %read_app_state

read_app_state:                                   ; preds = %entry
  %read_channel = call i32 @nanotube_channel_try_read(%struct.nanotube_context* %0, i32 2, i8* getelementptr inbounds (<{ [4 x i8], [1 x i8] }>, <{ [4 x i8], [1 x i8] }>* @app_state_stage_2, i32 0, i32 0, i32 0), i64 5)
  %try_fail = icmp eq i32 %read_channel, 0
  br i1 %try_fail, label %thread_wait_exit, label %read_app_state_post

thread_wait_exit:                                 ; preds = %read_packet_word, %read_app_state
  call void @nanotube_thread_wait()
  ret void

read_app_state_post:                              ; preds = %read_app_state
  store i1 true, i1* @have_app_state_stage_2
  br label %read_packet_word

read_packet_word:                                 ; preds = %read_app_state_post, %entry
  %packet_word = alloca i8, i64 65
  %read_channel1 = call i32 @nanotube_channel_try_read(%struct.nanotube_context* %0, i32 0, i8* %packet_word, i64 65)
  %try_fail2 = icmp eq i32 %read_channel1, 0
  br i1 %try_fail2, label %thread_wait_exit, label %entry_post_post

entry_post_post:                                  ; preds = %read_packet_word
  %eop = call i1 @nanotube_tap_packet_is_eop_sb(i8* %packet_word, %struct.nanotube_tap_packet_eop_state* @packet_eop_tap_state_stage_2)
  %in_packet = xor i1 %eop, true
  store i1 %in_packet, i1* @have_app_state_stage_2
  br label %unmarshal_stage_2

unmarshal_stage_2:                                ; preds = %entry_post_post
  %3 = bitcast [4 x i8]* getelementptr inbounds (<{ [4 x i8], [1 x i8] }>, <{ [4 x i8], [1 x i8] }>* @app_state_stage_2, i32 0, i32 0) to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %buffer_stack_stage_2, i8* %3, i64 4, i1 false)
  %4 = bitcast [1 x i8]* getelementptr inbounds (<{ [4 x i8], [1 x i8] }>, <{ [4 x i8], [1 x i8] }>* @app_state_stage_2, i32 0, i32 1) to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %mask_stack_stage_2, i8* %4, i64 1, i1 false)
  br label %entry3

entry3:                                           ; preds = %unmarshal_stage_2
  %packet_word.out = alloca i8, i64 65, !nanotube.pipeline !2
  %req = alloca %struct.nanotube_tap_packet_write_req
  %req.valid.p = getelementptr inbounds %struct.nanotube_tap_packet_write_req, %struct.nanotube_tap_packet_write_req* %req, i32 0, i32 0
  %req.write_offset.p = getelementptr inbounds %struct.nanotube_tap_packet_write_req, %struct.nanotube_tap_packet_write_req* %req, i32 0, i32 1
  %req.write_length.p = getelementptr inbounds %struct.nanotube_tap_packet_write_req, %struct.nanotube_tap_packet_write_req* %req, i32 0, i32 2
  store i8 1, i8* %req.valid.p
  store i16 0, i16* %req.write_offset.p
  store i16 4, i16* %req.write_length.p
  call void @nanotube_tap_packet_write_sb(i16 4, i8 2, i8* %packet_word.out, %struct.nanotube_tap_packet_write_state* @packet_write_tap_state_stage_2, i8* %packet_word, %struct.nanotube_tap_packet_write_req* %req, i8* %_stage_2, i8* %mask_stack_stage_2)
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %mask_stack_stage_2) #3
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %_stage_2) #3
  br label %stage_2_epilogue, !nanotube.pipeline !6

stage_2_epilogue:                                 ; preds = %entry3
  call void @nanotube_channel_write(%struct.nanotube_context* %0, i32 1, i8* %packet_word.out, i64 65)
  br label %exit

exit:                                             ; preds = %stage_2_epilogue
  ret void
}

define void @simple_stage_3(%struct.nanotube_context*, i8*) {
read_packet_word:
  %packet_word = alloca i8, i64 65
  %read_channel = call i32 @nanotube_channel_try_read(%struct.nanotube_context* %0, i32 0, i8* %packet_word, i64 65)
  %try_fail = icmp eq i32 %read_channel, 0
  br i1 %try_fail, label %thread_wait_exit, label %entry_post

thread_wait_exit:                                 ; preds = %read_packet_word
  call void @nanotube_thread_wait()
  ret void

entry_post:                                       ; preds = %read_packet_word
  %eop = call i1 @nanotube_tap_packet_is_eop_sb(i8* %packet_word, %struct.nanotube_tap_packet_eop_state* @packet_eop_tap_state_stage_3)
  br label %entry

entry:                                            ; preds = %entry_post
  br label %stage_3_epilogue, !nanotube.pipeline !6

stage_3_epilogue:                                 ; preds = %entry
  br i1 false, label %stage_3_epilogue_post, label %cond_packet_word_write

cond_packet_word_write:                           ; preds = %stage_3_epilogue
  call void @nanotube_channel_write(%struct.nanotube_context* %0, i32 1, i8* %packet_word, i64 65)
  br label %stage_3_epilogue_post

stage_3_epilogue_post:                            ; preds = %cond_packet_word_write, %stage_3_epilogue
  br label %exit

exit:                                             ; preds = %stage_3_epilogue_post
  ret void
}

declare i32 @nanotube_channel_try_read(%struct.nanotube_context*, i32, i8*, i64)

declare void @nanotube_thread_wait()

declare i1 @nanotube_tap_packet_is_eop_sb(i8*, %struct.nanotube_tap_packet_eop_state*)

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #0

declare void @nanotube_channel_write(%struct.nanotube_context*, i32, i8*, i64)

declare void @nanotube_tap_packet_read_sb(i16, i8, %struct.nanotube_tap_packet_read_resp*, i8*, %struct.nanotube_tap_packet_read_state*, i8*, %struct.nanotube_tap_packet_read_req*)

declare void @nanotube_tap_packet_write_sb(i16, i8, i8*, %struct.nanotube_tap_packet_write_state*, i8*, %struct.nanotube_tap_packet_write_req*, i8*, i8*)

declare %struct.nanotube_channel* @nanotube_channel_create(i8*, i64, i64)

declare void @nanotube_channel_export(%struct.nanotube_channel*, i32, i32)

declare %struct.nanotube_context* @nanotube_context_create()

declare void @nanotube_context_add_channel(%struct.nanotube_context*, i32, %struct.nanotube_channel*, i32)

declare void @nanotube_thread_create(%struct.nanotube_context*, i8*, void (%struct.nanotube_context*, i8*)*, i8*, i64)

attributes #0 = { argmemonly nounwind }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
!2 = !{!"app_entry"}
!3 = !{!4, !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C++ TBAA"}
!6 = !{!"app_exit"}
