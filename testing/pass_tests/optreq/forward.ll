;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'forward.mem2req.lower.inline.platform.bc'
source_filename = "llvm-link"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque

@0 = private unnamed_addr constant [12 x i8] c"ebpf_filter\00", align 1

define void @nanotube_setup() {
  call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @0, i32 0, i32 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* @ebpf_filter_nt.1, i32 0, i32 1)
  ret void
}

declare void @nanotube_add_plain_packet_kernel(i8*, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32)

declare i64 @nanotube_packet_bounded_length(%struct.nanotube_packet*, i64)

declare i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64)

; Function Attrs: nounwind readnone speculatable
declare i16 @llvm.bswap.i16(i16) #0

; Function Attrs: nounwind readnone speculatable
declare i32 @llvm.bswap.i32(i32) #0

; Function Attrs: alwaysinline uwtable
define dso_local i32 @map_op_adapter(%struct.nanotube_context* %ctx, i16 zeroext %map_id, i32 %type, i8* %key, i64 %key_length, i8* %data_in, i8* %data_out, i8* %mask, i64 %offset, i64 %data_length) local_unnamed_addr #1 !dbg !313 {
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
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

declare dso_local i64 @nanotube_map_op(%struct.nanotube_context*, i16 zeroext, i32, i8*, i64, i8*, i8*, i8*, i64, i64) local_unnamed_addr #2

; Function Attrs: alwaysinline uwtable
define dso_local i32 @packet_adjust_head_adapter(%struct.nanotube_packet* %p, i32 %offset) #1 !dbg !356 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %p, metadata !363, metadata !DIExpression()), !dbg !366
  call void @llvm.dbg.value(metadata i32 %offset, metadata !364, metadata !DIExpression()), !dbg !367
  %call = tail call i32 @nanotube_packet_resize(%struct.nanotube_packet* %p, i64 0, i32 %offset), !dbg !368
  call void @llvm.dbg.value(metadata i32 %call, metadata !365, metadata !DIExpression()), !dbg !369
  %tobool = icmp eq i32 %call, 0, !dbg !370
  %cond = sext i1 %tobool to i32, !dbg !370
  ret i32 %cond, !dbg !371
}

declare dso_local i32 @nanotube_packet_resize(%struct.nanotube_packet*, i64, i32) local_unnamed_addr #2

; Function Attrs: alwaysinline uwtable
define dso_local void @packet_handle_xdp_result(%struct.nanotube_packet* %packet, i32 %xdp_ret) #1 !dbg !372 {
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

declare dso_local void @nanotube_packet_set_port(%struct.nanotube_packet*, i8 zeroext) local_unnamed_addr #2

; Function Attrs: alwaysinline uwtable
define dso_local i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %data_in, i64 %offset, i64 %length) #1 !dbg !387 {
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
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1) #3

declare dso_local i64 @nanotube_packet_write_masked(%struct.nanotube_packet*, i8*, i8*, i64, i64) local_unnamed_addr #2

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
define dso_local i64 @nanotube_map_read(%struct.nanotube_context* %ctx, i16 zeroext %id, i8* %key, i64 %key_length, i8* %data_out, i64 %offset, i64 %data_length) local_unnamed_addr #1 !dbg !468 {
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
define dso_local i64 @nanotube_map_write(%struct.nanotube_context* %ctx, i16 zeroext %id, i8* %key, i64 %key_length, i8* %data_in, i64 %offset, i64 %data_length) local_unnamed_addr #1 !dbg !488 {
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

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #3

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #3

; Function Attrs: nounwind
declare i8* @llvm.stacksave() #5

; Function Attrs: nounwind
declare void @llvm.stackrestore(i8*) #5

; Function Attrs: nounwind uwtable
define private i32 @ebpf_filter_nt.1(%struct.nanotube_context* %nt_ctx, %struct.nanotube_packet* %packet) #6 section "prog" {
entry:
  %0 = alloca i8, i64 1, align 16, !dbg !404
  %1 = alloca i8, i64 1, align 16, !dbg !404
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
  %_buffer173 = alloca i8
  %_buffer172 = alloca i8
  %_buffer171 = alloca i16
  %103 = bitcast i16* %_buffer171 to i8*
  %_buffer170 = alloca i8
  %_buffer169 = alloca i8
  %_buffer168 = alloca i8
  %_buffer167 = alloca i8
  %_buffer166 = alloca i8
  %_buffer165 = alloca i8
  %_buffer164 = alloca i8
  %_buffer163 = alloca i8
  %_buffer162 = alloca i8
  %_buffer161 = alloca i8
  %_buffer160 = alloca i8
  %_buffer159 = alloca i8
  %_buffer158 = alloca i8
  %_buffer157 = alloca i8
  %_buffer156 = alloca i8
  %_buffer155 = alloca i8
  %_buffer154 = alloca i8
  %_buffer153 = alloca i8
  %_buffer152 = alloca i8
  %_buffer151 = alloca i8
  %_buffer150 = alloca i8
  %_buffer149 = alloca i32
  %104 = bitcast i32* %_buffer149 to i8*
  %_buffer148 = alloca i8
  %_buffer147 = alloca i8
  %_buffer146 = alloca i8
  %_buffer145 = alloca i8
  %_buffer144 = alloca i16
  %105 = bitcast i16* %_buffer144 to i8*
  %_buffer143 = alloca i16
  %106 = bitcast i16* %_buffer143 to i8*
  %_buffer142 = alloca i8
  %_buffer141 = alloca i8
  %_buffer140 = alloca i8
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
  %_buffer112 = alloca i16
  %107 = bitcast i16* %_buffer112 to i8*
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
  %_buffer90 = alloca i64
  %108 = bitcast i64* %_buffer90 to i8*
  %_buffer89 = alloca i8
  %_buffer88 = alloca i16
  %109 = bitcast i16* %_buffer88 to i8*
  %_buffer87 = alloca i8
  %_buffer86 = alloca i8
  %_buffer85 = alloca i8
  %_buffer84 = alloca i16
  %110 = bitcast i16* %_buffer84 to i8*
  %_buffer83 = alloca i16
  %111 = bitcast i16* %_buffer83 to i8*
  %_buffer82 = alloca i8
  %_buffer81 = alloca i8
  %_buffer80 = alloca i8
  %_buffer79 = alloca i16
  %112 = bitcast i16* %_buffer79 to i8*
  %_buffer78 = alloca i32
  %113 = bitcast i32* %_buffer78 to i8*
  %_buffer77 = alloca i8
  %_buffer76 = alloca i8
  %_buffer75 = alloca i8
  %_buffer74 = alloca i8
  %_buffer73 = alloca i32
  %114 = bitcast i32* %_buffer73 to i8*
  %_buffer72 = alloca i8
  %_buffer71 = alloca i8
  %_buffer70 = alloca i8
  %_buffer69 = alloca i8
  %_buffer68 = alloca i8
  %_buffer67 = alloca i8
  %_buffer66 = alloca i8
  %_buffer65 = alloca i8
  %_buffer64 = alloca i8
  %_buffer63 = alloca i8
  %_buffer62 = alloca i8
  %_buffer61 = alloca i8
  %_buffer60 = alloca i8
  %_buffer59 = alloca i8
  %_buffer58 = alloca i8
  %_buffer57 = alloca i16
  %115 = bitcast i16* %_buffer57 to i8*
  %_buffer56 = alloca i8
  %_buffer55 = alloca i8
  %_buffer54 = alloca i16
  %116 = bitcast i16* %_buffer54 to i8*
  %_buffer53 = alloca i8
  %_buffer52 = alloca i16
  %117 = bitcast i16* %_buffer52 to i8*
  %_buffer51 = alloca i32
  %118 = bitcast i32* %_buffer51 to i8*
  %_buffer50 = alloca i8
  %_buffer49 = alloca i8
  %_buffer48 = alloca i16
  %119 = bitcast i16* %_buffer48 to i8*
  %_buffer47 = alloca i8
  %_buffer46 = alloca i16
  %120 = bitcast i16* %_buffer46 to i8*
  %_buffer45 = alloca i16
  %121 = bitcast i16* %_buffer45 to i8*
  %_buffer44 = alloca i8
  %_buffer43 = alloca i16
  %122 = bitcast i16* %_buffer43 to i8*
  %_buffer42 = alloca i8
  %_buffer41 = alloca i8
  %_buffer40 = alloca i8
  %_buffer39 = alloca i8
  %_buffer38 = alloca i32
  %123 = bitcast i32* %_buffer38 to i8*
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
  %_buffer22 = alloca i16
  %124 = bitcast i16* %_buffer22 to i8*
  %_buffer21 = alloca i8
  %_buffer20 = alloca i16
  %125 = bitcast i16* %_buffer20 to i8*
  %_buffer19 = alloca i64
  %126 = bitcast i64* %_buffer19 to i8*
  %_buffer18 = alloca i8
  %_buffer17 = alloca i8
  %_buffer16 = alloca i8
  %_buffer15 = alloca i8
  %_buffer14 = alloca i8
  %_buffer13 = alloca i8
  %_buffer12 = alloca i8
  %_buffer11 = alloca i8
  %_buffer10 = alloca i8
  %_buffer9 = alloca i8
  %_buffer8 = alloca i8
  %_buffer7 = alloca i8
  %_buffer6 = alloca i8
  %_buffer5 = alloca i8
  %_buffer4 = alloca i8
  %_buffer3 = alloca i8
  %_buffer = alloca i8
  %127 = call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 32768)
  %128 = add i64 %127, -1
  %129 = icmp ugt i64 14, %128
  br i1 %129, label %cleanup3277, label %if.end1118

parse_ipv4:                                       ; preds = %if.end1118
  %130 = icmp ugt i64 34, %128
  br i1 %130, label %cleanup3277, label %if.end

if.end:                                           ; preds = %parse_ipv4
  %131 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer119, i64 15, i64 1)
  %132 = load i8, i8* %_buffer119
  %133 = lshr i8 %132, 4
  %134 = and i8 %132, 15
  %135 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer58, i64 16, i64 1)
  %136 = load i8, i8* %_buffer58
  %137 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %105, i64 17, i64 2)
  %138 = load i16, i16* %_buffer144
  %rev4038 = tail call i16 @llvm.bswap.i16(i16 %138)
  %hdr.sroa.314064.sroa.0.0.extract.trunc = trunc i16 %rev4038 to i8
  %hdr.sroa.314064.sroa.7.0.extract.shift = lshr i16 %rev4038, 8
  %hdr.sroa.314064.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.314064.sroa.7.0.extract.shift to i8
  %139 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %103, i64 19, i64 2)
  %140 = load i16, i16* %_buffer171
  %rev4037 = tail call i16 @llvm.bswap.i16(i16 %140)
  %hdr.sroa.36.sroa.0.0.extract.trunc4192 = trunc i16 %rev4037 to i8
  %hdr.sroa.36.sroa.7.0.extract.shift4193 = lshr i16 %rev4037, 8
  %hdr.sroa.36.sroa.7.0.extract.trunc4194 = trunc i16 %hdr.sroa.36.sroa.7.0.extract.shift4193 to i8
  %141 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer47, i64 21, i64 1)
  %142 = load i8, i8* %_buffer47
  %143 = lshr i8 %142, 5
  %144 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %120, i64 21, i64 2)
  %145 = load i16, i16* %_buffer46
  %146 = and i16 %145, -225
  %147 = tail call i16 @llvm.bswap.i16(i16 %146)
  %hdr.sroa.434069.sroa.0.0.extract.trunc = trunc i16 %147 to i8
  %hdr.sroa.434069.sroa.5.0.extract.shift = lshr i16 %147, 8
  %hdr.sroa.434069.sroa.5.0.extract.trunc = trunc i16 %hdr.sroa.434069.sroa.5.0.extract.shift to i8
  %148 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer153, i64 23, i64 1)
  %149 = load i8, i8* %_buffer153
  %150 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer44, i64 24, i64 1)
  %151 = load i8, i8* %_buffer44
  %152 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %112, i64 25, i64 2)
  %153 = load i16, i16* %_buffer79
  %rev4035 = tail call i16 @llvm.bswap.i16(i16 %153)
  %hdr.sroa.51.sroa.0.0.extract.trunc = trunc i16 %rev4035 to i8
  %hdr.sroa.51.sroa.7.0.extract.shift = lshr i16 %rev4035, 8
  %hdr.sroa.51.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.51.sroa.7.0.extract.shift to i8
  %154 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %123, i64 27, i64 4)
  %155 = load i32, i32* %_buffer38
  %or178 = tail call i32 @llvm.bswap.i32(i32 %155)
  %hdr.sroa.56.sroa.0.0.extract.trunc = trunc i32 %or178 to i8
  %hdr.sroa.56.sroa.7.0.extract.shift = lshr i32 %or178, 8
  %hdr.sroa.56.sroa.7.0.extract.trunc = trunc i32 %hdr.sroa.56.sroa.7.0.extract.shift to i8
  %hdr.sroa.56.sroa.8.0.extract.shift = lshr i32 %or178, 16
  %hdr.sroa.56.sroa.8.0.extract.trunc = trunc i32 %hdr.sroa.56.sroa.8.0.extract.shift to i8
  %hdr.sroa.56.sroa.9.0.extract.shift = lshr i32 %or178, 24
  %hdr.sroa.56.sroa.9.0.extract.trunc = trunc i32 %hdr.sroa.56.sroa.9.0.extract.shift to i8
  %156 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %104, i64 31, i64 4)
  %157 = load i32, i32* %_buffer149
  %or213 = tail call i32 @llvm.bswap.i32(i32 %157)
  %hdr.sroa.63.sroa.0.0.extract.trunc4175 = trunc i32 %or213 to i8
  %hdr.sroa.63.sroa.7.0.extract.shift4176 = lshr i32 %or213, 8
  %hdr.sroa.63.sroa.7.0.extract.trunc4177 = trunc i32 %hdr.sroa.63.sroa.7.0.extract.shift4176 to i8
  %hdr.sroa.63.sroa.8.0.extract.shift4178 = lshr i32 %or213, 16
  %hdr.sroa.63.sroa.8.0.extract.trunc4179 = trunc i32 %hdr.sroa.63.sroa.8.0.extract.shift4178 to i8
  %hdr.sroa.63.sroa.9.0.extract.shift4180 = lshr i32 %or213, 24
  %hdr.sroa.63.sroa.9.0.extract.trunc4181 = trunc i32 %hdr.sroa.63.sroa.9.0.extract.shift4180 to i8
  %switch.selectcmp4047 = icmp eq i8 %151, 17
  %switch.select4048 = select i1 %switch.selectcmp4047, i32 6, i32 7
  %switch.selectcmp4049 = icmp eq i8 %151, 6
  %switch.select4050 = select i1 %switch.selectcmp4049, i32 5, i32 %switch.select4048
  switch i32 %switch.select4050, label %cleanup32774040 [
    i32 5, label %parse_tcp
    i32 6, label %parse_udp
    i32 4, label %cleanup3277
    i32 7, label %if.then1258
  ]

parse_ipv6:                                       ; preds = %if.end1118
  %158 = icmp ugt i64 54, %128
  br i1 %158, label %cleanup3277, label %if.end247

if.end247:                                        ; preds = %parse_ipv6
  %159 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer21, i64 15, i64 1)
  %160 = load i8, i8* %_buffer21
  %161 = lshr i8 %160, 4
  %162 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %125, i64 15, i64 2)
  %163 = load i16, i16* %_buffer20
  %rev4032 = tail call i16 @llvm.bswap.i16(i16 %163)
  %164 = lshr i16 %rev4032, 4
  %conv285 = trunc i16 %164 to i8
  %165 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %118, i64 16, i64 4)
  %166 = load i32, i32* %_buffer51
  %or314 = tail call i32 @llvm.bswap.i32(i32 %166)
  %shr321 = lshr i32 %or314, 8
  %hdr.sroa.784084.sroa.0.0.extract.trunc = trunc i32 %shr321 to i8
  %hdr.sroa.784084.sroa.5.0.extract.shift = lshr i32 %or314, 16
  %hdr.sroa.784084.sroa.5.0.extract.trunc = trunc i32 %hdr.sroa.784084.sroa.5.0.extract.shift to i8
  %and322 = lshr i32 %or314, 24
  %167 = trunc i32 %and322 to i8
  %hdr.sroa.784084.sroa.6.0.extract.trunc = and i8 %167, 15
  %168 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %124, i64 19, i64 2)
  %169 = load i16, i16* %_buffer22
  %rev4030 = tail call i16 @llvm.bswap.i16(i16 %169)
  %hdr.sroa.82.sroa.0.0.extract.trunc4169 = trunc i16 %rev4030 to i8
  %hdr.sroa.82.sroa.7.0.extract.shift4170 = lshr i16 %rev4030, 8
  %hdr.sroa.82.sroa.7.0.extract.trunc4171 = trunc i16 %hdr.sroa.82.sroa.7.0.extract.shift4170 to i8
  %170 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer132, i64 21, i64 1)
  %171 = load i8, i8* %_buffer132
  %172 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer136, i64 22, i64 1)
  %173 = load i8, i8* %_buffer136
  %174 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer89, i64 23, i64 1)
  %175 = load i8, i8* %_buffer89
  %176 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer100, i64 24, i64 1)
  %177 = load i8, i8* %_buffer100
  %178 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer32, i64 25, i64 1)
  %179 = load i8, i8* %_buffer32
  %180 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer34, i64 26, i64 1)
  %181 = load i8, i8* %_buffer34
  %182 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer50, i64 27, i64 1)
  %183 = load i8, i8* %_buffer50
  %184 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer36, i64 28, i64 1)
  %185 = load i8, i8* %_buffer36
  %186 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer110, i64 29, i64 1)
  %187 = load i8, i8* %_buffer110
  %188 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer39, i64 30, i64 1)
  %189 = load i8, i8* %_buffer39
  %190 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer41, i64 31, i64 1)
  %191 = load i8, i8* %_buffer41
  %192 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer30, i64 32, i64 1)
  %193 = load i8, i8* %_buffer30
  %194 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer161, i64 33, i64 1)
  %195 = load i8, i8* %_buffer161
  %196 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer128, i64 34, i64 1)
  %197 = load i8, i8* %_buffer128
  %198 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer23, i64 35, i64 1)
  %199 = load i8, i8* %_buffer23
  %200 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer24, i64 36, i64 1)
  %201 = load i8, i8* %_buffer24
  %202 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer25, i64 37, i64 1)
  %203 = load i8, i8* %_buffer25
  %204 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer26, i64 38, i64 1)
  %205 = load i8, i8* %_buffer26
  %206 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer27, i64 39, i64 1)
  %207 = load i8, i8* %_buffer27
  %208 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer53, i64 40, i64 1)
  %209 = load i8, i8* %_buffer53
  %210 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer106, i64 41, i64 1)
  %211 = load i8, i8* %_buffer106
  %212 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer114, i64 42, i64 1)
  %213 = load i8, i8* %_buffer114
  %214 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer28, i64 43, i64 1)
  %215 = load i8, i8* %_buffer28
  %216 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer29, i64 44, i64 1)
  %217 = load i8, i8* %_buffer29
  %218 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer75, i64 45, i64 1)
  %219 = load i8, i8* %_buffer75
  %220 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer31, i64 46, i64 1)
  %221 = load i8, i8* %_buffer31
  %222 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer101, i64 47, i64 1)
  %223 = load i8, i8* %_buffer101
  %224 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer33, i64 48, i64 1)
  %225 = load i8, i8* %_buffer33
  %226 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer35, i64 49, i64 1)
  %227 = load i8, i8* %_buffer35
  %228 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer49, i64 50, i64 1)
  %229 = load i8, i8* %_buffer49
  %230 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer37, i64 51, i64 1)
  %231 = load i8, i8* %_buffer37
  %232 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer109, i64 52, i64 1)
  %233 = load i8, i8* %_buffer109
  %234 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer40, i64 53, i64 1)
  %235 = load i8, i8* %_buffer40
  %236 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer42, i64 54, i64 1)
  %237 = load i8, i8* %_buffer42
  %switch.selectcmp4051 = icmp eq i8 %171, 17
  %switch.select4052 = select i1 %switch.selectcmp4051, i32 6, i32 7
  %switch.selectcmp4053 = icmp eq i8 %171, 6
  %switch.select4054 = select i1 %switch.selectcmp4053, i32 5, i32 %switch.select4052
  switch i32 %switch.select4054, label %cleanup32774041 [
    i32 5, label %parse_tcp
    i32 6, label %parse_udp
    i32 4, label %cleanup3277
    i32 7, label %if.then1258
  ]

parse_tcp:                                        ; preds = %if.end247, %if.end
  %hdr.sroa.314064.sroa.7.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.314064.sroa.7.0.extract.trunc, %if.end ]
  %hdr.sroa.314064.sroa.0.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.314064.sroa.0.0.extract.trunc, %if.end ]
  %hdr.sroa.36.sroa.7.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.36.sroa.7.0.extract.trunc4194, %if.end ]
  %hdr.sroa.36.sroa.0.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.36.sroa.0.0.extract.trunc4192, %if.end ]
  %hdr.sroa.434069.sroa.5.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.434069.sroa.5.0.extract.trunc, %if.end ]
  %hdr.sroa.434069.sroa.0.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.434069.sroa.0.0.extract.trunc, %if.end ]
  %hdr.sroa.51.sroa.7.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.51.sroa.7.0.extract.trunc, %if.end ]
  %hdr.sroa.51.sroa.0.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.51.sroa.0.0.extract.trunc, %if.end ]
  %hdr.sroa.56.sroa.9.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.56.sroa.9.0.extract.trunc, %if.end ]
  %hdr.sroa.56.sroa.8.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.56.sroa.8.0.extract.trunc, %if.end ]
  %hdr.sroa.56.sroa.7.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.56.sroa.7.0.extract.trunc, %if.end ]
  %hdr.sroa.56.sroa.0.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.56.sroa.0.0.extract.trunc, %if.end ]
  %hdr.sroa.63.sroa.9.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.63.sroa.9.0.extract.trunc4181, %if.end ]
  %hdr.sroa.63.sroa.8.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.63.sroa.8.0.extract.trunc4179, %if.end ]
  %hdr.sroa.63.sroa.7.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.63.sroa.7.0.extract.trunc4177, %if.end ]
  %hdr.sroa.63.sroa.0.0 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.63.sroa.0.0.extract.trunc4175, %if.end ]
  %hdr.sroa.784084.sroa.6.0 = phi i8 [ %hdr.sroa.784084.sroa.6.0.extract.trunc, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.784084.sroa.5.0 = phi i8 [ %hdr.sroa.784084.sroa.5.0.extract.trunc, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.784084.sroa.0.0 = phi i8 [ %hdr.sroa.784084.sroa.0.0.extract.trunc, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.82.sroa.7.0 = phi i8 [ %hdr.sroa.82.sroa.7.0.extract.trunc4171, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.82.sroa.0.0 = phi i8 [ %hdr.sroa.82.sroa.0.0.extract.trunc4169, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.156.0 = phi i8 [ 1, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.154.0 = phi i8 [ %237, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.152.0 = phi i8 [ %235, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.150.0 = phi i8 [ %233, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.148.0 = phi i8 [ %231, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.146.0 = phi i8 [ %229, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.144.0 = phi i8 [ %227, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.142.0 = phi i8 [ %225, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.140.0 = phi i8 [ %223, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.138.0 = phi i8 [ %221, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.136.0 = phi i8 [ %219, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.134.0 = phi i8 [ %217, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.132.0 = phi i8 [ %215, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.130.0 = phi i8 [ %213, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.128.0 = phi i8 [ %211, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.126.0 = phi i8 [ %209, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.124.0 = phi i8 [ %207, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.122.0 = phi i8 [ %205, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.120.0 = phi i8 [ %203, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.118.0 = phi i8 [ %201, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.116.0 = phi i8 [ %199, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.114.0 = phi i8 [ %197, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.112.0 = phi i8 [ %195, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.110.0 = phi i8 [ %193, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.108.0 = phi i8 [ %191, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.106.0 = phi i8 [ %189, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.104.0 = phi i8 [ %187, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.102.0 = phi i8 [ %185, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.100.0 = phi i8 [ %183, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.98.0 = phi i8 [ %181, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.96.0 = phi i8 [ %179, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.94.0 = phi i8 [ %177, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.92.0 = phi i8 [ %175, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.90.0 = phi i8 [ %173, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.87.0 = phi i8 [ %171, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.76.0 = phi i8 [ %conv285, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.744082.0 = phi i8 [ %161, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.70.0 = phi i8 [ 0, %if.end247 ], [ 1, %if.end ]
  %hdr.sroa.48.0 = phi i8 [ 0, %if.end247 ], [ %151, %if.end ]
  %hdr.sroa.46.0 = phi i8 [ 0, %if.end247 ], [ %149, %if.end ]
  %hdr.sroa.41.0 = phi i8 [ 0, %if.end247 ], [ %143, %if.end ]
  %hdr.sroa.29.0 = phi i8 [ 0, %if.end247 ], [ %136, %if.end ]
  %hdr.sroa.27.0 = phi i8 [ 0, %if.end247 ], [ %134, %if.end ]
  %hdr.sroa.254063.0 = phi i8 [ 0, %if.end247 ], [ %133, %if.end ]
  %ebpf_packetOffsetInBits.0 = phi i32 [ 432, %if.end247 ], [ 272, %if.end ]
  %add709 = add nuw nsw i32 %ebpf_packetOffsetInBits.0, 160
  %div710 = lshr exact i32 %add709, 3
  %idx.ext711 = zext i32 %div710 to i64
  %238 = add i64 0, %idx.ext711
  %239 = icmp ugt i64 %238, %128
  br i1 %239, label %cleanup3277, label %if.end716

if.end716:                                        ; preds = %parse_tcp
  %div717 = lshr exact i32 %ebpf_packetOffsetInBits.0, 3
  %idx.ext718 = zext i32 %div717 to i64
  %240 = add i64 0, %idx.ext718
  %241 = add i64 %240, 1
  %242 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %122, i64 %241, i64 2)
  %243 = load i16, i16* %_buffer43
  %rev4029 = tail call i16 @llvm.bswap.i16(i16 %243)
  %hdr.sroa.1604093.sroa.0.0.extract.trunc = trunc i16 %rev4029 to i8
  %hdr.sroa.1604093.sroa.7.0.extract.shift = lshr i16 %rev4029, 8
  %hdr.sroa.1604093.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.1604093.sroa.7.0.extract.shift to i8
  %add744 = add nuw nsw i32 %ebpf_packetOffsetInBits.0, 16
  %div745 = lshr exact i32 %add744, 3
  %idx.ext746 = zext i32 %div745 to i64
  %244 = add i64 0, %idx.ext746
  %245 = add i64 %244, 1
  %246 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %121, i64 %245, i64 2)
  %247 = load i16, i16* %_buffer45
  %rev4028 = tail call i16 @llvm.bswap.i16(i16 %247)
  %hdr.sroa.165.sroa.0.0.extract.trunc = trunc i16 %rev4028 to i8
  %hdr.sroa.165.sroa.7.0.extract.shift = lshr i16 %rev4028, 8
  %hdr.sroa.165.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.165.sroa.7.0.extract.shift to i8
  %add773 = add nuw nsw i32 %ebpf_packetOffsetInBits.0, 32
  %div774 = lshr exact i32 %add773, 3
  %idx.ext775 = zext i32 %div774 to i64
  %248 = add i64 0, %idx.ext775
  %249 = add i64 %248, 1
  %250 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %113, i64 %249, i64 4)
  %251 = load i32, i32* %_buffer78
  %or800 = tail call i32 @llvm.bswap.i32(i32 %251)
  %hdr.sroa.170.sroa.0.0.extract.trunc = trunc i32 %or800 to i8
  %hdr.sroa.170.sroa.7.0.extract.shift = lshr i32 %or800, 8
  %hdr.sroa.170.sroa.7.0.extract.trunc = trunc i32 %hdr.sroa.170.sroa.7.0.extract.shift to i8
  %hdr.sroa.170.sroa.8.0.extract.shift = lshr i32 %or800, 16
  %hdr.sroa.170.sroa.8.0.extract.trunc = trunc i32 %hdr.sroa.170.sroa.8.0.extract.shift to i8
  %hdr.sroa.170.sroa.9.0.extract.shift = lshr i32 %or800, 24
  %hdr.sroa.170.sroa.9.0.extract.trunc = trunc i32 %hdr.sroa.170.sroa.9.0.extract.shift to i8
  %add808 = or i32 %ebpf_packetOffsetInBits.0, 64
  %div809 = lshr exact i32 %add808, 3
  %idx.ext810 = zext i32 %div809 to i64
  %252 = add i64 0, %idx.ext810
  %253 = add i64 %252, 1
  %254 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %114, i64 %253, i64 4)
  %255 = load i32, i32* %_buffer73
  %or835 = tail call i32 @llvm.bswap.i32(i32 %255)
  %hdr.sroa.177.sroa.0.0.extract.trunc = trunc i32 %or835 to i8
  %hdr.sroa.177.sroa.7.0.extract.shift = lshr i32 %or835, 8
  %hdr.sroa.177.sroa.7.0.extract.trunc = trunc i32 %hdr.sroa.177.sroa.7.0.extract.shift to i8
  %hdr.sroa.177.sroa.8.0.extract.shift = lshr i32 %or835, 16
  %hdr.sroa.177.sroa.8.0.extract.trunc = trunc i32 %hdr.sroa.177.sroa.8.0.extract.shift to i8
  %hdr.sroa.177.sroa.9.0.extract.shift = lshr i32 %or835, 24
  %hdr.sroa.177.sroa.9.0.extract.trunc = trunc i32 %hdr.sroa.177.sroa.9.0.extract.shift to i8
  %add843 = add nuw nsw i32 %add808, 32
  %div844 = lshr exact i32 %add843, 3
  %idx.ext845 = zext i32 %div844 to i64
  %256 = add i64 0, %idx.ext845
  %257 = add i64 %256, 1
  %258 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer77, i64 %257, i64 1)
  %259 = load i8, i8* %_buffer77
  %260 = lshr i8 %259, 4
  %add852 = add nuw nsw i32 %add808, 36
  %div853 = lshr i32 %add852, 3
  %idx.ext854 = zext i32 %div853 to i64
  %261 = add i64 0, %idx.ext854
  %262 = add i64 %261, 1
  %263 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %119, i64 %262, i64 2)
  %264 = load i16, i16* %_buffer48
  %rev4025 = tail call i16 @llvm.bswap.i16(i16 %264)
  %265 = lshr i16 %rev4025, 6
  %266 = trunc i16 %265 to i8
  %conv881 = and i8 %266, 63
  %add883 = add nuw nsw i32 %add808, 42
  %div884 = lshr i32 %add883, 3
  %idx.ext885 = zext i32 %div884 to i64
  %267 = add i64 0, %idx.ext885
  %268 = add i64 %267, 1
  %269 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer152, i64 %268, i64 1)
  %270 = load i8, i8* %_buffer152
  %271 = and i8 %270, 63
  %add892 = add nuw nsw i32 %add808, 48
  %div893 = lshr exact i32 %add892, 3
  %idx.ext894 = zext i32 %div893 to i64
  %272 = add i64 0, %idx.ext894
  %273 = add i64 %272, 1
  %274 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %115, i64 %273, i64 2)
  %275 = load i16, i16* %_buffer57
  %rev4024 = tail call i16 @llvm.bswap.i16(i16 %275)
  %hdr.sroa.1904102.sroa.0.0.extract.trunc = trunc i16 %rev4024 to i8
  %hdr.sroa.1904102.sroa.7.0.extract.shift = lshr i16 %rev4024, 8
  %hdr.sroa.1904102.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.1904102.sroa.7.0.extract.shift to i8
  %add921 = add nuw nsw i32 %add808, 64
  %div922 = lshr exact i32 %add921, 3
  %idx.ext923 = zext i32 %div922 to i64
  %276 = add i64 0, %idx.ext923
  %277 = add i64 %276, 1
  %278 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %106, i64 %277, i64 2)
  %279 = load i16, i16* %_buffer143
  %rev4023 = tail call i16 @llvm.bswap.i16(i16 %279)
  %hdr.sroa.195.sroa.0.0.extract.trunc4140 = trunc i16 %rev4023 to i8
  %hdr.sroa.195.sroa.7.0.extract.shift4141 = lshr i16 %rev4023, 8
  %hdr.sroa.195.sroa.7.0.extract.trunc4142 = trunc i16 %hdr.sroa.195.sroa.7.0.extract.shift4141 to i8
  %add950 = add nuw nsw i32 %add808, 80
  %div951 = lshr exact i32 %add950, 3
  %idx.ext952 = zext i32 %div951 to i64
  %280 = add i64 0, %idx.ext952
  %281 = add i64 %280, 1
  %282 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %109, i64 %281, i64 2)
  %283 = load i16, i16* %_buffer88
  %rev4022 = tail call i16 @llvm.bswap.i16(i16 %283)
  %hdr.sroa.200.sroa.0.0.extract.trunc4137 = trunc i16 %rev4022 to i8
  %hdr.sroa.200.sroa.7.0.extract.shift4138 = lshr i16 %rev4022, 8
  %hdr.sroa.200.sroa.7.0.extract.trunc4139 = trunc i16 %hdr.sroa.200.sroa.7.0.extract.shift4138 to i8
  %add979 = add nuw nsw i32 %add808, 96
  br label %if.else1190

parse_udp:                                        ; preds = %if.end247, %if.end
  %hdr.sroa.314064.sroa.7.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.314064.sroa.7.0.extract.trunc, %if.end ]
  %hdr.sroa.314064.sroa.0.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.314064.sroa.0.0.extract.trunc, %if.end ]
  %hdr.sroa.36.sroa.7.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.36.sroa.7.0.extract.trunc4194, %if.end ]
  %hdr.sroa.36.sroa.0.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.36.sroa.0.0.extract.trunc4192, %if.end ]
  %hdr.sroa.434069.sroa.5.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.434069.sroa.5.0.extract.trunc, %if.end ]
  %hdr.sroa.434069.sroa.0.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.434069.sroa.0.0.extract.trunc, %if.end ]
  %hdr.sroa.51.sroa.7.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.51.sroa.7.0.extract.trunc, %if.end ]
  %hdr.sroa.51.sroa.0.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.51.sroa.0.0.extract.trunc, %if.end ]
  %hdr.sroa.56.sroa.9.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.56.sroa.9.0.extract.trunc, %if.end ]
  %hdr.sroa.56.sroa.8.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.56.sroa.8.0.extract.trunc, %if.end ]
  %hdr.sroa.56.sroa.7.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.56.sroa.7.0.extract.trunc, %if.end ]
  %hdr.sroa.56.sroa.0.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.56.sroa.0.0.extract.trunc, %if.end ]
  %hdr.sroa.63.sroa.9.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.63.sroa.9.0.extract.trunc4181, %if.end ]
  %hdr.sroa.63.sroa.8.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.63.sroa.8.0.extract.trunc4179, %if.end ]
  %hdr.sroa.63.sroa.7.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.63.sroa.7.0.extract.trunc4177, %if.end ]
  %hdr.sroa.63.sroa.0.1 = phi i8 [ 0, %if.end247 ], [ %hdr.sroa.63.sroa.0.0.extract.trunc4175, %if.end ]
  %hdr.sroa.784084.sroa.6.1 = phi i8 [ %hdr.sroa.784084.sroa.6.0.extract.trunc, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.784084.sroa.5.1 = phi i8 [ %hdr.sroa.784084.sroa.5.0.extract.trunc, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.784084.sroa.0.1 = phi i8 [ %hdr.sroa.784084.sroa.0.0.extract.trunc, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.82.sroa.7.1 = phi i8 [ %hdr.sroa.82.sroa.7.0.extract.trunc4171, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.82.sroa.0.1 = phi i8 [ %hdr.sroa.82.sroa.0.0.extract.trunc4169, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.156.1 = phi i8 [ 1, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.154.1 = phi i8 [ %237, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.152.1 = phi i8 [ %235, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.150.1 = phi i8 [ %233, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.148.1 = phi i8 [ %231, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.146.1 = phi i8 [ %229, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.144.1 = phi i8 [ %227, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.142.1 = phi i8 [ %225, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.140.1 = phi i8 [ %223, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.138.1 = phi i8 [ %221, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.136.1 = phi i8 [ %219, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.134.1 = phi i8 [ %217, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.132.1 = phi i8 [ %215, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.130.1 = phi i8 [ %213, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.128.1 = phi i8 [ %211, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.126.1 = phi i8 [ %209, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.124.1 = phi i8 [ %207, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.122.1 = phi i8 [ %205, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.120.1 = phi i8 [ %203, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.118.1 = phi i8 [ %201, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.116.1 = phi i8 [ %199, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.114.1 = phi i8 [ %197, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.112.1 = phi i8 [ %195, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.110.1 = phi i8 [ %193, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.108.1 = phi i8 [ %191, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.106.1 = phi i8 [ %189, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.104.1 = phi i8 [ %187, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.102.1 = phi i8 [ %185, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.100.1 = phi i8 [ %183, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.98.1 = phi i8 [ %181, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.96.1 = phi i8 [ %179, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.94.1 = phi i8 [ %177, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.92.1 = phi i8 [ %175, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.90.1 = phi i8 [ %173, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.87.1 = phi i8 [ %171, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.76.1 = phi i8 [ %conv285, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.744082.1 = phi i8 [ %161, %if.end247 ], [ 0, %if.end ]
  %hdr.sroa.70.1 = phi i8 [ 0, %if.end247 ], [ 1, %if.end ]
  %hdr.sroa.48.1 = phi i8 [ 0, %if.end247 ], [ %151, %if.end ]
  %hdr.sroa.46.1 = phi i8 [ 0, %if.end247 ], [ %149, %if.end ]
  %hdr.sroa.41.1 = phi i8 [ 0, %if.end247 ], [ %143, %if.end ]
  %hdr.sroa.29.1 = phi i8 [ 0, %if.end247 ], [ %136, %if.end ]
  %hdr.sroa.27.1 = phi i8 [ 0, %if.end247 ], [ %134, %if.end ]
  %hdr.sroa.254063.1 = phi i8 [ 0, %if.end247 ], [ %133, %if.end ]
  %ebpf_packetOffsetInBits.1 = phi i32 [ 432, %if.end247 ], [ 272, %if.end ]
  %add982 = lshr exact i32 %ebpf_packetOffsetInBits.1, 3
  %div983 = or i32 %add982, 8
  %idx.ext984 = zext i32 %div983 to i64
  %284 = add i64 0, %idx.ext984
  %285 = icmp ugt i64 %284, %128
  br i1 %285, label %cleanup3277, label %if.end989

if.end989:                                        ; preds = %parse_udp
  %idx.ext991 = zext i32 %add982 to i64
  %286 = add i64 0, %idx.ext991
  %287 = add i64 %286, 1
  %288 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %117, i64 %287, i64 2)
  %289 = load i16, i16* %_buffer52
  %rev4021 = tail call i16 @llvm.bswap.i16(i16 %289)
  %hdr.sroa.2084110.sroa.0.0.extract.trunc = trunc i16 %rev4021 to i8
  %hdr.sroa.2084110.sroa.7.0.extract.shift = lshr i16 %rev4021, 8
  %hdr.sroa.2084110.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.2084110.sroa.7.0.extract.shift to i8
  %add1018 = add nuw nsw i32 %ebpf_packetOffsetInBits.1, 16
  %div1019 = lshr exact i32 %add1018, 3
  %idx.ext1020 = zext i32 %div1019 to i64
  %290 = add i64 0, %idx.ext1020
  %291 = add i64 %290, 1
  %292 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %111, i64 %291, i64 2)
  %293 = load i16, i16* %_buffer83
  %rev4020 = tail call i16 @llvm.bswap.i16(i16 %293)
  %hdr.sroa.213.sroa.0.0.extract.trunc4131 = trunc i16 %rev4020 to i8
  %hdr.sroa.213.sroa.7.0.extract.shift4132 = lshr i16 %rev4020, 8
  %hdr.sroa.213.sroa.7.0.extract.trunc4133 = trunc i16 %hdr.sroa.213.sroa.7.0.extract.shift4132 to i8
  %add1048 = add nuw nsw i32 %ebpf_packetOffsetInBits.1, 32
  %div1049 = lshr exact i32 %add1048, 3
  %idx.ext1050 = zext i32 %div1049 to i64
  %294 = add i64 0, %idx.ext1050
  %295 = add i64 %294, 1
  %296 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %107, i64 %295, i64 2)
  %297 = load i16, i16* %_buffer112
  %rev4019 = tail call i16 @llvm.bswap.i16(i16 %297)
  %hdr.sroa.218.sroa.0.0.extract.trunc4128 = trunc i16 %rev4019 to i8
  %hdr.sroa.218.sroa.7.0.extract.shift4129 = lshr i16 %rev4019, 8
  %hdr.sroa.218.sroa.7.0.extract.trunc4130 = trunc i16 %hdr.sroa.218.sroa.7.0.extract.shift4129 to i8
  %add1078 = add nuw nsw i32 %ebpf_packetOffsetInBits.1, 48
  %div1079 = lshr exact i32 %add1078, 3
  %idx.ext1080 = zext i32 %div1079 to i64
  %298 = add i64 0, %idx.ext1080
  %299 = add i64 %298, 1
  %300 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %116, i64 %299, i64 2)
  %301 = load i16, i16* %_buffer54
  %rev4018 = tail call i16 @llvm.bswap.i16(i16 %301)
  %hdr.sroa.223.sroa.0.0.extract.trunc4125 = trunc i16 %rev4018 to i8
  %hdr.sroa.223.sroa.7.0.extract.shift4126 = lshr i16 %rev4018, 8
  %hdr.sroa.223.sroa.7.0.extract.trunc4127 = trunc i16 %hdr.sroa.223.sroa.7.0.extract.shift4126 to i8
  %add1108 = or i32 %ebpf_packetOffsetInBits.1, 64
  br label %if.else1190

if.end1118:                                       ; preds = %entry
  %302 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %126, i64 1, i64 8)
  %303 = load i64, i64* %_buffer19
  %hdr.sroa.0.sroa.0.0.extract.trunc = trunc i64 %303 to i8
  %hdr.sroa.0.sroa.5.0.extract.shift = lshr i64 %303, 8
  %hdr.sroa.0.sroa.5.0.extract.trunc = trunc i64 %hdr.sroa.0.sroa.5.0.extract.shift to i8
  %hdr.sroa.0.sroa.6.0.extract.shift = lshr i64 %303, 16
  %hdr.sroa.0.sroa.6.0.extract.trunc = trunc i64 %hdr.sroa.0.sroa.6.0.extract.shift to i8
  %hdr.sroa.0.sroa.7.0.extract.shift = lshr i64 %303, 24
  %hdr.sroa.0.sroa.7.0.extract.trunc = trunc i64 %hdr.sroa.0.sroa.7.0.extract.shift to i8
  %hdr.sroa.0.sroa.8.0.extract.shift = lshr i64 %303, 32
  %hdr.sroa.0.sroa.8.0.extract.trunc = trunc i64 %hdr.sroa.0.sroa.8.0.extract.shift to i8
  %shl1122 = lshr i64 %303, 40
  %hdr.sroa.0.sroa.9.0.extract.trunc = trunc i64 %shl1122 to i8
  %304 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %108, i64 7, i64 8)
  %305 = load i64, i64* %_buffer90
  %hdr.sroa.10.sroa.0.0.extract.trunc = trunc i64 %305 to i8
  %hdr.sroa.10.sroa.5.0.extract.shift = lshr i64 %305, 8
  %hdr.sroa.10.sroa.5.0.extract.trunc = trunc i64 %hdr.sroa.10.sroa.5.0.extract.shift to i8
  %hdr.sroa.10.sroa.6.0.extract.shift = lshr i64 %305, 16
  %hdr.sroa.10.sroa.6.0.extract.trunc = trunc i64 %hdr.sroa.10.sroa.6.0.extract.shift to i8
  %hdr.sroa.10.sroa.7.0.extract.shift = lshr i64 %305, 24
  %hdr.sroa.10.sroa.7.0.extract.trunc = trunc i64 %hdr.sroa.10.sroa.7.0.extract.shift to i8
  %hdr.sroa.10.sroa.8.0.extract.shift = lshr i64 %305, 32
  %hdr.sroa.10.sroa.8.0.extract.trunc = trunc i64 %hdr.sroa.10.sroa.8.0.extract.shift to i8
  %shl1129 = lshr i64 %305, 40
  %hdr.sroa.10.sroa.9.0.extract.trunc = trunc i64 %shl1129 to i8
  %306 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %110, i64 13, i64 2)
  %307 = load i16, i16* %_buffer84
  %switch.selectcmp = icmp eq i16 %307, -8826
  %switch.select = select i1 %switch.selectcmp, i32 8, i32 7
  %switch.selectcmp4045 = icmp eq i16 %307, 8
  %switch.select4046 = select i1 %switch.selectcmp4045, i32 3, i32 %switch.select
  switch i32 %switch.select4046, label %cleanup3277 [
    i32 3, label %parse_ipv4
    i32 8, label %parse_ipv6
    i32 7, label %if.else1190.thread.thread
  ]

if.else1190:                                      ; preds = %if.end989, %if.end716
  %hdr.sroa.314064.sroa.7.2 = phi i8 [ %hdr.sroa.314064.sroa.7.1, %if.end989 ], [ %hdr.sroa.314064.sroa.7.0, %if.end716 ]
  %hdr.sroa.314064.sroa.0.2 = phi i8 [ %hdr.sroa.314064.sroa.0.1, %if.end989 ], [ %hdr.sroa.314064.sroa.0.0, %if.end716 ]
  %hdr.sroa.36.sroa.7.2 = phi i8 [ %hdr.sroa.36.sroa.7.1, %if.end989 ], [ %hdr.sroa.36.sroa.7.0, %if.end716 ]
  %hdr.sroa.36.sroa.0.2 = phi i8 [ %hdr.sroa.36.sroa.0.1, %if.end989 ], [ %hdr.sroa.36.sroa.0.0, %if.end716 ]
  %hdr.sroa.434069.sroa.5.2 = phi i8 [ %hdr.sroa.434069.sroa.5.1, %if.end989 ], [ %hdr.sroa.434069.sroa.5.0, %if.end716 ]
  %hdr.sroa.434069.sroa.0.2 = phi i8 [ %hdr.sroa.434069.sroa.0.1, %if.end989 ], [ %hdr.sroa.434069.sroa.0.0, %if.end716 ]
  %hdr.sroa.51.sroa.7.2 = phi i8 [ %hdr.sroa.51.sroa.7.1, %if.end989 ], [ %hdr.sroa.51.sroa.7.0, %if.end716 ]
  %hdr.sroa.51.sroa.0.2 = phi i8 [ %hdr.sroa.51.sroa.0.1, %if.end989 ], [ %hdr.sroa.51.sroa.0.0, %if.end716 ]
  %hdr.sroa.56.sroa.9.2 = phi i8 [ %hdr.sroa.56.sroa.9.1, %if.end989 ], [ %hdr.sroa.56.sroa.9.0, %if.end716 ]
  %hdr.sroa.56.sroa.8.2 = phi i8 [ %hdr.sroa.56.sroa.8.1, %if.end989 ], [ %hdr.sroa.56.sroa.8.0, %if.end716 ]
  %hdr.sroa.56.sroa.7.2 = phi i8 [ %hdr.sroa.56.sroa.7.1, %if.end989 ], [ %hdr.sroa.56.sroa.7.0, %if.end716 ]
  %hdr.sroa.56.sroa.0.2 = phi i8 [ %hdr.sroa.56.sroa.0.1, %if.end989 ], [ %hdr.sroa.56.sroa.0.0, %if.end716 ]
  %hdr.sroa.63.sroa.9.2 = phi i8 [ %hdr.sroa.63.sroa.9.1, %if.end989 ], [ %hdr.sroa.63.sroa.9.0, %if.end716 ]
  %hdr.sroa.63.sroa.8.2 = phi i8 [ %hdr.sroa.63.sroa.8.1, %if.end989 ], [ %hdr.sroa.63.sroa.8.0, %if.end716 ]
  %hdr.sroa.63.sroa.7.2 = phi i8 [ %hdr.sroa.63.sroa.7.1, %if.end989 ], [ %hdr.sroa.63.sroa.7.0, %if.end716 ]
  %hdr.sroa.63.sroa.0.2 = phi i8 [ %hdr.sroa.63.sroa.0.1, %if.end989 ], [ %hdr.sroa.63.sroa.0.0, %if.end716 ]
  %hdr.sroa.784084.sroa.6.2 = phi i8 [ %hdr.sroa.784084.sroa.6.1, %if.end989 ], [ %hdr.sroa.784084.sroa.6.0, %if.end716 ]
  %hdr.sroa.784084.sroa.5.2 = phi i8 [ %hdr.sroa.784084.sroa.5.1, %if.end989 ], [ %hdr.sroa.784084.sroa.5.0, %if.end716 ]
  %hdr.sroa.784084.sroa.0.2 = phi i8 [ %hdr.sroa.784084.sroa.0.1, %if.end989 ], [ %hdr.sroa.784084.sroa.0.0, %if.end716 ]
  %hdr.sroa.82.sroa.7.2 = phi i8 [ %hdr.sroa.82.sroa.7.1, %if.end989 ], [ %hdr.sroa.82.sroa.7.0, %if.end716 ]
  %hdr.sroa.82.sroa.0.2 = phi i8 [ %hdr.sroa.82.sroa.0.1, %if.end989 ], [ %hdr.sroa.82.sroa.0.0, %if.end716 ]
  %hdr.sroa.1604093.sroa.7.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.1604093.sroa.7.0.extract.trunc, %if.end716 ]
  %hdr.sroa.1604093.sroa.0.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.1604093.sroa.0.0.extract.trunc, %if.end716 ]
  %hdr.sroa.165.sroa.7.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.165.sroa.7.0.extract.trunc, %if.end716 ]
  %hdr.sroa.165.sroa.0.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.165.sroa.0.0.extract.trunc, %if.end716 ]
  %hdr.sroa.170.sroa.9.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.170.sroa.9.0.extract.trunc, %if.end716 ]
  %hdr.sroa.170.sroa.8.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.170.sroa.8.0.extract.trunc, %if.end716 ]
  %hdr.sroa.170.sroa.7.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.170.sroa.7.0.extract.trunc, %if.end716 ]
  %hdr.sroa.170.sroa.0.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.170.sroa.0.0.extract.trunc, %if.end716 ]
  %hdr.sroa.177.sroa.9.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.177.sroa.9.0.extract.trunc, %if.end716 ]
  %hdr.sroa.177.sroa.8.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.177.sroa.8.0.extract.trunc, %if.end716 ]
  %hdr.sroa.177.sroa.7.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.177.sroa.7.0.extract.trunc, %if.end716 ]
  %hdr.sroa.177.sroa.0.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.177.sroa.0.0.extract.trunc, %if.end716 ]
  %hdr.sroa.1904102.sroa.7.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.1904102.sroa.7.0.extract.trunc, %if.end716 ]
  %hdr.sroa.1904102.sroa.0.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.1904102.sroa.0.0.extract.trunc, %if.end716 ]
  %hdr.sroa.195.sroa.7.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.195.sroa.7.0.extract.trunc4142, %if.end716 ]
  %hdr.sroa.195.sroa.0.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.195.sroa.0.0.extract.trunc4140, %if.end716 ]
  %hdr.sroa.200.sroa.7.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.200.sroa.7.0.extract.trunc4139, %if.end716 ]
  %hdr.sroa.200.sroa.0.0 = phi i8 [ 0, %if.end989 ], [ %hdr.sroa.200.sroa.0.0.extract.trunc4137, %if.end716 ]
  %hdr.sroa.2084110.sroa.7.0 = phi i8 [ %hdr.sroa.2084110.sroa.7.0.extract.trunc, %if.end989 ], [ 0, %if.end716 ]
  %hdr.sroa.2084110.sroa.0.0 = phi i8 [ %hdr.sroa.2084110.sroa.0.0.extract.trunc, %if.end989 ], [ 0, %if.end716 ]
  %hdr.sroa.213.sroa.7.0 = phi i8 [ %hdr.sroa.213.sroa.7.0.extract.trunc4133, %if.end989 ], [ 0, %if.end716 ]
  %hdr.sroa.213.sroa.0.0 = phi i8 [ %hdr.sroa.213.sroa.0.0.extract.trunc4131, %if.end989 ], [ 0, %if.end716 ]
  %hdr.sroa.218.sroa.7.0 = phi i8 [ %hdr.sroa.218.sroa.7.0.extract.trunc4130, %if.end989 ], [ 0, %if.end716 ]
  %hdr.sroa.218.sroa.0.0 = phi i8 [ %hdr.sroa.218.sroa.0.0.extract.trunc4128, %if.end989 ], [ 0, %if.end716 ]
  %hdr.sroa.223.sroa.7.0 = phi i8 [ %hdr.sroa.223.sroa.7.0.extract.trunc4127, %if.end989 ], [ 0, %if.end716 ]
  %hdr.sroa.223.sroa.0.0 = phi i8 [ %hdr.sroa.223.sroa.0.0.extract.trunc4125, %if.end989 ], [ 0, %if.end716 ]
  %hdr.sroa.228.0 = phi i8 [ 1, %if.end989 ], [ 0, %if.end716 ]
  %hdr.sroa.205.0 = phi i8 [ 0, %if.end989 ], [ 1, %if.end716 ]
  %hdr.sroa.188.0 = phi i8 [ 0, %if.end989 ], [ %271, %if.end716 ]
  %hdr.sroa.186.0 = phi i8 [ 0, %if.end989 ], [ %conv881, %if.end716 ]
  %hdr.sroa.184.0 = phi i8 [ 0, %if.end989 ], [ %260, %if.end716 ]
  %hdr.sroa.156.2 = phi i8 [ %hdr.sroa.156.1, %if.end989 ], [ %hdr.sroa.156.0, %if.end716 ]
  %hdr.sroa.154.2 = phi i8 [ %hdr.sroa.154.1, %if.end989 ], [ %hdr.sroa.154.0, %if.end716 ]
  %hdr.sroa.152.2 = phi i8 [ %hdr.sroa.152.1, %if.end989 ], [ %hdr.sroa.152.0, %if.end716 ]
  %hdr.sroa.150.2 = phi i8 [ %hdr.sroa.150.1, %if.end989 ], [ %hdr.sroa.150.0, %if.end716 ]
  %hdr.sroa.148.2 = phi i8 [ %hdr.sroa.148.1, %if.end989 ], [ %hdr.sroa.148.0, %if.end716 ]
  %hdr.sroa.146.2 = phi i8 [ %hdr.sroa.146.1, %if.end989 ], [ %hdr.sroa.146.0, %if.end716 ]
  %hdr.sroa.144.2 = phi i8 [ %hdr.sroa.144.1, %if.end989 ], [ %hdr.sroa.144.0, %if.end716 ]
  %hdr.sroa.142.2 = phi i8 [ %hdr.sroa.142.1, %if.end989 ], [ %hdr.sroa.142.0, %if.end716 ]
  %hdr.sroa.140.2 = phi i8 [ %hdr.sroa.140.1, %if.end989 ], [ %hdr.sroa.140.0, %if.end716 ]
  %hdr.sroa.138.2 = phi i8 [ %hdr.sroa.138.1, %if.end989 ], [ %hdr.sroa.138.0, %if.end716 ]
  %hdr.sroa.136.2 = phi i8 [ %hdr.sroa.136.1, %if.end989 ], [ %hdr.sroa.136.0, %if.end716 ]
  %hdr.sroa.134.2 = phi i8 [ %hdr.sroa.134.1, %if.end989 ], [ %hdr.sroa.134.0, %if.end716 ]
  %hdr.sroa.132.2 = phi i8 [ %hdr.sroa.132.1, %if.end989 ], [ %hdr.sroa.132.0, %if.end716 ]
  %hdr.sroa.130.2 = phi i8 [ %hdr.sroa.130.1, %if.end989 ], [ %hdr.sroa.130.0, %if.end716 ]
  %hdr.sroa.128.2 = phi i8 [ %hdr.sroa.128.1, %if.end989 ], [ %hdr.sroa.128.0, %if.end716 ]
  %hdr.sroa.126.2 = phi i8 [ %hdr.sroa.126.1, %if.end989 ], [ %hdr.sroa.126.0, %if.end716 ]
  %hdr.sroa.124.2 = phi i8 [ %hdr.sroa.124.1, %if.end989 ], [ %hdr.sroa.124.0, %if.end716 ]
  %hdr.sroa.122.2 = phi i8 [ %hdr.sroa.122.1, %if.end989 ], [ %hdr.sroa.122.0, %if.end716 ]
  %hdr.sroa.120.2 = phi i8 [ %hdr.sroa.120.1, %if.end989 ], [ %hdr.sroa.120.0, %if.end716 ]
  %hdr.sroa.118.2 = phi i8 [ %hdr.sroa.118.1, %if.end989 ], [ %hdr.sroa.118.0, %if.end716 ]
  %hdr.sroa.116.2 = phi i8 [ %hdr.sroa.116.1, %if.end989 ], [ %hdr.sroa.116.0, %if.end716 ]
  %hdr.sroa.114.2 = phi i8 [ %hdr.sroa.114.1, %if.end989 ], [ %hdr.sroa.114.0, %if.end716 ]
  %hdr.sroa.112.2 = phi i8 [ %hdr.sroa.112.1, %if.end989 ], [ %hdr.sroa.112.0, %if.end716 ]
  %hdr.sroa.110.2 = phi i8 [ %hdr.sroa.110.1, %if.end989 ], [ %hdr.sroa.110.0, %if.end716 ]
  %hdr.sroa.108.2 = phi i8 [ %hdr.sroa.108.1, %if.end989 ], [ %hdr.sroa.108.0, %if.end716 ]
  %hdr.sroa.106.2 = phi i8 [ %hdr.sroa.106.1, %if.end989 ], [ %hdr.sroa.106.0, %if.end716 ]
  %hdr.sroa.104.2 = phi i8 [ %hdr.sroa.104.1, %if.end989 ], [ %hdr.sroa.104.0, %if.end716 ]
  %hdr.sroa.102.2 = phi i8 [ %hdr.sroa.102.1, %if.end989 ], [ %hdr.sroa.102.0, %if.end716 ]
  %hdr.sroa.100.2 = phi i8 [ %hdr.sroa.100.1, %if.end989 ], [ %hdr.sroa.100.0, %if.end716 ]
  %hdr.sroa.98.2 = phi i8 [ %hdr.sroa.98.1, %if.end989 ], [ %hdr.sroa.98.0, %if.end716 ]
  %hdr.sroa.96.2 = phi i8 [ %hdr.sroa.96.1, %if.end989 ], [ %hdr.sroa.96.0, %if.end716 ]
  %hdr.sroa.94.2 = phi i8 [ %hdr.sroa.94.1, %if.end989 ], [ %hdr.sroa.94.0, %if.end716 ]
  %hdr.sroa.92.2 = phi i8 [ %hdr.sroa.92.1, %if.end989 ], [ %hdr.sroa.92.0, %if.end716 ]
  %hdr.sroa.90.2 = phi i8 [ %hdr.sroa.90.1, %if.end989 ], [ %hdr.sroa.90.0, %if.end716 ]
  %hdr.sroa.87.2 = phi i8 [ %hdr.sroa.87.1, %if.end989 ], [ %hdr.sroa.87.0, %if.end716 ]
  %hdr.sroa.76.2 = phi i8 [ %hdr.sroa.76.1, %if.end989 ], [ %hdr.sroa.76.0, %if.end716 ]
  %hdr.sroa.744082.2 = phi i8 [ %hdr.sroa.744082.1, %if.end989 ], [ %hdr.sroa.744082.0, %if.end716 ]
  %hdr.sroa.70.2 = phi i8 [ %hdr.sroa.70.1, %if.end989 ], [ %hdr.sroa.70.0, %if.end716 ]
  %hdr.sroa.48.2 = phi i8 [ %hdr.sroa.48.1, %if.end989 ], [ %hdr.sroa.48.0, %if.end716 ]
  %hdr.sroa.46.2 = phi i8 [ %hdr.sroa.46.1, %if.end989 ], [ %hdr.sroa.46.0, %if.end716 ]
  %hdr.sroa.41.2 = phi i8 [ %hdr.sroa.41.1, %if.end989 ], [ %hdr.sroa.41.0, %if.end716 ]
  %hdr.sroa.29.2 = phi i8 [ %hdr.sroa.29.1, %if.end989 ], [ %hdr.sroa.29.0, %if.end716 ]
  %hdr.sroa.27.2 = phi i8 [ %hdr.sroa.27.1, %if.end989 ], [ %hdr.sroa.27.0, %if.end716 ]
  %hdr.sroa.254063.2 = phi i8 [ %hdr.sroa.254063.1, %if.end989 ], [ %hdr.sroa.254063.0, %if.end716 ]
  %ebpf_packetOffsetInBits.2 = phi i32 [ %add1108, %if.end989 ], [ %add979, %if.end716 ]
  %tobool1193 = icmp eq i8 %hdr.sroa.70.2, 0
  br i1 %tobool1193, label %if.else1190.thread, label %if.then1258

if.else1190.thread:                               ; preds = %if.else1190
  %tobool1205 = icmp eq i8 %hdr.sroa.156.2, 0
  br i1 %tobool1205, label %if.else1190.thread.thread, label %if.then1258

if.else1190.thread.thread:                        ; preds = %if.else1190.thread, %if.end1118
  %hdr.sroa.314064.sroa.7.242084693 = phi i8 [ %hdr.sroa.314064.sroa.7.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.314064.sroa.0.242104691 = phi i8 [ %hdr.sroa.314064.sroa.0.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.36.sroa.7.242124689 = phi i8 [ %hdr.sroa.36.sroa.7.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.36.sroa.0.242144687 = phi i8 [ %hdr.sroa.36.sroa.0.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.434069.sroa.5.242164685 = phi i8 [ %hdr.sroa.434069.sroa.5.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.434069.sroa.0.242184683 = phi i8 [ %hdr.sroa.434069.sroa.0.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.51.sroa.7.242204681 = phi i8 [ %hdr.sroa.51.sroa.7.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.51.sroa.0.242224679 = phi i8 [ %hdr.sroa.51.sroa.0.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.56.sroa.9.242244677 = phi i8 [ %hdr.sroa.56.sroa.9.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.56.sroa.8.242264675 = phi i8 [ %hdr.sroa.56.sroa.8.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.56.sroa.7.242284673 = phi i8 [ %hdr.sroa.56.sroa.7.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.56.sroa.0.242304671 = phi i8 [ %hdr.sroa.56.sroa.0.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.63.sroa.9.242324669 = phi i8 [ %hdr.sroa.63.sroa.9.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.63.sroa.8.242344667 = phi i8 [ %hdr.sroa.63.sroa.8.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.63.sroa.7.242364665 = phi i8 [ %hdr.sroa.63.sroa.7.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.63.sroa.0.242384663 = phi i8 [ %hdr.sroa.63.sroa.0.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.784084.sroa.6.242404661 = phi i8 [ %hdr.sroa.784084.sroa.6.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.784084.sroa.5.242424659 = phi i8 [ %hdr.sroa.784084.sroa.5.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.784084.sroa.0.242444657 = phi i8 [ %hdr.sroa.784084.sroa.0.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.82.sroa.7.242464655 = phi i8 [ %hdr.sroa.82.sroa.7.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.82.sroa.0.242484653 = phi i8 [ %hdr.sroa.82.sroa.0.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.1604093.sroa.7.042504651 = phi i8 [ %hdr.sroa.1604093.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.1604093.sroa.0.042524649 = phi i8 [ %hdr.sroa.1604093.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.165.sroa.7.042544647 = phi i8 [ %hdr.sroa.165.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.165.sroa.0.042564645 = phi i8 [ %hdr.sroa.165.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.170.sroa.9.042584643 = phi i8 [ %hdr.sroa.170.sroa.9.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.170.sroa.8.042604641 = phi i8 [ %hdr.sroa.170.sroa.8.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.170.sroa.7.042624639 = phi i8 [ %hdr.sroa.170.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.170.sroa.0.042644637 = phi i8 [ %hdr.sroa.170.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.177.sroa.9.042664635 = phi i8 [ %hdr.sroa.177.sroa.9.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.177.sroa.8.042684633 = phi i8 [ %hdr.sroa.177.sroa.8.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.177.sroa.7.042704631 = phi i8 [ %hdr.sroa.177.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.177.sroa.0.042724629 = phi i8 [ %hdr.sroa.177.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.1904102.sroa.7.042744627 = phi i8 [ %hdr.sroa.1904102.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.1904102.sroa.0.042764625 = phi i8 [ %hdr.sroa.1904102.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.195.sroa.7.042784623 = phi i8 [ %hdr.sroa.195.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.195.sroa.0.042804621 = phi i8 [ %hdr.sroa.195.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.200.sroa.7.042824619 = phi i8 [ %hdr.sroa.200.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.200.sroa.0.042844617 = phi i8 [ %hdr.sroa.200.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.2084110.sroa.7.042864615 = phi i8 [ %hdr.sroa.2084110.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.2084110.sroa.0.042884613 = phi i8 [ %hdr.sroa.2084110.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.213.sroa.7.042904611 = phi i8 [ %hdr.sroa.213.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.213.sroa.0.042924609 = phi i8 [ %hdr.sroa.213.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.218.sroa.7.042944607 = phi i8 [ %hdr.sroa.218.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.218.sroa.0.042964605 = phi i8 [ %hdr.sroa.218.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.223.sroa.7.042984603 = phi i8 [ %hdr.sroa.223.sroa.7.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.223.sroa.0.043004601 = phi i8 [ %hdr.sroa.223.sroa.0.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.228.043024599 = phi i8 [ %hdr.sroa.228.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.205.043044597 = phi i8 [ %hdr.sroa.205.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.188.043064595 = phi i8 [ %hdr.sroa.188.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.186.043084593 = phi i8 [ %hdr.sroa.186.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.184.043104591 = phi i8 [ %hdr.sroa.184.0, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.154.243144587 = phi i8 [ %hdr.sroa.154.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.152.243164585 = phi i8 [ %hdr.sroa.152.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.150.243184583 = phi i8 [ %hdr.sroa.150.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.148.243204581 = phi i8 [ %hdr.sroa.148.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.146.243224579 = phi i8 [ %hdr.sroa.146.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.144.243244577 = phi i8 [ %hdr.sroa.144.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.142.243264575 = phi i8 [ %hdr.sroa.142.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.140.243284573 = phi i8 [ %hdr.sroa.140.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.138.243304571 = phi i8 [ %hdr.sroa.138.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.136.243324569 = phi i8 [ %hdr.sroa.136.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.134.243344567 = phi i8 [ %hdr.sroa.134.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.132.243364565 = phi i8 [ %hdr.sroa.132.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.130.243384563 = phi i8 [ %hdr.sroa.130.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.128.243404561 = phi i8 [ %hdr.sroa.128.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.126.243424559 = phi i8 [ %hdr.sroa.126.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.124.243444557 = phi i8 [ %hdr.sroa.124.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.122.243464555 = phi i8 [ %hdr.sroa.122.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.120.243484553 = phi i8 [ %hdr.sroa.120.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.118.243504551 = phi i8 [ %hdr.sroa.118.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.116.243524549 = phi i8 [ %hdr.sroa.116.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.114.243544547 = phi i8 [ %hdr.sroa.114.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.112.243564545 = phi i8 [ %hdr.sroa.112.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.110.243584543 = phi i8 [ %hdr.sroa.110.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.108.243604541 = phi i8 [ %hdr.sroa.108.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.106.243624539 = phi i8 [ %hdr.sroa.106.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.104.243644537 = phi i8 [ %hdr.sroa.104.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.102.243664535 = phi i8 [ %hdr.sroa.102.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.100.243684533 = phi i8 [ %hdr.sroa.100.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.98.243704531 = phi i8 [ %hdr.sroa.98.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.96.243724529 = phi i8 [ %hdr.sroa.96.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.94.243744527 = phi i8 [ %hdr.sroa.94.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.92.243764525 = phi i8 [ %hdr.sroa.92.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.90.243784523 = phi i8 [ %hdr.sroa.90.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.87.243804521 = phi i8 [ %hdr.sroa.87.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.76.243824519 = phi i8 [ %hdr.sroa.76.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.744082.243844517 = phi i8 [ %hdr.sroa.744082.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.48.243864515 = phi i8 [ %hdr.sroa.48.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.46.243884513 = phi i8 [ %hdr.sroa.46.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.41.243904511 = phi i8 [ %hdr.sroa.41.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.29.243924509 = phi i8 [ %hdr.sroa.29.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.27.243944507 = phi i8 [ %hdr.sroa.27.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %hdr.sroa.254063.243964505 = phi i8 [ %hdr.sroa.254063.2, %if.else1190.thread ], [ 0, %if.end1118 ]
  %ebpf_packetOffsetInBits.243984503 = phi i32 [ %ebpf_packetOffsetInBits.2, %if.else1190.thread ], [ 112, %if.end1118 ]
  br label %if.then1258

if.then1258:                                      ; preds = %if.else1190.thread.thread, %if.else1190.thread, %if.else1190, %if.end247, %if.end
  %xout.sroa.0.14500 = phi i32 [ 3, %if.else1190 ], [ 3, %if.end ], [ 1, %if.else1190.thread.thread ], [ 3, %if.else1190.thread ], [ 3, %if.end247 ]
  %hdr.sroa.314064.sroa.7.242074499 = phi i8 [ %hdr.sroa.314064.sroa.7.2, %if.else1190 ], [ %hdr.sroa.314064.sroa.7.0.extract.trunc, %if.end ], [ %hdr.sroa.314064.sroa.7.242084693, %if.else1190.thread.thread ], [ %hdr.sroa.314064.sroa.7.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.314064.sroa.0.242094498 = phi i8 [ %hdr.sroa.314064.sroa.0.2, %if.else1190 ], [ %hdr.sroa.314064.sroa.0.0.extract.trunc, %if.end ], [ %hdr.sroa.314064.sroa.0.242104691, %if.else1190.thread.thread ], [ %hdr.sroa.314064.sroa.0.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.36.sroa.7.242114497 = phi i8 [ %hdr.sroa.36.sroa.7.2, %if.else1190 ], [ %hdr.sroa.36.sroa.7.0.extract.trunc4194, %if.end ], [ %hdr.sroa.36.sroa.7.242124689, %if.else1190.thread.thread ], [ %hdr.sroa.36.sroa.7.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.36.sroa.0.242134496 = phi i8 [ %hdr.sroa.36.sroa.0.2, %if.else1190 ], [ %hdr.sroa.36.sroa.0.0.extract.trunc4192, %if.end ], [ %hdr.sroa.36.sroa.0.242144687, %if.else1190.thread.thread ], [ %hdr.sroa.36.sroa.0.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.434069.sroa.5.242154495 = phi i8 [ %hdr.sroa.434069.sroa.5.2, %if.else1190 ], [ %hdr.sroa.434069.sroa.5.0.extract.trunc, %if.end ], [ %hdr.sroa.434069.sroa.5.242164685, %if.else1190.thread.thread ], [ %hdr.sroa.434069.sroa.5.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.434069.sroa.0.242174494 = phi i8 [ %hdr.sroa.434069.sroa.0.2, %if.else1190 ], [ %hdr.sroa.434069.sroa.0.0.extract.trunc, %if.end ], [ %hdr.sroa.434069.sroa.0.242184683, %if.else1190.thread.thread ], [ %hdr.sroa.434069.sroa.0.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.51.sroa.7.242194493 = phi i8 [ %hdr.sroa.51.sroa.7.2, %if.else1190 ], [ %hdr.sroa.51.sroa.7.0.extract.trunc, %if.end ], [ %hdr.sroa.51.sroa.7.242204681, %if.else1190.thread.thread ], [ %hdr.sroa.51.sroa.7.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.51.sroa.0.242214492 = phi i8 [ %hdr.sroa.51.sroa.0.2, %if.else1190 ], [ %hdr.sroa.51.sroa.0.0.extract.trunc, %if.end ], [ %hdr.sroa.51.sroa.0.242224679, %if.else1190.thread.thread ], [ %hdr.sroa.51.sroa.0.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.56.sroa.9.242234491 = phi i8 [ %hdr.sroa.56.sroa.9.2, %if.else1190 ], [ %hdr.sroa.56.sroa.9.0.extract.trunc, %if.end ], [ %hdr.sroa.56.sroa.9.242244677, %if.else1190.thread.thread ], [ %hdr.sroa.56.sroa.9.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.56.sroa.8.242254490 = phi i8 [ %hdr.sroa.56.sroa.8.2, %if.else1190 ], [ %hdr.sroa.56.sroa.8.0.extract.trunc, %if.end ], [ %hdr.sroa.56.sroa.8.242264675, %if.else1190.thread.thread ], [ %hdr.sroa.56.sroa.8.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.56.sroa.7.242274489 = phi i8 [ %hdr.sroa.56.sroa.7.2, %if.else1190 ], [ %hdr.sroa.56.sroa.7.0.extract.trunc, %if.end ], [ %hdr.sroa.56.sroa.7.242284673, %if.else1190.thread.thread ], [ %hdr.sroa.56.sroa.7.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.56.sroa.0.242294488 = phi i8 [ %hdr.sroa.56.sroa.0.2, %if.else1190 ], [ %hdr.sroa.56.sroa.0.0.extract.trunc, %if.end ], [ %hdr.sroa.56.sroa.0.242304671, %if.else1190.thread.thread ], [ %hdr.sroa.56.sroa.0.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.63.sroa.9.242314487 = phi i8 [ %hdr.sroa.63.sroa.9.2, %if.else1190 ], [ %hdr.sroa.63.sroa.9.0.extract.trunc4181, %if.end ], [ %hdr.sroa.63.sroa.9.242324669, %if.else1190.thread.thread ], [ %hdr.sroa.63.sroa.9.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.63.sroa.8.242334486 = phi i8 [ %hdr.sroa.63.sroa.8.2, %if.else1190 ], [ %hdr.sroa.63.sroa.8.0.extract.trunc4179, %if.end ], [ %hdr.sroa.63.sroa.8.242344667, %if.else1190.thread.thread ], [ %hdr.sroa.63.sroa.8.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.63.sroa.7.242354485 = phi i8 [ %hdr.sroa.63.sroa.7.2, %if.else1190 ], [ %hdr.sroa.63.sroa.7.0.extract.trunc4177, %if.end ], [ %hdr.sroa.63.sroa.7.242364665, %if.else1190.thread.thread ], [ %hdr.sroa.63.sroa.7.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.63.sroa.0.242374484 = phi i8 [ %hdr.sroa.63.sroa.0.2, %if.else1190 ], [ %hdr.sroa.63.sroa.0.0.extract.trunc4175, %if.end ], [ %hdr.sroa.63.sroa.0.242384663, %if.else1190.thread.thread ], [ %hdr.sroa.63.sroa.0.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.784084.sroa.6.242394483 = phi i8 [ %hdr.sroa.784084.sroa.6.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.784084.sroa.6.242404661, %if.else1190.thread.thread ], [ %hdr.sroa.784084.sroa.6.2, %if.else1190.thread ], [ %hdr.sroa.784084.sroa.6.0.extract.trunc, %if.end247 ]
  %hdr.sroa.784084.sroa.5.242414482 = phi i8 [ %hdr.sroa.784084.sroa.5.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.784084.sroa.5.242424659, %if.else1190.thread.thread ], [ %hdr.sroa.784084.sroa.5.2, %if.else1190.thread ], [ %hdr.sroa.784084.sroa.5.0.extract.trunc, %if.end247 ]
  %hdr.sroa.784084.sroa.0.242434481 = phi i8 [ %hdr.sroa.784084.sroa.0.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.784084.sroa.0.242444657, %if.else1190.thread.thread ], [ %hdr.sroa.784084.sroa.0.2, %if.else1190.thread ], [ %hdr.sroa.784084.sroa.0.0.extract.trunc, %if.end247 ]
  %hdr.sroa.82.sroa.7.242454480 = phi i8 [ %hdr.sroa.82.sroa.7.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.82.sroa.7.242464655, %if.else1190.thread.thread ], [ %hdr.sroa.82.sroa.7.2, %if.else1190.thread ], [ %hdr.sroa.82.sroa.7.0.extract.trunc4171, %if.end247 ]
  %hdr.sroa.82.sroa.0.242474479 = phi i8 [ %hdr.sroa.82.sroa.0.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.82.sroa.0.242484653, %if.else1190.thread.thread ], [ %hdr.sroa.82.sroa.0.2, %if.else1190.thread ], [ %hdr.sroa.82.sroa.0.0.extract.trunc4169, %if.end247 ]
  %hdr.sroa.1604093.sroa.7.042494478 = phi i8 [ %hdr.sroa.1604093.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.1604093.sroa.7.042504651, %if.else1190.thread.thread ], [ %hdr.sroa.1604093.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.1604093.sroa.0.042514477 = phi i8 [ %hdr.sroa.1604093.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.1604093.sroa.0.042524649, %if.else1190.thread.thread ], [ %hdr.sroa.1604093.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.165.sroa.7.042534476 = phi i8 [ %hdr.sroa.165.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.165.sroa.7.042544647, %if.else1190.thread.thread ], [ %hdr.sroa.165.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.165.sroa.0.042554475 = phi i8 [ %hdr.sroa.165.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.165.sroa.0.042564645, %if.else1190.thread.thread ], [ %hdr.sroa.165.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.170.sroa.9.042574474 = phi i8 [ %hdr.sroa.170.sroa.9.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.170.sroa.9.042584643, %if.else1190.thread.thread ], [ %hdr.sroa.170.sroa.9.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.170.sroa.8.042594473 = phi i8 [ %hdr.sroa.170.sroa.8.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.170.sroa.8.042604641, %if.else1190.thread.thread ], [ %hdr.sroa.170.sroa.8.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.170.sroa.7.042614472 = phi i8 [ %hdr.sroa.170.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.170.sroa.7.042624639, %if.else1190.thread.thread ], [ %hdr.sroa.170.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.170.sroa.0.042634471 = phi i8 [ %hdr.sroa.170.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.170.sroa.0.042644637, %if.else1190.thread.thread ], [ %hdr.sroa.170.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.177.sroa.9.042654470 = phi i8 [ %hdr.sroa.177.sroa.9.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.177.sroa.9.042664635, %if.else1190.thread.thread ], [ %hdr.sroa.177.sroa.9.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.177.sroa.8.042674469 = phi i8 [ %hdr.sroa.177.sroa.8.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.177.sroa.8.042684633, %if.else1190.thread.thread ], [ %hdr.sroa.177.sroa.8.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.177.sroa.7.042694468 = phi i8 [ %hdr.sroa.177.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.177.sroa.7.042704631, %if.else1190.thread.thread ], [ %hdr.sroa.177.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.177.sroa.0.042714467 = phi i8 [ %hdr.sroa.177.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.177.sroa.0.042724629, %if.else1190.thread.thread ], [ %hdr.sroa.177.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.1904102.sroa.7.042734466 = phi i8 [ %hdr.sroa.1904102.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.1904102.sroa.7.042744627, %if.else1190.thread.thread ], [ %hdr.sroa.1904102.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.1904102.sroa.0.042754465 = phi i8 [ %hdr.sroa.1904102.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.1904102.sroa.0.042764625, %if.else1190.thread.thread ], [ %hdr.sroa.1904102.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.195.sroa.7.042774464 = phi i8 [ %hdr.sroa.195.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.195.sroa.7.042784623, %if.else1190.thread.thread ], [ %hdr.sroa.195.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.195.sroa.0.042794463 = phi i8 [ %hdr.sroa.195.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.195.sroa.0.042804621, %if.else1190.thread.thread ], [ %hdr.sroa.195.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.200.sroa.7.042814462 = phi i8 [ %hdr.sroa.200.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.200.sroa.7.042824619, %if.else1190.thread.thread ], [ %hdr.sroa.200.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.200.sroa.0.042834461 = phi i8 [ %hdr.sroa.200.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.200.sroa.0.042844617, %if.else1190.thread.thread ], [ %hdr.sroa.200.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.2084110.sroa.7.042854460 = phi i8 [ %hdr.sroa.2084110.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.2084110.sroa.7.042864615, %if.else1190.thread.thread ], [ %hdr.sroa.2084110.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.2084110.sroa.0.042874459 = phi i8 [ %hdr.sroa.2084110.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.2084110.sroa.0.042884613, %if.else1190.thread.thread ], [ %hdr.sroa.2084110.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.213.sroa.7.042894458 = phi i8 [ %hdr.sroa.213.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.213.sroa.7.042904611, %if.else1190.thread.thread ], [ %hdr.sroa.213.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.213.sroa.0.042914457 = phi i8 [ %hdr.sroa.213.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.213.sroa.0.042924609, %if.else1190.thread.thread ], [ %hdr.sroa.213.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.218.sroa.7.042934456 = phi i8 [ %hdr.sroa.218.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.218.sroa.7.042944607, %if.else1190.thread.thread ], [ %hdr.sroa.218.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.218.sroa.0.042954455 = phi i8 [ %hdr.sroa.218.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.218.sroa.0.042964605, %if.else1190.thread.thread ], [ %hdr.sroa.218.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.223.sroa.7.042974454 = phi i8 [ %hdr.sroa.223.sroa.7.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.223.sroa.7.042984603, %if.else1190.thread.thread ], [ %hdr.sroa.223.sroa.7.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.223.sroa.0.042994453 = phi i8 [ %hdr.sroa.223.sroa.0.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.223.sroa.0.043004601, %if.else1190.thread.thread ], [ %hdr.sroa.223.sroa.0.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.228.043014452 = phi i8 [ %hdr.sroa.228.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.228.043024599, %if.else1190.thread.thread ], [ %hdr.sroa.228.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.205.043034451 = phi i8 [ %hdr.sroa.205.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.205.043044597, %if.else1190.thread.thread ], [ %hdr.sroa.205.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.188.043054450 = phi i8 [ %hdr.sroa.188.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.188.043064595, %if.else1190.thread.thread ], [ %hdr.sroa.188.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.186.043074449 = phi i8 [ %hdr.sroa.186.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.186.043084593, %if.else1190.thread.thread ], [ %hdr.sroa.186.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.184.043094448 = phi i8 [ %hdr.sroa.184.0, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.184.043104591, %if.else1190.thread.thread ], [ %hdr.sroa.184.0, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.156.243124447 = phi i8 [ %hdr.sroa.156.2, %if.else1190 ], [ 0, %if.end ], [ 0, %if.else1190.thread.thread ], [ 1, %if.else1190.thread ], [ 1, %if.end247 ]
  %hdr.sroa.154.243134446 = phi i8 [ %hdr.sroa.154.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.154.243144587, %if.else1190.thread.thread ], [ %hdr.sroa.154.2, %if.else1190.thread ], [ %237, %if.end247 ]
  %hdr.sroa.152.243154445 = phi i8 [ %hdr.sroa.152.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.152.243164585, %if.else1190.thread.thread ], [ %hdr.sroa.152.2, %if.else1190.thread ], [ %235, %if.end247 ]
  %hdr.sroa.150.243174444 = phi i8 [ %hdr.sroa.150.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.150.243184583, %if.else1190.thread.thread ], [ %hdr.sroa.150.2, %if.else1190.thread ], [ %233, %if.end247 ]
  %hdr.sroa.148.243194443 = phi i8 [ %hdr.sroa.148.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.148.243204581, %if.else1190.thread.thread ], [ %hdr.sroa.148.2, %if.else1190.thread ], [ %231, %if.end247 ]
  %hdr.sroa.146.243214442 = phi i8 [ %hdr.sroa.146.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.146.243224579, %if.else1190.thread.thread ], [ %hdr.sroa.146.2, %if.else1190.thread ], [ %229, %if.end247 ]
  %hdr.sroa.144.243234441 = phi i8 [ %hdr.sroa.144.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.144.243244577, %if.else1190.thread.thread ], [ %hdr.sroa.144.2, %if.else1190.thread ], [ %227, %if.end247 ]
  %hdr.sroa.142.243254440 = phi i8 [ %hdr.sroa.142.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.142.243264575, %if.else1190.thread.thread ], [ %hdr.sroa.142.2, %if.else1190.thread ], [ %225, %if.end247 ]
  %hdr.sroa.140.243274439 = phi i8 [ %hdr.sroa.140.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.140.243284573, %if.else1190.thread.thread ], [ %hdr.sroa.140.2, %if.else1190.thread ], [ %223, %if.end247 ]
  %hdr.sroa.138.243294438 = phi i8 [ %hdr.sroa.138.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.138.243304571, %if.else1190.thread.thread ], [ %hdr.sroa.138.2, %if.else1190.thread ], [ %221, %if.end247 ]
  %hdr.sroa.136.243314437 = phi i8 [ %hdr.sroa.136.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.136.243324569, %if.else1190.thread.thread ], [ %hdr.sroa.136.2, %if.else1190.thread ], [ %219, %if.end247 ]
  %hdr.sroa.134.243334436 = phi i8 [ %hdr.sroa.134.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.134.243344567, %if.else1190.thread.thread ], [ %hdr.sroa.134.2, %if.else1190.thread ], [ %217, %if.end247 ]
  %hdr.sroa.132.243354435 = phi i8 [ %hdr.sroa.132.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.132.243364565, %if.else1190.thread.thread ], [ %hdr.sroa.132.2, %if.else1190.thread ], [ %215, %if.end247 ]
  %hdr.sroa.130.243374434 = phi i8 [ %hdr.sroa.130.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.130.243384563, %if.else1190.thread.thread ], [ %hdr.sroa.130.2, %if.else1190.thread ], [ %213, %if.end247 ]
  %hdr.sroa.128.243394433 = phi i8 [ %hdr.sroa.128.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.128.243404561, %if.else1190.thread.thread ], [ %hdr.sroa.128.2, %if.else1190.thread ], [ %211, %if.end247 ]
  %hdr.sroa.126.243414432 = phi i8 [ %hdr.sroa.126.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.126.243424559, %if.else1190.thread.thread ], [ %hdr.sroa.126.2, %if.else1190.thread ], [ %209, %if.end247 ]
  %hdr.sroa.124.243434431 = phi i8 [ %hdr.sroa.124.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.124.243444557, %if.else1190.thread.thread ], [ %hdr.sroa.124.2, %if.else1190.thread ], [ %207, %if.end247 ]
  %hdr.sroa.122.243454430 = phi i8 [ %hdr.sroa.122.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.122.243464555, %if.else1190.thread.thread ], [ %hdr.sroa.122.2, %if.else1190.thread ], [ %205, %if.end247 ]
  %hdr.sroa.120.243474429 = phi i8 [ %hdr.sroa.120.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.120.243484553, %if.else1190.thread.thread ], [ %hdr.sroa.120.2, %if.else1190.thread ], [ %203, %if.end247 ]
  %hdr.sroa.118.243494428 = phi i8 [ %hdr.sroa.118.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.118.243504551, %if.else1190.thread.thread ], [ %hdr.sroa.118.2, %if.else1190.thread ], [ %201, %if.end247 ]
  %hdr.sroa.116.243514427 = phi i8 [ %hdr.sroa.116.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.116.243524549, %if.else1190.thread.thread ], [ %hdr.sroa.116.2, %if.else1190.thread ], [ %199, %if.end247 ]
  %hdr.sroa.114.243534426 = phi i8 [ %hdr.sroa.114.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.114.243544547, %if.else1190.thread.thread ], [ %hdr.sroa.114.2, %if.else1190.thread ], [ %197, %if.end247 ]
  %hdr.sroa.112.243554425 = phi i8 [ %hdr.sroa.112.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.112.243564545, %if.else1190.thread.thread ], [ %hdr.sroa.112.2, %if.else1190.thread ], [ %195, %if.end247 ]
  %hdr.sroa.110.243574424 = phi i8 [ %hdr.sroa.110.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.110.243584543, %if.else1190.thread.thread ], [ %hdr.sroa.110.2, %if.else1190.thread ], [ %193, %if.end247 ]
  %hdr.sroa.108.243594423 = phi i8 [ %hdr.sroa.108.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.108.243604541, %if.else1190.thread.thread ], [ %hdr.sroa.108.2, %if.else1190.thread ], [ %191, %if.end247 ]
  %hdr.sroa.106.243614422 = phi i8 [ %hdr.sroa.106.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.106.243624539, %if.else1190.thread.thread ], [ %hdr.sroa.106.2, %if.else1190.thread ], [ %189, %if.end247 ]
  %hdr.sroa.104.243634421 = phi i8 [ %hdr.sroa.104.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.104.243644537, %if.else1190.thread.thread ], [ %hdr.sroa.104.2, %if.else1190.thread ], [ %187, %if.end247 ]
  %hdr.sroa.102.243654420 = phi i8 [ %hdr.sroa.102.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.102.243664535, %if.else1190.thread.thread ], [ %hdr.sroa.102.2, %if.else1190.thread ], [ %185, %if.end247 ]
  %hdr.sroa.100.243674419 = phi i8 [ %hdr.sroa.100.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.100.243684533, %if.else1190.thread.thread ], [ %hdr.sroa.100.2, %if.else1190.thread ], [ %183, %if.end247 ]
  %hdr.sroa.98.243694418 = phi i8 [ %hdr.sroa.98.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.98.243704531, %if.else1190.thread.thread ], [ %hdr.sroa.98.2, %if.else1190.thread ], [ %181, %if.end247 ]
  %hdr.sroa.96.243714417 = phi i8 [ %hdr.sroa.96.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.96.243724529, %if.else1190.thread.thread ], [ %hdr.sroa.96.2, %if.else1190.thread ], [ %179, %if.end247 ]
  %hdr.sroa.94.243734416 = phi i8 [ %hdr.sroa.94.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.94.243744527, %if.else1190.thread.thread ], [ %hdr.sroa.94.2, %if.else1190.thread ], [ %177, %if.end247 ]
  %hdr.sroa.92.243754415 = phi i8 [ %hdr.sroa.92.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.92.243764525, %if.else1190.thread.thread ], [ %hdr.sroa.92.2, %if.else1190.thread ], [ %175, %if.end247 ]
  %hdr.sroa.90.243774414 = phi i8 [ %hdr.sroa.90.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.90.243784523, %if.else1190.thread.thread ], [ %hdr.sroa.90.2, %if.else1190.thread ], [ %173, %if.end247 ]
  %hdr.sroa.87.243794413 = phi i8 [ %hdr.sroa.87.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.87.243804521, %if.else1190.thread.thread ], [ %hdr.sroa.87.2, %if.else1190.thread ], [ %171, %if.end247 ]
  %hdr.sroa.76.243814412 = phi i8 [ %hdr.sroa.76.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.76.243824519, %if.else1190.thread.thread ], [ %hdr.sroa.76.2, %if.else1190.thread ], [ %conv285, %if.end247 ]
  %hdr.sroa.744082.243834411 = phi i8 [ %hdr.sroa.744082.2, %if.else1190 ], [ 0, %if.end ], [ %hdr.sroa.744082.243844517, %if.else1190.thread.thread ], [ %hdr.sroa.744082.2, %if.else1190.thread ], [ %161, %if.end247 ]
  %hdr.sroa.48.243854410 = phi i8 [ %hdr.sroa.48.2, %if.else1190 ], [ %151, %if.end ], [ %hdr.sroa.48.243864515, %if.else1190.thread.thread ], [ %hdr.sroa.48.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.46.243874409 = phi i8 [ %hdr.sroa.46.2, %if.else1190 ], [ %149, %if.end ], [ %hdr.sroa.46.243884513, %if.else1190.thread.thread ], [ %hdr.sroa.46.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.41.243894408 = phi i8 [ %hdr.sroa.41.2, %if.else1190 ], [ %143, %if.end ], [ %hdr.sroa.41.243904511, %if.else1190.thread.thread ], [ %hdr.sroa.41.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.29.243914407 = phi i8 [ %hdr.sroa.29.2, %if.else1190 ], [ %136, %if.end ], [ %hdr.sroa.29.243924509, %if.else1190.thread.thread ], [ %hdr.sroa.29.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.27.243934406 = phi i8 [ %hdr.sroa.27.2, %if.else1190 ], [ %134, %if.end ], [ %hdr.sroa.27.243944507, %if.else1190.thread.thread ], [ %hdr.sroa.27.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %hdr.sroa.254063.243954405 = phi i8 [ %hdr.sroa.254063.2, %if.else1190 ], [ %133, %if.end ], [ %hdr.sroa.254063.243964505, %if.else1190.thread.thread ], [ %hdr.sroa.254063.2, %if.else1190.thread ], [ 0, %if.end247 ]
  %ebpf_packetOffsetInBits.243974404 = phi i32 [ %ebpf_packetOffsetInBits.2, %if.else1190 ], [ 272, %if.end ], [ %ebpf_packetOffsetInBits.243984503, %if.else1190.thread.thread ], [ %ebpf_packetOffsetInBits.2, %if.else1190.thread ], [ 432, %if.end247 ]
  %tobool119343994403 = phi i1 [ false, %if.else1190 ], [ false, %if.end ], [ true, %if.else1190.thread.thread ], [ true, %if.else1190.thread ], [ true, %if.end247 ]
  %308 = phi i32 [ 272, %if.else1190 ], [ 272, %if.end ], [ 112, %if.else1190.thread.thread ], [ 112, %if.else1190.thread ], [ 112, %if.end247 ]
  %tobool1233 = icmp eq i8 %hdr.sroa.156.243124447, 0
  %add1235 = add nuw nsw i32 %308, 320
  %spec.select4042 = select i1 %tobool1233, i32 %308, i32 %add1235
  %tobool1239 = icmp eq i8 %hdr.sroa.205.043034451, 0
  %add1241 = add nuw nsw i32 %spec.select4042, 160
  %ebpf_outHeaderLength.3 = select i1 %tobool1239, i32 %spec.select4042, i32 %add1241
  %tobool1245 = icmp eq i8 %hdr.sroa.228.043014452, 0
  %add1247 = add nuw nsw i32 %ebpf_outHeaderLength.3, 64
  %spec.select4043 = select i1 %tobool1245, i32 %ebpf_outHeaderLength.3, i32 %add1247
  %div1249 = lshr i32 %ebpf_packetOffsetInBits.243974404, 3
  %div1250 = lshr i32 %spec.select4043, 3
  %sub = sub nsw i32 %div1249, %div1250
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !363, metadata !DIExpression()), !dbg !366
  call void @llvm.dbg.value(metadata i32 %sub, metadata !364, metadata !DIExpression()), !dbg !367
  %call.i = call i32 @nanotube_packet_resize(%struct.nanotube_packet* %packet, i64 1, i32 %sub), !dbg !368
  call void @llvm.dbg.value(metadata i32 %call.i, metadata !365, metadata !DIExpression()), !dbg !369
  %tobool.i = icmp eq i32 %call.i, 0, !dbg !370
  %cond.i = sext i1 %tobool.i to i32, !dbg !370
  %309 = call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 32768)
  %310 = add i64 %309, -1
  %311 = icmp ugt i64 14, %310
  br i1 %311, label %cleanup3277, label %if.end1474

if.end1474:                                       ; preds = %if.then1258
  store i8 %hdr.sroa.0.sroa.0.0.extract.trunc, i8* %_buffer
  %savedstack = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %102), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 0, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %102, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %102, i8 -1, i64 1, i1 false), !dbg !406
  %call.i1 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer, i8* nonnull %102, i64 1, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %102), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack), !dbg !408
  store i8 %hdr.sroa.0.sroa.5.0.extract.trunc, i8* %_buffer170
  %savedstack3 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %101), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer170, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 1, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %101, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %101, i8 -1, i64 1, i1 false), !dbg !406
  %call.i2 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer170, i8* nonnull %101, i64 2, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %101), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack3), !dbg !408
  store i8 %hdr.sroa.0.sroa.6.0.extract.trunc, i8* %_buffer10
  %savedstack5 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %100), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer10, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 2, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %100, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %100, i8 -1, i64 1, i1 false), !dbg !406
  %call.i4 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer10, i8* nonnull %100, i64 3, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %100), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack5), !dbg !408
  store i8 %hdr.sroa.0.sroa.7.0.extract.trunc, i8* %_buffer12
  %savedstack7 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %99), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer12, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 3, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %99, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %99, i8 -1, i64 1, i1 false), !dbg !406
  %call.i6 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer12, i8* nonnull %99, i64 4, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %99), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack7), !dbg !408
  store i8 %hdr.sroa.0.sroa.8.0.extract.trunc, i8* %_buffer172
  %savedstack9 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %98), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer172, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 4, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %98, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %98, i8 -1, i64 1, i1 false), !dbg !406
  %call.i8 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer172, i8* nonnull %98, i64 5, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %98), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack9), !dbg !408
  store i8 %hdr.sroa.0.sroa.9.0.extract.trunc, i8* %_buffer102
  %savedstack11 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %97), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer102, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 5, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %97, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %97, i8 -1, i64 1, i1 false), !dbg !406
  %call.i10 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer102, i8* nonnull %97, i64 6, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %97), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack11), !dbg !408
  store i8 %hdr.sroa.10.sroa.0.0.extract.trunc, i8* %_buffer3
  %savedstack13 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %96), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer3, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 6, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %96, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %96, i8 -1, i64 1, i1 false), !dbg !406
  %call.i12 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer3, i8* nonnull %96, i64 7, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %96), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack13), !dbg !408
  store i8 %hdr.sroa.10.sroa.5.0.extract.trunc, i8* %_buffer80
  %savedstack15 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %95), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer80, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 7, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %95, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %95, i8 -1, i64 1, i1 false), !dbg !406
  %call.i14 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer80, i8* nonnull %95, i64 8, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %95), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack15), !dbg !408
  store i8 %hdr.sroa.10.sroa.6.0.extract.trunc, i8* %_buffer4
  %savedstack17 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %94), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer4, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 8, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %94, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %94, i8 -1, i64 1, i1 false), !dbg !406
  %call.i16 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer4, i8* nonnull %94, i64 9, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %94), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack17), !dbg !408
  store i8 %hdr.sroa.10.sroa.7.0.extract.trunc, i8* %_buffer91
  %savedstack19 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %93), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer91, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 9, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %93, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %93, i8 -1, i64 1, i1 false), !dbg !406
  %call.i18 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer91, i8* nonnull %93, i64 10, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %93), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack19), !dbg !408
  store i8 %hdr.sroa.10.sroa.8.0.extract.trunc, i8* %_buffer63
  %savedstack21 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %92), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer63, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 10, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %92, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %92, i8 -1, i64 1, i1 false), !dbg !406
  %call.i20 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer63, i8* nonnull %92, i64 11, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %92), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack21), !dbg !408
  store i8 %hdr.sroa.10.sroa.9.0.extract.trunc, i8* %_buffer103
  %savedstack23 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %91), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer103, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 11, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %91, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %91, i8 -1, i64 1, i1 false), !dbg !406
  %call.i22 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer103, i8* nonnull %91, i64 12, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %91), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack23), !dbg !408
  %hdr.sroa.17.sroa.0.0.extract.trunc = trunc i16 %307 to i8
  %hdr.sroa.17.sroa.7.0.extract.shift = lshr i16 %307, 8
  %hdr.sroa.17.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.17.sroa.7.0.extract.shift to i8
  store i8 %hdr.sroa.17.sroa.0.0.extract.trunc, i8* %_buffer163
  %savedstack25 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %90), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer163, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 12, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %90, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %90, i8 -1, i64 1, i1 false), !dbg !406
  %call.i24 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer163, i8* nonnull %90, i64 13, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %90), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack25), !dbg !408
  store i8 %hdr.sroa.17.sroa.7.0.extract.trunc, i8* %_buffer5
  %savedstack27 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %89), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer5, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 13, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %89, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %89, i8 -1, i64 1, i1 false), !dbg !406
  %call.i26 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer5, i8* nonnull %89, i64 14, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %89), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack27), !dbg !408
  br i1 %tobool119343994403, label %if.end1942, label %if.then1478

if.then1478:                                      ; preds = %if.end1474
  %312 = icmp ugt i64 34, %310
  br i1 %312, label %cleanup3277, label %if.end1486

if.end1486:                                       ; preds = %if.then1478
  %shl1492 = shl i8 %hdr.sroa.254063.243954405, 4
  %313 = and i8 %hdr.sroa.27.243934406, 15
  %or15174010 = or i8 %shl1492, %313
  store i8 %or15174010, i8* %_buffer115
  %savedstack29 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %88), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer115, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 14, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %88, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %88, i8 -1, i64 1, i1 false), !dbg !406
  %call.i28 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer115, i8* nonnull %88, i64 15, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %88), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack29), !dbg !408
  store i8 %hdr.sroa.29.243914407, i8* %_buffer95
  %savedstack31 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %87), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer95, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 15, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %87, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %87, i8 -1, i64 1, i1 false), !dbg !406
  %call.i30 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer95, i8* nonnull %87, i64 16, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %87), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack31), !dbg !408
  %hdr.sroa.314064.sroa.7.0.insert.ext = zext i8 %hdr.sroa.314064.sroa.7.242074499 to i16
  %hdr.sroa.314064.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.314064.sroa.7.0.insert.ext, 8
  %hdr.sroa.314064.sroa.0.0.insert.ext = zext i8 %hdr.sroa.314064.sroa.0.242094498 to i16
  %hdr.sroa.314064.sroa.0.0.insert.insert = or i16 %hdr.sroa.314064.sroa.7.0.insert.shift, %hdr.sroa.314064.sroa.0.0.insert.ext
  %rev4016 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.314064.sroa.0.0.insert.insert)
  %hdr.sroa.314064.sroa.0.0.extract.trunc4195 = trunc i16 %rev4016 to i8
  %hdr.sroa.314064.sroa.7.0.extract.shift4196 = lshr i16 %rev4016, 8
  %hdr.sroa.314064.sroa.7.0.extract.trunc4197 = trunc i16 %hdr.sroa.314064.sroa.7.0.extract.shift4196 to i8
  store i8 %hdr.sroa.314064.sroa.0.0.extract.trunc4195, i8* %_buffer97
  %savedstack33 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %86), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer97, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 16, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %86, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %86, i8 -1, i64 1, i1 false), !dbg !406
  %call.i32 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer97, i8* nonnull %86, i64 17, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %86), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack33), !dbg !408
  store i8 %hdr.sroa.314064.sroa.7.0.extract.trunc4197, i8* %_buffer8
  %savedstack35 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %85), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer8, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 17, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %85, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %85, i8 -1, i64 1, i1 false), !dbg !406
  %call.i34 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer8, i8* nonnull %85, i64 18, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %85), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack35), !dbg !408
  %hdr.sroa.36.sroa.7.0.insert.ext = zext i8 %hdr.sroa.36.sroa.7.242114497 to i16
  %hdr.sroa.36.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.36.sroa.7.0.insert.ext, 8
  %hdr.sroa.36.sroa.0.0.insert.ext = zext i8 %hdr.sroa.36.sroa.0.242134496 to i16
  %hdr.sroa.36.sroa.0.0.insert.insert = or i16 %hdr.sroa.36.sroa.7.0.insert.shift, %hdr.sroa.36.sroa.0.0.insert.ext
  %rev4015 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.36.sroa.0.0.insert.insert)
  %hdr.sroa.36.sroa.0.0.extract.trunc = trunc i16 %rev4015 to i8
  %hdr.sroa.36.sroa.7.0.extract.shift = lshr i16 %rev4015, 8
  %hdr.sroa.36.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.36.sroa.7.0.extract.shift to i8
  store i8 %hdr.sroa.36.sroa.0.0.extract.trunc, i8* %_buffer168
  %savedstack37 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %84), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer168, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 18, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %84, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %84, i8 -1, i64 1, i1 false), !dbg !406
  %call.i36 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer168, i8* nonnull %84, i64 19, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %84), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack37), !dbg !408
  store i8 %hdr.sroa.36.sroa.7.0.extract.trunc, i8* %_buffer68
  %savedstack39 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %83), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer68, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 19, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %83, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %83, i8 -1, i64 1, i1 false), !dbg !406
  %call.i38 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer68, i8* nonnull %83, i64 20, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %83), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack39), !dbg !408
  %shl1649 = shl i8 %hdr.sroa.41.243894408, 5
  %314 = lshr i8 %hdr.sroa.434069.sroa.0.242174494, 2
  %315 = and i8 %314, 7
  %or16764011 = or i8 %shl1649, %315
  store i8 %or16764011, i8* %_buffer121
  %savedstack41 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %82), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer121, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 20, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %82, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %82, i8 -1, i64 1, i1 false), !dbg !406
  %call.i40 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer121, i8* nonnull %82, i64 21, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %82), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack41), !dbg !408
  store i8 %hdr.sroa.434069.sroa.5.242154495, i8* %_buffer6
  %savedstack43 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %81), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer6, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 21, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %81, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %81, i8 -1, i64 1, i1 false), !dbg !406
  %call.i42 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer6, i8* nonnull %81, i64 22, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %81), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack43), !dbg !408
  store i8 %hdr.sroa.46.243874409, i8* %_buffer18
  %savedstack45 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %80), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer18, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 22, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %80, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %80, i8 -1, i64 1, i1 false), !dbg !406
  %call.i44 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer18, i8* nonnull %80, i64 23, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %80), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack45), !dbg !408
  store i8 %hdr.sroa.48.243854410, i8* %_buffer7
  %savedstack47 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %79), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer7, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 23, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %79, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %79, i8 -1, i64 1, i1 false), !dbg !406
  %call.i46 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer7, i8* nonnull %79, i64 24, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %79), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack47), !dbg !408
  %hdr.sroa.51.sroa.7.0.insert.ext = zext i8 %hdr.sroa.51.sroa.7.242194493 to i16
  %hdr.sroa.51.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.51.sroa.7.0.insert.ext, 8
  %hdr.sroa.51.sroa.0.0.insert.ext = zext i8 %hdr.sroa.51.sroa.0.242214492 to i16
  %hdr.sroa.51.sroa.0.0.insert.insert = or i16 %hdr.sroa.51.sroa.7.0.insert.shift, %hdr.sroa.51.sroa.0.0.insert.ext
  %rev4014 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.51.sroa.0.0.insert.insert)
  %hdr.sroa.51.sroa.0.0.extract.trunc4189 = trunc i16 %rev4014 to i8
  %hdr.sroa.51.sroa.7.0.extract.shift4190 = lshr i16 %rev4014, 8
  %hdr.sroa.51.sroa.7.0.extract.trunc4191 = trunc i16 %hdr.sroa.51.sroa.7.0.extract.shift4190 to i8
  store i8 %hdr.sroa.51.sroa.0.0.extract.trunc4189, i8* %_buffer117
  %savedstack49 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %78), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer117, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 24, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %78, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %78, i8 -1, i64 1, i1 false), !dbg !406
  %call.i48 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer117, i8* nonnull %78, i64 25, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %78), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack49), !dbg !408
  store i8 %hdr.sroa.51.sroa.7.0.extract.trunc4191, i8* %_buffer159
  %savedstack51 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %77), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer159, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 25, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %77, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %77, i8 -1, i64 1, i1 false), !dbg !406
  %call.i50 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer159, i8* nonnull %77, i64 26, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %77), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack51), !dbg !408
  %hdr.sroa.56.sroa.9.0.insert.ext = zext i8 %hdr.sroa.56.sroa.9.242234491 to i32
  %hdr.sroa.56.sroa.9.0.insert.shift = shl nuw i32 %hdr.sroa.56.sroa.9.0.insert.ext, 24
  %hdr.sroa.56.sroa.8.0.insert.ext = zext i8 %hdr.sroa.56.sroa.8.242254490 to i32
  %hdr.sroa.56.sroa.8.0.insert.shift = shl nuw nsw i32 %hdr.sroa.56.sroa.8.0.insert.ext, 16
  %hdr.sroa.56.sroa.8.0.insert.insert = or i32 %hdr.sroa.56.sroa.8.0.insert.shift, %hdr.sroa.56.sroa.9.0.insert.shift
  %hdr.sroa.56.sroa.7.0.insert.ext = zext i8 %hdr.sroa.56.sroa.7.242274489 to i32
  %hdr.sroa.56.sroa.7.0.insert.shift = shl nuw nsw i32 %hdr.sroa.56.sroa.7.0.insert.ext, 8
  %hdr.sroa.56.sroa.7.0.insert.insert = or i32 %hdr.sroa.56.sroa.8.0.insert.insert, %hdr.sroa.56.sroa.7.0.insert.shift
  %hdr.sroa.56.sroa.0.0.insert.ext = zext i8 %hdr.sroa.56.sroa.0.242294488 to i32
  %hdr.sroa.56.sroa.0.0.insert.insert = or i32 %hdr.sroa.56.sroa.7.0.insert.insert, %hdr.sroa.56.sroa.0.0.insert.ext
  %or1799 = tail call i32 @llvm.bswap.i32(i32 %hdr.sroa.56.sroa.0.0.insert.insert)
  %hdr.sroa.56.sroa.0.0.extract.trunc4182 = trunc i32 %or1799 to i8
  %hdr.sroa.56.sroa.7.0.extract.shift4183 = lshr i32 %or1799, 8
  %hdr.sroa.56.sroa.7.0.extract.trunc4184 = trunc i32 %hdr.sroa.56.sroa.7.0.extract.shift4183 to i8
  %hdr.sroa.56.sroa.8.0.extract.shift4185 = lshr i32 %or1799, 16
  %hdr.sroa.56.sroa.8.0.extract.trunc4186 = trunc i32 %hdr.sroa.56.sroa.8.0.extract.shift4185 to i8
  %hdr.sroa.56.sroa.9.0.extract.shift4187 = lshr i32 %or1799, 24
  %hdr.sroa.56.sroa.9.0.extract.trunc4188 = trunc i32 %hdr.sroa.56.sroa.9.0.extract.shift4187 to i8
  store i8 %hdr.sroa.56.sroa.0.0.extract.trunc4182, i8* %_buffer67
  %savedstack53 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %76), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer67, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 26, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %76, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %76, i8 -1, i64 1, i1 false), !dbg !406
  %call.i52 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer67, i8* nonnull %76, i64 27, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %76), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack53), !dbg !408
  store i8 %hdr.sroa.56.sroa.7.0.extract.trunc4184, i8* %_buffer133
  %savedstack55 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %75), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer133, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 27, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %75, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %75, i8 -1, i64 1, i1 false), !dbg !406
  %call.i54 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer133, i8* nonnull %75, i64 28, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %75), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack55), !dbg !408
  store i8 %hdr.sroa.56.sroa.8.0.extract.trunc4186, i8* %_buffer98
  %savedstack57 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %74), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer98, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 28, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %74, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %74, i8 -1, i64 1, i1 false), !dbg !406
  %call.i56 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer98, i8* nonnull %74, i64 29, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %74), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack57), !dbg !408
  store i8 %hdr.sroa.56.sroa.9.0.extract.trunc4188, i8* %_buffer9
  %savedstack59 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %73), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer9, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 29, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %73, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %73, i8 -1, i64 1, i1 false), !dbg !406
  %call.i58 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer9, i8* nonnull %73, i64 30, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %73), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack59), !dbg !408
  %hdr.sroa.63.sroa.9.0.insert.ext = zext i8 %hdr.sroa.63.sroa.9.242314487 to i32
  %hdr.sroa.63.sroa.9.0.insert.shift = shl nuw i32 %hdr.sroa.63.sroa.9.0.insert.ext, 24
  %hdr.sroa.63.sroa.8.0.insert.ext = zext i8 %hdr.sroa.63.sroa.8.242334486 to i32
  %hdr.sroa.63.sroa.8.0.insert.shift = shl nuw nsw i32 %hdr.sroa.63.sroa.8.0.insert.ext, 16
  %hdr.sroa.63.sroa.8.0.insert.insert = or i32 %hdr.sroa.63.sroa.8.0.insert.shift, %hdr.sroa.63.sroa.9.0.insert.shift
  %hdr.sroa.63.sroa.7.0.insert.ext = zext i8 %hdr.sroa.63.sroa.7.242354485 to i32
  %hdr.sroa.63.sroa.7.0.insert.shift = shl nuw nsw i32 %hdr.sroa.63.sroa.7.0.insert.ext, 8
  %hdr.sroa.63.sroa.7.0.insert.insert = or i32 %hdr.sroa.63.sroa.8.0.insert.insert, %hdr.sroa.63.sroa.7.0.insert.shift
  %hdr.sroa.63.sroa.0.0.insert.ext = zext i8 %hdr.sroa.63.sroa.0.242374484 to i32
  %hdr.sroa.63.sroa.0.0.insert.insert = or i32 %hdr.sroa.63.sroa.7.0.insert.insert, %hdr.sroa.63.sroa.0.0.insert.ext
  %or1881 = tail call i32 @llvm.bswap.i32(i32 %hdr.sroa.63.sroa.0.0.insert.insert)
  %hdr.sroa.63.sroa.0.0.extract.trunc = trunc i32 %or1881 to i8
  %hdr.sroa.63.sroa.7.0.extract.shift = lshr i32 %or1881, 8
  %hdr.sroa.63.sroa.7.0.extract.trunc = trunc i32 %hdr.sroa.63.sroa.7.0.extract.shift to i8
  %hdr.sroa.63.sroa.8.0.extract.shift = lshr i32 %or1881, 16
  %hdr.sroa.63.sroa.8.0.extract.trunc = trunc i32 %hdr.sroa.63.sroa.8.0.extract.shift to i8
  %hdr.sroa.63.sroa.9.0.extract.shift = lshr i32 %or1881, 24
  %hdr.sroa.63.sroa.9.0.extract.trunc = trunc i32 %hdr.sroa.63.sroa.9.0.extract.shift to i8
  store i8 %hdr.sroa.63.sroa.0.0.extract.trunc, i8* %_buffer11
  %savedstack61 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %72), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer11, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 30, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %72, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %72, i8 -1, i64 1, i1 false), !dbg !406
  %call.i60 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer11, i8* nonnull %72, i64 31, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %72), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack61), !dbg !408
  store i8 %hdr.sroa.63.sroa.7.0.extract.trunc, i8* %_buffer81
  %savedstack63 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %71), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer81, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 31, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %71, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %71, i8 -1, i64 1, i1 false), !dbg !406
  %call.i62 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer81, i8* nonnull %71, i64 32, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %71), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack63), !dbg !408
  store i8 %hdr.sroa.63.sroa.8.0.extract.trunc, i8* %_buffer173
  %savedstack65 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %70), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer173, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 32, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %70, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %70, i8 -1, i64 1, i1 false), !dbg !406
  %call.i64 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer173, i8* nonnull %70, i64 33, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %70), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack65), !dbg !408
  store i8 %hdr.sroa.63.sroa.9.0.extract.trunc, i8* %_buffer111
  %savedstack67 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %69), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer111, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 33, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %69, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %69, i8 -1, i64 1, i1 false), !dbg !406
  %call.i66 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer111, i8* nonnull %69, i64 34, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %69), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack67), !dbg !408
  br label %if.end1942

if.end1942:                                       ; preds = %if.end1486, %if.end1474
  %ebpf_packetOffsetInBits.4 = phi i32 [ 272, %if.end1486 ], [ 112, %if.end1474 ]
  br i1 %tobool1233, label %if.end2547, label %if.then1946

if.then1946:                                      ; preds = %if.end1942
  %add1947 = add nuw nsw i32 %ebpf_packetOffsetInBits.4, 320
  %div1948 = lshr exact i32 %add1947, 3
  %idx.ext1949 = zext i32 %div1948 to i64
  %316 = add i64 0, %idx.ext1949
  %317 = icmp ugt i64 %316, %310
  br i1 %317, label %cleanup3277, label %if.end1954

if.end1954:                                       ; preds = %if.then1946
  %shl1960 = shl i8 %hdr.sroa.744082.243834411, 4
  %div1962 = lshr exact i32 %ebpf_packetOffsetInBits.4, 3
  %idx.ext1964 = zext i32 %div1962 to i64
  %318 = add i64 0, %idx.ext1964
  %319 = lshr i8 %hdr.sroa.76.243814412, 4
  %or19874007 = or i8 %shl1960, %319
  store i8 %or19874007, i8* %_buffer76
  %savedstack69 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %68), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer76, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %318, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %68, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %68, i8 -1, i64 1, i1 false), !dbg !406
  %320 = add i64 %318, 1
  %call.i68 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer76, i8* nonnull %68, i64 %320, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %68), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack69), !dbg !408
  %div2001 = or i32 %div1962, 1
  %idx.ext2002 = zext i32 %div2001 to i64
  %321 = add i64 0, %idx.ext2002
  %322 = add i64 %321, 1
  %323 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer135, i64 %322, i64 1)
  %324 = load i8, i8* %_buffer135
  %325 = and i8 %324, -16
  %326 = and i8 %hdr.sroa.784084.sroa.0.242434481, 15
  %or20144008 = or i8 %325, %326
  store i8 %or20144008, i8* %_buffer134
  %savedstack71 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %67), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer134, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %321, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %67, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %67, i8 -1, i64 1, i1 false), !dbg !406
  %327 = add i64 %321, 1
  %call.i70 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer134, i8* nonnull %67, i64 %327, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %67), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack71), !dbg !408
  %add2030 = add nuw nsw i32 %div2001, 1
  %idx.ext2031 = zext i32 %add2030 to i64
  %328 = add i64 0, %idx.ext2031
  store i8 %hdr.sroa.784084.sroa.5.242414482, i8* %_buffer71
  %savedstack73 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %66), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer71, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %328, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %66, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %66, i8 -1, i64 1, i1 false), !dbg !406
  %329 = add i64 %328, 1
  %call.i72 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer71, i8* nonnull %66, i64 %329, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %66), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack73), !dbg !408
  %add2043 = add nuw nsw i32 %div2001, 2
  %idx.ext2044 = zext i32 %add2043 to i64
  %330 = add i64 0, %idx.ext2044
  store i8 %hdr.sroa.784084.sroa.6.242394483, i8* %_buffer74
  %savedstack75 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %65), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer74, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %330, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %65, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %65, i8 -1, i64 1, i1 false), !dbg !406
  %331 = add i64 %330, 1
  %call.i74 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer74, i8* nonnull %65, i64 %331, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %65), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack75), !dbg !408
  %add2048 = add nuw nsw i32 %ebpf_packetOffsetInBits.4, 32
  %hdr.sroa.82.sroa.7.0.insert.ext = zext i8 %hdr.sroa.82.sroa.7.242454480 to i16
  %hdr.sroa.82.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.82.sroa.7.0.insert.ext, 8
  %hdr.sroa.82.sroa.0.0.insert.ext = zext i8 %hdr.sroa.82.sroa.0.242474479 to i16
  %hdr.sroa.82.sroa.0.0.insert.insert = or i16 %hdr.sroa.82.sroa.7.0.insert.shift, %hdr.sroa.82.sroa.0.0.insert.ext
  %rev4009 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.82.sroa.0.0.insert.insert)
  %hdr.sroa.82.sroa.0.0.extract.trunc = trunc i16 %rev4009 to i8
  %hdr.sroa.82.sroa.7.0.extract.shift = lshr i16 %rev4009, 8
  %hdr.sroa.82.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.82.sroa.7.0.extract.shift to i8
  %div2081 = lshr exact i32 %add2048, 3
  %idx.ext2083 = zext i32 %div2081 to i64
  %332 = add i64 0, %idx.ext2083
  store i8 %hdr.sroa.82.sroa.0.0.extract.trunc, i8* %_buffer13
  %savedstack77 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %64), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer13, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %332, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %64, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %64, i8 -1, i64 1, i1 false), !dbg !406
  %333 = add i64 %332, 1
  %call.i76 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer13, i8* nonnull %64, i64 %333, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %64), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack77), !dbg !408
  %add2095 = or i32 %div2081, 1
  %idx.ext2096 = zext i32 %add2095 to i64
  %334 = add i64 0, %idx.ext2096
  store i8 %hdr.sroa.82.sroa.7.0.extract.trunc, i8* %_buffer169
  %savedstack79 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %63), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer169, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %334, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %63, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %63, i8 -1, i64 1, i1 false), !dbg !406
  %335 = add i64 %334, 1
  %call.i78 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer169, i8* nonnull %63, i64 %335, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %63), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack79), !dbg !408
  %add2100 = add nuw nsw i32 %ebpf_packetOffsetInBits.4, 48
  %div2108 = lshr exact i32 %add2100, 3
  %idx.ext2110 = zext i32 %div2108 to i64
  %336 = add i64 0, %idx.ext2110
  store i8 %hdr.sroa.87.243794413, i8* %_buffer60
  %savedstack81 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %62), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer60, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %336, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %62, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %62, i8 -1, i64 1, i1 false), !dbg !406
  %337 = add i64 %336, 1
  %call.i80 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer60, i8* nonnull %62, i64 %337, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %62), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack81), !dbg !408
  %add2114 = add nuw nsw i32 %ebpf_packetOffsetInBits.4, 56
  %div2122 = lshr exact i32 %add2114, 3
  %idx.ext2124 = zext i32 %div2122 to i64
  %338 = add i64 0, %idx.ext2124
  store i8 %hdr.sroa.90.243774414, i8* %_buffer113
  %savedstack83 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %61), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer113, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %338, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %61, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %61, i8 -1, i64 1, i1 false), !dbg !406
  %339 = add i64 %338, 1
  %call.i82 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer113, i8* nonnull %61, i64 %339, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %61), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack83), !dbg !408
  %add2128 = add nuw nsw i32 %ebpf_packetOffsetInBits.4, 64
  %div2136 = lshr exact i32 %add2128, 3
  %idx.ext2138 = zext i32 %div2136 to i64
  %340 = add i64 0, %idx.ext2138
  store i8 %hdr.sroa.92.243754415, i8* %_buffer166
  %savedstack85 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %60), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer166, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %340, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %60, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %60, i8 -1, i64 1, i1 false), !dbg !406
  %341 = add i64 %340, 1
  %call.i84 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer166, i8* nonnull %60, i64 %341, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %60), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack85), !dbg !408
  %add2150 = or i32 %div2136, 1
  %idx.ext2151 = zext i32 %add2150 to i64
  %342 = add i64 0, %idx.ext2151
  store i8 %hdr.sroa.94.243734416, i8* %_buffer69
  %savedstack87 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %59), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer69, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %342, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %59, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %59, i8 -1, i64 1, i1 false), !dbg !406
  %343 = add i64 %342, 1
  %call.i86 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer69, i8* nonnull %59, i64 %343, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %59), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack87), !dbg !408
  %add2163 = add nuw nsw i32 %div2136, 2
  %idx.ext2164 = zext i32 %add2163 to i64
  %344 = add i64 0, %idx.ext2164
  store i8 %hdr.sroa.96.243714417, i8* %_buffer72
  %savedstack89 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %58), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer72, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %344, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %58, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %58, i8 -1, i64 1, i1 false), !dbg !406
  %345 = add i64 %344, 1
  %call.i88 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer72, i8* nonnull %58, i64 %345, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %58), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack89), !dbg !408
  %add2176 = add nuw nsw i32 %div2136, 3
  %idx.ext2177 = zext i32 %add2176 to i64
  %346 = add i64 0, %idx.ext2177
  store i8 %hdr.sroa.98.243694418, i8* %_buffer160
  %savedstack91 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %57), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer160, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %346, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %57, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %57, i8 -1, i64 1, i1 false), !dbg !406
  %347 = add i64 %346, 1
  %call.i90 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer160, i8* nonnull %57, i64 %347, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %57), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack91), !dbg !408
  %add2189 = add nuw nsw i32 %div2136, 4
  %idx.ext2190 = zext i32 %add2189 to i64
  %348 = add i64 0, %idx.ext2190
  store i8 %hdr.sroa.100.243674419, i8* %_buffer155
  %savedstack93 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %56), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer155, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %348, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %56, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %56, i8 -1, i64 1, i1 false), !dbg !406
  %349 = add i64 %348, 1
  %call.i92 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer155, i8* nonnull %56, i64 %349, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %56), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack93), !dbg !408
  %add2202 = add nuw nsw i32 %div2136, 5
  %idx.ext2203 = zext i32 %add2202 to i64
  %350 = add i64 0, %idx.ext2203
  store i8 %hdr.sroa.102.243654420, i8* %_buffer167
  %savedstack95 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %55), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer167, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %350, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %55, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %55, i8 -1, i64 1, i1 false), !dbg !406
  %351 = add i64 %350, 1
  %call.i94 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer167, i8* nonnull %55, i64 %351, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %55), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack95), !dbg !408
  %add2215 = add nuw nsw i32 %div2136, 6
  %idx.ext2216 = zext i32 %add2215 to i64
  %352 = add i64 0, %idx.ext2216
  store i8 %hdr.sroa.104.243634421, i8* %_buffer150
  %savedstack97 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %54), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer150, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %352, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %54, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %54, i8 -1, i64 1, i1 false), !dbg !406
  %353 = add i64 %352, 1
  %call.i96 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer150, i8* nonnull %54, i64 %353, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %54), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack97), !dbg !408
  %add2228 = add nuw nsw i32 %div2136, 7
  %idx.ext2229 = zext i32 %add2228 to i64
  %354 = add i64 0, %idx.ext2229
  store i8 %hdr.sroa.106.243614422, i8* %_buffer62
  %savedstack99 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %53), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer62, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %354, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %53, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %53, i8 -1, i64 1, i1 false), !dbg !406
  %355 = add i64 %354, 1
  %call.i98 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer62, i8* nonnull %53, i64 %355, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %53), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack99), !dbg !408
  %add2241 = add nuw nsw i32 %div2136, 8
  %idx.ext2242 = zext i32 %add2241 to i64
  %356 = add i64 0, %idx.ext2242
  store i8 %hdr.sroa.108.243594423, i8* %_buffer85
  %savedstack101 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %52), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer85, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %356, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %52, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %52, i8 -1, i64 1, i1 false), !dbg !406
  %357 = add i64 %356, 1
  %call.i100 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer85, i8* nonnull %52, i64 %357, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %52), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack101), !dbg !408
  %add2254 = add nuw nsw i32 %div2136, 9
  %idx.ext2255 = zext i32 %add2254 to i64
  %358 = add i64 0, %idx.ext2255
  store i8 %hdr.sroa.110.243574424, i8* %_buffer146
  %savedstack103 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %51), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer146, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %358, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %51, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %51, i8 -1, i64 1, i1 false), !dbg !406
  %359 = add i64 %358, 1
  %call.i102 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer146, i8* nonnull %51, i64 %359, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %51), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack103), !dbg !408
  %add2267 = add nuw nsw i32 %div2136, 10
  %idx.ext2268 = zext i32 %add2267 to i64
  %360 = add i64 0, %idx.ext2268
  store i8 %hdr.sroa.112.243554425, i8* %_buffer141
  %savedstack105 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %50), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer141, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %360, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %50, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %50, i8 -1, i64 1, i1 false), !dbg !406
  %361 = add i64 %360, 1
  %call.i104 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer141, i8* nonnull %50, i64 %361, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %50), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack105), !dbg !408
  %add2280 = add nuw nsw i32 %div2136, 11
  %idx.ext2281 = zext i32 %add2280 to i64
  %362 = add i64 0, %idx.ext2281
  store i8 %hdr.sroa.114.243534426, i8* %_buffer139
  %savedstack107 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %49), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer139, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %362, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %49, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %49, i8 -1, i64 1, i1 false), !dbg !406
  %363 = add i64 %362, 1
  %call.i106 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer139, i8* nonnull %49, i64 %363, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %49), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack107), !dbg !408
  %add2293 = add nuw nsw i32 %div2136, 12
  %idx.ext2294 = zext i32 %add2293 to i64
  %364 = add i64 0, %idx.ext2294
  store i8 %hdr.sroa.116.243514427, i8* %_buffer131
  %savedstack109 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %48), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer131, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %364, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %48, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %48, i8 -1, i64 1, i1 false), !dbg !406
  %365 = add i64 %364, 1
  %call.i108 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer131, i8* nonnull %48, i64 %365, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %48), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack109), !dbg !408
  %add2306 = add nuw nsw i32 %div2136, 13
  %idx.ext2307 = zext i32 %add2306 to i64
  %366 = add i64 0, %idx.ext2307
  store i8 %hdr.sroa.118.243494428, i8* %_buffer127
  %savedstack111 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %47), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer127, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %366, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %47, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %47, i8 -1, i64 1, i1 false), !dbg !406
  %367 = add i64 %366, 1
  %call.i110 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer127, i8* nonnull %47, i64 %367, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %47), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack111), !dbg !408
  %add2319 = add nuw nsw i32 %div2136, 14
  %idx.ext2320 = zext i32 %add2319 to i64
  %368 = add i64 0, %idx.ext2320
  store i8 %hdr.sroa.120.243474429, i8* %_buffer125
  %savedstack113 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %46), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer125, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %368, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %46, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %46, i8 -1, i64 1, i1 false), !dbg !406
  %369 = add i64 %368, 1
  %call.i112 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer125, i8* nonnull %46, i64 %369, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %46), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack113), !dbg !408
  %add2332 = add nuw nsw i32 %div2136, 15
  %idx.ext2333 = zext i32 %add2332 to i64
  %370 = add i64 0, %idx.ext2333
  store i8 %hdr.sroa.122.243454430, i8* %_buffer123
  %savedstack115 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %45), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer123, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %370, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %45, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %45, i8 -1, i64 1, i1 false), !dbg !406
  %371 = add i64 %370, 1
  %call.i114 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer123, i8* nonnull %45, i64 %371, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %45), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack115), !dbg !408
  %add2337 = add nuw nsw i32 %ebpf_packetOffsetInBits.4, 192
  %div2345 = lshr exact i32 %add2337, 3
  %idx.ext2347 = zext i32 %div2345 to i64
  %372 = add i64 0, %idx.ext2347
  store i8 %hdr.sroa.124.243434431, i8* %_buffer156
  %savedstack117 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %44), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer156, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %372, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %44, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %44, i8 -1, i64 1, i1 false), !dbg !406
  %373 = add i64 %372, 1
  %call.i116 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer156, i8* nonnull %44, i64 %373, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %44), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack117), !dbg !408
  %add2359 = or i32 %div2345, 1
  %idx.ext2360 = zext i32 %add2359 to i64
  %374 = add i64 0, %idx.ext2360
  store i8 %hdr.sroa.126.243414432, i8* %_buffer154
  %savedstack119 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %43), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer154, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %374, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %43, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %43, i8 -1, i64 1, i1 false), !dbg !406
  %375 = add i64 %374, 1
  %call.i118 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer154, i8* nonnull %43, i64 %375, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %43), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack119), !dbg !408
  %add2372 = add nuw nsw i32 %div2345, 2
  %idx.ext2373 = zext i32 %add2372 to i64
  %376 = add i64 0, %idx.ext2373
  store i8 %hdr.sroa.128.243394433, i8* %_buffer151
  %savedstack121 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %42), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer151, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %376, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %42, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %42, i8 -1, i64 1, i1 false), !dbg !406
  %377 = add i64 %376, 1
  %call.i120 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer151, i8* nonnull %42, i64 %377, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %42), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack121), !dbg !408
  %add2385 = add nuw nsw i32 %div2345, 3
  %idx.ext2386 = zext i32 %add2385 to i64
  %378 = add i64 0, %idx.ext2386
  store i8 %hdr.sroa.130.243374434, i8* %_buffer147
  %savedstack123 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %41), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer147, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %378, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %41, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %41, i8 -1, i64 1, i1 false), !dbg !406
  %379 = add i64 %378, 1
  %call.i122 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer147, i8* nonnull %41, i64 %379, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %41), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack123), !dbg !408
  %add2398 = add nuw nsw i32 %div2345, 4
  %idx.ext2399 = zext i32 %add2398 to i64
  %380 = add i64 0, %idx.ext2399
  store i8 %hdr.sroa.132.243354435, i8* %_buffer118
  %savedstack125 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %40), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer118, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %380, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %40, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %40, i8 -1, i64 1, i1 false), !dbg !406
  %381 = add i64 %380, 1
  %call.i124 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer118, i8* nonnull %40, i64 %381, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %40), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack125), !dbg !408
  %add2411 = add nuw nsw i32 %div2345, 5
  %idx.ext2412 = zext i32 %add2411 to i64
  %382 = add i64 0, %idx.ext2412
  store i8 %hdr.sroa.134.243334436, i8* %_buffer116
  %savedstack127 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %39), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer116, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %382, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %39, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %39, i8 -1, i64 1, i1 false), !dbg !406
  %383 = add i64 %382, 1
  %call.i126 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer116, i8* nonnull %39, i64 %383, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %39), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack127), !dbg !408
  %add2424 = add nuw nsw i32 %div2345, 6
  %idx.ext2425 = zext i32 %add2424 to i64
  %384 = add i64 0, %idx.ext2425
  store i8 %hdr.sroa.136.243314437, i8* %_buffer142
  %savedstack129 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %38), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer142, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %384, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %38, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %38, i8 -1, i64 1, i1 false), !dbg !406
  %385 = add i64 %384, 1
  %call.i128 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer142, i8* nonnull %38, i64 %385, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %38), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack129), !dbg !408
  %add2437 = add nuw nsw i32 %div2345, 7
  %idx.ext2438 = zext i32 %add2437 to i64
  %386 = add i64 0, %idx.ext2438
  store i8 %hdr.sroa.138.243294438, i8* %_buffer140
  %savedstack131 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %37), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer140, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %386, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %37, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %37, i8 -1, i64 1, i1 false), !dbg !406
  %387 = add i64 %386, 1
  %call.i130 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer140, i8* nonnull %37, i64 %387, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %37), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack131), !dbg !408
  %add2450 = add nuw nsw i32 %div2345, 8
  %idx.ext2451 = zext i32 %add2450 to i64
  %388 = add i64 0, %idx.ext2451
  store i8 %hdr.sroa.140.243274439, i8* %_buffer137
  %savedstack133 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %36), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer137, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %388, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %36, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %36, i8 -1, i64 1, i1 false), !dbg !406
  %389 = add i64 %388, 1
  %call.i132 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer137, i8* nonnull %36, i64 %389, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %36), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack133), !dbg !408
  %add2463 = add nuw nsw i32 %div2345, 9
  %idx.ext2464 = zext i32 %add2463 to i64
  %390 = add i64 0, %idx.ext2464
  store i8 %hdr.sroa.142.243254440, i8* %_buffer129
  %savedstack135 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %35), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer129, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %390, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %35, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %35, i8 -1, i64 1, i1 false), !dbg !406
  %391 = add i64 %390, 1
  %call.i134 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer129, i8* nonnull %35, i64 %391, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %35), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack135), !dbg !408
  %add2476 = add nuw nsw i32 %div2345, 10
  %idx.ext2477 = zext i32 %add2476 to i64
  %392 = add i64 0, %idx.ext2477
  store i8 %hdr.sroa.144.243234441, i8* %_buffer126
  %savedstack137 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %34), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer126, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %392, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %34, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %34, i8 -1, i64 1, i1 false), !dbg !406
  %393 = add i64 %392, 1
  %call.i136 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer126, i8* nonnull %34, i64 %393, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %34), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack137), !dbg !408
  %add2489 = add nuw nsw i32 %div2345, 11
  %idx.ext2490 = zext i32 %add2489 to i64
  %394 = add i64 0, %idx.ext2490
  store i8 %hdr.sroa.146.243214442, i8* %_buffer124
  %savedstack139 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %33), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer124, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %394, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %33, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %33, i8 -1, i64 1, i1 false), !dbg !406
  %395 = add i64 %394, 1
  %call.i138 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer124, i8* nonnull %33, i64 %395, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %33), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack139), !dbg !408
  %add2502 = add nuw nsw i32 %div2345, 12
  %idx.ext2503 = zext i32 %add2502 to i64
  %396 = add i64 0, %idx.ext2503
  store i8 %hdr.sroa.148.243194443, i8* %_buffer122
  %savedstack141 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %32), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer122, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %396, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %32, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %32, i8 -1, i64 1, i1 false), !dbg !406
  %397 = add i64 %396, 1
  %call.i140 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer122, i8* nonnull %32, i64 %397, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %32), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack141), !dbg !408
  %add2515 = add nuw nsw i32 %div2345, 13
  %idx.ext2516 = zext i32 %add2515 to i64
  %398 = add i64 0, %idx.ext2516
  store i8 %hdr.sroa.150.243174444, i8* %_buffer105
  %savedstack143 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %31), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer105, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %398, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %31, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %31, i8 -1, i64 1, i1 false), !dbg !406
  %399 = add i64 %398, 1
  %call.i142 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer105, i8* nonnull %31, i64 %399, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %31), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack143), !dbg !408
  %add2528 = add nuw nsw i32 %div2345, 14
  %idx.ext2529 = zext i32 %add2528 to i64
  %400 = add i64 0, %idx.ext2529
  store i8 %hdr.sroa.152.243154445, i8* %_buffer104
  %savedstack145 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %30), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer104, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %400, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %30, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %30, i8 -1, i64 1, i1 false), !dbg !406
  %401 = add i64 %400, 1
  %call.i144 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer104, i8* nonnull %30, i64 %401, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %30), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack145), !dbg !408
  %add2541 = add nuw nsw i32 %div2345, 15
  %idx.ext2542 = zext i32 %add2541 to i64
  %402 = add i64 0, %idx.ext2542
  store i8 %hdr.sroa.154.243134446, i8* %_buffer82
  %savedstack147 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %29), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer82, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %402, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %29, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %29, i8 -1, i64 1, i1 false), !dbg !406
  %403 = add i64 %402, 1
  %call.i146 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer82, i8* nonnull %29, i64 %403, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %29), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack147), !dbg !408
  br label %if.end2547

if.end2547:                                       ; preds = %if.end1954, %if.end1942
  %ebpf_packetOffsetInBits.5 = phi i32 [ %add1947, %if.end1954 ], [ %ebpf_packetOffsetInBits.4, %if.end1942 ]
  br i1 %tobool1239, label %if.end3052, label %if.then2551

if.then2551:                                      ; preds = %if.end2547
  %add2552 = add nsw i32 %ebpf_packetOffsetInBits.5, 160
  %div2553 = lshr i32 %add2552, 3
  %idx.ext2554 = zext i32 %div2553 to i64
  %404 = add i64 0, %idx.ext2554
  %405 = icmp ugt i64 %404, %310
  br i1 %405, label %cleanup3277, label %if.end2559

if.end2559:                                       ; preds = %if.then2551
  %hdr.sroa.1604093.sroa.7.0.insert.ext = zext i8 %hdr.sroa.1604093.sroa.7.042494478 to i16
  %hdr.sroa.1604093.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.1604093.sroa.7.0.insert.ext, 8
  %hdr.sroa.1604093.sroa.0.0.insert.ext = zext i8 %hdr.sroa.1604093.sroa.0.042514477 to i16
  %hdr.sroa.1604093.sroa.0.0.insert.insert = or i16 %hdr.sroa.1604093.sroa.7.0.insert.shift, %hdr.sroa.1604093.sroa.0.0.insert.ext
  %rev4006 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.1604093.sroa.0.0.insert.insert)
  %hdr.sroa.1604093.sroa.0.0.extract.trunc4163 = trunc i16 %rev4006 to i8
  %hdr.sroa.1604093.sroa.7.0.extract.shift4164 = lshr i16 %rev4006, 8
  %hdr.sroa.1604093.sroa.7.0.extract.trunc4165 = trunc i16 %hdr.sroa.1604093.sroa.7.0.extract.shift4164 to i8
  %div2592 = lshr i32 %ebpf_packetOffsetInBits.5, 3
  %idx.ext2594 = zext i32 %div2592 to i64
  %406 = add i64 0, %idx.ext2594
  store i8 %hdr.sroa.1604093.sroa.0.0.extract.trunc4163, i8* %_buffer92
  %savedstack149 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %28), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer92, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %406, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %28, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %28, i8 -1, i64 1, i1 false), !dbg !406
  %407 = add i64 %406, 1
  %call.i148 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer92, i8* nonnull %28, i64 %407, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %28), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack149), !dbg !408
  %add2606 = add nuw nsw i32 %div2592, 1
  %idx.ext2607 = zext i32 %add2606 to i64
  %408 = add i64 0, %idx.ext2607
  store i8 %hdr.sroa.1604093.sroa.7.0.extract.trunc4165, i8* %_buffer120
  %savedstack151 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %27), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer120, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %408, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %27, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %27, i8 -1, i64 1, i1 false), !dbg !406
  %409 = add i64 %408, 1
  %call.i150 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer120, i8* nonnull %27, i64 %409, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %27), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack151), !dbg !408
  %add2611 = add nsw i32 %ebpf_packetOffsetInBits.5, 16
  %hdr.sroa.165.sroa.7.0.insert.ext = zext i8 %hdr.sroa.165.sroa.7.042534476 to i16
  %hdr.sroa.165.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.165.sroa.7.0.insert.ext, 8
  %hdr.sroa.165.sroa.0.0.insert.ext = zext i8 %hdr.sroa.165.sroa.0.042554475 to i16
  %hdr.sroa.165.sroa.0.0.insert.insert = or i16 %hdr.sroa.165.sroa.7.0.insert.shift, %hdr.sroa.165.sroa.0.0.insert.ext
  %rev4005 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.165.sroa.0.0.insert.insert)
  %hdr.sroa.165.sroa.0.0.extract.trunc4160 = trunc i16 %rev4005 to i8
  %hdr.sroa.165.sroa.7.0.extract.shift4161 = lshr i16 %rev4005, 8
  %hdr.sroa.165.sroa.7.0.extract.trunc4162 = trunc i16 %hdr.sroa.165.sroa.7.0.extract.shift4161 to i8
  %div2644 = lshr i32 %add2611, 3
  %idx.ext2646 = zext i32 %div2644 to i64
  %410 = add i64 0, %idx.ext2646
  store i8 %hdr.sroa.165.sroa.0.0.extract.trunc4160, i8* %_buffer108
  %savedstack153 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %26), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer108, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %410, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %26, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %26, i8 -1, i64 1, i1 false), !dbg !406
  %411 = add i64 %410, 1
  %call.i152 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer108, i8* nonnull %26, i64 %411, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %26), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack153), !dbg !408
  %add2658 = add nuw nsw i32 %div2644, 1
  %idx.ext2659 = zext i32 %add2658 to i64
  %412 = add i64 0, %idx.ext2659
  store i8 %hdr.sroa.165.sroa.7.0.extract.trunc4162, i8* %_buffer107
  %savedstack155 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %25), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer107, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %412, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %25, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %25, i8 -1, i64 1, i1 false), !dbg !406
  %413 = add i64 %412, 1
  %call.i154 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer107, i8* nonnull %25, i64 %413, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %25), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack155), !dbg !408
  %add2663 = add nsw i32 %ebpf_packetOffsetInBits.5, 32
  %hdr.sroa.170.sroa.9.0.insert.ext = zext i8 %hdr.sroa.170.sroa.9.042574474 to i32
  %hdr.sroa.170.sroa.9.0.insert.shift = shl nuw i32 %hdr.sroa.170.sroa.9.0.insert.ext, 24
  %hdr.sroa.170.sroa.8.0.insert.ext = zext i8 %hdr.sroa.170.sroa.8.042594473 to i32
  %hdr.sroa.170.sroa.8.0.insert.shift = shl nuw nsw i32 %hdr.sroa.170.sroa.8.0.insert.ext, 16
  %hdr.sroa.170.sroa.8.0.insert.insert = or i32 %hdr.sroa.170.sroa.8.0.insert.shift, %hdr.sroa.170.sroa.9.0.insert.shift
  %hdr.sroa.170.sroa.7.0.insert.ext = zext i8 %hdr.sroa.170.sroa.7.042614472 to i32
  %hdr.sroa.170.sroa.7.0.insert.shift = shl nuw nsw i32 %hdr.sroa.170.sroa.7.0.insert.ext, 8
  %hdr.sroa.170.sroa.7.0.insert.insert = or i32 %hdr.sroa.170.sroa.8.0.insert.insert, %hdr.sroa.170.sroa.7.0.insert.shift
  %hdr.sroa.170.sroa.0.0.insert.ext = zext i8 %hdr.sroa.170.sroa.0.042634471 to i32
  %hdr.sroa.170.sroa.0.0.insert.insert = or i32 %hdr.sroa.170.sroa.7.0.insert.insert, %hdr.sroa.170.sroa.0.0.insert.ext
  %or2685 = tail call i32 @llvm.bswap.i32(i32 %hdr.sroa.170.sroa.0.0.insert.insert)
  %hdr.sroa.170.sroa.0.0.extract.trunc4153 = trunc i32 %or2685 to i8
  %hdr.sroa.170.sroa.7.0.extract.shift4154 = lshr i32 %or2685, 8
  %hdr.sroa.170.sroa.7.0.extract.trunc4155 = trunc i32 %hdr.sroa.170.sroa.7.0.extract.shift4154 to i8
  %hdr.sroa.170.sroa.8.0.extract.shift4156 = lshr i32 %or2685, 16
  %hdr.sroa.170.sroa.8.0.extract.trunc4157 = trunc i32 %hdr.sroa.170.sroa.8.0.extract.shift4156 to i8
  %hdr.sroa.170.sroa.9.0.extract.shift4158 = lshr i32 %or2685, 24
  %hdr.sroa.170.sroa.9.0.extract.trunc4159 = trunc i32 %hdr.sroa.170.sroa.9.0.extract.shift4158 to i8
  %div2700 = lshr i32 %add2663, 3
  %idx.ext2702 = zext i32 %div2700 to i64
  %414 = add i64 0, %idx.ext2702
  store i8 %hdr.sroa.170.sroa.0.0.extract.trunc4153, i8* %_buffer162
  %savedstack157 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %24), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer162, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %414, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %24, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %24, i8 -1, i64 1, i1 false), !dbg !406
  %415 = add i64 %414, 1
  %call.i156 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer162, i8* nonnull %24, i64 %415, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %24), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack157), !dbg !408
  %add2714 = add nuw nsw i32 %div2700, 1
  %idx.ext2715 = zext i32 %add2714 to i64
  %416 = add i64 0, %idx.ext2715
  store i8 %hdr.sroa.170.sroa.7.0.extract.trunc4155, i8* %_buffer66
  %savedstack159 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %23), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer66, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %416, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %23, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %23, i8 -1, i64 1, i1 false), !dbg !406
  %417 = add i64 %416, 1
  %call.i158 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer66, i8* nonnull %23, i64 %417, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %23), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack159), !dbg !408
  %add2727 = add nuw nsw i32 %div2700, 2
  %idx.ext2728 = zext i32 %add2727 to i64
  %418 = add i64 0, %idx.ext2728
  store i8 %hdr.sroa.170.sroa.8.0.extract.trunc4157, i8* %_buffer93
  %savedstack161 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %22), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer93, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %418, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %22, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %22, i8 -1, i64 1, i1 false), !dbg !406
  %419 = add i64 %418, 1
  %call.i160 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer93, i8* nonnull %22, i64 %419, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %22), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack161), !dbg !408
  %add2740 = add nuw nsw i32 %div2700, 3
  %idx.ext2741 = zext i32 %add2740 to i64
  %420 = add i64 0, %idx.ext2741
  store i8 %hdr.sroa.170.sroa.9.0.extract.trunc4159, i8* %_buffer158
  %savedstack163 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %21), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer158, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %420, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %21, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %21, i8 -1, i64 1, i1 false), !dbg !406
  %421 = add i64 %420, 1
  %call.i162 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer158, i8* nonnull %21, i64 %421, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %21), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack163), !dbg !408
  %add2745 = add nsw i32 %ebpf_packetOffsetInBits.5, 64
  %hdr.sroa.177.sroa.9.0.insert.ext = zext i8 %hdr.sroa.177.sroa.9.042654470 to i32
  %hdr.sroa.177.sroa.9.0.insert.shift = shl nuw i32 %hdr.sroa.177.sroa.9.0.insert.ext, 24
  %hdr.sroa.177.sroa.8.0.insert.ext = zext i8 %hdr.sroa.177.sroa.8.042674469 to i32
  %hdr.sroa.177.sroa.8.0.insert.shift = shl nuw nsw i32 %hdr.sroa.177.sroa.8.0.insert.ext, 16
  %hdr.sroa.177.sroa.8.0.insert.insert = or i32 %hdr.sroa.177.sroa.8.0.insert.shift, %hdr.sroa.177.sroa.9.0.insert.shift
  %hdr.sroa.177.sroa.7.0.insert.ext = zext i8 %hdr.sroa.177.sroa.7.042694468 to i32
  %hdr.sroa.177.sroa.7.0.insert.shift = shl nuw nsw i32 %hdr.sroa.177.sroa.7.0.insert.ext, 8
  %hdr.sroa.177.sroa.7.0.insert.insert = or i32 %hdr.sroa.177.sroa.8.0.insert.insert, %hdr.sroa.177.sroa.7.0.insert.shift
  %hdr.sroa.177.sroa.0.0.insert.ext = zext i8 %hdr.sroa.177.sroa.0.042714467 to i32
  %hdr.sroa.177.sroa.0.0.insert.insert = or i32 %hdr.sroa.177.sroa.7.0.insert.insert, %hdr.sroa.177.sroa.0.0.insert.ext
  %or2767 = tail call i32 @llvm.bswap.i32(i32 %hdr.sroa.177.sroa.0.0.insert.insert)
  %hdr.sroa.177.sroa.0.0.extract.trunc4146 = trunc i32 %or2767 to i8
  %hdr.sroa.177.sroa.7.0.extract.shift4147 = lshr i32 %or2767, 8
  %hdr.sroa.177.sroa.7.0.extract.trunc4148 = trunc i32 %hdr.sroa.177.sroa.7.0.extract.shift4147 to i8
  %hdr.sroa.177.sroa.8.0.extract.shift4149 = lshr i32 %or2767, 16
  %hdr.sroa.177.sroa.8.0.extract.trunc4150 = trunc i32 %hdr.sroa.177.sroa.8.0.extract.shift4149 to i8
  %hdr.sroa.177.sroa.9.0.extract.shift4151 = lshr i32 %or2767, 24
  %hdr.sroa.177.sroa.9.0.extract.trunc4152 = trunc i32 %hdr.sroa.177.sroa.9.0.extract.shift4151 to i8
  %div2782 = lshr i32 %add2745, 3
  %idx.ext2784 = zext i32 %div2782 to i64
  %422 = add i64 0, %idx.ext2784
  store i8 %hdr.sroa.177.sroa.0.0.extract.trunc4146, i8* %_buffer55
  %savedstack165 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %20), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer55, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %422, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %20, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %20, i8 -1, i64 1, i1 false), !dbg !406
  %423 = add i64 %422, 1
  %call.i164 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer55, i8* nonnull %20, i64 %423, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %20), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack165), !dbg !408
  %add2796 = add nuw nsw i32 %div2782, 1
  %idx.ext2797 = zext i32 %add2796 to i64
  %424 = add i64 0, %idx.ext2797
  store i8 %hdr.sroa.177.sroa.7.0.extract.trunc4148, i8* %_buffer14
  %savedstack167 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %19), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer14, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %424, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %19, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %19, i8 -1, i64 1, i1 false), !dbg !406
  %425 = add i64 %424, 1
  %call.i166 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer14, i8* nonnull %19, i64 %425, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %19), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack167), !dbg !408
  %add2809 = add nuw nsw i32 %div2782, 2
  %idx.ext2810 = zext i32 %add2809 to i64
  %426 = add i64 0, %idx.ext2810
  store i8 %hdr.sroa.177.sroa.8.0.extract.trunc4150, i8* %_buffer59
  %savedstack169 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %18), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer59, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %426, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %18, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %18, i8 -1, i64 1, i1 false), !dbg !406
  %427 = add i64 %426, 1
  %call.i168 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer59, i8* nonnull %18, i64 %427, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %18), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack169), !dbg !408
  %add2822 = add nuw nsw i32 %div2782, 3
  %idx.ext2823 = zext i32 %add2822 to i64
  %428 = add i64 0, %idx.ext2823
  store i8 %hdr.sroa.177.sroa.9.0.extract.trunc4152, i8* %_buffer70
  %savedstack171 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %17), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer70, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %428, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %17, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %17, i8 -1, i64 1, i1 false), !dbg !406
  %429 = add i64 %428, 1
  %call.i170 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer70, i8* nonnull %17, i64 %429, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %17), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack171), !dbg !408
  %add2827 = add nsw i32 %ebpf_packetOffsetInBits.5, 96
  %shl2833 = shl i8 %hdr.sroa.184.043094448, 4
  %div2835 = lshr i32 %add2827, 3
  %idx.ext2837 = zext i32 %div2835 to i64
  %430 = add i64 0, %idx.ext2837
  store i8 %shl2833, i8* %_buffer164
  %savedstack173 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %16), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer164, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %430, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %16, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %16, i8 -1, i64 1, i1 false), !dbg !406
  %431 = add i64 %430, 1
  %call.i172 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer164, i8* nonnull %16, i64 %431, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %16), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack173), !dbg !408
  %add2841 = add nsw i32 %ebpf_packetOffsetInBits.5, 100
  %div2847 = lshr i32 %add2841, 3
  %idx.ext2848 = zext i32 %div2847 to i64
  %432 = add i64 0, %idx.ext2848
  %433 = add i64 %432, 1
  %434 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer16, i64 %433, i64 1)
  %435 = load i8, i8* %_buffer16
  %436 = and i8 %435, -16
  %437 = lshr i8 %hdr.sroa.186.043074449, 2
  %438 = and i8 %437, 15
  %or28603998 = or i8 %436, %438
  store i8 %or28603998, i8* %_buffer15
  %savedstack175 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %15), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer15, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %432, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %15, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %15, i8 -1, i64 1, i1 false), !dbg !406
  %439 = add i64 %432, 1
  %call.i174 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer15, i8* nonnull %15, i64 %439, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %15), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack175), !dbg !408
  %add2868 = add nsw i32 %ebpf_packetOffsetInBits.5, 106
  %div2874 = lshr i32 %add2868, 3
  %idx.ext2875 = zext i32 %div2874 to i64
  %440 = add i64 0, %idx.ext2875
  %441 = add i64 %440, 1
  %442 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %_buffer148, i64 %441, i64 1)
  %443 = load i8, i8* %_buffer148
  %444 = and i8 %443, -4
  %445 = lshr i8 %hdr.sroa.188.043054450, 4
  %446 = and i8 %445, 3
  %or28873999 = or i8 %444, %446
  store i8 %or28873999, i8* %_buffer87
  %savedstack177 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %14), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer87, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %440, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %14, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %14, i8 -1, i64 1, i1 false), !dbg !406
  %447 = add i64 %440, 1
  %call.i176 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer87, i8* nonnull %14, i64 %447, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %14), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack177), !dbg !408
  %add2895 = add nsw i32 %ebpf_packetOffsetInBits.5, 112
  %hdr.sroa.1904102.sroa.7.0.insert.ext = zext i8 %hdr.sroa.1904102.sroa.7.042734466 to i16
  %hdr.sroa.1904102.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.1904102.sroa.7.0.insert.ext, 8
  %hdr.sroa.1904102.sroa.0.0.insert.ext = zext i8 %hdr.sroa.1904102.sroa.0.042754465 to i16
  %hdr.sroa.1904102.sroa.0.0.insert.insert = or i16 %hdr.sroa.1904102.sroa.7.0.insert.shift, %hdr.sroa.1904102.sroa.0.0.insert.ext
  %rev4002 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.1904102.sroa.0.0.insert.insert)
  %hdr.sroa.1904102.sroa.0.0.extract.trunc4143 = trunc i16 %rev4002 to i8
  %hdr.sroa.1904102.sroa.7.0.extract.shift4144 = lshr i16 %rev4002, 8
  %hdr.sroa.1904102.sroa.7.0.extract.trunc4145 = trunc i16 %hdr.sroa.1904102.sroa.7.0.extract.shift4144 to i8
  %div2928 = lshr i32 %add2895, 3
  %idx.ext2930 = zext i32 %div2928 to i64
  %448 = add i64 0, %idx.ext2930
  store i8 %hdr.sroa.1904102.sroa.0.0.extract.trunc4143, i8* %_buffer17
  %savedstack179 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %13), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer17, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %448, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %13, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %13, i8 -1, i64 1, i1 false), !dbg !406
  %449 = add i64 %448, 1
  %call.i178 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer17, i8* nonnull %13, i64 %449, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %13), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack179), !dbg !408
  %add2942 = add nuw nsw i32 %div2928, 1
  %idx.ext2943 = zext i32 %add2942 to i64
  %450 = add i64 0, %idx.ext2943
  store i8 %hdr.sroa.1904102.sroa.7.0.extract.trunc4145, i8* %_buffer56
  %savedstack181 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %12), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer56, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %450, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %12, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %12, i8 -1, i64 1, i1 false), !dbg !406
  %451 = add i64 %450, 1
  %call.i180 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer56, i8* nonnull %12, i64 %451, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %12), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack181), !dbg !408
  %add2947 = add nsw i32 %ebpf_packetOffsetInBits.5, 128
  %hdr.sroa.195.sroa.7.0.insert.ext = zext i8 %hdr.sroa.195.sroa.7.042774464 to i16
  %hdr.sroa.195.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.195.sroa.7.0.insert.ext, 8
  %hdr.sroa.195.sroa.0.0.insert.ext = zext i8 %hdr.sroa.195.sroa.0.042794463 to i16
  %hdr.sroa.195.sroa.0.0.insert.insert = or i16 %hdr.sroa.195.sroa.7.0.insert.shift, %hdr.sroa.195.sroa.0.0.insert.ext
  %rev4001 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.195.sroa.0.0.insert.insert)
  %hdr.sroa.195.sroa.0.0.extract.trunc = trunc i16 %rev4001 to i8
  %hdr.sroa.195.sroa.7.0.extract.shift = lshr i16 %rev4001, 8
  %hdr.sroa.195.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.195.sroa.7.0.extract.shift to i8
  %div2980 = lshr i32 %add2947, 3
  %idx.ext2982 = zext i32 %div2980 to i64
  %452 = add i64 0, %idx.ext2982
  store i8 %hdr.sroa.195.sroa.0.0.extract.trunc, i8* %_buffer65
  %savedstack183 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %11), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer65, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %452, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %11, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %11, i8 -1, i64 1, i1 false), !dbg !406
  %453 = add i64 %452, 1
  %call.i182 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer65, i8* nonnull %11, i64 %453, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %11), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack183), !dbg !408
  %add2994 = add nuw nsw i32 %div2980, 1
  %idx.ext2995 = zext i32 %add2994 to i64
  %454 = add i64 0, %idx.ext2995
  store i8 %hdr.sroa.195.sroa.7.0.extract.trunc, i8* %_buffer165
  %savedstack185 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %10), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer165, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %454, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %10, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %10, i8 -1, i64 1, i1 false), !dbg !406
  %455 = add i64 %454, 1
  %call.i184 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer165, i8* nonnull %10, i64 %455, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %10), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack185), !dbg !408
  %add2999 = add nsw i32 %ebpf_packetOffsetInBits.5, 144
  %hdr.sroa.200.sroa.7.0.insert.ext = zext i8 %hdr.sroa.200.sroa.7.042814462 to i16
  %hdr.sroa.200.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.200.sroa.7.0.insert.ext, 8
  %hdr.sroa.200.sroa.0.0.insert.ext = zext i8 %hdr.sroa.200.sroa.0.042834461 to i16
  %hdr.sroa.200.sroa.0.0.insert.insert = or i16 %hdr.sroa.200.sroa.7.0.insert.shift, %hdr.sroa.200.sroa.0.0.insert.ext
  %rev4000 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.200.sroa.0.0.insert.insert)
  %hdr.sroa.200.sroa.0.0.extract.trunc = trunc i16 %rev4000 to i8
  %hdr.sroa.200.sroa.7.0.extract.shift = lshr i16 %rev4000, 8
  %hdr.sroa.200.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.200.sroa.7.0.extract.shift to i8
  %div3032 = lshr i32 %add2999, 3
  %idx.ext3034 = zext i32 %div3032 to i64
  %456 = add i64 0, %idx.ext3034
  store i8 %hdr.sroa.200.sroa.0.0.extract.trunc, i8* %_buffer86
  %savedstack187 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %9), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer86, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %456, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %9, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %9, i8 -1, i64 1, i1 false), !dbg !406
  %457 = add i64 %456, 1
  %call.i186 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer86, i8* nonnull %9, i64 %457, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %9), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack187), !dbg !408
  %add3046 = add nuw nsw i32 %div3032, 1
  %idx.ext3047 = zext i32 %add3046 to i64
  %458 = add i64 0, %idx.ext3047
  store i8 %hdr.sroa.200.sroa.7.0.extract.trunc, i8* %_buffer145
  %savedstack189 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %8), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer145, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %458, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %8, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %8, i8 -1, i64 1, i1 false), !dbg !406
  %459 = add i64 %458, 1
  %call.i188 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer145, i8* nonnull %8, i64 %459, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %8), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack189), !dbg !408
  br label %if.end3052

if.end3052:                                       ; preds = %if.end2559, %if.end2547
  %ebpf_packetOffsetInBits.6 = phi i32 [ %add2552, %if.end2559 ], [ %ebpf_packetOffsetInBits.5, %if.end2547 ]
  br i1 %tobool1245, label %cleanup3277, label %if.then3056

if.then3056:                                      ; preds = %if.end3052
  %add3057 = add nsw i32 %ebpf_packetOffsetInBits.6, 64
  %div3058 = lshr i32 %add3057, 3
  %idx.ext3059 = zext i32 %div3058 to i64
  %460 = add i64 0, %idx.ext3059
  %461 = icmp ugt i64 %460, %310
  br i1 %461, label %cleanup3277, label %if.end3064

if.end3064:                                       ; preds = %if.then3056
  %hdr.sroa.2084110.sroa.7.0.insert.ext = zext i8 %hdr.sroa.2084110.sroa.7.042854460 to i16
  %hdr.sroa.2084110.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.2084110.sroa.7.0.insert.ext, 8
  %hdr.sroa.2084110.sroa.0.0.insert.ext = zext i8 %hdr.sroa.2084110.sroa.0.042874459 to i16
  %hdr.sroa.2084110.sroa.0.0.insert.insert = or i16 %hdr.sroa.2084110.sroa.7.0.insert.shift, %hdr.sroa.2084110.sroa.0.0.insert.ext
  %rev3997 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.2084110.sroa.0.0.insert.insert)
  %hdr.sroa.2084110.sroa.0.0.extract.trunc4134 = trunc i16 %rev3997 to i8
  %hdr.sroa.2084110.sroa.7.0.extract.shift4135 = lshr i16 %rev3997, 8
  %hdr.sroa.2084110.sroa.7.0.extract.trunc4136 = trunc i16 %hdr.sroa.2084110.sroa.7.0.extract.shift4135 to i8
  %div3097 = lshr i32 %ebpf_packetOffsetInBits.6, 3
  %idx.ext3099 = zext i32 %div3097 to i64
  %462 = add i64 0, %idx.ext3099
  store i8 %hdr.sroa.2084110.sroa.0.0.extract.trunc4134, i8* %_buffer99
  %savedstack191 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %7), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer99, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %462, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %7, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %7, i8 -1, i64 1, i1 false), !dbg !406
  %463 = add i64 %462, 1
  %call.i190 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer99, i8* nonnull %7, i64 %463, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %7), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack191), !dbg !408
  %add3111 = add nuw nsw i32 %div3097, 1
  %idx.ext3112 = zext i32 %add3111 to i64
  %464 = add i64 0, %idx.ext3112
  store i8 %hdr.sroa.2084110.sroa.7.0.extract.trunc4136, i8* %_buffer96
  %savedstack193 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %6), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer96, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %464, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %6, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %6, i8 -1, i64 1, i1 false), !dbg !406
  %465 = add i64 %464, 1
  %call.i192 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer96, i8* nonnull %6, i64 %465, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %6), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack193), !dbg !408
  %add3116 = add nsw i32 %ebpf_packetOffsetInBits.6, 16
  %hdr.sroa.213.sroa.7.0.insert.ext = zext i8 %hdr.sroa.213.sroa.7.042894458 to i16
  %hdr.sroa.213.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.213.sroa.7.0.insert.ext, 8
  %hdr.sroa.213.sroa.0.0.insert.ext = zext i8 %hdr.sroa.213.sroa.0.042914457 to i16
  %hdr.sroa.213.sroa.0.0.insert.insert = or i16 %hdr.sroa.213.sroa.7.0.insert.shift, %hdr.sroa.213.sroa.0.0.insert.ext
  %rev3996 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.213.sroa.0.0.insert.insert)
  %hdr.sroa.213.sroa.0.0.extract.trunc = trunc i16 %rev3996 to i8
  %hdr.sroa.213.sroa.7.0.extract.shift = lshr i16 %rev3996, 8
  %hdr.sroa.213.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.213.sroa.7.0.extract.shift to i8
  %div3149 = lshr i32 %add3116, 3
  %idx.ext3151 = zext i32 %div3149 to i64
  %466 = add i64 0, %idx.ext3151
  store i8 %hdr.sroa.213.sroa.0.0.extract.trunc, i8* %_buffer94
  %savedstack195 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %5), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer94, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %466, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %5, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %5, i8 -1, i64 1, i1 false), !dbg !406
  %467 = add i64 %466, 1
  %call.i194 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer94, i8* nonnull %5, i64 %467, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %5), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack195), !dbg !408
  %add3163 = add nuw nsw i32 %div3149, 1
  %idx.ext3164 = zext i32 %add3163 to i64
  %468 = add i64 0, %idx.ext3164
  store i8 %hdr.sroa.213.sroa.7.0.extract.trunc, i8* %_buffer157
  %savedstack197 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %4), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer157, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %468, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %4, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %4, i8 -1, i64 1, i1 false), !dbg !406
  %469 = add i64 %468, 1
  %call.i196 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer157, i8* nonnull %4, i64 %469, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %4), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack197), !dbg !408
  %add3168 = add nsw i32 %ebpf_packetOffsetInBits.6, 32
  %hdr.sroa.218.sroa.7.0.insert.ext = zext i8 %hdr.sroa.218.sroa.7.042934456 to i16
  %hdr.sroa.218.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.218.sroa.7.0.insert.ext, 8
  %hdr.sroa.218.sroa.0.0.insert.ext = zext i8 %hdr.sroa.218.sroa.0.042954455 to i16
  %hdr.sroa.218.sroa.0.0.insert.insert = or i16 %hdr.sroa.218.sroa.7.0.insert.shift, %hdr.sroa.218.sroa.0.0.insert.ext
  %rev3995 = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.218.sroa.0.0.insert.insert)
  %hdr.sroa.218.sroa.0.0.extract.trunc = trunc i16 %rev3995 to i8
  %hdr.sroa.218.sroa.7.0.extract.shift = lshr i16 %rev3995, 8
  %hdr.sroa.218.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.218.sroa.7.0.extract.shift to i8
  %div3201 = lshr i32 %add3168, 3
  %idx.ext3203 = zext i32 %div3201 to i64
  %470 = add i64 0, %idx.ext3203
  store i8 %hdr.sroa.218.sroa.0.0.extract.trunc, i8* %_buffer138
  %savedstack199 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %3), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer138, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %470, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %3, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %3, i8 -1, i64 1, i1 false), !dbg !406
  %471 = add i64 %470, 1
  %call.i198 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer138, i8* nonnull %3, i64 %471, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %3), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack199), !dbg !408
  %add3215 = add nuw nsw i32 %div3201, 1
  %idx.ext3216 = zext i32 %add3215 to i64
  %472 = add i64 0, %idx.ext3216
  store i8 %hdr.sroa.218.sroa.7.0.extract.trunc, i8* %_buffer130
  %savedstack201 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %2), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer130, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %472, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %2, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %2, i8 -1, i64 1, i1 false), !dbg !406
  %473 = add i64 %472, 1
  %call.i200 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer130, i8* nonnull %2, i64 %473, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %2), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack201), !dbg !408
  %add3220 = add nsw i32 %ebpf_packetOffsetInBits.6, 48
  %hdr.sroa.223.sroa.7.0.insert.ext = zext i8 %hdr.sroa.223.sroa.7.042974454 to i16
  %hdr.sroa.223.sroa.7.0.insert.shift = shl nuw i16 %hdr.sroa.223.sroa.7.0.insert.ext, 8
  %hdr.sroa.223.sroa.0.0.insert.ext = zext i8 %hdr.sroa.223.sroa.0.042994453 to i16
  %hdr.sroa.223.sroa.0.0.insert.insert = or i16 %hdr.sroa.223.sroa.7.0.insert.shift, %hdr.sroa.223.sroa.0.0.insert.ext
  %rev = tail call i16 @llvm.bswap.i16(i16 %hdr.sroa.223.sroa.0.0.insert.insert)
  %hdr.sroa.223.sroa.0.0.extract.trunc = trunc i16 %rev to i8
  %hdr.sroa.223.sroa.7.0.extract.shift = lshr i16 %rev, 8
  %hdr.sroa.223.sroa.7.0.extract.trunc = trunc i16 %hdr.sroa.223.sroa.7.0.extract.shift to i8
  %div3253 = lshr i32 %add3220, 3
  %idx.ext3255 = zext i32 %div3253 to i64
  %474 = add i64 0, %idx.ext3255
  store i8 %hdr.sroa.223.sroa.0.0.extract.trunc, i8* %_buffer61
  %savedstack203 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %1), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer61, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %474, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %1, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %1, i8 -1, i64 1, i1 false), !dbg !406
  %475 = add i64 %474, 1
  %call.i202 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer61, i8* nonnull %1, i64 %475, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %1), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack203), !dbg !408
  %add3267 = add nuw nsw i32 %div3253, 1
  %idx.ext3268 = zext i32 %add3267 to i64
  %476 = add i64 0, %idx.ext3268
  store i8 %hdr.sroa.223.sroa.7.0.extract.trunc, i8* %_buffer64
  %savedstack205 = call i8* @llvm.stacksave(), !dbg !397
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %0), !dbg !397
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !391, metadata !DIExpression()), !dbg !397
  call void @llvm.dbg.value(metadata i8* %_buffer64, metadata !392, metadata !DIExpression()), !dbg !398
  call void @llvm.dbg.value(metadata i64 %476, metadata !393, metadata !DIExpression()), !dbg !399
  call void @llvm.dbg.value(metadata i64 1, metadata !394, metadata !DIExpression()), !dbg !400
  call void @llvm.dbg.value(metadata i64 1, metadata !395, metadata !DIExpression()), !dbg !403
  call void @llvm.dbg.value(metadata i8* %0, metadata !396, metadata !DIExpression()), !dbg !405
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %0, i8 -1, i64 1, i1 false), !dbg !406
  %477 = add i64 %476, 1
  %call.i204 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %_buffer64, i8* nonnull %0, i64 %477, i64 1), !dbg !407
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %0), !dbg !408
  call void @llvm.stackrestore(i8* %savedstack205), !dbg !408
  br label %cleanup3277

cleanup32774040:                                  ; preds = %if.end
  unreachable

cleanup32774041:                                  ; preds = %if.end247
  unreachable

cleanup3277:                                      ; preds = %if.end3064, %if.then3056, %if.end3052, %if.then2551, %if.then1946, %if.then1478, %if.then1258, %if.end1118, %parse_udp, %parse_tcp, %if.end247, %parse_ipv6, %if.end, %parse_ipv4, %entry
  %retval.1 = phi i32 [ 0, %entry ], [ 0, %parse_udp ], [ 0, %parse_tcp ], [ 0, %parse_ipv6 ], [ 0, %parse_ipv4 ], [ 0, %if.end1118 ], [ 0, %if.end247 ], [ 0, %if.end ], [ %xout.sroa.0.14500, %if.end3052 ], [ %xout.sroa.0.14500, %if.end3064 ], [ 0, %if.then1258 ], [ 0, %if.then1478 ], [ 0, %if.then1946 ], [ 0, %if.then2551 ], [ 0, %if.then3056 ]
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !376, metadata !DIExpression()), !dbg !378
  call void @llvm.dbg.value(metadata i32 %retval.1, metadata !377, metadata !DIExpression()), !dbg !379
  %478 = alloca i8, !dbg !384
  store i8 -1, i8* %478, !dbg !384
  %479 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %478, i64 0, i64 1), !dbg !384
  ret i32 0
}

attributes #0 = { nounwind readnone speculatable }
attributes #1 = { alwaysinline uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { argmemonly nounwind }
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
