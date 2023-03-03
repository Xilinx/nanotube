;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'rfc781demo.ebpf2nanotube.mem2req.lower.inline.platform.bc'
source_filename = "llvm-link"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.bpf_elf_map = type { i32, i32, i32, i32, i32, i32, i32 }
%struct.headers = type { %struct.pseudo_hdr_t, %struct.eth_mac_t, [2 x %struct.vlan_t], %struct.ipv4_t, %struct.ipv4_rfc781_hdr_t, %struct.ipv4_rfc781_t, %struct.ipv4_rfc781_t, %struct.ipv4_rfc781_t, %struct.ipv4_rfc781_t }
%struct.pseudo_hdr_t = type { [28 x i8], i8 }
%struct.eth_mac_t = type { i64, i64, i16, i8 }
%struct.vlan_t = type { i8, i8, i16, i16, i8 }
%struct.ipv4_t = type { i8, i8, i8, i16, i16, i8, i16, i8, i8, i16, i32, i32, i8 }
%struct.ipv4_rfc781_hdr_t = type { i8, i8, i8, i8, i8, i8 }
%struct.ipv4_rfc781_t = type { i32, i32, i8 }
%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque
%struct.xdp_output = type { i32, i32 }
%struct.MyProcessing_IPV4_CNT_key = type { i8 }
%struct.MyProcessing_MATCH_RFC781_key = type { i32 }

@perf_event = dso_local global %struct.bpf_elf_map { i32 4, i32 4, i32 4, i32 2, i32 0, i32 0, i32 1 }, section "maps", align 4
@hdr = dso_local local_unnamed_addr global %struct.headers zeroinitializer, align 8
@llvm.used = appending global [1 x i8*] [i8* bitcast (%struct.bpf_elf_map* @perf_event to i8*)], section "llvm.metadata"
@MyProcessing_BLACKLIST_RFC781_id = internal constant i16 0
@MyProcessing_BLACKLIST_RFC781_defaultAction_id = internal constant i16 1
@MyProcessing_DROP_CNT_id = internal constant i16 2
@MyProcessing_DROP_CNT_defaultAction_id = internal constant i16 3
@MyProcessing_IPV4_CNT_id = internal constant i16 4
@MyProcessing_IPV4_CNT_defaultAction_id = internal constant i16 5
@MyProcessing_MATCH_RFC781_id = internal constant i16 6
@MyProcessing_MATCH_RFC781_defaultAction_id = internal constant i16 7
@MyProcessing_TSTAMP_MOD_CNT_id = internal constant i16 8
@MyProcessing_TSTAMP_MOD_CNT_defaultAction_id = internal constant i16 9
@ebpf_outTable_id = internal constant i16 10
@0 = private unnamed_addr constant [12 x i8] c"ebpf_filter\00", align 1
@onemask1 = internal constant [1 x i8] c"\FF"

define void @nanotube_setup() {
  %1 = call i8* @nanotube_map_create(i16 0, i32 0, i64 4, i64 4)
  %2 = call i8* @nanotube_map_create(i16 1, i32 2, i64 4, i64 4)
  %3 = call i8* @nanotube_map_create(i16 2, i32 2, i64 1, i64 4)
  %4 = call i8* @nanotube_map_create(i16 3, i32 2, i64 4, i64 4)
  %5 = call i8* @nanotube_map_create(i16 4, i32 2, i64 1, i64 4)
  %6 = call i8* @nanotube_map_create(i16 5, i32 2, i64 4, i64 4)
  %7 = call i8* @nanotube_map_create(i16 6, i32 0, i64 4, i64 4)
  %8 = call i8* @nanotube_map_create(i16 7, i32 2, i64 4, i64 4)
  %9 = call i8* @nanotube_map_create(i16 8, i32 2, i64 1, i64 4)
  %10 = call i8* @nanotube_map_create(i16 9, i32 2, i64 4, i64 4)
  %11 = call i8* @nanotube_map_create(i16 10, i32 2, i64 4, i64 4)
  call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @0, i32 0, i32 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* @ebpf_filter_nt.1, i32 0, i32 1)
  ret void
}

declare i8* @nanotube_map_create(i16, i32, i64, i64)

declare void @nanotube_add_plain_packet_kernel(i8*, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32)

declare i64 @nanotube_packet_bounded_length(%struct.nanotube_packet*, i64)

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #0

declare i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64)

; Function Attrs: nounwind readnone speculatable
declare i16 @llvm.bswap.i16(i16) #1

; Function Attrs: nounwind readnone speculatable
declare i32 @llvm.bswap.i32(i32) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #0

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #0

; Function Attrs: alwaysinline uwtable
define dso_local i32 @map_op_adapter(%struct.nanotube_context* %ctx, i16 zeroext %map_id, i32 %type, i8* %key, i64 %key_length, i8* %data_in, i8* %data_out, i8* %mask, i64 %offset, i64 %data_length) #2 !dbg !313 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %ctx, metadata !329, metadata !DIExpression()), !dbg !340
  call void @llvm.dbg.value(metadata i16 %map_id, metadata !330, metadata !DIExpression()), !dbg !341
  call void @llvm.dbg.value(metadata i32 %type, metadata !331, metadata !DIExpression()), !dbg !342
  call void @llvm.dbg.value(metadata i8* %key, metadata !332, metadata !DIExpression()), !dbg !343
  call void @llvm.dbg.value(metadata i64 %key_length, metadata !333, metadata !DIExpression()), !dbg !344
  call void @llvm.dbg.value(metadata i8* %data_in, metadata !334, metadata !DIExpression()), !dbg !345
  call void @llvm.dbg.value(metadata i8* %data_out, metadata !335, metadata !DIExpression()), !dbg !346
  call void @llvm.dbg.value(metadata i8* %mask, metadata !336, metadata !DIExpression()), !dbg !347
  call void @llvm.dbg.value(metadata i64 %offset, metadata !337, metadata !DIExpression()), !dbg !348
  call void @llvm.dbg.value(metadata i64 %data_length, metadata !338, metadata !DIExpression()), !dbg !349
  %call = tail call i64 @nanotube_map_op(%struct.nanotube_context* %ctx, i16 zeroext %map_id, i32 %type, i8* %key, i64 %key_length, i8* %data_in, i8* %data_out, i8* %mask, i64 %offset, i64 %data_length), !dbg !350
  call void @llvm.dbg.value(metadata i64 %call, metadata !339, metadata !DIExpression()), !dbg !351
  %cmp = icmp eq i64 %call, %data_length, !dbg !352
  %. = select i1 %cmp, i32 0, i32 -22, !dbg !354
  ret i32 %., !dbg !355
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

declare dso_local i64 @nanotube_map_op(%struct.nanotube_context*, i16 zeroext, i32, i8*, i64, i8*, i8*, i8*, i64, i64) local_unnamed_addr #3

; Function Attrs: alwaysinline uwtable
define dso_local i32 @packet_adjust_head_adapter(%struct.nanotube_packet* %p, i32 %offset) #2 !dbg !356 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %p, metadata !363, metadata !DIExpression()), !dbg !366
  call void @llvm.dbg.value(metadata i32 %offset, metadata !364, metadata !DIExpression()), !dbg !367
  %call = tail call i32 @nanotube_packet_resize(%struct.nanotube_packet* %p, i64 0, i32 %offset), !dbg !368
  call void @llvm.dbg.value(metadata i32 %call, metadata !365, metadata !DIExpression()), !dbg !369
  %tobool = icmp eq i32 %call, 0, !dbg !370
  %cond = sext i1 %tobool to i32, !dbg !370
  ret i32 %cond, !dbg !371
}

declare dso_local i32 @nanotube_packet_resize(%struct.nanotube_packet*, i64, i32) local_unnamed_addr #3

; Function Attrs: alwaysinline uwtable
define dso_local void @packet_handle_xdp_result(%struct.nanotube_packet* %packet, i32 %xdp_ret) #2 !dbg !372 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !376, metadata !DIExpression()), !dbg !378
  call void @llvm.dbg.value(metadata i32 %xdp_ret, metadata !377, metadata !DIExpression()), !dbg !379
  %cond = icmp eq i32 %xdp_ret, 2, !dbg !380
  br i1 %cond, label %sw.bb, label %sw.default, !dbg !380

sw.bb:                                            ; preds = %entry
  tail call void @nanotube_packet_set_port(%struct.nanotube_packet* %packet, i8 zeroext 0), !dbg !381
  br label %sw.epilog, !dbg !383

sw.default:                                       ; preds = %entry
  tail call void @nanotube_packet_set_port(%struct.nanotube_packet* %packet, i8 zeroext -1), !dbg !384
  br label %sw.epilog, !dbg !385

sw.epilog:                                        ; preds = %sw.default, %sw.bb
  ret void, !dbg !386
}

declare dso_local void @nanotube_packet_set_port(%struct.nanotube_packet*, i8 zeroext) local_unnamed_addr #3

; Function Attrs: alwaysinline uwtable
define dso_local i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %data_in, i64 %offset, i64 %length) #2 !dbg !387 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %data_in, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %offset, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 %length, metadata !394, metadata !DIExpression()), !dbg !400
  %add = add i64 %length, 7, !dbg !401
  %div = lshr i64 %add, 3, !dbg !402
  call void @llvm.dbg.value(metadata i64 %div, metadata !395, metadata !DIExpression()), !dbg !403
  %0 = alloca i8, i64 %div, align 16, !dbg !404
  call void @llvm.dbg.value(metadata i8* %0, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %0, i8 -1, i64 %div, i1 false), !dbg !406
  %call = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %data_in, i8* nonnull %0, i64 %offset, i64 %length), !dbg !407
  ret i64 %call, !dbg !408
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1) #0

declare dso_local i64 @nanotube_packet_write_masked(%struct.nanotube_packet*, i8*, i8*, i64, i64) local_unnamed_addr #3

; Function Attrs: alwaysinline norecurse nounwind uwtable
define dso_local void @nanotube_merge_data_mask(i8* nocapture %inout_data, i8* nocapture %inout_mask, i8* nocapture readonly %in_data, i8* nocapture readonly %in_mask, i64 %offset, i64 %data_length) local_unnamed_addr #4 !dbg !409 {
entry:
  call void @llvm.dbg.value(metadata i8* %inout_data, metadata !413, metadata !DIExpression()), !dbg !432
  call void @llvm.dbg.value(metadata i8* %inout_mask, metadata !414, metadata !DIExpression()), !dbg !433
  call void @llvm.dbg.value(metadata i8* %in_data, metadata !415, metadata !DIExpression()), !dbg !434
  call void @llvm.dbg.value(metadata i8* %in_mask, metadata !416, metadata !DIExpression()), !dbg !435
  call void @llvm.dbg.value(metadata i64 %offset, metadata !417, metadata !DIExpression()), !dbg !436
  call void @llvm.dbg.value(metadata i64 %data_length, metadata !418, metadata !DIExpression()), !dbg !437
  call void @llvm.dbg.value(metadata i64 0, metadata !419, metadata !DIExpression()), !dbg !438
  %cmp25 = icmp eq i64 %data_length, 0, !dbg !439
  br i1 %cmp25, label %for.cond.cleanup, label %for.body, !dbg !440

for.cond.cleanup:                                 ; preds = %if.end, %entry
  ret void, !dbg !441

for.body:                                         ; preds = %entry, %if.end
  %i.026 = phi i64 [ %inc, %if.end ], [ 0, %entry ]
  call void @llvm.dbg.value(metadata i64 %i.026, metadata !419, metadata !DIExpression()), !dbg !438
  %div = lshr i64 %i.026, 3, !dbg !442
  call void @llvm.dbg.value(metadata i64 %div, metadata !421, metadata !DIExpression()), !dbg !443
  %0 = trunc i64 %i.026 to i32, !dbg !444
  %conv = and i32 %0, 7, !dbg !444
  %add = add i64 %i.026, %offset, !dbg !445
  call void @llvm.dbg.value(metadata i64 %add, metadata !425, metadata !DIExpression()), !dbg !446
  %arrayidx = getelementptr inbounds i8, i8* %in_mask, i64 %div, !dbg !447
  %1 = load i8, i8* %arrayidx, align 1, !dbg !447, !tbaa !448
  %conv1 = zext i8 %1 to i32, !dbg !447
  %shl = shl i32 1, %conv, !dbg !451
  %and = and i32 %shl, %conv1, !dbg !452
  %tobool = icmp eq i32 %and, 0, !dbg !447
  br i1 %tobool, label %if.end, label %if.then, !dbg !453

if.then:                                          ; preds = %for.body
  %arrayidx4 = getelementptr inbounds i8, i8* %in_data, i64 %i.026, !dbg !454
  %2 = load i8, i8* %arrayidx4, align 1, !dbg !454, !tbaa !448
  %arrayidx5 = getelementptr inbounds i8, i8* %inout_data, i64 %add, !dbg !455
  store i8 %2, i8* %arrayidx5, align 1, !dbg !456, !tbaa !448
  %div6 = lshr i64 %add, 3, !dbg !457
  call void @llvm.dbg.value(metadata i64 %div6, metadata !428, metadata !DIExpression()), !dbg !458
  %3 = trunc i64 %add to i32, !dbg !459
  %conv8 = and i32 %3, 7, !dbg !459
  call void @llvm.dbg.value(metadata i32 %conv8, metadata !431, metadata !DIExpression()), !dbg !460
  %shl10 = shl i32 1, %conv8, !dbg !461
  %arrayidx11 = getelementptr inbounds i8, i8* %inout_mask, i64 %div6, !dbg !462
  %4 = load i8, i8* %arrayidx11, align 1, !dbg !463, !tbaa !448
  %5 = trunc i32 %shl10 to i8, !dbg !463
  %conv13 = or i8 %4, %5, !dbg !463
  store i8 %conv13, i8* %arrayidx11, align 1, !dbg !463, !tbaa !448
  br label %if.end, !dbg !464

if.end:                                           ; preds = %for.body, %if.then
  %inc = add nuw i64 %i.026, 1, !dbg !465
  call void @llvm.dbg.value(metadata i64 %inc, metadata !419, metadata !DIExpression()), !dbg !438
  %exitcond = icmp eq i64 %inc, %data_length, !dbg !439
  br i1 %exitcond, label %for.cond.cleanup, label %for.body, !dbg !440, !llvm.loop !466
}

; Function Attrs: alwaysinline uwtable
define dso_local i64 @nanotube_map_read(%struct.nanotube_context* %ctx, i16 zeroext %id, i8* %key, i64 %key_length, i8* %data_out, i64 %offset, i64 %data_length) #2 !dbg !468 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 %id, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %key, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 %key_length, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %data_out, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 %offset, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 %data_length, metadata !478, metadata !DIExpression()), !dbg !485
  %call = tail call i64 @nanotube_map_op(%struct.nanotube_context* %ctx, i16 zeroext %id, i32 0, i8* %key, i64 %key_length, i8* null, i8* %data_out, i8* null, i64 %offset, i64 %data_length), !dbg !486
  ret i64 %call, !dbg !487
}

; Function Attrs: alwaysinline uwtable
define dso_local i64 @nanotube_map_write(%struct.nanotube_context* %ctx, i16 zeroext %id, i8* %key, i64 %key_length, i8* %data_in, i64 %offset, i64 %data_length) local_unnamed_addr #2 !dbg !488 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %ctx, metadata !492, metadata !DIExpression()), !dbg !501
  call void @llvm.dbg.value(metadata i16 %id, metadata !493, metadata !DIExpression()), !dbg !502
  call void @llvm.dbg.value(metadata i8* %key, metadata !494, metadata !DIExpression()), !dbg !503
  call void @llvm.dbg.value(metadata i64 %key_length, metadata !495, metadata !DIExpression()), !dbg !504
  call void @llvm.dbg.value(metadata i8* %data_in, metadata !496, metadata !DIExpression()), !dbg !505
  call void @llvm.dbg.value(metadata i64 %offset, metadata !497, metadata !DIExpression()), !dbg !506
  call void @llvm.dbg.value(metadata i64 %data_length, metadata !498, metadata !DIExpression()), !dbg !507
  %add = add i64 %data_length, 7, !dbg !508
  %div = lshr i64 %add, 3, !dbg !509
  call void @llvm.dbg.value(metadata i64 %div, metadata !499, metadata !DIExpression()), !dbg !510
  %0 = alloca i8, i64 %div, align 16, !dbg !511
  call void @llvm.dbg.value(metadata i8* %0, metadata !500, metadata !DIExpression()), !dbg !512
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %0, i8 -1, i64 %div, i1 false), !dbg !513
  %call = call i64 @nanotube_map_op(%struct.nanotube_context* %ctx, i16 zeroext %id, i32 3, i8* %key, i64 %key_length, i8* %data_in, i8* null, i8* nonnull %0, i64 %offset, i64 %data_length), !dbg !514
  ret i64 %call, !dbg !515
}

; Function Attrs: nounwind
declare i8* @llvm.stacksave() #5

; Function Attrs: nounwind
declare void @llvm.stackrestore(i8*) #5

; Function Attrs: nounwind uwtable
define private i32 @ebpf_filter_nt.1(%struct.nanotube_context* %nt_ctx, %struct.nanotube_packet* %packet) #6 section "prog" {
entry:
  %0 = alloca i8, i64 1, align 16, !dbg !516
  %1 = alloca i8, i64 1, align 16, !dbg !518
  %2 = alloca i8, i64 1, align 16, !dbg !404
  %3 = alloca i8, i64 1, align 16, !dbg !404
  %4 = alloca i8, i64 1, align 16, !dbg !404
  %5 = alloca i8, i64 1, align 16, !dbg !404
  %6 = alloca i8, i64 1, align 16, !dbg !404
  %7 = alloca i8, i64 1, align 16, !dbg !404
  %8 = alloca i8, i64 1, align 16, !dbg !404
  %9 = alloca i8, i64 1, align 16, !dbg !404
  %10 = alloca i8, i64 1, align 16, !dbg !404
  %11 = alloca i8, i64 1, align 16, !dbg !404
  %12 = alloca i8, i64 1, align 16, !dbg !404
  %13 = alloca i8, i64 1, align 16, !dbg !404
  %14 = alloca i8, i64 1, align 16, !dbg !404
  %15 = alloca i8, i64 1, align 16, !dbg !404
  %16 = alloca i8, i64 1, align 16, !dbg !404
  %17 = alloca i8, i64 1, align 16, !dbg !404
  %18 = alloca i8, i64 1, align 16, !dbg !404
  %19 = alloca i8, i64 1, align 16, !dbg !404
  %20 = alloca i8, i64 1, align 16, !dbg !404
  %21 = alloca i8, i64 1, align 16, !dbg !404
  %22 = alloca i8, i64 1, align 16, !dbg !404
  %23 = alloca i8, i64 1, align 16, !dbg !404
  %24 = alloca i8, i64 1, align 16, !dbg !404
  %25 = alloca i8, i64 1, align 16, !dbg !404
  %26 = alloca i8, i64 1, align 16, !dbg !404
  %27 = alloca i8, i64 1, align 16, !dbg !404
  %28 = alloca i8, i64 1, align 16, !dbg !404
  %29 = alloca i8, i64 1, align 16, !dbg !404
  %30 = alloca i8, i64 1, align 16, !dbg !404
  %31 = alloca i8, i64 1, align 16, !dbg !404
  %32 = alloca i8, i64 1, align 16, !dbg !404
  %33 = alloca i8, i64 1, align 16, !dbg !404
  %34 = alloca i8, i64 1, align 16, !dbg !404
  %35 = alloca i8, i64 1, align 16, !dbg !404
  %36 = alloca i8, i64 1, align 16, !dbg !404
  %37 = alloca i8, i64 1, align 16, !dbg !404
  %38 = alloca i8, i64 1, align 16, !dbg !404
  %39 = alloca i8, i64 1, align 16, !dbg !404
  %40 = alloca i8, i64 1, align 16, !dbg !404
  %41 = alloca i8, i64 1, align 16, !dbg !404
  %42 = alloca i8, i64 1, align 16, !dbg !404
  %43 = alloca i8, i64 1, align 16, !dbg !404
  %44 = alloca i8, i64 1, align 16, !dbg !404
  %45 = alloca i8, i64 1, align 16, !dbg !404
  %46 = alloca i8, i64 1, align 16, !dbg !404
  %47 = alloca i8, i64 1, align 16, !dbg !404
  %48 = alloca i8, i64 1, align 16, !dbg !404
  %49 = alloca i8, i64 1, align 16, !dbg !404
  %50 = alloca i8, i64 1, align 16, !dbg !404
  %51 = alloca i8, i64 1, align 16, !dbg !404
  %52 = alloca i8, i64 1, align 16, !dbg !404
  %53 = alloca i8, i64 1, align 16, !dbg !404
  %54 = alloca i8, i64 1, align 16, !dbg !404
  %55 = alloca i8, i64 1, align 16, !dbg !404
  %56 = alloca i8, i64 1, align 16, !dbg !404
  %57 = alloca i8, i64 1, align 16, !dbg !404
  %58 = alloca i8, i64 1, align 16, !dbg !404
  %59 = alloca i8, i64 1, align 16, !dbg !404
  %60 = alloca i8, i64 1, align 16, !dbg !404
  %61 = alloca i8, i64 1, align 16, !dbg !404
  %62 = alloca i8, i64 1, align 16, !dbg !404
  %63 = alloca i8, i64 1, align 16, !dbg !404
  %64 = alloca i8, i64 1, align 16, !dbg !404
  %65 = alloca i8, i64 1, align 16, !dbg !404
  %66 = alloca i8, i64 1, align 16, !dbg !404
  %67 = alloca i8, i64 1, align 16, !dbg !404
  %68 = alloca i8, i64 1, align 16, !dbg !404
  %69 = alloca i8, i64 1, align 16, !dbg !404
  %70 = alloca i8, i64 1, align 16, !dbg !404
  %71 = alloca i8, i64 1, align 16, !dbg !404
  %72 = alloca i8, i64 1, align 16, !dbg !404
  %73 = alloca i8, i64 1, align 16, !dbg !404
  %74 = alloca i8, i64 1, align 16, !dbg !404
  %75 = alloca i8, i64 1, align 16, !dbg !404
  %76 = alloca i8, i64 1, align 16, !dbg !404
  %77 = alloca i8, i64 1, align 16, !dbg !404
  %78 = alloca i8, i64 1, align 16, !dbg !404
  %79 = alloca i8, i64 1, align 16, !dbg !404
  %80 = alloca i8, i64 1, align 16, !dbg !404
  %81 = alloca i8, i64 1, align 16, !dbg !404
  %82 = alloca i8, i64 1, align 16, !dbg !404
  %83 = alloca i8, i64 1, align 16, !dbg !404
  %84 = alloca i8, i64 1, align 16, !dbg !404
  %85 = alloca i8, i64 1, align 16, !dbg !404
  %86 = alloca i8, i64 1, align 16, !dbg !404
  %87 = alloca i8, i64 1, align 16, !dbg !404
  %88 = alloca i8, i64 1, align 16, !dbg !404
  %89 = alloca i8, i64 1, align 16, !dbg !404
  %90 = alloca i8, i64 1, align 16, !dbg !404
  %91 = alloca i8, i64 1, align 16, !dbg !404
  %92 = alloca i8, i64 1, align 16, !dbg !404
  %93 = alloca i8, i64 1, align 16, !dbg !404
  %94 = alloca i8, i64 1, align 16, !dbg !404
  %95 = alloca i8, i64 1, align 16, !dbg !404
  %96 = alloca i8, i64 1, align 16, !dbg !404
  %97 = alloca i8, i64 1, align 16, !dbg !404
  %98 = alloca i8, i64 1, align 16, !dbg !404
  %99 = alloca i8, i64 1, align 16, !dbg !404
  %100 = alloca i8, i64 1, align 16, !dbg !404
  %101 = alloca i8, i64 1, align 16, !dbg !404
  %102 = alloca i8, i64 1, align 16, !dbg !404
  %103 = alloca i8, i64 1, align 16, !dbg !404
  %104 = alloca i8, i64 1, align 16, !dbg !404
  %105 = alloca i8, i64 1, align 16, !dbg !404
  %106 = alloca i8, i64 1, align 16, !dbg !404
  %107 = alloca i8, i64 1, align 16, !dbg !404
  %108 = alloca i8, i64 1, align 16, !dbg !404
  %109 = alloca i8, i64 1, align 16, !dbg !404
  %110 = alloca i8, i64 1, align 16, !dbg !404
  %111 = alloca i8, i64 1, align 16, !dbg !404
  %112 = alloca i8, i64 1, align 16, !dbg !404
  %113 = alloca i8, i64 1, align 16, !dbg !404
  %114 = alloca i8, i64 1, align 16, !dbg !404
  %_buffer221 = alloca i8
  %_buffer220 = alloca i8
  %_buffer219 = alloca i8
  %_buffer218 = alloca i8
  %_buffer217 = alloca i8
  %_buffer216 = alloca i8
  %_buffer215 = alloca i8
  %_buffer214 = alloca i8
  %_buffer213 = alloca i8
  %_buffer212 = alloca i8
  %_buffer211 = alloca i8
  %_buffer210 = alloca i8
  %_buffer209 = alloca i8
  %_buffer208 = alloca i32
  %115 = bitcast i32* %_buffer208 to i8*
  %ebpf_zero.c_copy205 = alloca i8, i64 4
  %dummy_rd_data206 = alloca i8
  %_buffer204 = alloca i32
  %116 = bitcast i32* %_buffer204 to i8*
  %key1416.c_copy = alloca i8, i64 4
  %dummy_rd_data202 = alloca i8
  %_buffer201 = alloca i8
  %_buffer200 = alloca i8
  %_buffer199 = alloca i16
  %117 = bitcast i16* %_buffer199 to i8*
  %_buffer198 = alloca i16
  %118 = bitcast i16* %_buffer198 to i8*
  %_buffer197 = alloca i8
  %_buffer196 = alloca i8
  %_buffer195 = alloca i32
  %119 = bitcast i32* %_buffer195 to i8*
  %_buffer194 = alloca i64
  %120 = bitcast i64* %_buffer194 to i8*
  %_buffer193 = alloca i8
  %_buffer192 = alloca i8
  %_buffer191 = alloca i16
  %121 = bitcast i16* %_buffer191 to i8*
  %_buffer190 = alloca i8
  %_buffer189 = alloca i8
  %_buffer188 = alloca i16
  %122 = bitcast i16* %_buffer188 to i8*
  %_buffer187 = alloca i8
  %_buffer186 = alloca i8
  %_buffer185 = alloca i8
  %_buffer184 = alloca i8
  %_buffer183 = alloca i8
  %_buffer182 = alloca i8
  %_buffer181 = alloca i8
  %_buffer180 = alloca i8
  %_buffer179 = alloca i8
  %_buffer178 = alloca i8
  %_buffer177 = alloca i32
  %123 = bitcast i32* %_buffer177 to i8*
  %_buffer176 = alloca i8
  %_buffer175 = alloca i8
  %_buffer174 = alloca i8
  %_buffer173 = alloca i8
  %_buffer172 = alloca i8
  %_buffer171 = alloca i8
  %_buffer170 = alloca i8
  %_buffer169 = alloca i8
  %_buffer168 = alloca i8
  %_buffer167 = alloca i8
  %_buffer166 = alloca i8
  %_buffer165 = alloca i8
  %_buffer164 = alloca i32
  %124 = bitcast i32* %_buffer164 to i8*
  %_buffer163 = alloca i32
  %125 = bitcast i32* %_buffer163 to i8*
  %_buffer162 = alloca i8
  %_buffer161 = alloca i16
  %126 = bitcast i16* %_buffer161 to i8*
  %_buffer160 = alloca i8
  %_buffer159 = alloca i16
  %127 = bitcast i16* %_buffer159 to i8*
  %_buffer158 = alloca i64
  %128 = bitcast i64* %_buffer158 to i8*
  %_buffer157 = alloca i16
  %129 = bitcast i16* %_buffer157 to i8*
  %_buffer156 = alloca i8
  %_buffer155 = alloca i32
  %130 = bitcast i32* %_buffer155 to i8*
  %_buffer154 = alloca i8
  %_buffer153 = alloca i8
  %_buffer152 = alloca i16
  %131 = bitcast i16* %_buffer152 to i8*
  %_buffer151 = alloca i32
  %132 = bitcast i32* %_buffer151 to i8*
  %_buffer150 = alloca i32
  %133 = bitcast i32* %_buffer150 to i8*
  %_buffer149 = alloca i32
  %134 = bitcast i32* %_buffer149 to i8*
  %_buffer148 = alloca i32
  %135 = bitcast i32* %_buffer148 to i8*
  %_buffer147 = alloca i8
  %_buffer146 = alloca i8
  %_buffer145 = alloca i8
  %_buffer144 = alloca i8
  %_buffer143 = alloca i32
  %136 = bitcast i32* %_buffer143 to i8*
  %_buffer142 = alloca i8
  %_buffer141 = alloca i8
  %_buffer140 = alloca i16
  %137 = bitcast i16* %_buffer140 to i8*
  %_buffer139 = alloca i8
  %_buffer138 = alloca i8
  %_buffer137 = alloca i8
  %_buffer136 = alloca i8
  %_buffer135 = alloca i8
  %_buffer134 = alloca i8
  %_buffer133 = alloca i8
  %_buffer132 = alloca i8
  %_buffer131 = alloca i8
  %_buffer130 = alloca i8
  %_buffer129 = alloca i8
  %_buffer128 = alloca i8
  %_buffer127 = alloca i8
  %_buffer126 = alloca i8
  %_buffer125 = alloca i8
  %_buffer124 = alloca i8
  %_buffer123 = alloca i8
  %_buffer122 = alloca i8
  %_buffer121 = alloca i8
  %_buffer120 = alloca i8
  %_buffer119 = alloca i8
  %_buffer118 = alloca i8
  %_buffer117 = alloca i8
  %_buffer116 = alloca i8
  %_buffer115 = alloca i8
  %_buffer114 = alloca i8
  %_buffer113 = alloca i8
  %_buffer112 = alloca i8
  %_buffer111 = alloca i8
  %_buffer110 = alloca i8
  %_buffer109 = alloca i8
  %_buffer108 = alloca i8
  %_buffer107 = alloca i8
  %_buffer106 = alloca i8
  %_buffer105 = alloca i8
  %_buffer104 = alloca i8
  %_buffer103 = alloca i8
  %_buffer102 = alloca i8
  %_buffer101 = alloca i8
  %_buffer100 = alloca i8
  %_buffer99 = alloca i8
  %_buffer98 = alloca i8
  %_buffer97 = alloca i8
  %_buffer96 = alloca i8
  %_buffer95 = alloca i8
  %_buffer94 = alloca i8
  %_buffer93 = alloca i8
  %_buffer92 = alloca i8
  %_buffer91 = alloca i8
  %_buffer90 = alloca i8
  %_buffer89 = alloca i8
  %_buffer88 = alloca i8
  %_buffer87 = alloca i8
  %_buffer86 = alloca i8
  %_buffer85 = alloca i8
  %_buffer84 = alloca i8
  %_buffer83 = alloca i8
  %ebpf_zero.c_copy80 = alloca i8, i64 4
  %dummy_rd_data81 = alloca i8
  %_buffer79 = alloca i8
  %_buffer78 = alloca i8
  %_buffer77 = alloca i8
  %_buffer76 = alloca i32
  %138 = bitcast i32* %_buffer76 to i8*
  %ebpf_zero.c_copy73 = alloca i8, i64 4
  %dummy_rd_data74 = alloca i8
  %_buffer72 = alloca i8
  %_buffer71 = alloca i8
  %_buffer70 = alloca i8
  %_buffer69 = alloca i8
  %key1449.gep_copy = alloca i8, i64 1
  %dummy_rd_data67 = alloca i8
  %_buffer66 = alloca i8
  %_buffer65 = alloca i8
  %key1475.gep_copy = alloca i8, i64 1
  %dummy_rd_data63 = alloca i8
  %_buffer62 = alloca i8
  %_buffer61 = alloca i8
  %_buffer60 = alloca i32
  %139 = bitcast i32* %_buffer60 to i8*
  %ebpf_zero.c_copy57 = alloca i8, i64 4
  %dummy_rd_data58 = alloca i8
  %_buffer56 = alloca i8
  %_buffer55 = alloca i8
  %_buffer54 = alloca i8
  %_buffer53 = alloca i8
  %_buffer52 = alloca i8
  %_buffer51 = alloca i32
  %140 = bitcast i32* %_buffer51 to i8*
  %key1385.c_copy = alloca i8, i64 4
  %dummy_rd_data49 = alloca i8
  %_buffer48 = alloca i8
  %_buffer47 = alloca i8
  %_buffer46 = alloca i8
  %_buffer45 = alloca i8
  %key.gep_copy = alloca i8, i64 1
  %dummy_rd_data43 = alloca i8
  %_buffer42 = alloca i8
  %_buffer41 = alloca i8
  %_buffer40 = alloca i8
  %_buffer39 = alloca i8
  %_buffer38 = alloca i8
  %ebpf_zero.c_copy = alloca i8, i64 4
  %dummy_rd_data = alloca i8
  %_buffer37 = alloca i8
  %_buffer36 = alloca i8
  %_buffer35 = alloca i8
  %_buffer34 = alloca i8
  %_buffer33 = alloca i8
  %_buffer32 = alloca i8
  %_buffer31 = alloca i8
  %_buffer30 = alloca i8
  %_buffer29 = alloca i8
  %_buffer28 = alloca i8
  %_buffer27 = alloca i8
  %_buffer26 = alloca i8
  %_buffer25 = alloca i8
  %_buffer24 = alloca i8
  %_buffer23 = alloca i8
  %_buffer22 = alloca i8
  %_buffer21 = alloca i8
  %_buffer20 = alloca i8
  %_buffer19 = alloca i8
  %_buffer18 = alloca i8
  %_buffer17 = alloca i8
  %_buffer16 = alloca i8
  %_buffer15 = alloca i8
  %_buffer14 = alloca i8
  %_buffer13 = alloca i8
  %_buffer12 = alloca i8
  %_buffer = alloca i8
  %ebpf_zero = alloca i32, align 4
  %xout = alloca %struct.xdp_output, align 4
  %key = alloca %struct.MyProcessing_IPV4_CNT_key, align 1
  %key1385 = alloca %struct.MyProcessing_MATCH_RFC781_key, align 4
  %key1416 = alloca %struct.MyProcessing_MATCH_RFC781_key, align 4
  %key1449 = alloca %struct.MyProcessing_IPV4_CNT_key, align 1
  %key1475 = alloca %struct.MyProcessing_IPV4_CNT_key, align 1
  %141 = call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 32768)
  %142 = add i64 %141, -1
  %143 = bitcast i32* %ebpf_zero to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %143) #5
  store i32 0, i32* %ebpf_zero, align 4, !tbaa !520
  %144 = bitcast %struct.xdp_output* %xout to i8*
  call void @llvm.lifetime.start.p0i8(i64 8, i8* nonnull %144) #5
  %145 = icmp ugt i64 28, %142
  br i1 %145, label %cleanup3166, label %if.end820

parse_ipv4:                                       ; preds = %if.end1025, %if.end701, %if.end582
  %ebpf_packetOffsetInBits.0 = phi i32 [ 368, %if.end582 ], [ 400, %if.end701 ], [ 336, %if.end1025 ]
  %add = add nuw nsw i32 %ebpf_packetOffsetInBits.0, 160
  %div = lshr exact i32 %add, 3
  %idx.ext = zext i32 %div to i64
  %146 = add i64 0, %idx.ext
  %147 = icmp ugt i64 %146, %142
  br i1 %147, label %cleanup3166, label %if.end

if.end:                                           ; preds = %parse_ipv4
  %div3 = lshr exact i32 %ebpf_packetOffsetInBits.0, 3
  %idx.ext4 = zext i32 %div3 to i64
  %148 = add i64 0, %idx.ext4
  %149 = add i64 %148, 1
  %150 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer169, i64 %149, i64 1)
  %151 = load i8, i8* %_buffer169
  %152 = lshr i8 %151, 4
  store i8 %152, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 0), align 8, !tbaa !524
  %add8 = or i32 %ebpf_packetOffsetInBits.0, 4
  %153 = add i64 %148, 1
  %154 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer168, i64 %153, i64 1)
  %155 = load i8, i8* %_buffer168
  %156 = and i8 %155, 15
  store i8 %156, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 1), align 1, !tbaa !533
  %add15 = add nuw nsw i32 %add8, 4
  %div16 = lshr exact i32 %add15, 3
  %idx.ext17 = zext i32 %div16 to i64
  %157 = add i64 0, %idx.ext17
  %158 = add i64 %157, 1
  %159 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer139, i64 %158, i64 1)
  %160 = load i8, i8* %_buffer139
  store i8 %160, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 2), align 2, !tbaa !534
  %add19 = add nuw nsw i32 %add8, 12
  %div20 = lshr exact i32 %add19, 3
  %idx.ext21 = zext i32 %div20 to i64
  %161 = add i64 0, %idx.ext21
  %162 = add i64 %161, 1
  %163 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %126, i64 %162, i64 2)
  %164 = load i16, i16* %_buffer161
  %rev4023 = tail call i16 @llvm.bswap.i16(i16 %164)
  store i16 %rev4023, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 3), align 4, !tbaa !535
  %add41 = add nuw nsw i32 %add8, 28
  %div42 = lshr exact i32 %add41, 3
  %idx.ext43 = zext i32 %div42 to i64
  %165 = add i64 0, %idx.ext43
  %166 = add i64 %165, 1
  %167 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %137, i64 %166, i64 2)
  %168 = load i16, i16* %_buffer140
  %rev4022 = tail call i16 @llvm.bswap.i16(i16 %168)
  store i16 %rev4022, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 4), align 2, !tbaa !536
  %add69 = add nuw nsw i32 %add8, 44
  %div70 = lshr exact i32 %add69, 3
  %idx.ext71 = zext i32 %div70 to i64
  %169 = add i64 0, %idx.ext71
  %170 = add i64 %169, 1
  %171 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer141, i64 %170, i64 1)
  %172 = load i8, i8* %_buffer141
  %173 = lshr i8 %172, 5
  store i8 %173, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 5), align 8, !tbaa !537
  %add77 = add nuw nsw i32 %add8, 47
  %div78 = lshr i32 %add77, 3
  %idx.ext79 = zext i32 %div78 to i64
  %174 = add i64 0, %idx.ext79
  %175 = add i64 %174, 1
  %176 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %127, i64 %175, i64 2)
  %177 = load i16, i16* %_buffer159
  %178 = and i16 %177, -225
  %179 = tail call i16 @llvm.bswap.i16(i16 %178)
  store i16 %179, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 6), align 2, !tbaa !538
  %add106 = add nuw nsw i32 %add8, 60
  %div107 = lshr exact i32 %add106, 3
  %idx.ext108 = zext i32 %div107 to i64
  %180 = add i64 0, %idx.ext108
  %181 = add i64 %180, 1
  %182 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer193, i64 %181, i64 1)
  %183 = load i8, i8* %_buffer193
  store i8 %183, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 7), align 4, !tbaa !539
  %add110 = add nuw nsw i32 %add8, 68
  %div111 = lshr exact i32 %add110, 3
  %idx.ext112 = zext i32 %div111 to i64
  %184 = add i64 0, %idx.ext112
  %185 = add i64 %184, 1
  %186 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer142, i64 %185, i64 1)
  %187 = load i8, i8* %_buffer142
  store i8 %187, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 8), align 1, !tbaa !540
  %add114 = add nuw nsw i32 %add8, 76
  %div115 = lshr exact i32 %add114, 3
  %idx.ext116 = zext i32 %div115 to i64
  %188 = add i64 0, %idx.ext116
  %189 = add i64 %188, 1
  %190 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %117, i64 %189, i64 2)
  %191 = load i16, i16* %_buffer199
  %rev4020 = tail call i16 @llvm.bswap.i16(i16 %191)
  store i16 %rev4020, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 9), align 2, !tbaa !541
  %add142 = add nuw nsw i32 %add8, 92
  %div143 = lshr exact i32 %add142, 3
  %idx.ext144 = zext i32 %div143 to i64
  %192 = add i64 0, %idx.ext144
  %193 = add i64 %192, 1
  %194 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %136, i64 %193, i64 4)
  %195 = load i32, i32* %_buffer143
  %or169 = tail call i32 @llvm.bswap.i32(i32 %195)
  store i32 %or169, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 10), align 8, !tbaa !542
  %add176 = add nuw nsw i32 %add8, 124
  %div177 = lshr exact i32 %add176, 3
  %idx.ext178 = zext i32 %div177 to i64
  %196 = add i64 0, %idx.ext178
  %197 = add i64 %196, 1
  %198 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %130, i64 %197, i64 4)
  %199 = load i32, i32* %_buffer155
  %or203 = tail call i32 @llvm.bswap.i32(i32 %199)
  store i32 %or203, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 11), align 4, !tbaa !543
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 12), align 8, !tbaa !544
  %add211 = add nuw nsw i32 %add8, 188
  %div212 = lshr exact i32 %add211, 3
  %idx.ext213 = zext i32 %div212 to i64
  %200 = add i64 0, %idx.ext213
  %201 = icmp ugt i64 %200, %142
  br i1 %201, label %cleanup3166, label %if.end218

if.end218:                                        ; preds = %if.end
  %add210 = add nuw nsw i32 %add8, 156
  %div219 = lshr exact i32 %add210, 3
  %idx.ext220 = zext i32 %div219 to i64
  %202 = add i64 0, %idx.ext220
  %203 = add i64 %202, 1
  %204 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer144, i64 %203, i64 1)
  %205 = load i8, i8* %_buffer144
  store i8 %205, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 0), align 4, !tbaa !545
  %add222 = add nuw nsw i32 %add8, 164
  %div223 = lshr exact i32 %add222, 3
  %idx.ext224 = zext i32 %div223 to i64
  %206 = add i64 0, %idx.ext224
  %207 = add i64 %206, 1
  %208 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer156, i64 %207, i64 1)
  %209 = load i8, i8* %_buffer156
  store i8 %209, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 1), align 1, !tbaa !546
  %add226 = add nuw nsw i32 %add8, 172
  %div227 = lshr exact i32 %add226, 3
  %idx.ext228 = zext i32 %div227 to i64
  %210 = add i64 0, %idx.ext228
  %211 = add i64 %210, 1
  %212 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer145, i64 %211, i64 1)
  %213 = load i8, i8* %_buffer145
  store i8 %213, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 2), align 2, !tbaa !547
  %add230 = add nuw nsw i32 %add8, 180
  %div231 = lshr exact i32 %add230, 3
  %idx.ext232 = zext i32 %div231 to i64
  %214 = add i64 0, %idx.ext232
  %215 = add i64 %214, 1
  %216 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer146, i64 %215, i64 1)
  %217 = load i8, i8* %_buffer146
  %218 = lshr i8 %217, 4
  store i8 %218, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 3), align 1, !tbaa !548
  %add238 = add nuw nsw i32 %add8, 184
  %div239 = lshr i32 %add238, 3
  %idx.ext240 = zext i32 %div239 to i64
  %219 = add i64 0, %idx.ext240
  %220 = add i64 %219, 1
  %221 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer147, i64 %220, i64 1)
  %222 = load i8, i8* %_buffer147
  %223 = and i8 %222, 15
  store i8 %223, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 4), align 4, !tbaa !549
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 5), align 1, !tbaa !550
  %224 = add i8 %213, -5
  %225 = lshr i8 %224, 3
  %226 = shl i8 %224, 5
  %227 = or i8 %225, %226
  switch i8 %227, label %accept [
    i8 3, label %parse_ipv4_rfc781_0
    i8 2, label %parse_ipv4_rfc781_1
    i8 1, label %parse_ipv4_rfc781_2
    i8 0, label %parse_ipv4_rfc781_3
  ]

parse_ipv4_rfc781_0:                              ; preds = %if.end218
  %add271 = add nuw nsw i32 %add8, 252
  %div272 = lshr exact i32 %add271, 3
  %idx.ext273 = zext i32 %div272 to i64
  %228 = add i64 0, %idx.ext273
  %229 = icmp ugt i64 %228, %142
  br i1 %229, label %cleanup3166, label %if.end278

if.end278:                                        ; preds = %parse_ipv4_rfc781_0
  %230 = add i64 %200, 1
  %231 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %124, i64 %230, i64 4)
  %232 = load i32, i32* %_buffer164
  %or305 = tail call i32 @llvm.bswap.i32(i32 %232)
  store i32 %or305, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 0), align 4, !tbaa !551
  %add312 = add nuw nsw i32 %add8, 220
  %div313 = lshr exact i32 %add312, 3
  %idx.ext314 = zext i32 %div313 to i64
  %233 = add i64 0, %idx.ext314
  %234 = add i64 %233, 1
  %235 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %135, i64 %234, i64 4)
  %236 = load i32, i32* %_buffer148
  %or339 = tail call i32 @llvm.bswap.i32(i32 %236)
  store i32 %or339, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 1), align 4, !tbaa !552
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 2), align 4, !tbaa !553
  br label %parse_ipv4_rfc781_1

parse_ipv4_rfc781_1:                              ; preds = %if.end278, %if.end218
  %ebpf_packetOffsetInBits.1 = phi i32 [ %add271, %if.end278 ], [ %add211, %if.end218 ]
  %add347 = add nsw i32 %ebpf_packetOffsetInBits.1, 64
  %div348 = lshr i32 %add347, 3
  %idx.ext349 = zext i32 %div348 to i64
  %237 = add i64 0, %idx.ext349
  %238 = icmp ugt i64 %237, %142
  br i1 %238, label %cleanup3166, label %if.end354

if.end354:                                        ; preds = %parse_ipv4_rfc781_1
  %div355 = lshr i32 %ebpf_packetOffsetInBits.1, 3
  %idx.ext356 = zext i32 %div355 to i64
  %239 = add i64 0, %idx.ext356
  %240 = add i64 %239, 1
  %241 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %123, i64 %240, i64 4)
  %242 = load i32, i32* %_buffer177
  %or381 = tail call i32 @llvm.bswap.i32(i32 %242)
  store i32 %or381, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 0), align 8, !tbaa !554
  %add388 = add nsw i32 %ebpf_packetOffsetInBits.1, 32
  %div389 = lshr i32 %add388, 3
  %idx.ext390 = zext i32 %div389 to i64
  %243 = add i64 0, %idx.ext390
  %244 = add i64 %243, 1
  %245 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %119, i64 %244, i64 4)
  %246 = load i32, i32* %_buffer195
  %or415 = tail call i32 @llvm.bswap.i32(i32 %246)
  store i32 %or415, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 1), align 4, !tbaa !555
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 2), align 8, !tbaa !556
  br label %parse_ipv4_rfc781_2

parse_ipv4_rfc781_2:                              ; preds = %if.end354, %if.end218
  %ebpf_packetOffsetInBits.2 = phi i32 [ %add347, %if.end354 ], [ %add211, %if.end218 ]
  %add423 = add i32 %ebpf_packetOffsetInBits.2, 64
  %div424 = lshr i32 %add423, 3
  %idx.ext425 = zext i32 %div424 to i64
  %247 = add i64 0, %idx.ext425
  %248 = icmp ugt i64 %247, %142
  br i1 %248, label %cleanup3166, label %if.end430

if.end430:                                        ; preds = %parse_ipv4_rfc781_2
  %div431 = lshr i32 %ebpf_packetOffsetInBits.2, 3
  %idx.ext432 = zext i32 %div431 to i64
  %249 = add i64 0, %idx.ext432
  %250 = add i64 %249, 1
  %251 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %134, i64 %250, i64 4)
  %252 = load i32, i32* %_buffer149
  %or457 = tail call i32 @llvm.bswap.i32(i32 %252)
  store i32 %or457, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 0), align 4, !tbaa !557
  %add464 = add i32 %ebpf_packetOffsetInBits.2, 32
  %div465 = lshr i32 %add464, 3
  %idx.ext466 = zext i32 %div465 to i64
  %253 = add i64 0, %idx.ext466
  %254 = add i64 %253, 1
  %255 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %133, i64 %254, i64 4)
  %256 = load i32, i32* %_buffer150
  %or491 = tail call i32 @llvm.bswap.i32(i32 %256)
  store i32 %or491, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 1), align 4, !tbaa !558
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 2), align 4, !tbaa !559
  br label %parse_ipv4_rfc781_3

parse_ipv4_rfc781_3:                              ; preds = %if.end430, %if.end218
  %ebpf_packetOffsetInBits.3 = phi i32 [ %add423, %if.end430 ], [ %add211, %if.end218 ]
  %add499 = add i32 %ebpf_packetOffsetInBits.3, 64
  %div500 = lshr i32 %add499, 3
  %idx.ext501 = zext i32 %div500 to i64
  %257 = add i64 0, %idx.ext501
  %258 = icmp ugt i64 %257, %142
  br i1 %258, label %cleanup3166, label %if.end506

if.end506:                                        ; preds = %parse_ipv4_rfc781_3
  %div507 = lshr i32 %ebpf_packetOffsetInBits.3, 3
  %idx.ext508 = zext i32 %div507 to i64
  %259 = add i64 0, %idx.ext508
  %260 = add i64 %259, 1
  %261 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %132, i64 %260, i64 4)
  %262 = load i32, i32* %_buffer151
  %or533 = tail call i32 @llvm.bswap.i32(i32 %262)
  store i32 %or533, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 0), align 8, !tbaa !560
  %add540 = add i32 %ebpf_packetOffsetInBits.3, 32
  %div541 = lshr i32 %add540, 3
  %idx.ext542 = zext i32 %div541 to i64
  %263 = add i64 0, %idx.ext542
  %264 = add i64 %263, 1
  %265 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %125, i64 %264, i64 4)
  %266 = load i32, i32* %_buffer163
  %or567 = tail call i32 @llvm.bswap.i32(i32 %266)
  store i32 %or567, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 1), align 4, !tbaa !561
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 2), align 8, !tbaa !562
  br label %accept

parse_vlan:                                       ; preds = %if.end1025, %if.end1025, %if.end1025, %if.end1025, %if.end1025
  %267 = icmp ugt i64 46, %142
  br i1 %267, label %cleanup3166, label %if.end582

if.end582:                                        ; preds = %parse_vlan
  %268 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer192, i64 43, i64 1)
  %269 = load i8, i8* %_buffer192
  %270 = lshr i8 %269, 5
  store i8 %270, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 0), align 8, !tbaa !563
  %271 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer210, i64 43, i64 1)
  %272 = load i8, i8* %_buffer210
  %273 = lshr i8 %272, 4
  %274 = and i8 %273, 1
  store i8 %274, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 1), align 1, !tbaa !565
  %275 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %121, i64 43, i64 2)
  %276 = load i16, i16* %_buffer191
  %277 = and i16 %276, -241
  %278 = tail call i16 @llvm.bswap.i16(i16 %277)
  store i16 %278, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 2), align 2, !tbaa !566
  %279 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %129, i64 45, i64 2)
  %280 = load i16, i16* %_buffer157
  %rev4026 = tail call i16 @llvm.bswap.i16(i16 %280)
  store i16 %rev4026, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 3), align 4, !tbaa !567
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 4), align 2, !tbaa !568
  switch i16 %rev4026, label %accept [
    i16 -32512, label %parse_vlan1
    i16 -30552, label %parse_vlan1
    i16 -28416, label %parse_vlan1
    i16 -28160, label %parse_vlan1
    i16 -27904, label %parse_vlan1
    i16 2048, label %parse_ipv4
  ]

parse_vlan1:                                      ; preds = %if.end582, %if.end582, %if.end582, %if.end582, %if.end582
  %281 = icmp ugt i64 50, %142
  br i1 %281, label %cleanup3166, label %if.end701

if.end701:                                        ; preds = %parse_vlan1
  %282 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer153, i64 47, i64 1)
  %283 = load i8, i8* %_buffer153
  %284 = lshr i8 %283, 5
  store i8 %284, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 0), align 8, !tbaa !563
  %285 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer154, i64 47, i64 1)
  %286 = load i8, i8* %_buffer154
  %287 = lshr i8 %286, 4
  %288 = and i8 %287, 1
  store i8 %288, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 1), align 1, !tbaa !565
  %289 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %131, i64 47, i64 2)
  %290 = load i16, i16* %_buffer152
  %291 = and i16 %290, -241
  %292 = tail call i16 @llvm.bswap.i16(i16 %291)
  store i16 %292, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 2), align 2, !tbaa !566
  %293 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %122, i64 49, i64 2)
  %294 = load i16, i16* %_buffer188
  %rev4024 = tail call i16 @llvm.bswap.i16(i16 %294)
  store i16 %rev4024, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 3), align 4, !tbaa !567
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 4), align 2, !tbaa !568
  switch i16 %rev4024, label %accept [
    i16 -32512, label %cleanup3166
    i16 -30552, label %cleanup3166
    i16 -28416, label %cleanup3166
    i16 -28160, label %cleanup3166
    i16 -27904, label %cleanup3166
    i16 2048, label %parse_ipv4
  ]

if.end820:                                        ; preds = %entry
  %295 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer160, i64 1, i64 1)
  %296 = load i8, i8* %_buffer160
  store i8 %296, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 0), align 8, !tbaa !569
  %297 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer162, i64 2, i64 1)
  %298 = load i8, i8* %_buffer162
  store i8 %298, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 1), align 1, !tbaa !569
  %299 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer165, i64 3, i64 1)
  %300 = load i8, i8* %_buffer165
  store i8 %300, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 2), align 2, !tbaa !569
  %301 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer166, i64 4, i64 1)
  %302 = load i8, i8* %_buffer166
  store i8 %302, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 3), align 1, !tbaa !569
  %303 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer167, i64 5, i64 1)
  %304 = load i8, i8* %_buffer167
  store i8 %304, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 4), align 4, !tbaa !569
  %305 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer170, i64 6, i64 1)
  %306 = load i8, i8* %_buffer170
  store i8 %306, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 5), align 1, !tbaa !569
  %307 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer196, i64 7, i64 1)
  %308 = load i8, i8* %_buffer196
  store i8 %308, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 6), align 2, !tbaa !569
  %309 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer178, i64 8, i64 1)
  %310 = load i8, i8* %_buffer178
  store i8 %310, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 7), align 1, !tbaa !569
  %311 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer180, i64 9, i64 1)
  %312 = load i8, i8* %_buffer180
  store i8 %312, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 8), align 8, !tbaa !569
  %313 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer182, i64 10, i64 1)
  %314 = load i8, i8* %_buffer182
  store i8 %314, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 9), align 1, !tbaa !569
  %315 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer200, i64 11, i64 1)
  %316 = load i8, i8* %_buffer200
  store i8 %316, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 10), align 2, !tbaa !569
  %317 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer184, i64 12, i64 1)
  %318 = load i8, i8* %_buffer184
  store i8 %318, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 11), align 1, !tbaa !569
  %319 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer186, i64 13, i64 1)
  %320 = load i8, i8* %_buffer186
  store i8 %320, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 12), align 4, !tbaa !569
  %321 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer171, i64 14, i64 1)
  %322 = load i8, i8* %_buffer171
  store i8 %322, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 13), align 1, !tbaa !569
  %323 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer172, i64 15, i64 1)
  %324 = load i8, i8* %_buffer172
  store i8 %324, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 14), align 2, !tbaa !569
  %325 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer173, i64 16, i64 1)
  %326 = load i8, i8* %_buffer173
  store i8 %326, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 15), align 1, !tbaa !569
  %327 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer174, i64 17, i64 1)
  %328 = load i8, i8* %_buffer174
  store i8 %328, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 16), align 8, !tbaa !569
  %329 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer175, i64 18, i64 1)
  %330 = load i8, i8* %_buffer175
  store i8 %330, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 17), align 1, !tbaa !569
  %331 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer176, i64 19, i64 1)
  %332 = load i8, i8* %_buffer176
  store i8 %332, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 18), align 2, !tbaa !569
  %333 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer197, i64 20, i64 1)
  %334 = load i8, i8* %_buffer197
  store i8 %334, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 19), align 1, !tbaa !569
  %335 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer179, i64 21, i64 1)
  %336 = load i8, i8* %_buffer179
  store i8 %336, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 20), align 4, !tbaa !569
  %337 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer181, i64 22, i64 1)
  %338 = load i8, i8* %_buffer181
  store i8 %338, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 21), align 1, !tbaa !569
  %339 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer183, i64 23, i64 1)
  %340 = load i8, i8* %_buffer183
  store i8 %340, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 22), align 2, !tbaa !569
  %341 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer201, i64 24, i64 1)
  %342 = load i8, i8* %_buffer201
  store i8 %342, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 23), align 1, !tbaa !569
  %343 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer185, i64 25, i64 1)
  %344 = load i8, i8* %_buffer185
  store i8 %344, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 24), align 8, !tbaa !569
  %345 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer187, i64 26, i64 1)
  %346 = load i8, i8* %_buffer187
  store i8 %346, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 25), align 1, !tbaa !569
  %347 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer189, i64 27, i64 1)
  %348 = load i8, i8* %_buffer189
  store i8 %348, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 26), align 2, !tbaa !569
  %349 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer190, i64 28, i64 1)
  %350 = load i8, i8* %_buffer190
  store i8 %350, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 27), align 1, !tbaa !569
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 1), align 4, !tbaa !570
  %351 = icmp ugt i64 42, %142
  br i1 %351, label %cleanup3166, label %if.end1025

if.end1025:                                       ; preds = %if.end820
  %352 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %128, i64 29, i64 8)
  %353 = load i64, i64* %_buffer158
  %shl1029 = and i64 %353, 281474976710655
  store i64 %shl1029, i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 0), align 8, !tbaa !571
  %354 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %120, i64 35, i64 8)
  %355 = load i64, i64* %_buffer194
  %shl1036 = and i64 %355, 281474976710655
  store i64 %shl1036, i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 1), align 8, !tbaa !572
  %356 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %118, i64 41, i64 2)
  %357 = load i16, i16* %_buffer198
  %rev4028 = tail call i16 @llvm.bswap.i16(i16 %357)
  store i16 %rev4028, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 2), align 8, !tbaa !573
  store i8 1, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 3), align 2, !tbaa !574
  switch i16 %rev4028, label %accept [
    i16 -32512, label %parse_vlan
    i16 -30552, label %parse_vlan
    i16 -28416, label %parse_vlan
    i16 -28160, label %parse_vlan
    i16 -27904, label %parse_vlan
    i16 2048, label %parse_ipv4
  ]

accept:                                           ; preds = %if.end1025, %if.end701, %if.end582, %if.end506, %if.end218
  %ebpf_packetOffsetInBits.4 = phi i32 [ %add499, %if.end506 ], [ %add211, %if.end218 ], [ 368, %if.end582 ], [ 400, %if.end701 ], [ 336, %if.end1025 ]
  %output_port = getelementptr inbounds %struct.xdp_output, %struct.xdp_output* %xout, i64 0, i32 1
  store i32 0, i32* %output_port, align 4, !tbaa !575
  %output_action = getelementptr inbounds %struct.xdp_output, %struct.xdp_output* %xout, i64 0, i32 0
  store i32 2, i32* %output_action, align 4, !tbaa !577
  %358 = load i16, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 9), align 2, !tbaa !541
  %conv1106 = zext i16 %358 to i32
  %neg = xor i32 %conv1106, 524287
  %359 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 1), align 4, !tbaa !561
  %conv1108 = and i32 %359, 65535
  %sub = sub nsw i32 %neg, %conv1108
  %conv1111 = and i32 %sub, 65535
  %shr1118 = lshr i32 %sub, 16
  %sub1122 = sub nsw i32 %conv1111, %shr1118
  %shr1124 = lshr i32 %359, 16
  %sub1127 = sub nsw i32 %sub1122, %shr1124
  %conv1130 = and i32 %sub1127, 65535
  %and1128 = lshr i32 %sub1127, 16
  %conv1158 = and i32 %and1128, 7
  %sub1160 = sub nsw i32 %conv1130, %conv1158
  %conv1163 = and i32 %sub1160, 65535
  %and1161 = lshr i32 %sub1160, 16
  %conv1224 = and i32 %and1161, 7
  %add1226 = add nuw nsw i32 %conv1224, %conv1163
  %shr1354 = lshr i32 %add1226, 16
  %add1359 = add nuw nsw i32 %shr1354, %add1226
  %360 = trunc i32 %add1359 to i16
  %conv1361 = xor i16 %360, -1
  %361 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 12), align 8, !tbaa !544
  %tobool = icmp eq i8 %361, 0
  br i1 %tobool, label %if.end1377, label %if.then1362

if.then1362:                                      ; preds = %accept
  %362 = getelementptr inbounds %struct.MyProcessing_IPV4_CNT_key, %struct.MyProcessing_IPV4_CNT_key* %key, i64 0, i32 0
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %362) #5
  store i8 0, i8* %362, align 1, !tbaa !578
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %key.gep_copy, i8* %362, i64 1, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 4, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %key.gep_copy, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 1, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data43, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 4, i32 0, i8* %key.gep_copy, i64 1, i8* null, i8* %dummy_rd_data43, i8* null, i64 0, i64 1), !dbg !486
  %363 = icmp eq i64 %call.i, 0
  br i1 %363, label %if.end1368, label %if.then1371

if.end1368:                                       ; preds = %if.then1362
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %ebpf_zero.c_copy, i8* %143, i64 4, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 5, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %ebpf_zero.c_copy, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i1 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 5, i32 0, i8* %ebpf_zero.c_copy, i64 4, i8* null, i8* %dummy_rd_data, i8* null, i64 0, i64 1), !dbg !486
  %364 = icmp eq i64 %call.i1, 0
  br i1 %364, label %cleanup1374.thread, label %if.then1371

if.then1371:                                      ; preds = %if.end1368, %if.then1362
  %365 = phi i16 [ 5, %if.end1368 ], [ 4, %if.then1362 ]
  %366 = phi i8* [ %ebpf_zero.c_copy, %if.end1368 ], [ %key.gep_copy, %if.then1362 ]
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 %365, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %366, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %140, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 4, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i2 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext %365, i32 0, i8* %366, i64 4, i8* null, i8* %140, i8* null, i64 0, i64 4), !dbg !486
  %367 = load i32, i32* %_buffer51
  %cond3184 = icmp eq i32 %367, 0
  br i1 %cond3184, label %cleanup1374, label %cleanup1374.thread

cleanup1374.thread:                               ; preds = %if.then1371, %if.end1368
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %362) #5
  br label %cleanup3166

cleanup1374:                                      ; preds = %if.then1371
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %362) #5
  br label %if.end1377

if.end1377:                                       ; preds = %cleanup1374, %accept
  %368 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 2), align 8, !tbaa !562
  %tobool1378 = icmp eq i8 %368, 0
  br i1 %tobool1378, label %cleanup.cont1507, label %if.then1379

if.then1379:                                      ; preds = %if.end1377
  %369 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 2), align 2, !tbaa !547
  %370 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 1), align 1, !tbaa !546
  %cmp1382 = icmp ult i8 %369, %370
  br i1 %cmp1382, label %if.then1384, label %cleanup.cont1507

if.then1384:                                      ; preds = %if.then1379
  %371 = bitcast %struct.MyProcessing_MATCH_RFC781_key* %key1385 to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %371) #5
  %372 = getelementptr inbounds %struct.MyProcessing_MATCH_RFC781_key, %struct.MyProcessing_MATCH_RFC781_key* %key1385, i64 0, i32 0
  %373 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 0), align 8, !tbaa !560
  store i32 %373, i32* %372, align 4, !tbaa !580
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %key1385.c_copy, i8* %371, i64 4, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 6, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %key1385.c_copy, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data49, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i3 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 6, i32 0, i8* %key1385.c_copy, i64 4, i8* null, i8* %dummy_rd_data49, i8* null, i64 0, i64 1), !dbg !486
  %374 = icmp eq i64 %call.i3, 0
  br i1 %374, label %if.end1394, label %if.then1397

if.end1394:                                       ; preds = %if.then1384
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %ebpf_zero.c_copy57, i8* %143, i64 4, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 7, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %ebpf_zero.c_copy57, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data58, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i4 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 7, i32 0, i8* %ebpf_zero.c_copy57, i64 4, i8* null, i8* %dummy_rd_data58, i8* null, i64 0, i64 1), !dbg !486
  %375 = icmp eq i64 %call.i4, 0
  br i1 %375, label %cleanup1408.thread, label %if.then1397

if.then1397:                                      ; preds = %if.end1394, %if.then1384
  %376 = phi i16 [ 7, %if.end1394 ], [ 6, %if.then1384 ]
  %377 = phi i8* [ %ebpf_zero.c_copy57, %if.end1394 ], [ %key1385.c_copy, %if.then1384 ]
  %hit.04056 = phi i8 [ 0, %if.end1394 ], [ 1, %if.then1384 ]
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 %376, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %377, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %139, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 4, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i5 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext %376, i32 0, i8* %377, i64 4, i8* null, i8* %139, i8* null, i64 0, i64 4), !dbg !486
  %378 = load i32, i32* %_buffer60
  switch i32 %378, label %cleanup1408.thread [
    i32 0, label %sw.bb1399
    i32 1, label %cleanup.cont1411
  ]

sw.bb1399:                                        ; preds = %if.then1397
  %379 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 2), align 2, !tbaa !547
  %add1401 = add i8 %379, 8
  store i8 %add1401, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 2), align 2, !tbaa !547
  store i16 %conv1361, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 9), align 2, !tbaa !541
  br label %cleanup.cont1411

cleanup1408.thread:                               ; preds = %if.then1397, %if.end1394
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %371) #5
  br label %cleanup3166

cleanup.cont1411:                                 ; preds = %sw.bb1399, %if.then1397
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %371) #5
  %tobool1412 = icmp eq i8 %hit.04056, 0
  %380 = bitcast %struct.MyProcessing_MATCH_RFC781_key* %key1416 to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %380) #5
  %381 = getelementptr inbounds %struct.MyProcessing_MATCH_RFC781_key, %struct.MyProcessing_MATCH_RFC781_key* %key1416, i64 0, i32 0
  %382 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 0), align 8, !tbaa !560
  store i32 %382, i32* %381, align 4, !tbaa !582
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %key1416.c_copy, i8* %380, i64 4, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 0, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %key1416.c_copy, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data202, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i6 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 0, i32 0, i8* %key1416.c_copy, i64 4, i8* null, i8* %dummy_rd_data202, i8* null, i64 0, i64 1), !dbg !486
  %383 = icmp eq i64 %call.i6, 0
  br i1 %383, label %if.end1425, label %if.then1428

if.end1425:                                       ; preds = %cleanup.cont1411
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %ebpf_zero.c_copy80, i8* %143, i64 4, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 1, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %ebpf_zero.c_copy80, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data81, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i7 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 1, i32 0, i8* %ebpf_zero.c_copy80, i64 4, i8* null, i8* %dummy_rd_data81, i8* null, i64 0, i64 1), !dbg !486
  %384 = icmp eq i64 %call.i7, 0
  br i1 %384, label %cleanup1437.thread, label %if.then1428

if.then1428:                                      ; preds = %if.end1425, %cleanup.cont1411
  %385 = phi i16 [ 1, %if.end1425 ], [ 0, %cleanup.cont1411 ]
  %386 = phi i8* [ %ebpf_zero.c_copy80, %if.end1425 ], [ %key1416.c_copy, %cleanup.cont1411 ]
  %hit.14061 = phi i8 [ 0, %if.end1425 ], [ 1, %cleanup.cont1411 ]
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 %385, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %386, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %116, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 4, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i8 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext %385, i32 0, i8* %386, i64 4, i8* null, i8* %116, i8* null, i64 0, i64 4), !dbg !486
  %387 = load i32, i32* %_buffer204
  switch i32 %387, label %cleanup1437.thread [
    i32 0, label %sw.bb1430
    i32 1, label %cleanup.cont1440
  ]

sw.bb1430:                                        ; preds = %if.then1428
  store i32 1, i32* %output_action, align 4, !tbaa !577
  br label %cleanup.cont1440

cleanup1437.thread:                               ; preds = %if.then1428, %if.end1425
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %380) #5
  br label %cleanup3166

cleanup.cont1440:                                 ; preds = %sw.bb1430, %if.then1428
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %380) #5
  %tobool1441 = icmp ne i8 %hit.14061, 0
  %or.cond = or i1 %tobool1412, %tobool1441
  br i1 %or.cond, label %if.end1472, label %if.then1448

if.then1448:                                      ; preds = %cleanup.cont1440
  %388 = getelementptr inbounds %struct.MyProcessing_IPV4_CNT_key, %struct.MyProcessing_IPV4_CNT_key* %key1449, i64 0, i32 0
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %388) #5
  store i8 0, i8* %388, align 1, !tbaa !584
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %key1449.gep_copy, i8* %388, i64 1, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 8, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %key1449.gep_copy, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 1, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data67, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i9 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 8, i32 0, i8* %key1449.gep_copy, i64 1, i8* null, i8* %dummy_rd_data67, i8* null, i64 0, i64 1), !dbg !486
  %389 = icmp eq i64 %call.i9, 0
  br i1 %389, label %if.end1458, label %if.then1461

if.end1458:                                       ; preds = %if.then1448
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %ebpf_zero.c_copy205, i8* %143, i64 4, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 9, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %ebpf_zero.c_copy205, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data206, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i10 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 9, i32 0, i8* %ebpf_zero.c_copy205, i64 4, i8* null, i8* %dummy_rd_data206, i8* null, i64 0, i64 1), !dbg !486
  %390 = icmp eq i64 %call.i10, 0
  br i1 %390, label %cleanup1468.thread, label %if.then1461

if.then1461:                                      ; preds = %if.end1458, %if.then1448
  %391 = phi i16 [ 9, %if.end1458 ], [ 8, %if.then1448 ]
  %392 = phi i8* [ %ebpf_zero.c_copy205, %if.end1458 ], [ %key1449.gep_copy, %if.then1448 ]
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 %391, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %392, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %115, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 4, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i11 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext %391, i32 0, i8* %392, i64 4, i8* null, i8* %115, i8* null, i64 0, i64 4), !dbg !486
  %393 = load i32, i32* %_buffer208
  %cond3182 = icmp eq i32 %393, 0
  br i1 %cond3182, label %cleanup1468, label %cleanup1468.thread

cleanup1468.thread:                               ; preds = %if.then1461, %if.end1458
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %388) #5
  br label %cleanup3166

cleanup1468:                                      ; preds = %if.then1461
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %388) #5
  br label %if.end1472

if.end1472:                                       ; preds = %cleanup1468, %cleanup.cont1440
  br i1 %tobool1441, label %if.then1474, label %cleanup.cont1507

if.then1474:                                      ; preds = %if.end1472
  %394 = getelementptr inbounds %struct.MyProcessing_IPV4_CNT_key, %struct.MyProcessing_IPV4_CNT_key* %key1475, i64 0, i32 0
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %394) #5
  store i8 0, i8* %394, align 1, !tbaa !586
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %key1475.gep_copy, i8* %394, i64 1, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 2, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %key1475.gep_copy, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 1, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data63, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i12 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 2, i32 0, i8* %key1475.gep_copy, i64 1, i8* null, i8* %dummy_rd_data63, i8* null, i64 0, i64 1), !dbg !486
  %395 = icmp eq i64 %call.i12, 0
  br i1 %395, label %if.end1484, label %if.then1487

if.end1484:                                       ; preds = %if.then1474
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %ebpf_zero.c_copy73, i8* %143, i64 4, i1 false)
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 3, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %ebpf_zero.c_copy73, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %dummy_rd_data74, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 1, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i13 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 3, i32 0, i8* %ebpf_zero.c_copy73, i64 4, i8* null, i8* %dummy_rd_data74, i8* null, i64 0, i64 1), !dbg !486
  %396 = icmp eq i64 %call.i13, 0
  br i1 %396, label %cleanup1494.thread, label %if.then1487

if.then1487:                                      ; preds = %if.end1484, %if.then1474
  %397 = phi i16 [ 3, %if.end1484 ], [ 2, %if.then1474 ]
  %398 = phi i8* [ %ebpf_zero.c_copy73, %if.end1484 ], [ %key1475.gep_copy, %if.then1474 ]
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !472, metadata !DIExpression()), !dbg !479
  call void @llvm.dbg.value(metadata i16 %397, metadata !473, metadata !DIExpression()), !dbg !480
  call void @llvm.dbg.value(metadata i8* %398, metadata !474, metadata !DIExpression()), !dbg !481
  call void @llvm.dbg.value(metadata i64 4, metadata !475, metadata !DIExpression()), !dbg !482
  call void @llvm.dbg.value(metadata i8* %138, metadata !476, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i64 0, metadata !477, metadata !DIExpression()), !dbg !484
  call void @llvm.dbg.value(metadata i64 4, metadata !478, metadata !DIExpression()), !dbg !485
  %call.i14 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext %397, i32 0, i8* %398, i64 4, i8* null, i8* %138, i8* null, i64 0, i64 4), !dbg !486
  %399 = load i32, i32* %_buffer76
  %cond3180 = icmp eq i32 %399, 0
  br i1 %cond3180, label %cleanup1494, label %cleanup1494.thread

cleanup1494.thread:                               ; preds = %if.then1487, %if.end1484
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %394) #5
  br label %cleanup3166

cleanup1494:                                      ; preds = %if.then1487
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %394) #5
  br label %cleanup.cont1507

cleanup.cont1507:                                 ; preds = %cleanup1494, %if.end1472, %if.then1379, %if.end1377
  %400 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 1), align 4, !tbaa !570
  %tobool1508 = icmp eq i8 %400, 0
  %spec.select = select i1 %tobool1508, i32 0, i32 224
  %401 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 3), align 2, !tbaa !574
  %tobool1512 = icmp eq i8 %401, 0
  %add1514 = add nuw nsw i32 %spec.select, 112
  %ebpf_outHeaderLength.1 = select i1 %tobool1512, i32 %spec.select, i32 %add1514
  %402 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 4), align 2, !tbaa !568
  %tobool1516 = icmp eq i8 %402, 0
  %add1518 = add nuw nsw i32 %ebpf_outHeaderLength.1, 32
  %spec.select4033 = select i1 %tobool1516, i32 %ebpf_outHeaderLength.1, i32 %add1518
  %403 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 4), align 2, !tbaa !568
  %tobool1520 = icmp eq i8 %403, 0
  %add1522 = add nuw nsw i32 %spec.select4033, 32
  %ebpf_outHeaderLength.3 = select i1 %tobool1520, i32 %spec.select4033, i32 %add1522
  %404 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 12), align 8, !tbaa !544
  %tobool1524 = icmp eq i8 %404, 0
  %add1526 = add i32 %ebpf_outHeaderLength.3, 160
  %spec.select4034 = select i1 %tobool1524, i32 %ebpf_outHeaderLength.3, i32 %add1526
  %405 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 5), align 1, !tbaa !550
  %tobool1528 = icmp eq i8 %405, 0
  %add1530 = add i32 %spec.select4034, 32
  %ebpf_outHeaderLength.5 = select i1 %tobool1528, i32 %spec.select4034, i32 %add1530
  %406 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 2), align 4, !tbaa !553
  %tobool1532 = icmp eq i8 %406, 0
  %add1534 = add i32 %ebpf_outHeaderLength.5, 64
  %spec.select4035 = select i1 %tobool1532, i32 %ebpf_outHeaderLength.5, i32 %add1534
  %407 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 2), align 8, !tbaa !556
  %tobool1536 = icmp eq i8 %407, 0
  %add1538 = add i32 %spec.select4035, 64
  %ebpf_outHeaderLength.7 = select i1 %tobool1536, i32 %spec.select4035, i32 %add1538
  %408 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 2), align 4, !tbaa !559
  %tobool1540 = icmp eq i8 %408, 0
  %add1542 = add i32 %ebpf_outHeaderLength.7, 64
  %spec.select4036 = select i1 %tobool1540, i32 %ebpf_outHeaderLength.7, i32 %add1542
  %409 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 2), align 8, !tbaa !562
  %tobool1544 = icmp eq i8 %409, 0
  %add1546 = add i32 %spec.select4036, 64
  %ebpf_outHeaderLength.9 = select i1 %tobool1544, i32 %spec.select4036, i32 %add1546
  %div1548 = lshr i32 %ebpf_packetOffsetInBits.4, 3
  %div1549 = lshr i32 %ebpf_outHeaderLength.9, 3
  %sub1550 = sub nsw i32 %div1548, %div1549
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !363, metadata !DIExpression()), !dbg !366
  call void @llvm.dbg.value(metadata i32 %sub1550, metadata !364, metadata !DIExpression()), !dbg !367
  %call.i15 = call i32 @nanotube_packet_resize(%struct.nanotube_packet* %packet, i64 1, i32 %sub1550), !dbg !368
  call void @llvm.dbg.value(metadata i32 %call.i15, metadata !365, metadata !DIExpression()), !dbg !369
  %tobool.i = icmp eq i32 %call.i15, 0, !dbg !370
  %cond.i = sext i1 %tobool.i to i32, !dbg !370
  %410 = call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 32768)
  %411 = add i64 %410, -1
  %412 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 1), align 4, !tbaa !570
  %tobool1556 = icmp eq i8 %412, 0
  br i1 %tobool1556, label %if.end1844, label %if.then1557

if.then1557:                                      ; preds = %cleanup.cont1507
  %413 = icmp ugt i64 28, %411
  br i1 %413, label %cleanup3166, label %if.end1565

if.end1565:                                       ; preds = %if.then1557
  %414 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 0), align 8, !tbaa !569
  store i8 %414, i8* %_buffer26
  %savedstack = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %114), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer26, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 0, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %114, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %114, i8 -1, i64 1, i1 false), !dbg !406
  %call.i16 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer26, i8* nonnull %114, i64 1, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %114), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack), !dbg !408
  %415 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 1), align 1, !tbaa !569
  store i8 %415, i8* %_buffer30
  %savedstack18 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %113), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer30, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 1, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %113, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %113, i8 -1, i64 1, i1 false), !dbg !406
  %call.i17 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer30, i8* nonnull %113, i64 2, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %113), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack18), !dbg !408
  %416 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 2), align 2, !tbaa !569
  store i8 %416, i8* %_buffer66
  %savedstack20 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %112), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer66, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 2, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %112, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %112, i8 -1, i64 1, i1 false), !dbg !406
  %call.i19 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer66, i8* nonnull %112, i64 3, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %112), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack20), !dbg !408
  %417 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 3), align 1, !tbaa !569
  store i8 %417, i8* %_buffer54
  %savedstack22 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %111), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer54, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 3, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %111, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %111, i8 -1, i64 1, i1 false), !dbg !406
  %call.i21 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer54, i8* nonnull %111, i64 4, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %111), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack22), !dbg !408
  %418 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 4), align 4, !tbaa !569
  store i8 %418, i8* %_buffer
  %savedstack24 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %110), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 4, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %110, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %110, i8 -1, i64 1, i1 false), !dbg !406
  %call.i23 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer, i8* nonnull %110, i64 5, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %110), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack24), !dbg !408
  %419 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 5), align 1, !tbaa !569
  store i8 %419, i8* %_buffer20
  %savedstack26 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %109), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer20, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 5, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %109, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %109, i8 -1, i64 1, i1 false), !dbg !406
  %call.i25 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer20, i8* nonnull %109, i64 6, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %109), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack26), !dbg !408
  %420 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 6), align 2, !tbaa !569
  store i8 %420, i8* %_buffer216
  %savedstack28 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %108), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer216, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 6, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %108, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %108, i8 -1, i64 1, i1 false), !dbg !406
  %call.i27 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer216, i8* nonnull %108, i64 7, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %108), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack28), !dbg !408
  %421 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 7), align 1, !tbaa !569
  store i8 %421, i8* %_buffer12
  %savedstack30 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %107), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer12, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 7, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %107, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %107, i8 -1, i64 1, i1 false), !dbg !406
  %call.i29 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer12, i8* nonnull %107, i64 8, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %107), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack30), !dbg !408
  %422 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 8), align 8, !tbaa !569
  store i8 %422, i8* %_buffer104
  %savedstack32 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %106), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer104, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 8, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %106, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %106, i8 -1, i64 1, i1 false), !dbg !406
  %call.i31 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer104, i8* nonnull %106, i64 9, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %106), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack32), !dbg !408
  %423 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 9), align 1, !tbaa !569
  store i8 %423, i8* %_buffer99
  %savedstack34 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %105), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer99, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 9, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %105, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %105, i8 -1, i64 1, i1 false), !dbg !406
  %call.i33 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer99, i8* nonnull %105, i64 10, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %105), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack34), !dbg !408
  %424 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 10), align 2, !tbaa !569
  store i8 %424, i8* %_buffer94
  %savedstack36 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %104), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer94, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 10, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %104, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %104, i8 -1, i64 1, i1 false), !dbg !406
  %call.i35 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer94, i8* nonnull %104, i64 11, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %104), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack36), !dbg !408
  %425 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 11), align 1, !tbaa !569
  store i8 %425, i8* %_buffer118
  %savedstack38 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %103), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer118, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 11, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %103, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %103, i8 -1, i64 1, i1 false), !dbg !406
  %call.i37 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer118, i8* nonnull %103, i64 12, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %103), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack38), !dbg !408
  %426 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 12), align 4, !tbaa !569
  store i8 %426, i8* %_buffer114
  %savedstack40 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %102), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer114, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 12, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %102, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %102, i8 -1, i64 1, i1 false), !dbg !406
  %call.i39 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer114, i8* nonnull %102, i64 13, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %102), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack40), !dbg !408
  %427 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 13), align 1, !tbaa !569
  store i8 %427, i8* %_buffer90
  %savedstack42 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %101), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer90, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 13, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %101, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %101, i8 -1, i64 1, i1 false), !dbg !406
  %call.i41 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer90, i8* nonnull %101, i64 14, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %101), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack42), !dbg !408
  %428 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 14), align 2, !tbaa !569
  store i8 %428, i8* %_buffer37
  %savedstack44 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %100), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer37, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 14, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %100, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %100, i8 -1, i64 1, i1 false), !dbg !406
  %call.i43 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer37, i8* nonnull %100, i64 15, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %100), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack44), !dbg !408
  %429 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 15), align 1, !tbaa !569
  store i8 %429, i8* %_buffer38
  %savedstack46 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %99), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer38, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 15, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %99, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %99, i8 -1, i64 1, i1 false), !dbg !406
  %call.i45 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer38, i8* nonnull %99, i64 16, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %99), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack46), !dbg !408
  %430 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 16), align 8, !tbaa !569
  store i8 %430, i8* %_buffer41
  %savedstack48 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %98), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer41, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 16, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %98, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %98, i8 -1, i64 1, i1 false), !dbg !406
  %call.i47 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer41, i8* nonnull %98, i64 17, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %98), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack48), !dbg !408
  %431 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 17), align 1, !tbaa !569
  store i8 %431, i8* %_buffer29
  %savedstack50 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %97), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer29, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 17, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %97, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %97, i8 -1, i64 1, i1 false), !dbg !406
  %call.i49 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer29, i8* nonnull %97, i64 18, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %97), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack50), !dbg !408
  %432 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 18), align 2, !tbaa !569
  store i8 %432, i8* %_buffer65
  %savedstack52 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %96), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer65, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 18, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %96, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %96, i8 -1, i64 1, i1 false), !dbg !406
  %call.i51 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer65, i8* nonnull %96, i64 19, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %96), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack52), !dbg !408
  %433 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 19), align 1, !tbaa !569
  store i8 %433, i8* %_buffer53
  %savedstack54 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %95), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer53, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 19, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %95, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %95, i8 -1, i64 1, i1 false), !dbg !406
  %call.i53 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer53, i8* nonnull %95, i64 20, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %95), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack54), !dbg !408
  %434 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 20), align 4, !tbaa !569
  store i8 %434, i8* %_buffer17
  %savedstack56 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %94), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer17, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 20, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %94, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %94, i8 -1, i64 1, i1 false), !dbg !406
  %call.i55 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer17, i8* nonnull %94, i64 21, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %94), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack56), !dbg !408
  %435 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 21), align 1, !tbaa !569
  store i8 %435, i8* %_buffer22
  %savedstack58 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %93), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer22, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 21, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %93, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %93, i8 -1, i64 1, i1 false), !dbg !406
  %call.i57 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer22, i8* nonnull %93, i64 22, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %93), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack58), !dbg !408
  %436 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 22), align 2, !tbaa !569
  store i8 %436, i8* %_buffer13
  %savedstack60 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %92), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer13, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 22, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %92, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %92, i8 -1, i64 1, i1 false), !dbg !406
  %call.i59 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer13, i8* nonnull %92, i64 23, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %92), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack60), !dbg !408
  %437 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 23), align 1, !tbaa !569
  store i8 %437, i8* %_buffer19
  %savedstack62 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %91), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer19, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 23, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %91, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %91, i8 -1, i64 1, i1 false), !dbg !406
  %call.i61 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer19, i8* nonnull %91, i64 24, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %91), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack62), !dbg !408
  %438 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 24), align 8, !tbaa !569
  store i8 %438, i8* %_buffer14
  %savedstack64 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %90), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer14, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 24, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %90, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %90, i8 -1, i64 1, i1 false), !dbg !406
  %call.i63 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer14, i8* nonnull %90, i64 25, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %90), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack64), !dbg !408
  %439 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 25), align 1, !tbaa !569
  store i8 %439, i8* %_buffer15
  %savedstack66 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %89), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer15, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 25, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %89, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %89, i8 -1, i64 1, i1 false), !dbg !406
  %call.i65 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer15, i8* nonnull %89, i64 26, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %89), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack66), !dbg !408
  %440 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 26), align 2, !tbaa !569
  store i8 %440, i8* %_buffer132
  %savedstack68 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %88), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer132, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 26, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %88, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %88, i8 -1, i64 1, i1 false), !dbg !406
  %call.i67 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer132, i8* nonnull %88, i64 27, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %88), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack68), !dbg !408
  %441 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 0, i32 0, i64 27), align 1, !tbaa !569
  store i8 %441, i8* %_buffer35
  %savedstack70 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %87), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer35, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 27, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %87, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %87, i8 -1, i64 1, i1 false), !dbg !406
  %call.i69 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer35, i8* nonnull %87, i64 28, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %87), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack70), !dbg !408
  br label %if.end1844

if.end1844:                                       ; preds = %if.end1565, %cleanup.cont1507
  %ebpf_packetOffsetInBits.5 = phi i32 [ 224, %if.end1565 ], [ 0, %cleanup.cont1507 ]
  %442 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 3), align 2, !tbaa !574
  %tobool1845 = icmp eq i8 %442, 0
  br i1 %tobool1845, label %if.end2013, label %if.then1846

if.then1846:                                      ; preds = %if.end1844
  %add1847 = add nuw nsw i32 %ebpf_packetOffsetInBits.5, 112
  %div1848 = lshr exact i32 %add1847, 3
  %idx.ext1849 = zext i32 %div1848 to i64
  %443 = add i64 0, %idx.ext1849
  %444 = icmp ugt i64 %443, %411
  br i1 %444, label %cleanup3166, label %if.end1854

if.end1854:                                       ; preds = %if.then1846
  %445 = load i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 0) to i8*), align 8, !tbaa !569
  %div1859 = lshr exact i32 %ebpf_packetOffsetInBits.5, 3
  %idx.ext1861 = zext i32 %div1859 to i64
  %446 = add i64 0, %idx.ext1861
  store i8 %445, i8* %_buffer16
  %savedstack72 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %86), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer16, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %446, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %86, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %86, i8 -1, i64 1, i1 false), !dbg !406
  %447 = add i64 %446, 1
  %call.i71 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer16, i8* nonnull %86, i64 %447, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %86), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack72), !dbg !408
  %448 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 0) to i8*), i64 1), align 1, !tbaa !569
  %add1870 = or i32 %div1859, 1
  %idx.ext1871 = zext i32 %add1870 to i64
  %449 = add i64 0, %idx.ext1871
  store i8 %448, i8* %_buffer18
  %savedstack74 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %85), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer18, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %449, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %85, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %85, i8 -1, i64 1, i1 false), !dbg !406
  %450 = add i64 %449, 1
  %call.i73 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer18, i8* nonnull %85, i64 %450, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %85), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack74), !dbg !408
  %451 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 0) to i8*), i64 2), align 2, !tbaa !569
  %add1880 = or i32 %div1859, 2
  %idx.ext1881 = zext i32 %add1880 to i64
  %452 = add i64 0, %idx.ext1881
  store i8 %451, i8* %_buffer32
  %savedstack76 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %84), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer32, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %452, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %84, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %84, i8 -1, i64 1, i1 false), !dbg !406
  %453 = add i64 %452, 1
  %call.i75 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer32, i8* nonnull %84, i64 %453, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %84), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack76), !dbg !408
  %454 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 0) to i8*), i64 3), align 1, !tbaa !569
  %add1890 = or i32 %div1859, 3
  %idx.ext1891 = zext i32 %add1890 to i64
  %455 = add i64 0, %idx.ext1891
  store i8 %454, i8* %_buffer42
  %savedstack78 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %83), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer42, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %455, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %83, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %83, i8 -1, i64 1, i1 false), !dbg !406
  %456 = add i64 %455, 1
  %call.i77 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer42, i8* nonnull %83, i64 %456, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %83), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack78), !dbg !408
  %457 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 0) to i8*), i64 4), align 4, !tbaa !569
  %add1900 = add nuw nsw i32 %div1859, 4
  %idx.ext1901 = zext i32 %add1900 to i64
  %458 = add i64 0, %idx.ext1901
  store i8 %457, i8* %_buffer112
  %savedstack80 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %82), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer112, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %458, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %82, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %82, i8 -1, i64 1, i1 false), !dbg !406
  %459 = add i64 %458, 1
  %call.i79 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer112, i8* nonnull %82, i64 %459, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %82), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack80), !dbg !408
  %460 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 0) to i8*), i64 5), align 1, !tbaa !569
  %add1910 = add nuw nsw i32 %div1859, 5
  %idx.ext1911 = zext i32 %add1910 to i64
  %461 = add i64 0, %idx.ext1911
  store i8 %460, i8* %_buffer21
  %savedstack82 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %81), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer21, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %461, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %81, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %81, i8 -1, i64 1, i1 false), !dbg !406
  %462 = add i64 %461, 1
  %call.i81 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer21, i8* nonnull %81, i64 %462, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %81), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack82), !dbg !408
  %add1915 = add nuw nsw i32 %ebpf_packetOffsetInBits.5, 48
  %463 = load i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 1) to i8*), align 8, !tbaa !569
  %div1920 = lshr exact i32 %add1915, 3
  %idx.ext1922 = zext i32 %div1920 to i64
  %464 = add i64 0, %idx.ext1922
  store i8 %463, i8* %_buffer24
  %savedstack84 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %80), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer24, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %464, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %80, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %80, i8 -1, i64 1, i1 false), !dbg !406
  %465 = add i64 %464, 1
  %call.i83 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer24, i8* nonnull %80, i64 %465, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %80), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack84), !dbg !408
  %466 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 1) to i8*), i64 1), align 1, !tbaa !569
  %add1931 = or i32 %div1920, 1
  %idx.ext1932 = zext i32 %add1931 to i64
  %467 = add i64 0, %idx.ext1932
  store i8 %466, i8* %_buffer212
  %savedstack86 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %79), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer212, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %467, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %79, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %79, i8 -1, i64 1, i1 false), !dbg !406
  %468 = add i64 %467, 1
  %call.i85 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer212, i8* nonnull %79, i64 %468, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %79), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack86), !dbg !408
  %469 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 1) to i8*), i64 2), align 2, !tbaa !569
  %add1941 = add nuw nsw i32 %div1920, 2
  %idx.ext1942 = zext i32 %add1941 to i64
  %470 = add i64 0, %idx.ext1942
  store i8 %469, i8* %_buffer47
  %savedstack88 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %78), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer47, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %470, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %78, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %78, i8 -1, i64 1, i1 false), !dbg !406
  %471 = add i64 %470, 1
  %call.i87 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer47, i8* nonnull %78, i64 %471, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %78), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack88), !dbg !408
  %472 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 1) to i8*), i64 3), align 1, !tbaa !569
  %add1951 = add nuw nsw i32 %div1920, 3
  %idx.ext1952 = zext i32 %add1951 to i64
  %473 = add i64 0, %idx.ext1952
  store i8 %472, i8* %_buffer33
  %savedstack90 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %77), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer33, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %473, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %77, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %77, i8 -1, i64 1, i1 false), !dbg !406
  %474 = add i64 %473, 1
  %call.i89 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer33, i8* nonnull %77, i64 %474, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %77), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack90), !dbg !408
  %475 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 1) to i8*), i64 4), align 4, !tbaa !569
  %add1961 = add nuw nsw i32 %div1920, 4
  %idx.ext1962 = zext i32 %add1961 to i64
  %476 = add i64 0, %idx.ext1962
  store i8 %475, i8* %_buffer95
  %savedstack92 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %76), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer95, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %476, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %76, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %76, i8 -1, i64 1, i1 false), !dbg !406
  %477 = add i64 %476, 1
  %call.i91 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer95, i8* nonnull %76, i64 %477, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %76), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack92), !dbg !408
  %478 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i64* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 1) to i8*), i64 5), align 1, !tbaa !569
  %add1971 = add nuw nsw i32 %div1920, 5
  %idx.ext1972 = zext i32 %add1971 to i64
  %479 = add i64 0, %idx.ext1972
  store i8 %478, i8* %_buffer92
  %savedstack94 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %75), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer92, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %479, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %75, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %75, i8 -1, i64 1, i1 false), !dbg !406
  %480 = add i64 %479, 1
  %call.i93 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer92, i8* nonnull %75, i64 %480, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %75), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack94), !dbg !408
  %add1976 = add nuw nsw i32 %ebpf_packetOffsetInBits.5, 96
  %481 = load i16, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 2), align 8, !tbaa !573
  %rev4009 = call i16 @llvm.bswap.i16(i16 %481)
  store i16 %rev4009, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 2), align 8, !tbaa !573
  %482 = trunc i16 %rev4009 to i8
  %div1996 = lshr exact i32 %add1976, 3
  %idx.ext1998 = zext i32 %div1996 to i64
  %483 = add i64 0, %idx.ext1998
  store i8 %482, i8* %_buffer23
  %savedstack96 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %74), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer23, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %483, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %74, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %74, i8 -1, i64 1, i1 false), !dbg !406
  %484 = add i64 %483, 1
  %call.i95 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer23, i8* nonnull %74, i64 %484, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %74), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack96), !dbg !408
  %485 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 1, i32 2) to i8*), i64 1), align 1, !tbaa !569
  %add2007 = or i32 %div1996, 1
  %idx.ext2008 = zext i32 %add2007 to i64
  %486 = add i64 0, %idx.ext2008
  store i8 %485, i8* %_buffer25
  %savedstack98 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %73), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer25, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %486, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %73, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %73, i8 -1, i64 1, i1 false), !dbg !406
  %487 = add i64 %486, 1
  %call.i97 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer25, i8* nonnull %73, i64 %487, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %73), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack98), !dbg !408
  br label %if.end2013

if.end2013:                                       ; preds = %if.end1854, %if.end1844
  %ebpf_packetOffsetInBits.6 = phi i32 [ %add1847, %if.end1854 ], [ %ebpf_packetOffsetInBits.5, %if.end1844 ]
  %488 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 4), align 2, !tbaa !568
  %tobool2014 = icmp eq i8 %488, 0
  br i1 %tobool2014, label %if.end2128, label %if.then2015

if.then2015:                                      ; preds = %if.end2013
  %add2016 = add nsw i32 %ebpf_packetOffsetInBits.6, 32
  %div2017 = lshr i32 %add2016, 3
  %idx.ext2018 = zext i32 %div2017 to i64
  %489 = add i64 0, %idx.ext2018
  %490 = icmp ugt i64 %489, %411
  br i1 %490, label %cleanup3166, label %if.end2023

if.end2023:                                       ; preds = %if.then2015
  %491 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 0), align 8, !tbaa !569
  %shl2026 = shl i8 %491, 5
  %div2028 = lshr i32 %ebpf_packetOffsetInBits.6, 3
  %idx.ext2030 = zext i32 %div2028 to i64
  %492 = add i64 0, %idx.ext2030
  store i8 %shl2026, i8* %_buffer27
  %savedstack100 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %72), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer27, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %492, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %72, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %72, i8 -1, i64 1, i1 false), !dbg !406
  %493 = add i64 %492, 1
  %call.i99 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer27, i8* nonnull %72, i64 %493, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %72), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack100), !dbg !408
  %add2034 = add nsw i32 %ebpf_packetOffsetInBits.6, 3
  %494 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 1), align 1, !tbaa !569
  %div2036 = lshr i32 %add2034, 3
  %idx.ext2037 = zext i32 %div2036 to i64
  %495 = add i64 0, %idx.ext2037
  %496 = add i64 %495, 1
  %497 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer40, i64 %496, i64 1)
  %498 = load i8, i8* %_buffer40
  %499 = and i8 %498, -8
  %shl2045 = shl i8 %494, 2
  %and2048 = and i8 %shl2045, 4
  %or2049 = or i8 %and2048, %499
  store i8 %or2049, i8* %_buffer39
  %savedstack102 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %71), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer39, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %495, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %71, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %71, i8 -1, i64 1, i1 false), !dbg !406
  %500 = add i64 %495, 1
  %call.i101 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer39, i8* nonnull %71, i64 %500, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %71), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack102), !dbg !408
  %add2057 = add nsw i32 %ebpf_packetOffsetInBits.6, 4
  %501 = load i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 2) to i8*), align 2, !tbaa !569
  %div2060 = lshr i32 %add2057, 3
  %idx.ext2061 = zext i32 %div2060 to i64
  %502 = add i64 0, %idx.ext2061
  %503 = add i64 %502, 1
  %504 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer84, i64 %503, i64 1)
  %505 = load i8, i8* %_buffer84
  %506 = and i8 %505, -16
  %507 = and i8 %501, 15
  %or20734007 = or i8 %506, %507
  store i8 %or20734007, i8* %_buffer83
  %savedstack104 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %70), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer83, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %502, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %70, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %70, i8 -1, i64 1, i1 false), !dbg !406
  %508 = add i64 %502, 1
  %call.i103 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer83, i8* nonnull %70, i64 %508, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %70), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack104), !dbg !408
  %509 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 2) to i8*), i64 1), align 1, !tbaa !569
  %add2086 = add nuw nsw i32 %div2060, 1
  %idx.ext2087 = zext i32 %add2086 to i64
  %510 = add i64 0, %idx.ext2087
  store i8 %509, i8* %_buffer131
  %savedstack106 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %69), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer131, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %510, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %69, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %69, i8 -1, i64 1, i1 false), !dbg !406
  %511 = add i64 %510, 1
  %call.i105 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer131, i8* nonnull %69, i64 %511, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %69), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack106), !dbg !408
  %add2091 = add nsw i32 %ebpf_packetOffsetInBits.6, 16
  %512 = load i16, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 3), align 4, !tbaa !567
  %rev4008 = call i16 @llvm.bswap.i16(i16 %512)
  store i16 %rev4008, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 3), align 4, !tbaa !567
  %513 = trunc i16 %rev4008 to i8
  %div2111 = lshr i32 %add2091, 3
  %idx.ext2113 = zext i32 %div2111 to i64
  %514 = add i64 0, %idx.ext2113
  store i8 %513, i8* %_buffer98
  %savedstack108 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %68), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer98, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %514, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %68, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %68, i8 -1, i64 1, i1 false), !dbg !406
  %515 = add i64 %514, 1
  %call.i107 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer98, i8* nonnull %68, i64 %515, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %68), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack108), !dbg !408
  %516 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 0, i32 3) to i8*), i64 1), align 1, !tbaa !569
  %add2122 = add nuw nsw i32 %div2111, 1
  %idx.ext2123 = zext i32 %add2122 to i64
  %517 = add i64 0, %idx.ext2123
  store i8 %516, i8* %_buffer77
  %savedstack110 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %67), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer77, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %517, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %67, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %67, i8 -1, i64 1, i1 false), !dbg !406
  %518 = add i64 %517, 1
  %call.i109 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer77, i8* nonnull %67, i64 %518, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %67), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack110), !dbg !408
  br label %if.end2128

if.end2128:                                       ; preds = %if.end2023, %if.end2013
  %ebpf_packetOffsetInBits.7 = phi i32 [ %add2016, %if.end2023 ], [ %ebpf_packetOffsetInBits.6, %if.end2013 ]
  %519 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 4), align 2, !tbaa !568
  %tobool2129 = icmp eq i8 %519, 0
  br i1 %tobool2129, label %if.end2244, label %if.then2130

if.then2130:                                      ; preds = %if.end2128
  %add2131 = add nsw i32 %ebpf_packetOffsetInBits.7, 32
  %div2132 = lshr i32 %add2131, 3
  %idx.ext2133 = zext i32 %div2132 to i64
  %520 = add i64 0, %idx.ext2133
  %521 = icmp ugt i64 %520, %411
  br i1 %521, label %cleanup3166, label %if.end2138

if.end2138:                                       ; preds = %if.then2130
  %522 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 0), align 8, !tbaa !569
  %shl2141 = shl i8 %522, 5
  %div2143 = lshr i32 %ebpf_packetOffsetInBits.7, 3
  %idx.ext2145 = zext i32 %div2143 to i64
  %523 = add i64 0, %idx.ext2145
  store i8 %shl2141, i8* %_buffer28
  %savedstack112 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %66), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer28, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %523, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %66, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %66, i8 -1, i64 1, i1 false), !dbg !406
  %524 = add i64 %523, 1
  %call.i111 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer28, i8* nonnull %66, i64 %524, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %66), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack112), !dbg !408
  %add2149 = add nsw i32 %ebpf_packetOffsetInBits.7, 3
  %525 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 1), align 1, !tbaa !569
  %div2152 = lshr i32 %add2149, 3
  %idx.ext2153 = zext i32 %div2152 to i64
  %526 = add i64 0, %idx.ext2153
  %527 = add i64 %526, 1
  %528 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer56, i64 %527, i64 1)
  %529 = load i8, i8* %_buffer56
  %530 = and i8 %529, -8
  %shl2161 = shl i8 %525, 2
  %and2164 = and i8 %shl2161, 4
  %or2165 = or i8 %and2164, %530
  store i8 %or2165, i8* %_buffer55
  %savedstack114 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %65), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer55, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %526, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %65, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %65, i8 -1, i64 1, i1 false), !dbg !406
  %531 = add i64 %526, 1
  %call.i113 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer55, i8* nonnull %65, i64 %531, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %65), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack114), !dbg !408
  %add2173 = add nsw i32 %ebpf_packetOffsetInBits.7, 4
  %532 = load i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 2) to i8*), align 2, !tbaa !569
  %div2176 = lshr i32 %add2173, 3
  %idx.ext2177 = zext i32 %div2176 to i64
  %533 = add i64 0, %idx.ext2177
  %534 = add i64 %533, 1
  %535 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer122, i64 %534, i64 1)
  %536 = load i8, i8* %_buffer122
  %537 = and i8 %536, -16
  %538 = and i8 %532, 15
  %or21894005 = or i8 %537, %538
  store i8 %or21894005, i8* %_buffer126
  %savedstack116 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %64), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer126, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %533, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %64, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %64, i8 -1, i64 1, i1 false), !dbg !406
  %539 = add i64 %533, 1
  %call.i115 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer126, i8* nonnull %64, i64 %539, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %64), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack116), !dbg !408
  %540 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 2) to i8*), i64 1), align 1, !tbaa !569
  %add2202 = add nuw nsw i32 %div2176, 1
  %idx.ext2203 = zext i32 %add2202 to i64
  %541 = add i64 0, %idx.ext2203
  store i8 %540, i8* %_buffer217
  %savedstack118 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %63), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer217, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %541, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %63, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %63, i8 -1, i64 1, i1 false), !dbg !406
  %542 = add i64 %541, 1
  %call.i117 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer217, i8* nonnull %63, i64 %542, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %63), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack118), !dbg !408
  %add2207 = add nsw i32 %ebpf_packetOffsetInBits.7, 16
  %543 = load i16, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 3), align 4, !tbaa !567
  %rev4006 = call i16 @llvm.bswap.i16(i16 %543)
  store i16 %rev4006, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 3), align 4, !tbaa !567
  %544 = trunc i16 %rev4006 to i8
  %div2227 = lshr i32 %add2207, 3
  %idx.ext2229 = zext i32 %div2227 to i64
  %545 = add i64 0, %idx.ext2229
  store i8 %544, i8* %_buffer136
  %savedstack120 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %62), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer136, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %545, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %62, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %62, i8 -1, i64 1, i1 false), !dbg !406
  %546 = add i64 %545, 1
  %call.i119 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer136, i8* nonnull %62, i64 %546, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %62), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack120), !dbg !408
  %547 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 2, i64 1, i32 3) to i8*), i64 1), align 1, !tbaa !569
  %add2238 = add nuw nsw i32 %div2227, 1
  %idx.ext2239 = zext i32 %add2238 to i64
  %548 = add i64 0, %idx.ext2239
  store i8 %547, i8* %_buffer31
  %savedstack122 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %61), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer31, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %548, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %61, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %61, i8 -1, i64 1, i1 false), !dbg !406
  %549 = add i64 %548, 1
  %call.i121 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer31, i8* nonnull %61, i64 %549, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %61), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack122), !dbg !408
  br label %if.end2244

if.end2244:                                       ; preds = %if.end2138, %if.end2128
  %ebpf_packetOffsetInBits.8 = phi i32 [ %add2131, %if.end2138 ], [ %ebpf_packetOffsetInBits.7, %if.end2128 ]
  %550 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 12), align 8, !tbaa !544
  %tobool2245 = icmp eq i8 %550, 0
  br i1 %tobool2245, label %if.end2588, label %if.then2246

if.then2246:                                      ; preds = %if.end2244
  %add2247 = add i32 %ebpf_packetOffsetInBits.8, 160
  %div2248 = lshr i32 %add2247, 3
  %idx.ext2249 = zext i32 %div2248 to i64
  %551 = add i64 0, %idx.ext2249
  %552 = icmp ugt i64 %551, %411
  br i1 %552, label %cleanup3166, label %if.end2254

if.end2254:                                       ; preds = %if.then2246
  %553 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 0), align 8, !tbaa !569
  %shl2257 = shl i8 %553, 4
  %div2259 = lshr i32 %ebpf_packetOffsetInBits.8, 3
  %idx.ext2261 = zext i32 %div2259 to i64
  %554 = add i64 0, %idx.ext2261
  store i8 %shl2257, i8* %_buffer87
  %savedstack124 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %60), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer87, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %554, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %60, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %60, i8 -1, i64 1, i1 false), !dbg !406
  %555 = add i64 %554, 1
  %call.i123 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer87, i8* nonnull %60, i64 %555, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %60), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack124), !dbg !408
  %add2265 = add i32 %ebpf_packetOffsetInBits.8, 4
  %556 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 1), align 1, !tbaa !569
  %div2268 = lshr i32 %add2265, 3
  %idx.ext2269 = zext i32 %div2268 to i64
  %557 = add i64 0, %idx.ext2269
  %558 = add i64 %557, 1
  %559 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer62, i64 %558, i64 1)
  %560 = load i8, i8* %_buffer62
  %561 = and i8 %560, -16
  %562 = and i8 %556, 15
  %or22814001 = or i8 %561, %562
  store i8 %or22814001, i8* %_buffer219
  %savedstack126 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %59), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer219, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %557, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %59, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %59, i8 -1, i64 1, i1 false), !dbg !406
  %563 = add i64 %557, 1
  %call.i125 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer219, i8* nonnull %59, i64 %563, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %59), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack126), !dbg !408
  %add2289 = add i32 %ebpf_packetOffsetInBits.8, 8
  %564 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 2), align 2, !tbaa !569
  %div2294 = lshr i32 %add2289, 3
  %idx.ext2296 = zext i32 %div2294 to i64
  %565 = add i64 0, %idx.ext2296
  store i8 %564, i8* %_buffer129
  %savedstack128 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %58), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer129, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %565, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %58, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %58, i8 -1, i64 1, i1 false), !dbg !406
  %566 = add i64 %565, 1
  %call.i127 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer129, i8* nonnull %58, i64 %566, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %58), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack128), !dbg !408
  %add2300 = add i32 %ebpf_packetOffsetInBits.8, 16
  %567 = load i16, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 3), align 4, !tbaa !535
  %rev4004 = call i16 @llvm.bswap.i16(i16 %567)
  store i16 %rev4004, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 3), align 4, !tbaa !535
  %568 = trunc i16 %rev4004 to i8
  %div2320 = lshr i32 %add2300, 3
  %idx.ext2322 = zext i32 %div2320 to i64
  %569 = add i64 0, %idx.ext2322
  store i8 %568, i8* %_buffer46
  %savedstack130 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %57), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer46, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %569, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %57, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %57, i8 -1, i64 1, i1 false), !dbg !406
  %570 = add i64 %569, 1
  %call.i129 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer46, i8* nonnull %57, i64 %570, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %57), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack130), !dbg !408
  %571 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 3) to i8*), i64 1), align 1, !tbaa !569
  %add2331 = add nuw nsw i32 %div2320, 1
  %idx.ext2332 = zext i32 %add2331 to i64
  %572 = add i64 0, %idx.ext2332
  store i8 %571, i8* %_buffer61
  %savedstack132 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %56), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer61, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %572, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %56, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %56, i8 -1, i64 1, i1 false), !dbg !406
  %573 = add i64 %572, 1
  %call.i131 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer61, i8* nonnull %56, i64 %573, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %56), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack132), !dbg !408
  %add2336 = add i32 %ebpf_packetOffsetInBits.8, 32
  %574 = load i16, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 4), align 2, !tbaa !536
  %rev4003 = call i16 @llvm.bswap.i16(i16 %574)
  store i16 %rev4003, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 4), align 2, !tbaa !536
  %575 = trunc i16 %rev4003 to i8
  %div2356 = lshr i32 %add2336, 3
  %idx.ext2358 = zext i32 %div2356 to i64
  %576 = add i64 0, %idx.ext2358
  store i8 %575, i8* %_buffer125
  %savedstack134 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %55), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer125, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %576, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %55, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %55, i8 -1, i64 1, i1 false), !dbg !406
  %577 = add i64 %576, 1
  %call.i133 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer125, i8* nonnull %55, i64 %577, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %55), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack134), !dbg !408
  %578 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 4) to i8*), i64 1), align 1, !tbaa !569
  %add2367 = add nuw nsw i32 %div2356, 1
  %idx.ext2368 = zext i32 %add2367 to i64
  %579 = add i64 0, %idx.ext2368
  store i8 %578, i8* %_buffer218
  %savedstack136 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %54), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer218, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %579, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %54, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %54, i8 -1, i64 1, i1 false), !dbg !406
  %580 = add i64 %579, 1
  %call.i135 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer218, i8* nonnull %54, i64 %580, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %54), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack136), !dbg !408
  %add2372 = add i32 %ebpf_packetOffsetInBits.8, 48
  %581 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 5), align 8, !tbaa !569
  %shl2375 = shl i8 %581, 5
  %div2377 = lshr i32 %add2372, 3
  %idx.ext2379 = zext i32 %div2377 to i64
  %582 = add i64 0, %idx.ext2379
  store i8 %shl2375, i8* %_buffer78
  %savedstack138 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %53), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer78, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %582, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %53, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %53, i8 -1, i64 1, i1 false), !dbg !406
  %583 = add i64 %582, 1
  %call.i137 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer78, i8* nonnull %53, i64 %583, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %53), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack138), !dbg !408
  %add2383 = add i32 %ebpf_packetOffsetInBits.8, 51
  %584 = load i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 6) to i8*), align 2, !tbaa !569
  %div2386 = lshr i32 %add2383, 3
  %idx.ext2387 = zext i32 %div2386 to i64
  %585 = add i64 0, %idx.ext2387
  %586 = add i64 %585, 1
  %587 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer214, i64 %586, i64 1)
  %588 = load i8, i8* %_buffer214
  %589 = and i8 %588, -8
  %590 = lshr i8 %584, 2
  %591 = and i8 %590, 7
  %or23994002 = or i8 %589, %591
  store i8 %or23994002, i8* %_buffer213
  %savedstack140 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %52), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer213, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %585, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %52, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %52, i8 -1, i64 1, i1 false), !dbg !406
  %592 = add i64 %585, 1
  %call.i139 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer213, i8* nonnull %52, i64 %592, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %52), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack140), !dbg !408
  %593 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 6) to i8*), i64 1), align 1, !tbaa !569
  %add2412 = add nuw nsw i32 %div2386, 1
  %idx.ext2413 = zext i32 %add2412 to i64
  %594 = add i64 0, %idx.ext2413
  store i8 %593, i8* %_buffer88
  %savedstack142 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %51), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer88, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %594, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %51, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %51, i8 -1, i64 1, i1 false), !dbg !406
  %595 = add i64 %594, 1
  %call.i141 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer88, i8* nonnull %51, i64 %595, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %51), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack142), !dbg !408
  %add2417 = add i32 %ebpf_packetOffsetInBits.8, 64
  %596 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 7), align 4, !tbaa !569
  %div2422 = lshr i32 %add2417, 3
  %idx.ext2424 = zext i32 %div2422 to i64
  %597 = add i64 0, %idx.ext2424
  store i8 %596, i8* %_buffer135
  %savedstack144 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %50), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer135, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %597, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %50, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %50, i8 -1, i64 1, i1 false), !dbg !406
  %598 = add i64 %597, 1
  %call.i143 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer135, i8* nonnull %50, i64 %598, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %50), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack144), !dbg !408
  %add2428 = add i32 %ebpf_packetOffsetInBits.8, 72
  %599 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 8), align 1, !tbaa !569
  %div2433 = lshr i32 %add2428, 3
  %idx.ext2435 = zext i32 %div2433 to i64
  %600 = add i64 0, %idx.ext2435
  store i8 %599, i8* %_buffer133
  %savedstack146 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %49), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer133, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %600, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %49, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %49, i8 -1, i64 1, i1 false), !dbg !406
  %601 = add i64 %600, 1
  %call.i145 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer133, i8* nonnull %49, i64 %601, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %49), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack146), !dbg !408
  %add2439 = add i32 %ebpf_packetOffsetInBits.8, 80
  %602 = load i16, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 9), align 2, !tbaa !541
  %rev = call i16 @llvm.bswap.i16(i16 %602)
  store i16 %rev, i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 9), align 2, !tbaa !541
  %603 = trunc i16 %rev to i8
  %div2459 = lshr i32 %add2439, 3
  %idx.ext2461 = zext i32 %div2459 to i64
  %604 = add i64 0, %idx.ext2461
  store i8 %603, i8* %_buffer101
  %savedstack148 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %48), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer101, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %604, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %48, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %48, i8 -1, i64 1, i1 false), !dbg !406
  %605 = add i64 %604, 1
  %call.i147 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer101, i8* nonnull %48, i64 %605, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %48), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack148), !dbg !408
  %606 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i16* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 9) to i8*), i64 1), align 1, !tbaa !569
  %add2470 = add nuw nsw i32 %div2459, 1
  %idx.ext2471 = zext i32 %add2470 to i64
  %607 = add i64 0, %idx.ext2471
  store i8 %606, i8* %_buffer86
  %savedstack150 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %47), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer86, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %607, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %47, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %47, i8 -1, i64 1, i1 false), !dbg !406
  %608 = add i64 %607, 1
  %call.i149 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer86, i8* nonnull %47, i64 %608, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %47), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack150), !dbg !408
  %add2475 = add i32 %ebpf_packetOffsetInBits.8, 96
  %609 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 10), align 8, !tbaa !542
  %or2487 = call i32 @llvm.bswap.i32(i32 %609)
  store i32 %or2487, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 10), align 8, !tbaa !542
  %610 = trunc i32 %or2487 to i8
  %div2495 = lshr i32 %add2475, 3
  %idx.ext2497 = zext i32 %div2495 to i64
  %611 = add i64 0, %idx.ext2497
  store i8 %610, i8* %_buffer121
  %savedstack152 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %46), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer121, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %611, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %46, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %46, i8 -1, i64 1, i1 false), !dbg !406
  %612 = add i64 %611, 1
  %call.i151 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer121, i8* nonnull %46, i64 %612, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %46), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack152), !dbg !408
  %613 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 10) to i8*), i64 1), align 1, !tbaa !569
  %add2506 = add nuw nsw i32 %div2495, 1
  %idx.ext2507 = zext i32 %add2506 to i64
  %614 = add i64 0, %idx.ext2507
  store i8 %613, i8* %_buffer72
  %savedstack154 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %45), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer72, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %614, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %45, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %45, i8 -1, i64 1, i1 false), !dbg !406
  %615 = add i64 %614, 1
  %call.i153 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer72, i8* nonnull %45, i64 %615, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %45), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack154), !dbg !408
  %616 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 10) to i8*), i64 2), align 2, !tbaa !569
  %add2516 = add nuw nsw i32 %div2495, 2
  %idx.ext2517 = zext i32 %add2516 to i64
  %617 = add i64 0, %idx.ext2517
  store i8 %616, i8* %_buffer221
  %savedstack156 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %44), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer221, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %617, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %44, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %44, i8 -1, i64 1, i1 false), !dbg !406
  %618 = add i64 %617, 1
  %call.i155 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer221, i8* nonnull %44, i64 %618, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %44), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack156), !dbg !408
  %619 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 10) to i8*), i64 3), align 1, !tbaa !569
  %add2526 = add nuw nsw i32 %div2495, 3
  %idx.ext2527 = zext i32 %add2526 to i64
  %620 = add i64 0, %idx.ext2527
  store i8 %619, i8* %_buffer45
  %savedstack158 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %43), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer45, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %620, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %43, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %43, i8 -1, i64 1, i1 false), !dbg !406
  %621 = add i64 %620, 1
  %call.i157 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer45, i8* nonnull %43, i64 %621, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %43), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack158), !dbg !408
  %add2531 = add i32 %ebpf_packetOffsetInBits.8, 128
  %622 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 11), align 4, !tbaa !543
  %or2543 = call i32 @llvm.bswap.i32(i32 %622)
  store i32 %or2543, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 11), align 4, !tbaa !543
  %623 = trunc i32 %or2543 to i8
  %div2551 = lshr i32 %add2531, 3
  %idx.ext2553 = zext i32 %div2551 to i64
  %624 = add i64 0, %idx.ext2553
  store i8 %623, i8* %_buffer111
  %savedstack160 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %42), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer111, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %624, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %42, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %42, i8 -1, i64 1, i1 false), !dbg !406
  %625 = add i64 %624, 1
  %call.i159 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer111, i8* nonnull %42, i64 %625, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %42), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack160), !dbg !408
  %626 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 11) to i8*), i64 1), align 1, !tbaa !569
  %add2562 = add nuw nsw i32 %div2551, 1
  %idx.ext2563 = zext i32 %add2562 to i64
  %627 = add i64 0, %idx.ext2563
  store i8 %626, i8* %_buffer106
  %savedstack162 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %41), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer106, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %627, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %41, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %41, i8 -1, i64 1, i1 false), !dbg !406
  %628 = add i64 %627, 1
  %call.i161 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer106, i8* nonnull %41, i64 %628, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %41), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack162), !dbg !408
  %629 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 11) to i8*), i64 2), align 2, !tbaa !569
  %add2572 = add nuw nsw i32 %div2551, 2
  %idx.ext2573 = zext i32 %add2572 to i64
  %630 = add i64 0, %idx.ext2573
  store i8 %629, i8* %_buffer209
  %savedstack164 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %40), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer209, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %630, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %40, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %40, i8 -1, i64 1, i1 false), !dbg !406
  %631 = add i64 %630, 1
  %call.i163 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer209, i8* nonnull %40, i64 %631, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %40), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack164), !dbg !408
  %632 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 3, i32 11) to i8*), i64 3), align 1, !tbaa !569
  %add2582 = add nuw nsw i32 %div2551, 3
  %idx.ext2583 = zext i32 %add2582 to i64
  %633 = add i64 0, %idx.ext2583
  store i8 %632, i8* %_buffer119
  %savedstack166 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %39), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer119, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %633, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %39, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %39, i8 -1, i64 1, i1 false), !dbg !406
  %634 = add i64 %633, 1
  %call.i165 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer119, i8* nonnull %39, i64 %634, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %39), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack166), !dbg !408
  br label %if.end2588

if.end2588:                                       ; preds = %if.end2254, %if.end2244
  %ebpf_packetOffsetInBits.9 = phi i32 [ %add2247, %if.end2254 ], [ %ebpf_packetOffsetInBits.8, %if.end2244 ]
  %635 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 5), align 1, !tbaa !550
  %tobool2589 = icmp eq i8 %635, 0
  br i1 %tobool2589, label %if.end2667, label %if.then2590

if.then2590:                                      ; preds = %if.end2588
  %add2591 = add i32 %ebpf_packetOffsetInBits.9, 32
  %div2592 = lshr i32 %add2591, 3
  %idx.ext2593 = zext i32 %div2592 to i64
  %636 = add i64 0, %idx.ext2593
  %637 = icmp ugt i64 %636, %411
  br i1 %637, label %cleanup3166, label %if.end2598

if.end2598:                                       ; preds = %if.then2590
  %638 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 0), align 4, !tbaa !569
  %div2603 = lshr i32 %ebpf_packetOffsetInBits.9, 3
  %idx.ext2605 = zext i32 %div2603 to i64
  %639 = add i64 0, %idx.ext2605
  store i8 %638, i8* %_buffer134
  %savedstack168 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %38), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer134, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %639, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %38, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %38, i8 -1, i64 1, i1 false), !dbg !406
  %640 = add i64 %639, 1
  %call.i167 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer134, i8* nonnull %38, i64 %640, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %38), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack168), !dbg !408
  %add2609 = add i32 %ebpf_packetOffsetInBits.9, 8
  %641 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 1), align 1, !tbaa !569
  %div2614 = lshr i32 %add2609, 3
  %idx.ext2616 = zext i32 %div2614 to i64
  %642 = add i64 0, %idx.ext2616
  store i8 %641, i8* %_buffer130
  %savedstack170 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %37), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer130, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %642, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %37, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %37, i8 -1, i64 1, i1 false), !dbg !406
  %643 = add i64 %642, 1
  %call.i169 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer130, i8* nonnull %37, i64 %643, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %37), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack170), !dbg !408
  %add2620 = add i32 %ebpf_packetOffsetInBits.9, 16
  %644 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 2), align 2, !tbaa !569
  %div2625 = lshr i32 %add2620, 3
  %idx.ext2627 = zext i32 %div2625 to i64
  %645 = add i64 0, %idx.ext2627
  store i8 %644, i8* %_buffer128
  %savedstack172 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %36), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer128, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %645, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %36, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %36, i8 -1, i64 1, i1 false), !dbg !406
  %646 = add i64 %645, 1
  %call.i171 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer128, i8* nonnull %36, i64 %646, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %36), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack172), !dbg !408
  %add2631 = add i32 %ebpf_packetOffsetInBits.9, 24
  %647 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 3), align 1, !tbaa !569
  %shl2634 = shl i8 %647, 4
  %div2636 = lshr i32 %add2631, 3
  %idx.ext2638 = zext i32 %div2636 to i64
  %648 = add i64 0, %idx.ext2638
  store i8 %shl2634, i8* %_buffer127
  %savedstack174 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %35), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer127, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %648, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %35, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %35, i8 -1, i64 1, i1 false), !dbg !406
  %649 = add i64 %648, 1
  %call.i173 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer127, i8* nonnull %35, i64 %649, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %35), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack174), !dbg !408
  %add2642 = add i32 %ebpf_packetOffsetInBits.9, 28
  %650 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 4, i32 4), align 4, !tbaa !569
  %div2645 = lshr i32 %add2642, 3
  %idx.ext2646 = zext i32 %div2645 to i64
  %651 = add i64 0, %idx.ext2646
  %652 = add i64 %651, 1
  %653 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer124, i64 %652, i64 1)
  %654 = load i8, i8* %_buffer124
  %655 = and i8 %654, -16
  %656 = and i8 %650, 15
  %or26584000 = or i8 %655, %656
  store i8 %or26584000, i8* %_buffer123
  %savedstack176 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %34), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer123, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %651, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %34, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %34, i8 -1, i64 1, i1 false), !dbg !406
  %657 = add i64 %651, 1
  %call.i175 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer123, i8* nonnull %34, i64 %657, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %34), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack176), !dbg !408
  br label %if.end2667

if.end2667:                                       ; preds = %if.end2598, %if.end2588
  %ebpf_packetOffsetInBits.10 = phi i32 [ %add2591, %if.end2598 ], [ %ebpf_packetOffsetInBits.9, %if.end2588 ]
  %658 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 2), align 4, !tbaa !553
  %tobool2668 = icmp eq i8 %658, 0
  br i1 %tobool2668, label %if.end2790, label %if.then2669

if.then2669:                                      ; preds = %if.end2667
  %add2670 = add i32 %ebpf_packetOffsetInBits.10, 64
  %div2671 = lshr i32 %add2670, 3
  %idx.ext2672 = zext i32 %div2671 to i64
  %659 = add i64 0, %idx.ext2672
  %660 = icmp ugt i64 %659, %411
  br i1 %660, label %cleanup3166, label %if.end2677

if.end2677:                                       ; preds = %if.then2669
  %661 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 0), align 4, !tbaa !551
  %or2689 = call i32 @llvm.bswap.i32(i32 %661)
  store i32 %or2689, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 0), align 4, !tbaa !551
  %662 = trunc i32 %or2689 to i8
  %div2697 = lshr i32 %ebpf_packetOffsetInBits.10, 3
  %idx.ext2699 = zext i32 %div2697 to i64
  %663 = add i64 0, %idx.ext2699
  store i8 %662, i8* %_buffer100
  %savedstack178 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %33), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer100, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %663, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %33, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %33, i8 -1, i64 1, i1 false), !dbg !406
  %664 = add i64 %663, 1
  %call.i177 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer100, i8* nonnull %33, i64 %664, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %33), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack178), !dbg !408
  %665 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 0) to i8*), i64 1), align 1, !tbaa !569
  %add2708 = add nuw nsw i32 %div2697, 1
  %idx.ext2709 = zext i32 %add2708 to i64
  %666 = add i64 0, %idx.ext2709
  store i8 %665, i8* %_buffer85
  %savedstack180 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %32), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer85, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %666, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %32, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %32, i8 -1, i64 1, i1 false), !dbg !406
  %667 = add i64 %666, 1
  %call.i179 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer85, i8* nonnull %32, i64 %667, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %32), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack180), !dbg !408
  %668 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 0) to i8*), i64 2), align 2, !tbaa !569
  %add2718 = add nuw nsw i32 %div2697, 2
  %idx.ext2719 = zext i32 %add2718 to i64
  %669 = add i64 0, %idx.ext2719
  store i8 %668, i8* %_buffer115
  %savedstack182 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %31), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer115, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %669, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %31, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %31, i8 -1, i64 1, i1 false), !dbg !406
  %670 = add i64 %669, 1
  %call.i181 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer115, i8* nonnull %31, i64 %670, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %31), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack182), !dbg !408
  %671 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 0) to i8*), i64 3), align 1, !tbaa !569
  %add2728 = add nuw nsw i32 %div2697, 3
  %idx.ext2729 = zext i32 %add2728 to i64
  %672 = add i64 0, %idx.ext2729
  store i8 %671, i8* %_buffer69
  %savedstack184 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %30), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer69, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %672, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %30, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %30, i8 -1, i64 1, i1 false), !dbg !406
  %673 = add i64 %672, 1
  %call.i183 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer69, i8* nonnull %30, i64 %673, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %30), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack184), !dbg !408
  %add2733 = add i32 %ebpf_packetOffsetInBits.10, 32
  %674 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 1), align 4, !tbaa !552
  %or2745 = call i32 @llvm.bswap.i32(i32 %674)
  store i32 %or2745, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 1), align 4, !tbaa !552
  %675 = trunc i32 %or2745 to i8
  %div2753 = lshr i32 %add2733, 3
  %idx.ext2755 = zext i32 %div2753 to i64
  %676 = add i64 0, %idx.ext2755
  store i8 %675, i8* %_buffer107
  %savedstack186 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %29), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer107, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %676, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %29, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %29, i8 -1, i64 1, i1 false), !dbg !406
  %677 = add i64 %676, 1
  %call.i185 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer107, i8* nonnull %29, i64 %677, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %29), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack186), !dbg !408
  %678 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 1) to i8*), i64 1), align 1, !tbaa !569
  %add2764 = add nuw nsw i32 %div2753, 1
  %idx.ext2765 = zext i32 %add2764 to i64
  %679 = add i64 0, %idx.ext2765
  store i8 %678, i8* %_buffer215
  %savedstack188 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %28), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer215, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %679, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %28, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %28, i8 -1, i64 1, i1 false), !dbg !406
  %680 = add i64 %679, 1
  %call.i187 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer215, i8* nonnull %28, i64 %680, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %28), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack188), !dbg !408
  %681 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 1) to i8*), i64 2), align 2, !tbaa !569
  %add2774 = add nuw nsw i32 %div2753, 2
  %idx.ext2775 = zext i32 %add2774 to i64
  %682 = add i64 0, %idx.ext2775
  store i8 %681, i8* %_buffer108
  %savedstack190 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %27), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer108, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %682, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %27, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %27, i8 -1, i64 1, i1 false), !dbg !406
  %683 = add i64 %682, 1
  %call.i189 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer108, i8* nonnull %27, i64 %683, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %27), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack190), !dbg !408
  %684 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 5, i32 1) to i8*), i64 3), align 1, !tbaa !569
  %add2784 = add nuw nsw i32 %div2753, 3
  %idx.ext2785 = zext i32 %add2784 to i64
  %685 = add i64 0, %idx.ext2785
  store i8 %684, i8* %_buffer102
  %savedstack192 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %26), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer102, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %685, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %26, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %26, i8 -1, i64 1, i1 false), !dbg !406
  %686 = add i64 %685, 1
  %call.i191 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer102, i8* nonnull %26, i64 %686, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %26), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack192), !dbg !408
  br label %if.end2790

if.end2790:                                       ; preds = %if.end2677, %if.end2667
  %ebpf_packetOffsetInBits.11 = phi i32 [ %add2670, %if.end2677 ], [ %ebpf_packetOffsetInBits.10, %if.end2667 ]
  %687 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 2), align 8, !tbaa !556
  %tobool2791 = icmp eq i8 %687, 0
  br i1 %tobool2791, label %if.end2913, label %if.then2792

if.then2792:                                      ; preds = %if.end2790
  %add2793 = add i32 %ebpf_packetOffsetInBits.11, 64
  %div2794 = lshr i32 %add2793, 3
  %idx.ext2795 = zext i32 %div2794 to i64
  %688 = add i64 0, %idx.ext2795
  %689 = icmp ugt i64 %688, %411
  br i1 %689, label %cleanup3166, label %if.end2800

if.end2800:                                       ; preds = %if.then2792
  %690 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 0), align 8, !tbaa !554
  %or2812 = call i32 @llvm.bswap.i32(i32 %690)
  store i32 %or2812, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 0), align 8, !tbaa !554
  %691 = trunc i32 %or2812 to i8
  %div2820 = lshr i32 %ebpf_packetOffsetInBits.11, 3
  %idx.ext2822 = zext i32 %div2820 to i64
  %692 = add i64 0, %idx.ext2822
  store i8 %691, i8* %_buffer97
  %savedstack194 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %25), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer97, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %692, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %25, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %25, i8 -1, i64 1, i1 false), !dbg !406
  %693 = add i64 %692, 1
  %call.i193 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer97, i8* nonnull %25, i64 %693, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %25), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack194), !dbg !408
  %694 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 0) to i8*), i64 1), align 1, !tbaa !569
  %add2831 = add nuw nsw i32 %div2820, 1
  %idx.ext2832 = zext i32 %add2831 to i64
  %695 = add i64 0, %idx.ext2832
  store i8 %694, i8* %_buffer93
  %savedstack196 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %24), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer93, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %695, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %24, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %24, i8 -1, i64 1, i1 false), !dbg !406
  %696 = add i64 %695, 1
  %call.i195 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer93, i8* nonnull %24, i64 %696, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %24), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack196), !dbg !408
  %697 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 0) to i8*), i64 2), align 2, !tbaa !569
  %add2841 = add nuw nsw i32 %div2820, 2
  %idx.ext2842 = zext i32 %add2841 to i64
  %698 = add i64 0, %idx.ext2842
  store i8 %697, i8* %_buffer91
  %savedstack198 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %23), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer91, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %698, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %23, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %23, i8 -1, i64 1, i1 false), !dbg !406
  %699 = add i64 %698, 1
  %call.i197 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer91, i8* nonnull %23, i64 %699, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %23), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack198), !dbg !408
  %700 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 0) to i8*), i64 3), align 1, !tbaa !569
  %add2851 = add nuw nsw i32 %div2820, 3
  %idx.ext2852 = zext i32 %add2851 to i64
  %701 = add i64 0, %idx.ext2852
  store i8 %700, i8* %_buffer89
  %savedstack200 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %22), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer89, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %701, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %22, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %22, i8 -1, i64 1, i1 false), !dbg !406
  %702 = add i64 %701, 1
  %call.i199 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer89, i8* nonnull %22, i64 %702, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %22), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack200), !dbg !408
  %add2856 = add i32 %ebpf_packetOffsetInBits.11, 32
  %703 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 1), align 4, !tbaa !555
  %or2868 = call i32 @llvm.bswap.i32(i32 %703)
  store i32 %or2868, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 1), align 4, !tbaa !555
  %704 = trunc i32 %or2868 to i8
  %div2876 = lshr i32 %add2856, 3
  %idx.ext2878 = zext i32 %div2876 to i64
  %705 = add i64 0, %idx.ext2878
  store i8 %704, i8* %_buffer211
  %savedstack202 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %21), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer211, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %705, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %21, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %21, i8 -1, i64 1, i1 false), !dbg !406
  %706 = add i64 %705, 1
  %call.i201 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer211, i8* nonnull %21, i64 %706, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %21), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack202), !dbg !408
  %707 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 1) to i8*), i64 1), align 1, !tbaa !569
  %add2887 = add nuw nsw i32 %div2876, 1
  %idx.ext2888 = zext i32 %add2887 to i64
  %708 = add i64 0, %idx.ext2888
  store i8 %707, i8* %_buffer48
  %savedstack204 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %20), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer48, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %708, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %20, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %20, i8 -1, i64 1, i1 false), !dbg !406
  %709 = add i64 %708, 1
  %call.i203 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer48, i8* nonnull %20, i64 %709, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %20), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack204), !dbg !408
  %710 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 1) to i8*), i64 2), align 2, !tbaa !569
  %add2897 = add nuw nsw i32 %div2876, 2
  %idx.ext2898 = zext i32 %add2897 to i64
  %711 = add i64 0, %idx.ext2898
  store i8 %710, i8* %_buffer34
  %savedstack206 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %19), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer34, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %711, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %19, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %19, i8 -1, i64 1, i1 false), !dbg !406
  %712 = add i64 %711, 1
  %call.i205 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer34, i8* nonnull %19, i64 %712, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %19), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack206), !dbg !408
  %713 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 6, i32 1) to i8*), i64 3), align 1, !tbaa !569
  %add2907 = add nuw nsw i32 %div2876, 3
  %idx.ext2908 = zext i32 %add2907 to i64
  %714 = add i64 0, %idx.ext2908
  store i8 %713, i8* %_buffer96
  %savedstack208 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %18), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer96, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %714, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %18, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %18, i8 -1, i64 1, i1 false), !dbg !406
  %715 = add i64 %714, 1
  %call.i207 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer96, i8* nonnull %18, i64 %715, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %18), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack208), !dbg !408
  br label %if.end2913

if.end2913:                                       ; preds = %if.end2800, %if.end2790
  %ebpf_packetOffsetInBits.12 = phi i32 [ %add2793, %if.end2800 ], [ %ebpf_packetOffsetInBits.11, %if.end2790 ]
  %716 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 2), align 4, !tbaa !559
  %tobool2914 = icmp eq i8 %716, 0
  br i1 %tobool2914, label %if.end3036, label %if.then2915

if.then2915:                                      ; preds = %if.end2913
  %add2916 = add i32 %ebpf_packetOffsetInBits.12, 64
  %div2917 = lshr i32 %add2916, 3
  %idx.ext2918 = zext i32 %div2917 to i64
  %717 = add i64 0, %idx.ext2918
  %718 = icmp ugt i64 %717, %411
  br i1 %718, label %cleanup3166, label %if.end2923

if.end2923:                                       ; preds = %if.then2915
  %719 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 0), align 4, !tbaa !557
  %or2935 = call i32 @llvm.bswap.i32(i32 %719)
  store i32 %or2935, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 0), align 4, !tbaa !557
  %720 = trunc i32 %or2935 to i8
  %div2943 = lshr i32 %ebpf_packetOffsetInBits.12, 3
  %idx.ext2945 = zext i32 %div2943 to i64
  %721 = add i64 0, %idx.ext2945
  store i8 %720, i8* %_buffer117
  %savedstack210 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %17), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer117, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %721, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %17, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %17, i8 -1, i64 1, i1 false), !dbg !406
  %722 = add i64 %721, 1
  %call.i209 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer117, i8* nonnull %17, i64 %722, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %17), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack210), !dbg !408
  %723 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 0) to i8*), i64 1), align 1, !tbaa !569
  %add2954 = add nuw nsw i32 %div2943, 1
  %idx.ext2955 = zext i32 %add2954 to i64
  %724 = add i64 0, %idx.ext2955
  store i8 %723, i8* %_buffer70
  %savedstack212 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %16), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer70, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %724, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %16, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %16, i8 -1, i64 1, i1 false), !dbg !406
  %725 = add i64 %724, 1
  %call.i211 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer70, i8* nonnull %16, i64 %725, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %16), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack212), !dbg !408
  %726 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 0) to i8*), i64 2), align 2, !tbaa !569
  %add2964 = add nuw nsw i32 %div2943, 2
  %idx.ext2965 = zext i32 %add2964 to i64
  %727 = add i64 0, %idx.ext2965
  store i8 %726, i8* %_buffer220
  %savedstack214 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %15), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer220, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %727, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %15, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %15, i8 -1, i64 1, i1 false), !dbg !406
  %728 = add i64 %727, 1
  %call.i213 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer220, i8* nonnull %15, i64 %728, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %15), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack214), !dbg !408
  %729 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 0) to i8*), i64 3), align 1, !tbaa !569
  %add2974 = add nuw nsw i32 %div2943, 3
  %idx.ext2975 = zext i32 %add2974 to i64
  %730 = add i64 0, %idx.ext2975
  store i8 %729, i8* %_buffer79
  %savedstack216 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %14), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer79, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %730, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %14, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %14, i8 -1, i64 1, i1 false), !dbg !406
  %731 = add i64 %730, 1
  %call.i215 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer79, i8* nonnull %14, i64 %731, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %14), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack216), !dbg !408
  %add2979 = add i32 %ebpf_packetOffsetInBits.12, 32
  %732 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 1), align 4, !tbaa !558
  %or2991 = call i32 @llvm.bswap.i32(i32 %732)
  store i32 %or2991, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 1), align 4, !tbaa !558
  %733 = trunc i32 %or2991 to i8
  %div2999 = lshr i32 %add2979, 3
  %idx.ext3001 = zext i32 %div2999 to i64
  %734 = add i64 0, %idx.ext3001
  store i8 %733, i8* %_buffer109
  %savedstack218 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %13), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer109, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %734, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %13, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %13, i8 -1, i64 1, i1 false), !dbg !406
  %735 = add i64 %734, 1
  %call.i217 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer109, i8* nonnull %13, i64 %735, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %13), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack218), !dbg !408
  %736 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 1) to i8*), i64 1), align 1, !tbaa !569
  %add3010 = add nuw nsw i32 %div2999, 1
  %idx.ext3011 = zext i32 %add3010 to i64
  %737 = add i64 0, %idx.ext3011
  store i8 %736, i8* %_buffer103
  %savedstack220 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %12), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer103, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %737, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %12, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %12, i8 -1, i64 1, i1 false), !dbg !406
  %738 = add i64 %737, 1
  %call.i219 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer103, i8* nonnull %12, i64 %738, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %12), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack220), !dbg !408
  %739 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 1) to i8*), i64 2), align 2, !tbaa !569
  %add3020 = add nuw nsw i32 %div2999, 2
  %idx.ext3021 = zext i32 %add3020 to i64
  %740 = add i64 0, %idx.ext3021
  store i8 %739, i8* %_buffer137
  %savedstack222 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %11), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer137, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %740, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %11, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %11, i8 -1, i64 1, i1 false), !dbg !406
  %741 = add i64 %740, 1
  %call.i221 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer137, i8* nonnull %11, i64 %741, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %11), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack222), !dbg !408
  %742 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 7, i32 1) to i8*), i64 3), align 1, !tbaa !569
  %add3030 = add nuw nsw i32 %div2999, 3
  %idx.ext3031 = zext i32 %add3030 to i64
  %743 = add i64 0, %idx.ext3031
  store i8 %742, i8* %_buffer116
  %savedstack224 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %10), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer116, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %743, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %10, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %10, i8 -1, i64 1, i1 false), !dbg !406
  %744 = add i64 %743, 1
  %call.i223 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer116, i8* nonnull %10, i64 %744, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %10), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack224), !dbg !408
  br label %if.end3036

if.end3036:                                       ; preds = %if.end2923, %if.end2913
  %ebpf_packetOffsetInBits.13 = phi i32 [ %add2916, %if.end2923 ], [ %ebpf_packetOffsetInBits.12, %if.end2913 ]
  %745 = load i8, i8* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 2), align 8, !tbaa !562
  %tobool3037 = icmp eq i8 %745, 0
  br i1 %tobool3037, label %ebpf_end, label %if.then3038

if.then3038:                                      ; preds = %if.end3036
  %add3039 = add i32 %ebpf_packetOffsetInBits.13, 64
  %div3040 = lshr i32 %add3039, 3
  %idx.ext3041 = zext i32 %div3040 to i64
  %746 = add i64 0, %idx.ext3041
  %747 = icmp ugt i64 %746, %411
  br i1 %747, label %cleanup3166, label %if.end3046

if.end3046:                                       ; preds = %if.then3038
  %748 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 0), align 8, !tbaa !560
  %or3058 = call i32 @llvm.bswap.i32(i32 %748)
  store i32 %or3058, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 0), align 8, !tbaa !560
  %749 = trunc i32 %or3058 to i8
  %div3066 = lshr i32 %ebpf_packetOffsetInBits.13, 3
  %idx.ext3068 = zext i32 %div3066 to i64
  %750 = add i64 0, %idx.ext3068
  store i8 %749, i8* %_buffer113
  %savedstack226 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %9), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer113, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %750, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %9, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %9, i8 -1, i64 1, i1 false), !dbg !406
  %751 = add i64 %750, 1
  %call.i225 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer113, i8* nonnull %9, i64 %751, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %9), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack226), !dbg !408
  %752 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 0) to i8*), i64 1), align 1, !tbaa !569
  %add3077 = add nuw nsw i32 %div3066, 1
  %idx.ext3078 = zext i32 %add3077 to i64
  %753 = add i64 0, %idx.ext3078
  store i8 %752, i8* %_buffer138
  %savedstack228 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %8), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer138, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %753, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %8, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %8, i8 -1, i64 1, i1 false), !dbg !406
  %754 = add i64 %753, 1
  %call.i227 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer138, i8* nonnull %8, i64 %754, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %8), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack228), !dbg !408
  %755 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 0) to i8*), i64 2), align 2, !tbaa !569
  %add3087 = add nuw nsw i32 %div3066, 2
  %idx.ext3088 = zext i32 %add3087 to i64
  %756 = add i64 0, %idx.ext3088
  store i8 %755, i8* %_buffer120
  %savedstack230 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %7), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer120, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %756, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %7, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %7, i8 -1, i64 1, i1 false), !dbg !406
  %757 = add i64 %756, 1
  %call.i229 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer120, i8* nonnull %7, i64 %757, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %7), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack230), !dbg !408
  %758 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 0) to i8*), i64 3), align 1, !tbaa !569
  %add3097 = add nuw nsw i32 %div3066, 3
  %idx.ext3098 = zext i32 %add3097 to i64
  %759 = add i64 0, %idx.ext3098
  store i8 %758, i8* %_buffer71
  %savedstack232 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %6), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer71, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %759, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %6, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %6, i8 -1, i64 1, i1 false), !dbg !406
  %760 = add i64 %759, 1
  %call.i231 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer71, i8* nonnull %6, i64 %760, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %6), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack232), !dbg !408
  %add3102 = add i32 %ebpf_packetOffsetInBits.13, 32
  %761 = load i32, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 1), align 4, !tbaa !561
  %or3114 = call i32 @llvm.bswap.i32(i32 %761)
  store i32 %or3114, i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 1), align 4, !tbaa !561
  %762 = trunc i32 %or3114 to i8
  %div3122 = lshr i32 %add3102, 3
  %idx.ext3124 = zext i32 %div3122 to i64
  %763 = add i64 0, %idx.ext3124
  store i8 %762, i8* %_buffer36
  %savedstack234 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %5), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer36, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %763, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %5, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %5, i8 -1, i64 1, i1 false), !dbg !406
  %764 = add i64 %763, 1
  %call.i233 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer36, i8* nonnull %5, i64 %764, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %5), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack234), !dbg !408
  %765 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 1) to i8*), i64 1), align 1, !tbaa !569
  %add3133 = add nuw nsw i32 %div3122, 1
  %idx.ext3134 = zext i32 %add3133 to i64
  %766 = add i64 0, %idx.ext3134
  store i8 %765, i8* %_buffer52
  %savedstack236 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %4), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer52, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %766, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %4, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %4, i8 -1, i64 1, i1 false), !dbg !406
  %767 = add i64 %766, 1
  %call.i235 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer52, i8* nonnull %4, i64 %767, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %4), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack236), !dbg !408
  %768 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 1) to i8*), i64 2), align 2, !tbaa !569
  %add3143 = add nuw nsw i32 %div3122, 2
  %idx.ext3144 = zext i32 %add3143 to i64
  %769 = add i64 0, %idx.ext3144
  store i8 %768, i8* %_buffer110
  %savedstack238 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %3), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer110, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %769, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %3, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %3, i8 -1, i64 1, i1 false), !dbg !406
  %770 = add i64 %769, 1
  %call.i237 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer110, i8* nonnull %3, i64 %770, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %3), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack238), !dbg !408
  %771 = load i8, i8* getelementptr inbounds (i8, i8* bitcast (i32* getelementptr inbounds (%struct.headers, %struct.headers* @hdr, i64 0, i32 8, i32 1) to i8*), i64 3), align 1, !tbaa !569
  %add3153 = add nuw nsw i32 %div3122, 3
  %idx.ext3154 = zext i32 %add3153 to i64
  %772 = add i64 0, %idx.ext3154
  store i8 %771, i8* %_buffer105
  %savedstack240 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %2), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer105, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %772, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %2, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %2, i8 -1, i64 1, i1 false), !dbg !406
  %773 = add i64 %772, 1
  %call.i239 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer105, i8* nonnull %2, i64 %773, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %2), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack240), !dbg !408
  br label %ebpf_end

ebpf_end:                                         ; preds = %if.end3046, %if.end3036
  %774 = bitcast i32* %output_port to i8*
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %nt_ctx, metadata !329, metadata !DIExpression()), !dbg !340
  call void @llvm.dbg.value(metadata i16 10, metadata !330, metadata !DIExpression()), !dbg !341
  call void @llvm.dbg.value(metadata i32 3, metadata !331, metadata !DIExpression()), !dbg !342
  call void @llvm.dbg.value(metadata i8* %143, metadata !332, metadata !DIExpression()), !dbg !343
  call void @llvm.dbg.value(metadata i64 4, metadata !333, metadata !DIExpression()), !dbg !344
  call void @llvm.dbg.value(metadata i8* %774, metadata !334, metadata !DIExpression()), !dbg !345
  call void @llvm.dbg.value(metadata i8* null, metadata !335, metadata !DIExpression()), !dbg !346
  call void @llvm.dbg.value(metadata i8* getelementptr inbounds ([1 x i8], [1 x i8]* @onemask1, i32 0, i32 0), metadata !336, metadata !DIExpression()), !dbg !347
  call void @llvm.dbg.value(metadata i64 0, metadata !337, metadata !DIExpression()), !dbg !348
  call void @llvm.dbg.value(metadata i64 4, metadata !338, metadata !DIExpression()), !dbg !349
  %call.i241 = call i64 @nanotube_map_op(%struct.nanotube_context* %nt_ctx, i16 zeroext 10, i32 3, i8* %143, i64 4, i8* %774, i8* null, i8* getelementptr inbounds ([1 x i8], [1 x i8]* @onemask1, i32 0, i32 0), i64 0, i64 4), !dbg !350
  call void @llvm.dbg.value(metadata i64 %call.i241, metadata !339, metadata !DIExpression()), !dbg !351
  %cmp.i = icmp eq i64 %call.i241, 4, !dbg !352
  %..i = select i1 %cmp.i, i32 0, i32 -22, !dbg !354
  %775 = load i32, i32* %output_action, align 4, !tbaa !577
  br label %cleanup3166

cleanup3166:                                      ; preds = %ebpf_end, %if.then3038, %if.then2915, %if.then2792, %if.then2669, %if.then2590, %if.then2246, %if.then2130, %if.then2015, %if.then1846, %if.then1557, %cleanup1494.thread, %cleanup1468.thread, %cleanup1437.thread, %cleanup1408.thread, %cleanup1374.thread, %if.end820, %if.end701, %if.end701, %if.end701, %if.end701, %if.end701, %parse_vlan1, %parse_vlan, %parse_ipv4_rfc781_3, %parse_ipv4_rfc781_2, %parse_ipv4_rfc781_1, %parse_ipv4_rfc781_0, %if.end, %parse_ipv4, %entry
  %retval.10 = phi i32 [ %775, %ebpf_end ], [ 0, %if.end820 ], [ 0, %entry ], [ 0, %parse_vlan1 ], [ 0, %parse_vlan ], [ 0, %parse_ipv4_rfc781_3 ], [ 0, %parse_ipv4_rfc781_2 ], [ 0, %parse_ipv4_rfc781_1 ], [ 0, %parse_ipv4_rfc781_0 ], [ 0, %if.end ], [ 0, %parse_ipv4 ], [ 0, %if.then1557 ], [ 0, %if.then1846 ], [ 0, %if.then2015 ], [ 0, %if.then2130 ], [ 0, %if.then2246 ], [ 0, %if.then2590 ], [ 0, %if.then2669 ], [ 0, %if.then2792 ], [ 0, %if.then2915 ], [ 0, %if.then3038 ], [ 0, %if.end701 ], [ 0, %if.end701 ], [ 0, %if.end701 ], [ 0, %if.end701 ], [ 0, %if.end701 ], [ 0, %cleanup1374.thread ], [ 0, %cleanup1408.thread ], [ 0, %cleanup1437.thread ], [ 0, %cleanup1468.thread ], [ 0, %cleanup1494.thread ]
  call void @llvm.lifetime.end.p0i8(i64 8, i8* nonnull %144) #5
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %143) #5
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !376, metadata !DIExpression()), !dbg !378
  call void @llvm.dbg.value(metadata i32 %retval.10, metadata !377, metadata !DIExpression()), !dbg !379
  %cond.i242 = icmp eq i32 %retval.10, 2, !dbg !380
  br i1 %cond.i242, label %sw.bb.i, label %sw.default.i, !dbg !380

sw.bb.i:                                          ; preds = %cleanup3166
  %776 = alloca i8, !dbg !381
  store i8 0, i8* %776, !dbg !381
  %savedstack19 = call i8* @llvm.stacksave(), !dbg !588
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %1), !dbg !588
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !588
  call void @llvm.dbg.value(metadata i8* %776, metadata !392, metadata !DIExpression()), !dbg !589
  call void @llvm.dbg.value(metadata i64 0, metadata !393, metadata !DIExpression()), !dbg !590
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !591
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !592
  call void @llvm.dbg.value(metadata i8* %1, metadata !396, metadata !DIExpression()), !dbg !593
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %1, i8 -1, i64 1, i1 false), !dbg !594
  %call.i18 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %776, i8* nonnull %1, i64 0, i64 1), !dbg !595
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %1), !dbg !596
  call void @llvm.stackrestore(i8* %savedstack19), !dbg !596
  br label %packet_handle_xdp_result.exit, !dbg !383

sw.default.i:                                     ; preds = %cleanup3166
  %777 = alloca i8, !dbg !384
  store i8 -1, i8* %777, !dbg !384
  %savedstack21 = call i8* @llvm.stacksave(), !dbg !597
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %0), !dbg !597
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !597
  call void @llvm.dbg.value(metadata i8* %777, metadata !392, metadata !DIExpression()), !dbg !598
  call void @llvm.dbg.value(metadata i64 0, metadata !393, metadata !DIExpression()), !dbg !599
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !600
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !601
  call void @llvm.dbg.value(metadata i8* %0, metadata !396, metadata !DIExpression()), !dbg !602
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %0, i8 -1, i64 1, i1 false), !dbg !603
  %call.i20 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %777, i8* nonnull %0, i64 0, i64 1), !dbg !604
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %0), !dbg !605
  call void @llvm.stackrestore(i8* %savedstack21), !dbg !605
  br label %packet_handle_xdp_result.exit, !dbg !385

packet_handle_xdp_result.exit:                    ; preds = %sw.default.i, %sw.bb.i
  ret i32 0
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { alwaysinline uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { alwaysinline norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nounwind }
attributes #6 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0, !0, !0, !0}
!llvm.module.flags = !{!1, !2, !3}
!llvm.dbg.cu = !{!4, !27, !278}

!0 = !{!"clang version 8.0.0 "}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 2, !"Dwarf Version", i32 4}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !5, producer: "clang version 8.0.0 ", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6, globals: !17, nameTableKind: None)
!5 = !DIFile(filename: "libnt/ebpf_nt_adapter.cpp", directory: "/home/stephand/20-Projects/2019-12_eBPF_proto/nanotube")
!6 = !{!7}
!7 = distinct !DICompositeType(tag: DW_TAG_enumeration_type, name: "map_access_t", file: !8, line: 396, baseType: !9, size: 32, elements: !10, identifier: "_ZTS12map_access_t")
!8 = !DIFile(filename: "include/nanotube_api.h", directory: "/home/stephand/20-Projects/2019-12_eBPF_proto/nanotube")
!9 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!10 = !{!11, !12, !13, !14, !15, !16}
!11 = !DIEnumerator(name: "NANOTUBE_MAP_READ", value: 0, isUnsigned: true)
!12 = !DIEnumerator(name: "NANOTUBE_MAP_INSERT", value: 1, isUnsigned: true)
!13 = !DIEnumerator(name: "NANOTUBE_MAP_UPDATE", value: 2, isUnsigned: true)
!14 = !DIEnumerator(name: "NANOTUBE_MAP_WRITE", value: 3, isUnsigned: true)
!15 = !DIEnumerator(name: "NANOTUBE_MAP_REMOVE", value: 4, isUnsigned: true)
!16 = !DIEnumerator(name: "NANOTUBE_MAP_NOP", value: 5, isUnsigned: true)
!17 = !{!18}
!18 = !DIGlobalVariableExpression(var: !19, expr: !DIExpression(DW_OP_constu, 255, DW_OP_stack_value))
!19 = distinct !DIGlobalVariable(name: "NANOTUBE_PORT_DISCARD", scope: !4, file: !8, line: 64, type: !20, isLocal: true, isDefinition: true)
!20 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !21)
!21 = !DIDerivedType(tag: DW_TAG_typedef, name: "nanotube_packet_port_t", file: !8, line: 62, baseType: !22)
!22 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint8_t", file: !23, line: 24, baseType: !24)
!23 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/stdint-uintn.h", directory: "")
!24 = !DIDerivedType(tag: DW_TAG_typedef, name: "__uint8_t", file: !25, line: 37, baseType: !26)
!25 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types.h", directory: "")
!26 = !DIBasicType(name: "unsigned char", size: 8, encoding: DW_ATE_unsigned_char)
!27 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !28, producer: "clang version 8.0.0 ", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !29, retainedTypes: !30, imports: !32, nameTableKind: None)
!28 = !DIFile(filename: "libnt/packet_highlevel.cpp", directory: "/home/stephand/20-Projects/2019-12_eBPF_proto/nanotube")
!29 = !{}
!30 = !{!31}
!31 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !22, size: 64)
!32 = !{!33, !41, !45, !52, !56, !61, !63, !72, !76, !80, !95, !99, !103, !107, !111, !116, !120, !124, !128, !132, !140, !144, !148, !150, !154, !158, !162, !168, !172, !176, !178, !186, !190, !198, !200, !204, !208, !212, !216, !221, !226, !231, !232, !233, !234, !236, !237, !238, !239, !240, !241, !242, !244, !245, !246, !247, !248, !249, !250, !254, !255, !256, !257, !258, !259, !260, !261, !262, !263, !264, !265, !266, !267, !268, !269, !270, !271, !272, !273, !274, !275, !276, !277}
!33 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !35, file: !40, line: 52)
!34 = !DINamespace(name: "std", scope: null)
!35 = !DISubprogram(name: "abs", scope: !36, file: !36, line: 837, type: !37, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!36 = !DIFile(filename: "/usr/include/stdlib.h", directory: "")
!37 = !DISubroutineType(types: !38)
!38 = !{!39, !39}
!39 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!40 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/7.5.0/../../../../include/c++/7.5.0/bits/std_abs.h", directory: "")
!41 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !42, file: !44, line: 127)
!42 = !DIDerivedType(tag: DW_TAG_typedef, name: "div_t", file: !36, line: 62, baseType: !43)
!43 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !36, line: 58, flags: DIFlagFwdDecl, identifier: "_ZTS5div_t")
!44 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/7.5.0/../../../../include/c++/7.5.0/cstdlib", directory: "")
!45 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !46, file: !44, line: 128)
!46 = !DIDerivedType(tag: DW_TAG_typedef, name: "ldiv_t", file: !36, line: 70, baseType: !47)
!47 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !36, line: 66, size: 128, flags: DIFlagTypePassByValue | DIFlagTrivial, elements: !48, identifier: "_ZTS6ldiv_t")
!48 = !{!49, !51}
!49 = !DIDerivedType(tag: DW_TAG_member, name: "quot", scope: !47, file: !36, line: 68, baseType: !50, size: 64)
!50 = !DIBasicType(name: "long int", size: 64, encoding: DW_ATE_signed)
!51 = !DIDerivedType(tag: DW_TAG_member, name: "rem", scope: !47, file: !36, line: 69, baseType: !50, size: 64, offset: 64)
!52 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !53, file: !44, line: 130)
!53 = !DISubprogram(name: "abort", scope: !36, file: !36, line: 588, type: !54, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: DISPFlagOptimized)
!54 = !DISubroutineType(types: !55)
!55 = !{null}
!56 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !57, file: !44, line: 134)
!57 = !DISubprogram(name: "atexit", scope: !36, file: !36, line: 592, type: !58, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!58 = !DISubroutineType(types: !59)
!59 = !{!39, !60}
!60 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !54, size: 64)
!61 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !62, file: !44, line: 137)
!62 = !DISubprogram(name: "at_quick_exit", scope: !36, file: !36, line: 597, type: !58, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!63 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !64, file: !44, line: 140)
!64 = !DISubprogram(name: "atof", scope: !65, file: !65, line: 25, type: !66, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!65 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/stdlib-float.h", directory: "")
!66 = !DISubroutineType(types: !67)
!67 = !{!68, !69}
!68 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!69 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !70, size: 64)
!70 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !71)
!71 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!72 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !73, file: !44, line: 141)
!73 = !DISubprogram(name: "atoi", scope: !36, file: !36, line: 361, type: !74, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!74 = !DISubroutineType(types: !75)
!75 = !{!39, !69}
!76 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !77, file: !44, line: 142)
!77 = !DISubprogram(name: "atol", scope: !36, file: !36, line: 366, type: !78, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!78 = !DISubroutineType(types: !79)
!79 = !{!50, !69}
!80 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !81, file: !44, line: 143)
!81 = !DISubprogram(name: "bsearch", scope: !82, file: !82, line: 20, type: !83, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!82 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/stdlib-bsearch.h", directory: "")
!83 = !DISubroutineType(types: !84)
!84 = !{!85, !86, !86, !88, !88, !91}
!85 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!86 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !87, size: 64)
!87 = !DIDerivedType(tag: DW_TAG_const_type, baseType: null)
!88 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !89, line: 62, baseType: !90)
!89 = !DIFile(filename: "/scratch/stephand/40-Archive/Software/llvm-project/build/lib/clang/8.0.0/include/stddef.h", directory: "")
!90 = !DIBasicType(name: "long unsigned int", size: 64, encoding: DW_ATE_unsigned)
!91 = !DIDerivedType(tag: DW_TAG_typedef, name: "__compar_fn_t", file: !36, line: 805, baseType: !92)
!92 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !93, size: 64)
!93 = !DISubroutineType(types: !94)
!94 = !{!39, !86, !86}
!95 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !96, file: !44, line: 144)
!96 = !DISubprogram(name: "calloc", scope: !36, file: !36, line: 541, type: !97, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!97 = !DISubroutineType(types: !98)
!98 = !{!85, !88, !88}
!99 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !100, file: !44, line: 145)
!100 = !DISubprogram(name: "div", scope: !36, file: !36, line: 849, type: !101, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!101 = !DISubroutineType(types: !102)
!102 = !{!42, !39, !39}
!103 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !104, file: !44, line: 146)
!104 = !DISubprogram(name: "exit", scope: !36, file: !36, line: 614, type: !105, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: DISPFlagOptimized)
!105 = !DISubroutineType(types: !106)
!106 = !{null, !39}
!107 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !108, file: !44, line: 147)
!108 = !DISubprogram(name: "free", scope: !36, file: !36, line: 563, type: !109, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!109 = !DISubroutineType(types: !110)
!110 = !{null, !85}
!111 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !112, file: !44, line: 148)
!112 = !DISubprogram(name: "getenv", scope: !36, file: !36, line: 631, type: !113, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!113 = !DISubroutineType(types: !114)
!114 = !{!115, !69}
!115 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !71, size: 64)
!116 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !117, file: !44, line: 149)
!117 = !DISubprogram(name: "labs", scope: !36, file: !36, line: 838, type: !118, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!118 = !DISubroutineType(types: !119)
!119 = !{!50, !50}
!120 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !121, file: !44, line: 150)
!121 = !DISubprogram(name: "ldiv", scope: !36, file: !36, line: 851, type: !122, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!122 = !DISubroutineType(types: !123)
!123 = !{!46, !50, !50}
!124 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !125, file: !44, line: 151)
!125 = !DISubprogram(name: "malloc", scope: !36, file: !36, line: 539, type: !126, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!126 = !DISubroutineType(types: !127)
!127 = !{!85, !88}
!128 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !129, file: !44, line: 153)
!129 = !DISubprogram(name: "mblen", scope: !36, file: !36, line: 919, type: !130, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!130 = !DISubroutineType(types: !131)
!131 = !{!39, !69, !88}
!132 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !133, file: !44, line: 154)
!133 = !DISubprogram(name: "mbstowcs", scope: !36, file: !36, line: 930, type: !134, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!134 = !DISubroutineType(types: !135)
!135 = !{!88, !136, !139, !88}
!136 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !137)
!137 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !138, size: 64)
!138 = !DIBasicType(name: "wchar_t", size: 32, encoding: DW_ATE_signed)
!139 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !69)
!140 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !141, file: !44, line: 155)
!141 = !DISubprogram(name: "mbtowc", scope: !36, file: !36, line: 922, type: !142, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!142 = !DISubroutineType(types: !143)
!143 = !{!39, !136, !139, !88}
!144 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !145, file: !44, line: 157)
!145 = !DISubprogram(name: "qsort", scope: !36, file: !36, line: 827, type: !146, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!146 = !DISubroutineType(types: !147)
!147 = !{null, !85, !88, !88, !91}
!148 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !149, file: !44, line: 160)
!149 = !DISubprogram(name: "quick_exit", scope: !36, file: !36, line: 620, type: !105, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: DISPFlagOptimized)
!150 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !151, file: !44, line: 163)
!151 = !DISubprogram(name: "rand", scope: !36, file: !36, line: 453, type: !152, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!152 = !DISubroutineType(types: !153)
!153 = !{!39}
!154 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !155, file: !44, line: 164)
!155 = !DISubprogram(name: "realloc", scope: !36, file: !36, line: 549, type: !156, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!156 = !DISubroutineType(types: !157)
!157 = !{!85, !85, !88}
!158 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !159, file: !44, line: 165)
!159 = !DISubprogram(name: "srand", scope: !36, file: !36, line: 455, type: !160, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!160 = !DISubroutineType(types: !161)
!161 = !{null, !9}
!162 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !163, file: !44, line: 166)
!163 = !DISubprogram(name: "strtod", scope: !36, file: !36, line: 117, type: !164, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!164 = !DISubroutineType(types: !165)
!165 = !{!68, !139, !166}
!166 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !167)
!167 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !115, size: 64)
!168 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !169, file: !44, line: 167)
!169 = !DISubprogram(name: "strtol", scope: !36, file: !36, line: 176, type: !170, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!170 = !DISubroutineType(types: !171)
!171 = !{!50, !139, !166, !39}
!172 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !173, file: !44, line: 168)
!173 = !DISubprogram(name: "strtoul", scope: !36, file: !36, line: 180, type: !174, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!174 = !DISubroutineType(types: !175)
!175 = !{!90, !139, !166, !39}
!176 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !177, file: !44, line: 169)
!177 = !DISubprogram(name: "system", scope: !36, file: !36, line: 781, type: !74, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!178 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !179, file: !44, line: 171)
!179 = !DISubprogram(name: "wcstombs", scope: !36, file: !36, line: 933, type: !180, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!180 = !DISubroutineType(types: !181)
!181 = !{!88, !182, !183, !88}
!182 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !115)
!183 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !184)
!184 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !185, size: 64)
!185 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !138)
!186 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !187, file: !44, line: 172)
!187 = !DISubprogram(name: "wctomb", scope: !36, file: !36, line: 926, type: !188, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!188 = !DISubroutineType(types: !189)
!189 = !{!39, !115, !138}
!190 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !191, entity: !192, file: !44, line: 200)
!191 = !DINamespace(name: "__gnu_cxx", scope: null)
!192 = !DIDerivedType(tag: DW_TAG_typedef, name: "lldiv_t", file: !36, line: 80, baseType: !193)
!193 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !36, line: 76, size: 128, flags: DIFlagTypePassByValue | DIFlagTrivial, elements: !194, identifier: "_ZTS7lldiv_t")
!194 = !{!195, !197}
!195 = !DIDerivedType(tag: DW_TAG_member, name: "quot", scope: !193, file: !36, line: 78, baseType: !196, size: 64)
!196 = !DIBasicType(name: "long long int", size: 64, encoding: DW_ATE_signed)
!197 = !DIDerivedType(tag: DW_TAG_member, name: "rem", scope: !193, file: !36, line: 79, baseType: !196, size: 64, offset: 64)
!198 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !191, entity: !199, file: !44, line: 206)
!199 = !DISubprogram(name: "_Exit", scope: !36, file: !36, line: 626, type: !105, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: DISPFlagOptimized)
!200 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !191, entity: !201, file: !44, line: 210)
!201 = !DISubprogram(name: "llabs", scope: !36, file: !36, line: 841, type: !202, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!202 = !DISubroutineType(types: !203)
!203 = !{!196, !196}
!204 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !191, entity: !205, file: !44, line: 216)
!205 = !DISubprogram(name: "lldiv", scope: !36, file: !36, line: 855, type: !206, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!206 = !DISubroutineType(types: !207)
!207 = !{!192, !196, !196}
!208 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !191, entity: !209, file: !44, line: 227)
!209 = !DISubprogram(name: "atoll", scope: !36, file: !36, line: 373, type: !210, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!210 = !DISubroutineType(types: !211)
!211 = !{!196, !69}
!212 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !191, entity: !213, file: !44, line: 228)
!213 = !DISubprogram(name: "strtoll", scope: !36, file: !36, line: 200, type: !214, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!214 = !DISubroutineType(types: !215)
!215 = !{!196, !139, !166, !39}
!216 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !191, entity: !217, file: !44, line: 229)
!217 = !DISubprogram(name: "strtoull", scope: !36, file: !36, line: 205, type: !218, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!218 = !DISubroutineType(types: !219)
!219 = !{!220, !139, !166, !39}
!220 = !DIBasicType(name: "long long unsigned int", size: 64, encoding: DW_ATE_unsigned)
!221 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !191, entity: !222, file: !44, line: 231)
!222 = !DISubprogram(name: "strtof", scope: !36, file: !36, line: 123, type: !223, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!223 = !DISubroutineType(types: !224)
!224 = !{!225, !139, !166}
!225 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!226 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !191, entity: !227, file: !44, line: 232)
!227 = !DISubprogram(name: "strtold", scope: !36, file: !36, line: 126, type: !228, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!228 = !DISubroutineType(types: !229)
!229 = !{!230, !139, !166}
!230 = !DIBasicType(name: "long double", size: 128, encoding: DW_ATE_float)
!231 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !192, file: !44, line: 240)
!232 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !199, file: !44, line: 242)
!233 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !201, file: !44, line: 244)
!234 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !235, file: !44, line: 245)
!235 = !DISubprogram(name: "div", linkageName: "_ZN9__gnu_cxx3divExx", scope: !191, file: !44, line: 213, type: !206, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!236 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !205, file: !44, line: 246)
!237 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !209, file: !44, line: 248)
!238 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !222, file: !44, line: 249)
!239 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !213, file: !44, line: 250)
!240 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !217, file: !44, line: 251)
!241 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !34, entity: !227, file: !44, line: 252)
!242 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !53, file: !243, line: 38)
!243 = !DIFile(filename: "/usr/lib/gcc/x86_64-linux-gnu/7.5.0/../../../../include/c++/7.5.0/stdlib.h", directory: "")
!244 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !57, file: !243, line: 39)
!245 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !104, file: !243, line: 40)
!246 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !62, file: !243, line: 43)
!247 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !149, file: !243, line: 46)
!248 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !42, file: !243, line: 51)
!249 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !46, file: !243, line: 52)
!250 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !251, file: !243, line: 54)
!251 = !DISubprogram(name: "abs", linkageName: "_ZSt3abse", scope: !34, file: !40, line: 78, type: !252, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!252 = !DISubroutineType(types: !253)
!253 = !{!230, !230}
!254 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !64, file: !243, line: 55)
!255 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !73, file: !243, line: 56)
!256 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !77, file: !243, line: 57)
!257 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !81, file: !243, line: 58)
!258 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !96, file: !243, line: 59)
!259 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !235, file: !243, line: 60)
!260 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !108, file: !243, line: 61)
!261 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !112, file: !243, line: 62)
!262 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !117, file: !243, line: 63)
!263 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !121, file: !243, line: 64)
!264 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !125, file: !243, line: 65)
!265 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !129, file: !243, line: 67)
!266 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !133, file: !243, line: 68)
!267 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !141, file: !243, line: 69)
!268 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !145, file: !243, line: 71)
!269 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !151, file: !243, line: 72)
!270 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !155, file: !243, line: 73)
!271 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !159, file: !243, line: 74)
!272 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !163, file: !243, line: 75)
!273 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !169, file: !243, line: 76)
!274 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !173, file: !243, line: 77)
!275 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !177, file: !243, line: 78)
!276 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !179, file: !243, line: 80)
!277 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !27, entity: !187, file: !243, line: 81)
!278 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !279, producer: "clang version 8.0.0 ", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6, retainedTypes: !30, imports: !280, nameTableKind: None)
!279 = !DIFile(filename: "libnt/map_highlevel.cpp", directory: "/home/stephand/20-Projects/2019-12_eBPF_proto/nanotube")
!280 = !{!33, !41, !45, !52, !56, !61, !63, !72, !76, !80, !95, !99, !103, !107, !111, !116, !120, !124, !128, !132, !140, !144, !148, !150, !154, !158, !162, !168, !172, !176, !178, !186, !190, !198, !200, !204, !208, !212, !216, !221, !226, !231, !232, !233, !234, !236, !237, !238, !239, !240, !241, !281, !282, !283, !284, !285, !286, !287, !288, !289, !290, !291, !292, !293, !294, !295, !296, !297, !298, !299, !300, !301, !302, !303, !304, !305, !306, !307, !308, !309, !310, !311, !312}
!281 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !53, file: !243, line: 38)
!282 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !57, file: !243, line: 39)
!283 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !104, file: !243, line: 40)
!284 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !62, file: !243, line: 43)
!285 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !149, file: !243, line: 46)
!286 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !42, file: !243, line: 51)
!287 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !46, file: !243, line: 52)
!288 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !251, file: !243, line: 54)
!289 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !64, file: !243, line: 55)
!290 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !73, file: !243, line: 56)
!291 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !77, file: !243, line: 57)
!292 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !81, file: !243, line: 58)
!293 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !96, file: !243, line: 59)
!294 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !235, file: !243, line: 60)
!295 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !108, file: !243, line: 61)
!296 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !112, file: !243, line: 62)
!297 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !117, file: !243, line: 63)
!298 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !121, file: !243, line: 64)
!299 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !125, file: !243, line: 65)
!300 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !129, file: !243, line: 67)
!301 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !133, file: !243, line: 68)
!302 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !141, file: !243, line: 69)
!303 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !145, file: !243, line: 71)
!304 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !151, file: !243, line: 72)
!305 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !155, file: !243, line: 73)
!306 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !159, file: !243, line: 74)
!307 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !163, file: !243, line: 75)
!308 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !169, file: !243, line: 76)
!309 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !173, file: !243, line: 77)
!310 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !177, file: !243, line: 78)
!311 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !179, file: !243, line: 80)
!312 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !278, entity: !187, file: !243, line: 81)
!313 = distinct !DISubprogram(name: "map_op_adapter", scope: !5, file: !5, line: 27, type: !314, scopeLine: 31, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !4, retainedNodes: !328)
!314 = !DISubroutineType(types: !315)
!315 = !{!316, !319, !322, !7, !326, !88, !326, !31, !326, !88, !88}
!316 = !DIDerivedType(tag: DW_TAG_typedef, name: "int32_t", file: !317, line: 26, baseType: !318)
!317 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/stdint-intn.h", directory: "")
!318 = !DIDerivedType(tag: DW_TAG_typedef, name: "__int32_t", file: !25, line: 40, baseType: !39)
!319 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !320, size: 64)
!320 = !DIDerivedType(tag: DW_TAG_typedef, name: "nanotube_context_t", file: !8, line: 71, baseType: !321)
!321 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "nanotube_context", file: !8, line: 71, flags: DIFlagFwdDecl, identifier: "_ZTS16nanotube_context")
!322 = !DIDerivedType(tag: DW_TAG_typedef, name: "nanotube_map_id_t", file: !8, line: 68, baseType: !323)
!323 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint16_t", file: !23, line: 25, baseType: !324)
!324 = !DIDerivedType(tag: DW_TAG_typedef, name: "__uint16_t", file: !25, line: 39, baseType: !325)
!325 = !DIBasicType(name: "unsigned short", size: 16, encoding: DW_ATE_unsigned)
!326 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !327, size: 64)
!327 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !22)
!328 = !{!329, !330, !331, !332, !333, !334, !335, !336, !337, !338, !339}
!329 = !DILocalVariable(name: "ctx", arg: 1, scope: !313, file: !5, line: 27, type: !319)
!330 = !DILocalVariable(name: "map_id", arg: 2, scope: !313, file: !5, line: 27, type: !322)
!331 = !DILocalVariable(name: "type", arg: 3, scope: !313, file: !5, line: 28, type: !7)
!332 = !DILocalVariable(name: "key", arg: 4, scope: !313, file: !5, line: 28, type: !326)
!333 = !DILocalVariable(name: "key_length", arg: 5, scope: !313, file: !5, line: 29, type: !88)
!334 = !DILocalVariable(name: "data_in", arg: 6, scope: !313, file: !5, line: 29, type: !326)
!335 = !DILocalVariable(name: "data_out", arg: 7, scope: !313, file: !5, line: 30, type: !31)
!336 = !DILocalVariable(name: "mask", arg: 8, scope: !313, file: !5, line: 30, type: !326)
!337 = !DILocalVariable(name: "offset", arg: 9, scope: !313, file: !5, line: 31, type: !88)
!338 = !DILocalVariable(name: "data_length", arg: 10, scope: !313, file: !5, line: 31, type: !88)
!339 = !DILocalVariable(name: "ret", scope: !313, file: !5, line: 33, type: !88)
!340 = !DILocation(line: 27, column: 44, scope: !313)
!341 = !DILocation(line: 27, column: 67, scope: !313)
!342 = !DILocation(line: 28, column: 42, scope: !313)
!343 = !DILocation(line: 28, column: 63, scope: !313)
!344 = !DILocation(line: 29, column: 31, scope: !313)
!345 = !DILocation(line: 29, column: 58, scope: !313)
!346 = !DILocation(line: 30, column: 33, scope: !313)
!347 = !DILocation(line: 30, column: 58, scope: !313)
!348 = !DILocation(line: 31, column: 31, scope: !313)
!349 = !DILocation(line: 31, column: 46, scope: !313)
!350 = !DILocation(line: 33, column: 16, scope: !313)
!351 = !DILocation(line: 33, column: 10, scope: !313)
!352 = !DILocation(line: 36, column: 11, scope: !353)
!353 = distinct !DILexicalBlock(scope: !313, file: !5, line: 36, column: 7)
!354 = !DILocation(line: 0, scope: !313)
!355 = !DILocation(line: 42, column: 1, scope: !313)
!356 = distinct !DISubprogram(name: "packet_adjust_head_adapter", scope: !5, file: !5, line: 47, type: !357, scopeLine: 47, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !4, retainedNodes: !362)
!357 = !DISubroutineType(types: !358)
!358 = !{!316, !359, !316}
!359 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !360, size: 64)
!360 = !DIDerivedType(tag: DW_TAG_typedef, name: "nanotube_packet_t", file: !8, line: 73, baseType: !361)
!361 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "nanotube_packet", file: !8, line: 73, flags: DIFlagFwdDecl, identifier: "_ZTS15nanotube_packet")
!362 = !{!363, !364, !365}
!363 = !DILocalVariable(name: "p", arg: 1, scope: !356, file: !5, line: 47, type: !359)
!364 = !DILocalVariable(name: "offset", arg: 2, scope: !356, file: !5, line: 47, type: !316)
!365 = !DILocalVariable(name: "success", scope: !356, file: !5, line: 48, type: !39)
!366 = !DILocation(line: 47, column: 55, scope: !356)
!367 = !DILocation(line: 47, column: 66, scope: !356)
!368 = !DILocation(line: 48, column: 18, scope: !356)
!369 = !DILocation(line: 48, column: 8, scope: !356)
!370 = !DILocation(line: 50, column: 10, scope: !356)
!371 = !DILocation(line: 50, column: 3, scope: !356)
!372 = distinct !DISubprogram(name: "packet_handle_xdp_result", scope: !5, file: !5, line: 56, type: !373, scopeLine: 56, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !4, retainedNodes: !375)
!373 = !DISubroutineType(types: !374)
!374 = !{null, !359, !39}
!375 = !{!376, !377}
!376 = !DILocalVariable(name: "packet", arg: 1, scope: !372, file: !5, line: 56, type: !359)
!377 = !DILocalVariable(name: "xdp_ret", arg: 2, scope: !372, file: !5, line: 56, type: !39)
!378 = !DILocation(line: 56, column: 50, scope: !372)
!379 = !DILocation(line: 56, column: 62, scope: !372)
!380 = !DILocation(line: 57, column: 3, scope: !372)
!381 = !DILocation(line: 59, column: 7, scope: !382)
!382 = distinct !DILexicalBlock(scope: !372, file: !5, line: 57, column: 21)
!383 = !DILocation(line: 60, column: 7, scope: !382)
!384 = !DILocation(line: 64, column: 7, scope: !382)
!385 = !DILocation(line: 65, column: 3, scope: !382)
!386 = !DILocation(line: 66, column: 1, scope: !372)
!387 = distinct !DISubprogram(name: "nanotube_packet_write", scope: !28, file: !28, line: 19, type: !388, scopeLine: 21, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !27, retainedNodes: !390)
!388 = !DISubroutineType(types: !389)
!389 = !{!88, !359, !326, !88, !88}
!390 = !{!391, !392, !393, !394, !395, !396}
!391 = !DILocalVariable(name: "packet", arg: 1, scope: !387, file: !28, line: 19, type: !359)
!392 = !DILocalVariable(name: "data_in", arg: 2, scope: !387, file: !28, line: 20, type: !326)
!393 = !DILocalVariable(name: "offset", arg: 3, scope: !387, file: !28, line: 20, type: !88)
!394 = !DILocalVariable(name: "length", arg: 4, scope: !387, file: !28, line: 21, type: !88)
!395 = !DILocalVariable(name: "mask_size", scope: !387, file: !28, line: 22, type: !88)
!396 = !DILocalVariable(name: "all_one", scope: !387, file: !28, line: 23, type: !31)
!397 = !DILocation(line: 19, column: 49, scope: !387)
!398 = !DILocation(line: 20, column: 45, scope: !387)
!399 = !DILocation(line: 20, column: 61, scope: !387)
!400 = !DILocation(line: 21, column: 37, scope: !387)
!401 = !DILocation(line: 22, column: 30, scope: !387)
!402 = !DILocation(line: 22, column: 35, scope: !387)
!403 = !DILocation(line: 22, column: 10, scope: !387)
!404 = !DILocation(line: 23, column: 32, scope: !387)
!405 = !DILocation(line: 23, column: 12, scope: !387)
!406 = !DILocation(line: 24, column: 3, scope: !387)
!407 = !DILocation(line: 26, column: 10, scope: !387)
!408 = !DILocation(line: 26, column: 3, scope: !387)
!409 = distinct !DISubprogram(name: "nanotube_merge_data_mask", scope: !28, file: !28, line: 33, type: !410, scopeLine: 35, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !27, retainedNodes: !412)
!410 = !DISubroutineType(types: !411)
!411 = !{null, !31, !31, !326, !326, !88, !88}
!412 = !{!413, !414, !415, !416, !417, !418, !419, !421, !424, !425, !426, !428, !431}
!413 = !DILocalVariable(name: "inout_data", arg: 1, scope: !409, file: !28, line: 33, type: !31)
!414 = !DILocalVariable(name: "inout_mask", arg: 2, scope: !409, file: !28, line: 33, type: !31)
!415 = !DILocalVariable(name: "in_data", arg: 3, scope: !409, file: !28, line: 34, type: !326)
!416 = !DILocalVariable(name: "in_mask", arg: 4, scope: !409, file: !28, line: 34, type: !326)
!417 = !DILocalVariable(name: "offset", arg: 5, scope: !409, file: !28, line: 35, type: !88)
!418 = !DILocalVariable(name: "data_length", arg: 6, scope: !409, file: !28, line: 35, type: !88)
!419 = !DILocalVariable(name: "i", scope: !420, file: !28, line: 36, type: !88)
!420 = distinct !DILexicalBlock(scope: !409, file: !28, line: 36, column: 3)
!421 = !DILocalVariable(name: "maskidx", scope: !422, file: !28, line: 37, type: !88)
!422 = distinct !DILexicalBlock(scope: !423, file: !28, line: 36, column: 45)
!423 = distinct !DILexicalBlock(scope: !420, file: !28, line: 36, column: 3)
!424 = !DILocalVariable(name: "bitidx", scope: !422, file: !28, line: 38, type: !22)
!425 = !DILocalVariable(name: "out_pos", scope: !422, file: !28, line: 39, type: !88)
!426 = !DILocalVariable(name: "update", scope: !422, file: !28, line: 40, type: !427)
!427 = !DIBasicType(name: "bool", size: 8, encoding: DW_ATE_boolean)
!428 = !DILocalVariable(name: "out_maskidx", scope: !429, file: !28, line: 45, type: !88)
!429 = distinct !DILexicalBlock(scope: !430, file: !28, line: 42, column: 18)
!430 = distinct !DILexicalBlock(scope: !422, file: !28, line: 42, column: 9)
!431 = !DILocalVariable(name: "out_bitidx", scope: !429, file: !28, line: 46, type: !22)
!432 = !DILocation(line: 33, column: 35, scope: !409)
!433 = !DILocation(line: 33, column: 56, scope: !409)
!434 = !DILocation(line: 34, column: 41, scope: !409)
!435 = !DILocation(line: 34, column: 65, scope: !409)
!436 = !DILocation(line: 35, column: 33, scope: !409)
!437 = !DILocation(line: 35, column: 48, scope: !409)
!438 = !DILocation(line: 36, column: 15, scope: !420)
!439 = !DILocation(line: 36, column: 24, scope: !423)
!440 = !DILocation(line: 36, column: 3, scope: !420)
!441 = !DILocation(line: 50, column: 1, scope: !409)
!442 = !DILocation(line: 37, column: 24, scope: !422)
!443 = !DILocation(line: 37, column: 12, scope: !422)
!444 = !DILocation(line: 38, column: 22, scope: !422)
!445 = !DILocation(line: 39, column: 29, scope: !422)
!446 = !DILocation(line: 39, column: 12, scope: !422)
!447 = !DILocation(line: 40, column: 19, scope: !422)
!448 = !{!449, !449, i64 0}
!449 = !{!"omnipotent char", !450, i64 0}
!450 = !{!"Simple C++ TBAA"}
!451 = !DILocation(line: 40, column: 41, scope: !422)
!452 = !DILocation(line: 40, column: 36, scope: !422)
!453 = !DILocation(line: 42, column: 9, scope: !422)
!454 = !DILocation(line: 44, column: 29, scope: !429)
!455 = !DILocation(line: 44, column: 7, scope: !429)
!456 = !DILocation(line: 44, column: 27, scope: !429)
!457 = !DILocation(line: 45, column: 36, scope: !429)
!458 = !DILocation(line: 45, column: 14, scope: !429)
!459 = !DILocation(line: 46, column: 28, scope: !429)
!460 = !DILocation(line: 46, column: 15, scope: !429)
!461 = !DILocation(line: 47, column: 37, scope: !429)
!462 = !DILocation(line: 47, column: 7, scope: !429)
!463 = !DILocation(line: 47, column: 31, scope: !429)
!464 = !DILocation(line: 48, column: 5, scope: !429)
!465 = !DILocation(line: 36, column: 40, scope: !423)
!466 = distinct !{!466, !440, !467}
!467 = !DILocation(line: 49, column: 3, scope: !420)
!468 = distinct !DISubprogram(name: "nanotube_map_read", scope: !279, file: !279, line: 19, type: !469, scopeLine: 22, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !278, retainedNodes: !471)
!469 = !DISubroutineType(types: !470)
!470 = !{!88, !319, !322, !326, !88, !31, !88, !88}
!471 = !{!472, !473, !474, !475, !476, !477, !478}
!472 = !DILocalVariable(name: "ctx", arg: 1, scope: !468, file: !279, line: 19, type: !319)
!473 = !DILocalVariable(name: "id", arg: 2, scope: !468, file: !279, line: 19, type: !322)
!474 = !DILocalVariable(name: "key", arg: 3, scope: !468, file: !279, line: 20, type: !326)
!475 = !DILocalVariable(name: "key_length", arg: 4, scope: !468, file: !279, line: 20, type: !88)
!476 = !DILocalVariable(name: "data_out", arg: 5, scope: !468, file: !279, line: 21, type: !31)
!477 = !DILocalVariable(name: "offset", arg: 6, scope: !468, file: !279, line: 21, type: !88)
!478 = !DILocalVariable(name: "data_length", arg: 7, scope: !468, file: !279, line: 22, type: !88)
!479 = !DILocation(line: 19, column: 46, scope: !468)
!480 = !DILocation(line: 19, column: 69, scope: !468)
!481 = !DILocation(line: 20, column: 41, scope: !468)
!482 = !DILocation(line: 20, column: 53, scope: !468)
!483 = !DILocation(line: 21, column: 35, scope: !468)
!484 = !DILocation(line: 21, column: 52, scope: !468)
!485 = !DILocation(line: 22, column: 33, scope: !468)
!486 = !DILocation(line: 23, column: 10, scope: !468)
!487 = !DILocation(line: 23, column: 3, scope: !468)
!488 = distinct !DISubprogram(name: "nanotube_map_write", scope: !279, file: !279, line: 31, type: !489, scopeLine: 34, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !278, retainedNodes: !491)
!489 = !DISubroutineType(types: !490)
!490 = !{!88, !319, !322, !326, !88, !326, !88, !88}
!491 = !{!492, !493, !494, !495, !496, !497, !498, !499, !500}
!492 = !DILocalVariable(name: "ctx", arg: 1, scope: !488, file: !279, line: 31, type: !319)
!493 = !DILocalVariable(name: "id", arg: 2, scope: !488, file: !279, line: 31, type: !322)
!494 = !DILocalVariable(name: "key", arg: 3, scope: !488, file: !279, line: 32, type: !326)
!495 = !DILocalVariable(name: "key_length", arg: 4, scope: !488, file: !279, line: 32, type: !88)
!496 = !DILocalVariable(name: "data_in", arg: 5, scope: !488, file: !279, line: 33, type: !326)
!497 = !DILocalVariable(name: "offset", arg: 6, scope: !488, file: !279, line: 33, type: !88)
!498 = !DILocalVariable(name: "data_length", arg: 7, scope: !488, file: !279, line: 34, type: !88)
!499 = !DILocalVariable(name: "mask_size", scope: !488, file: !279, line: 35, type: !88)
!500 = !DILocalVariable(name: "all_one", scope: !488, file: !279, line: 36, type: !31)
!501 = !DILocation(line: 31, column: 47, scope: !488)
!502 = !DILocation(line: 31, column: 70, scope: !488)
!503 = !DILocation(line: 32, column: 42, scope: !488)
!504 = !DILocation(line: 32, column: 54, scope: !488)
!505 = !DILocation(line: 33, column: 42, scope: !488)
!506 = !DILocation(line: 33, column: 58, scope: !488)
!507 = !DILocation(line: 34, column: 34, scope: !488)
!508 = !DILocation(line: 35, column: 35, scope: !488)
!509 = !DILocation(line: 35, column: 40, scope: !488)
!510 = !DILocation(line: 35, column: 10, scope: !488)
!511 = !DILocation(line: 36, column: 32, scope: !488)
!512 = !DILocation(line: 36, column: 12, scope: !488)
!513 = !DILocation(line: 37, column: 3, scope: !488)
!514 = !DILocation(line: 38, column: 10, scope: !488)
!515 = !DILocation(line: 38, column: 3, scope: !488)
!516 = !DILocation(line: 23, column: 32, scope: !387, inlinedAt: !517)
!517 = distinct !DILocation(line: 64, column: 7, scope: !382)
!518 = !DILocation(line: 23, column: 32, scope: !387, inlinedAt: !519)
!519 = distinct !DILocation(line: 59, column: 7, scope: !382)
!520 = !{!521, !521, i64 0}
!521 = !{!"int", !522, i64 0}
!522 = !{!"omnipotent char", !523, i64 0}
!523 = !{!"Simple C/C++ TBAA"}
!524 = !{!525, !522, i64 72}
!525 = !{!"headers", !526, i64 0, !527, i64 32, !522, i64 56, !530, i64 72, !531, i64 100, !532, i64 108, !532, i64 120, !532, i64 132, !532, i64 144}
!526 = !{!"pseudo_hdr_t", !522, i64 0, !522, i64 28}
!527 = !{!"eth_mac_t", !528, i64 0, !528, i64 8, !529, i64 16, !522, i64 18}
!528 = !{!"long long", !522, i64 0}
!529 = !{!"short", !522, i64 0}
!530 = !{!"ipv4_t", !522, i64 0, !522, i64 1, !522, i64 2, !529, i64 4, !529, i64 6, !522, i64 8, !529, i64 10, !522, i64 12, !522, i64 13, !529, i64 14, !521, i64 16, !521, i64 20, !522, i64 24}
!531 = !{!"ipv4_rfc781_hdr_t", !522, i64 0, !522, i64 1, !522, i64 2, !522, i64 3, !522, i64 4, !522, i64 5}
!532 = !{!"ipv4_rfc781_t", !521, i64 0, !521, i64 4, !522, i64 8}
!533 = !{!525, !522, i64 73}
!534 = !{!525, !522, i64 74}
!535 = !{!525, !529, i64 76}
!536 = !{!525, !529, i64 78}
!537 = !{!525, !522, i64 80}
!538 = !{!525, !529, i64 82}
!539 = !{!525, !522, i64 84}
!540 = !{!525, !522, i64 85}
!541 = !{!525, !529, i64 86}
!542 = !{!525, !521, i64 88}
!543 = !{!525, !521, i64 92}
!544 = !{!525, !522, i64 96}
!545 = !{!525, !522, i64 100}
!546 = !{!525, !522, i64 101}
!547 = !{!525, !522, i64 102}
!548 = !{!525, !522, i64 103}
!549 = !{!525, !522, i64 104}
!550 = !{!525, !522, i64 105}
!551 = !{!525, !521, i64 108}
!552 = !{!525, !521, i64 112}
!553 = !{!525, !522, i64 116}
!554 = !{!525, !521, i64 120}
!555 = !{!525, !521, i64 124}
!556 = !{!525, !522, i64 128}
!557 = !{!525, !521, i64 132}
!558 = !{!525, !521, i64 136}
!559 = !{!525, !522, i64 140}
!560 = !{!525, !521, i64 144}
!561 = !{!525, !521, i64 148}
!562 = !{!525, !522, i64 152}
!563 = !{!564, !522, i64 0}
!564 = !{!"vlan_t", !522, i64 0, !522, i64 1, !529, i64 2, !529, i64 4, !522, i64 6}
!565 = !{!564, !522, i64 1}
!566 = !{!564, !529, i64 2}
!567 = !{!564, !529, i64 4}
!568 = !{!564, !522, i64 6}
!569 = !{!522, !522, i64 0}
!570 = !{!525, !522, i64 28}
!571 = !{!525, !528, i64 32}
!572 = !{!525, !528, i64 40}
!573 = !{!525, !529, i64 48}
!574 = !{!525, !522, i64 50}
!575 = !{!576, !521, i64 4}
!576 = !{!"xdp_output", !522, i64 0, !521, i64 4}
!577 = !{!576, !522, i64 0}
!578 = !{!579, !522, i64 0}
!579 = !{!"MyProcessing_IPV4_CNT_key", !522, i64 0}
!580 = !{!581, !521, i64 0}
!581 = !{!"MyProcessing_MATCH_RFC781_key", !521, i64 0}
!582 = !{!583, !521, i64 0}
!583 = !{!"MyProcessing_BLACKLIST_RFC781_key", !521, i64 0}
!584 = !{!585, !522, i64 0}
!585 = !{!"MyProcessing_TSTAMP_MOD_CNT_key", !522, i64 0}
!586 = !{!587, !522, i64 0}
!587 = !{!"MyProcessing_DROP_CNT_key", !522, i64 0}
!588 = !DILocation(line: 19, column: 49, scope: !387, inlinedAt: !519)
!589 = !DILocation(line: 20, column: 45, scope: !387, inlinedAt: !519)
!590 = !DILocation(line: 20, column: 61, scope: !387, inlinedAt: !519)
!591 = !DILocation(line: 21, column: 37, scope: !387, inlinedAt: !519)
!592 = !DILocation(line: 22, column: 10, scope: !387, inlinedAt: !519)
!593 = !DILocation(line: 23, column: 12, scope: !387, inlinedAt: !519)
!594 = !DILocation(line: 24, column: 3, scope: !387, inlinedAt: !519)
!595 = !DILocation(line: 26, column: 10, scope: !387, inlinedAt: !519)
!596 = !DILocation(line: 26, column: 3, scope: !387, inlinedAt: !519)
!597 = !DILocation(line: 19, column: 49, scope: !387, inlinedAt: !517)
!598 = !DILocation(line: 20, column: 45, scope: !387, inlinedAt: !517)
!599 = !DILocation(line: 20, column: 61, scope: !387, inlinedAt: !517)
!600 = !DILocation(line: 21, column: 37, scope: !387, inlinedAt: !517)
!601 = !DILocation(line: 22, column: 10, scope: !387, inlinedAt: !517)
!602 = !DILocation(line: 23, column: 12, scope: !387, inlinedAt: !517)
!603 = !DILocation(line: 24, column: 3, scope: !387, inlinedAt: !517)
!604 = !DILocation(line: 26, column: 10, scope: !387, inlinedAt: !517)
!605 = !DILocation(line: 26, column: 3, scope: !387, inlinedAt: !517)
