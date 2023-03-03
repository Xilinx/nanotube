;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "llvm-link"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque

@__const._Z12complex_testP16nanotube_contextP15nanotube_packet.d = private unnamed_addr constant [11 x i8] c"\00\01\02\03\04\05\06\07\08\09\0A", align 1

; Function Attrs: uwtable
define dso_local void @_Z12complex_testP16nanotube_contextP15nanotube_packet(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) local_unnamed_addr #0 {
entry:
  %write_mask_off12 = alloca i8, !dbg !313
  call void @llvm.memset.p0i8.i64(i8* %write_mask_off12, i8 0, i64 1, i1 false), !dbg !313
  %nanotube_packet_write_masked_buf_off11 = alloca i8, i32 2, !dbg !313
  %write_mask_off1 = alloca i8, !dbg !313
  call void @llvm.memset.p0i8.i64(i8* %write_mask_off1, i8 0, i64 1, i1 false), !dbg !313
  %nanotube_packet_write_masked_buf_off1 = alloca i8, i32 3, !dbg !313
  %nanotube_packet_read_buf_off1 = alloca i8, !dbg !313
  %0 = alloca i8, i64 1, align 16, !dbg !313
  %1 = alloca i8, i64 1, align 16, !dbg !313
  %2 = alloca i8, i64 1, align 16, !dbg !313
  %3 = alloca i8, i64 1, align 16, !dbg !313
  %4 = alloca i8, i64 1, align 16, !dbg !313
  %5 = alloca i8, i64 1, align 16, !dbg !313
  %6 = alloca i8, i64 1, align 16, !dbg !313
  %7 = alloca i8, i64 1, align 16, !dbg !313
  %8 = alloca i8, i64 1, align 16, !dbg !313
  %9 = alloca i8, i64 1, align 16, !dbg !313
  %pos = alloca i8, align 1
  %d = alloca [11 x i8], align 1
  %tmp = alloca i8, align 1
  %call = tail call i64 @nanotube_packet_bounded_length(%struct.nanotube_packet* %packet, i64 19)
  %cmp = icmp ult i64 %call, 19
  br i1 %cmp, label %return, label %if.end

if.end:                                           ; preds = %entry
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %pos) #7
  %call1 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %pos, i64 0, i64 1)
  %10 = load i8, i8* %pos, align 1, !tbaa !329
  %conv = zext i8 %10 to i32
  %and = and i32 %conv, 1
  %tobool = icmp eq i32 %and, 0
  %and3 = and i32 %conv, 2
  %tobool4 = icmp ne i32 %and3, 0
  %and7 = and i32 %conv, 4
  %tobool8 = icmp eq i32 %and7, 0
  %and11 = and i32 %conv, 8
  %tobool12 = icmp eq i32 %and11, 0
  %and15 = and i8 %10, 15
  %11 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 11, i8* nonnull %11) #7
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* nonnull align 1 %11, i8* align 1 getelementptr inbounds ([11 x i8], [11 x i8]* @__const._Z12complex_testP16nanotube_contextP15nanotube_packet.d, i64 0, i64 0), i64 11, i1 false)
  br i1 %tobool, label %if.else31, label %if.then18

if.then18:                                        ; preds = %if.end
  br i1 %tobool4, label %if.then20, label %if.else

if.then20:                                        ; preds = %if.then18
  %arrayidx = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 1
  %savedstack = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %9), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 1, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %9, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %9, i8 -1, i64 1, i1 false), !dbg !338
  call void @nanotube_merge_data_mask(i8* %nanotube_packet_write_masked_buf_off1, i8* %write_mask_off1, i8* %arrayidx, i8* %9, i64 0, i64 1), !dbg !339
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %9), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack), !dbg !340
  %arrayidx22 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 2
  %savedstack2 = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %8), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx22, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 2, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %8, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %8, i8 -1, i64 1, i1 false), !dbg !338
  call void @nanotube_merge_data_mask(i8* %nanotube_packet_write_masked_buf_off1, i8* %write_mask_off1, i8* %arrayidx22, i8* %8, i64 1, i64 1), !dbg !339
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %8), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack2), !dbg !340
  br label %if.end28

if.else:                                          ; preds = %if.then18
  %arrayidx24 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 3
  %savedstack4 = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %7), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx24, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 2, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %7, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %7, i8 -1, i64 1, i1 false), !dbg !338
  call void @nanotube_merge_data_mask(i8* %nanotube_packet_write_masked_buf_off1, i8* %write_mask_off1, i8* %arrayidx24, i8* %7, i64 1, i64 1), !dbg !339
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %7), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack4), !dbg !340
  %arrayidx26 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 4
  %savedstack6 = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %6), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx26, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 1, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %6, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %6, i8 -1, i64 1, i1 false), !dbg !338
  call void @nanotube_merge_data_mask(i8* %nanotube_packet_write_masked_buf_off1, i8* %write_mask_off1, i8* %arrayidx26, i8* %6, i64 0, i64 1), !dbg !339
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %6), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack6), !dbg !340
  br label %if.end28

if.end28:                                         ; preds = %if.else, %if.then20
  %arrayidx29 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 5
  %savedstack8 = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %5), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx29, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 3, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %5, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %5, i8 -1, i64 1, i1 false), !dbg !338
  call void @nanotube_merge_data_mask(i8* %nanotube_packet_write_masked_buf_off1, i8* %write_mask_off1, i8* %arrayidx29, i8* %5, i64 2, i64 1), !dbg !339
  %12 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %nanotube_packet_write_masked_buf_off1, i8* %write_mask_off1, i64 1, i64 3), !dbg !340
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %5), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack8), !dbg !340
  br label %if.end76

if.else31:                                        ; preds = %if.end
  br i1 %tobool4, label %if.then33, label %if.else43

if.then33:                                        ; preds = %if.else31
  %arrayidx34 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 6
  %narrow86 = add nuw nsw i8 %and15, 1
  %add = zext i8 %narrow86 to i64
  %savedstack10 = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %4), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx34, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 %add, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %4, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %4, i8 -1, i64 1, i1 false), !dbg !338
  call void @nanotube_merge_data_mask(i8* %nanotube_packet_write_masked_buf_off11, i8* %write_mask_off12, i8* %arrayidx34, i8* %4, i64 0, i64 1), !dbg !339
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %4), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack10), !dbg !340
  %arrayidx38 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 7
  %13 = add i8 %and15, 0
  %conv39 = zext i8 %13 to i64
  %add40 = add nuw nsw i64 %conv39, 2
  %savedstack12 = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %3), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx38, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 %add40, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %3, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %3, i8 -1, i64 1, i1 false), !dbg !338
  call void @nanotube_merge_data_mask(i8* %nanotube_packet_write_masked_buf_off11, i8* %write_mask_off12, i8* %arrayidx38, i8* %3, i64 1, i64 1), !dbg !339
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %3), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack12), !dbg !340
  br label %if.end54

if.else43:                                        ; preds = %if.else31
  %arrayidx44 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 8
  %narrow = add nuw nsw i8 %and15, 2
  %add46 = zext i8 %narrow to i64
  %savedstack14 = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %2), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx44, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 %add46, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %2, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %2, i8 -1, i64 1, i1 false), !dbg !338
  call void @nanotube_merge_data_mask(i8* %nanotube_packet_write_masked_buf_off11, i8* %write_mask_off12, i8* %arrayidx44, i8* %2, i64 1, i64 1), !dbg !339
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %2), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack14), !dbg !340
  %arrayidx49 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 9
  %14 = add i8 %and15, 0
  %conv50 = zext i8 %14 to i64
  %add51 = add nuw nsw i64 %conv50, 1
  %savedstack16 = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %1), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx49, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 %add51, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %1, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %1, i8 -1, i64 1, i1 false), !dbg !338
  call void @nanotube_merge_data_mask(i8* %nanotube_packet_write_masked_buf_off11, i8* %write_mask_off12, i8* %arrayidx49, i8* %1, i64 0, i64 1), !dbg !339
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %1), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack16), !dbg !340
  br label %if.end54

if.end54:                                         ; preds = %if.else43, %if.then33
  %and15.scaled = zext i8 %and15 to i64
  %15 = add i64 %and15.scaled, 1
  %16 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %nanotube_packet_write_masked_buf_off11, i8* %write_mask_off12, i64 %15, i64 2)
  br i1 %tobool8, label %if.end61, label %if.then56

if.then56:                                        ; preds = %if.end54
  %arrayidx57 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 10
  %17 = load i8, i8* %arrayidx57, align 1, !tbaa !329
  %add59 = add i8 %17, 37
  store i8 %add59, i8* %arrayidx57, align 1, !tbaa !329
  br label %if.end61

if.end61:                                         ; preds = %if.then56, %if.end54
  br i1 %tobool12, label %if.end70, label %if.then63

if.then63:                                        ; preds = %if.end61
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %tmp) #7
  %18 = getelementptr inbounds i8, i8* %nanotube_packet_read_buf_off1, i32 0
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 1 %tmp, i8* align 1 %18, i64 1, i1 false)
  %19 = load i8, i8* %tmp, align 1, !tbaa !329
  %arrayidx66 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 10
  %20 = load i8, i8* %arrayidx66, align 1, !tbaa !329
  %add68 = add i8 %20, %19
  store i8 %add68, i8* %arrayidx66, align 1, !tbaa !329
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %tmp) #7
  %21 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %nanotube_packet_read_buf_off1, i64 1, i64 1)
  br label %if.end70

if.end70:                                         ; preds = %if.then63, %if.end61
  %arrayidx71 = getelementptr inbounds [11 x i8], [11 x i8]* %d, i64 0, i64 10
  %22 = add i8 %and15, 0
  %conv72 = zext i8 %22 to i64
  %add73 = add nuw nsw i64 %conv72, 3
  %savedstack18 = call i8* @llvm.stacksave(), !dbg !332
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %0), !dbg !332
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %arrayidx71, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 %add73, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 1, metadata !326, metadata !DIExpression()), !dbg !335
  call void @llvm.dbg.value(metadata i64 1, metadata !327, metadata !DIExpression()), !dbg !336
  call void @llvm.dbg.value(metadata i8* %0, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %0, i8 -1, i64 1, i1 false), !dbg !338
  %call.i17 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %arrayidx71, i8* nonnull %0, i64 %add73, i64 1), !dbg !339
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %0), !dbg !340
  call void @llvm.stackrestore(i8* %savedstack18), !dbg !340
  br label %if.end76

if.end76:                                         ; preds = %if.end70, %if.end28
  call void @llvm.lifetime.end.p0i8(i64 11, i8* nonnull %11) #7
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %pos) #7
  br label %return

return:                                           ; preds = %if.end76, %entry
  ret void
}

declare dso_local i64 @nanotube_packet_bounded_length(%struct.nanotube_packet*, i64) local_unnamed_addr #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #2

; Function Attrs: inaccessiblemem_or_argmemonly
declare dso_local i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64) local_unnamed_addr #3

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #2

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #2

; Function Attrs: alwaysinline uwtable
define dso_local i32 @map_op_adapter(%struct.nanotube_context* %ctx, i16 zeroext %map_id, i32 %type, i8* %key, i64 %key_length, i8* %data_in, i8* %data_out, i8* %mask, i64 %offset, i64 %data_length) local_unnamed_addr #4 !dbg !341 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_context* %ctx, metadata !355, metadata !DIExpression()), !dbg !366
  call void @llvm.dbg.value(metadata i16 %map_id, metadata !356, metadata !DIExpression()), !dbg !367
  call void @llvm.dbg.value(metadata i32 %type, metadata !357, metadata !DIExpression()), !dbg !368
  call void @llvm.dbg.value(metadata i8* %key, metadata !358, metadata !DIExpression()), !dbg !369
  call void @llvm.dbg.value(metadata i64 %key_length, metadata !359, metadata !DIExpression()), !dbg !370
  call void @llvm.dbg.value(metadata i8* %data_in, metadata !360, metadata !DIExpression()), !dbg !371
  call void @llvm.dbg.value(metadata i8* %data_out, metadata !361, metadata !DIExpression()), !dbg !372
  call void @llvm.dbg.value(metadata i8* %mask, metadata !362, metadata !DIExpression()), !dbg !373
  call void @llvm.dbg.value(metadata i64 %offset, metadata !363, metadata !DIExpression()), !dbg !374
  call void @llvm.dbg.value(metadata i64 %data_length, metadata !364, metadata !DIExpression()), !dbg !375
  %call = tail call i64 @nanotube_map_op(%struct.nanotube_context* %ctx, i16 zeroext %map_id, i32 %type, i8* %key, i64 %key_length, i8* %data_in, i8* %data_out, i8* %mask, i64 %offset, i64 %data_length), !dbg !376
  call void @llvm.dbg.value(metadata i64 %call, metadata !365, metadata !DIExpression()), !dbg !377
  %cmp = icmp eq i64 %call, %data_length, !dbg !378
  %. = select i1 %cmp, i32 0, i32 -22, !dbg !380
  ret i32 %., !dbg !381
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #5

declare dso_local i64 @nanotube_map_op(%struct.nanotube_context*, i16 zeroext, i32, i8*, i64, i8*, i8*, i8*, i64, i64) local_unnamed_addr #1

; Function Attrs: alwaysinline uwtable
define dso_local i32 @packet_adjust_head_adapter(%struct.nanotube_packet* %p, i32 %offset) local_unnamed_addr #4 !dbg !382 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %p, metadata !386, metadata !DIExpression()), !dbg !389
  call void @llvm.dbg.value(metadata i32 %offset, metadata !387, metadata !DIExpression()), !dbg !390
  %call = tail call i32 @nanotube_packet_resize(%struct.nanotube_packet* %p, i64 0, i32 %offset), !dbg !391
  call void @llvm.dbg.value(metadata i32 %call, metadata !388, metadata !DIExpression()), !dbg !392
  %tobool = icmp eq i32 %call, 0, !dbg !393
  %cond = sext i1 %tobool to i32, !dbg !393
  ret i32 %cond, !dbg !394
}

declare dso_local i32 @nanotube_packet_resize(%struct.nanotube_packet*, i64, i32) local_unnamed_addr #1

; Function Attrs: alwaysinline uwtable
define dso_local void @packet_handle_xdp_result(%struct.nanotube_packet* %packet, i32 %xdp_ret) local_unnamed_addr #4 !dbg !395 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !399, metadata !DIExpression()), !dbg !401
  call void @llvm.dbg.value(metadata i32 %xdp_ret, metadata !400, metadata !DIExpression()), !dbg !402
  %cond = icmp eq i32 %xdp_ret, 2, !dbg !403
  br i1 %cond, label %sw.bb, label %sw.default, !dbg !403

sw.bb:                                            ; preds = %entry
  tail call void @nanotube_packet_set_port(%struct.nanotube_packet* %packet, i8 zeroext 0), !dbg !404
  br label %sw.epilog, !dbg !406

sw.default:                                       ; preds = %entry
  tail call void @nanotube_packet_set_port(%struct.nanotube_packet* %packet, i8 zeroext -1), !dbg !407
  br label %sw.epilog, !dbg !408

sw.epilog:                                        ; preds = %sw.default, %sw.bb
  ret void, !dbg !409
}

declare dso_local void @nanotube_packet_set_port(%struct.nanotube_packet*, i8 zeroext) local_unnamed_addr #1

; Function Attrs: alwaysinline uwtable
define dso_local i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %data_in, i64 %offset, i64 %length) local_unnamed_addr #4 !dbg !314 {
entry:
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !323, metadata !DIExpression()), !dbg !332
  call void @llvm.dbg.value(metadata i8* %data_in, metadata !324, metadata !DIExpression()), !dbg !333
  call void @llvm.dbg.value(metadata i64 %offset, metadata !325, metadata !DIExpression()), !dbg !334
  call void @llvm.dbg.value(metadata i64 %length, metadata !326, metadata !DIExpression()), !dbg !335
  %add = add i64 %length, 7, !dbg !410
  %div = lshr i64 %add, 3, !dbg !411
  call void @llvm.dbg.value(metadata i64 %div, metadata !327, metadata !DIExpression()), !dbg !336
  %0 = alloca i8, i64 %div, align 16, !dbg !313
  call void @llvm.dbg.value(metadata i8* %0, metadata !328, metadata !DIExpression()), !dbg !337
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %0, i8 -1, i64 %div, i1 false), !dbg !338
  %call = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %data_in, i8* nonnull %0, i64 %offset, i64 %length), !dbg !339
  ret i64 %call, !dbg !340
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1) #2

; Function Attrs: inaccessiblemem_or_argmemonly
declare dso_local i64 @nanotube_packet_write_masked(%struct.nanotube_packet*, i8*, i8*, i64, i64) local_unnamed_addr #3

; Function Attrs: alwaysinline norecurse nounwind uwtable
define dso_local void @nanotube_merge_data_mask(i8* nocapture %inout_data, i8* nocapture %inout_mask, i8* nocapture readonly %in_data, i8* nocapture readonly %in_mask, i64 %offset, i64 %data_length) local_unnamed_addr #6 !dbg !412 {
entry:
  call void @llvm.dbg.value(metadata i8* %inout_data, metadata !416, metadata !DIExpression()), !dbg !435
  call void @llvm.dbg.value(metadata i8* %inout_mask, metadata !417, metadata !DIExpression()), !dbg !436
  call void @llvm.dbg.value(metadata i8* %in_data, metadata !418, metadata !DIExpression()), !dbg !437
  call void @llvm.dbg.value(metadata i8* %in_mask, metadata !419, metadata !DIExpression()), !dbg !438
  call void @llvm.dbg.value(metadata i64 %offset, metadata !420, metadata !DIExpression()), !dbg !439
  call void @llvm.dbg.value(metadata i64 %data_length, metadata !421, metadata !DIExpression()), !dbg !440
  call void @llvm.dbg.value(metadata i64 0, metadata !422, metadata !DIExpression()), !dbg !441
  %cmp25 = icmp eq i64 %data_length, 0, !dbg !442
  br i1 %cmp25, label %for.cond.cleanup, label %for.body, !dbg !443

for.cond.cleanup:                                 ; preds = %if.end, %entry
  ret void, !dbg !444

for.body:                                         ; preds = %if.end, %entry
  %i.026 = phi i64 [ %inc, %if.end ], [ 0, %entry ]
  call void @llvm.dbg.value(metadata i64 %i.026, metadata !422, metadata !DIExpression()), !dbg !441
  %div = lshr i64 %i.026, 3, !dbg !445
  call void @llvm.dbg.value(metadata i64 %div, metadata !424, metadata !DIExpression()), !dbg !446
  %0 = trunc i64 %i.026 to i32, !dbg !447
  %conv = and i32 %0, 7, !dbg !447
  %add = add i64 %i.026, %offset, !dbg !448
  call void @llvm.dbg.value(metadata i64 %add, metadata !428, metadata !DIExpression()), !dbg !449
  %arrayidx = getelementptr inbounds i8, i8* %in_mask, i64 %div, !dbg !450
  %1 = load i8, i8* %arrayidx, align 1, !dbg !450, !tbaa !329
  %conv1 = zext i8 %1 to i32, !dbg !450
  %shl = shl i32 1, %conv, !dbg !451
  %and = and i32 %shl, %conv1, !dbg !452
  %tobool = icmp eq i32 %and, 0, !dbg !450
  br i1 %tobool, label %if.end, label %if.then, !dbg !453

if.then:                                          ; preds = %for.body
  %arrayidx4 = getelementptr inbounds i8, i8* %in_data, i64 %i.026, !dbg !454
  %2 = load i8, i8* %arrayidx4, align 1, !dbg !454, !tbaa !329
  %arrayidx5 = getelementptr inbounds i8, i8* %inout_data, i64 %add, !dbg !455
  store i8 %2, i8* %arrayidx5, align 1, !dbg !456, !tbaa !329
  %div6 = lshr i64 %add, 3, !dbg !457
  call void @llvm.dbg.value(metadata i64 %div6, metadata !431, metadata !DIExpression()), !dbg !458
  %3 = trunc i64 %add to i32, !dbg !459
  %conv8 = and i32 %3, 7, !dbg !459
  call void @llvm.dbg.value(metadata i32 %conv8, metadata !434, metadata !DIExpression()), !dbg !460
  %shl10 = shl i32 1, %conv8, !dbg !461
  %arrayidx11 = getelementptr inbounds i8, i8* %inout_mask, i64 %div6, !dbg !462
  %4 = load i8, i8* %arrayidx11, align 1, !dbg !463, !tbaa !329
  %5 = trunc i32 %shl10 to i8, !dbg !463
  %conv13 = or i8 %4, %5, !dbg !463
  store i8 %conv13, i8* %arrayidx11, align 1, !dbg !463, !tbaa !329
  br label %if.end, !dbg !464

if.end:                                           ; preds = %if.then, %for.body
  %inc = add nuw i64 %i.026, 1, !dbg !465
  call void @llvm.dbg.value(metadata i64 %inc, metadata !422, metadata !DIExpression()), !dbg !441
  %exitcond = icmp eq i64 %inc, %data_length, !dbg !442
  br i1 %exitcond, label %for.cond.cleanup, label %for.body, !dbg !443, !llvm.loop !466
}

; Function Attrs: alwaysinline uwtable
define dso_local i64 @nanotube_map_read(%struct.nanotube_context* %ctx, i16 zeroext %id, i8* %key, i64 %key_length, i8* %data_out, i64 %offset, i64 %data_length) local_unnamed_addr #4 !dbg !468 {
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
define dso_local i64 @nanotube_map_write(%struct.nanotube_context* %ctx, i16 zeroext %id, i8* %key, i64 %key_length, i8* %data_in, i64 %offset, i64 %data_length) local_unnamed_addr #4 !dbg !488 {
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
declare i8* @llvm.stacksave() #7

; Function Attrs: nounwind
declare void @llvm.stackrestore(i8*) #7

attributes #0 = { uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { argmemonly nounwind }
attributes #3 = { inaccessiblemem_or_argmemonly "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { alwaysinline uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nounwind readnone speculatable }
attributes #6 = { alwaysinline norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #7 = { nounwind }

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
!313 = !DILocation(line: 23, column: 32, scope: !314)
!314 = distinct !DISubprogram(name: "nanotube_packet_write", scope: !28, file: !28, line: 19, type: !315, scopeLine: 21, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !27, retainedNodes: !322)
!315 = !DISubroutineType(types: !316)
!316 = !{!88, !317, !320, !88, !88}
!317 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !318, size: 64)
!318 = !DIDerivedType(tag: DW_TAG_typedef, name: "nanotube_packet_t", file: !8, line: 73, baseType: !319)
!319 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "nanotube_packet", file: !8, line: 73, flags: DIFlagFwdDecl, identifier: "_ZTS15nanotube_packet")
!320 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !321, size: 64)
!321 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !22)
!322 = !{!323, !324, !325, !326, !327, !328}
!323 = !DILocalVariable(name: "packet", arg: 1, scope: !314, file: !28, line: 19, type: !317)
!324 = !DILocalVariable(name: "data_in", arg: 2, scope: !314, file: !28, line: 20, type: !320)
!325 = !DILocalVariable(name: "offset", arg: 3, scope: !314, file: !28, line: 20, type: !88)
!326 = !DILocalVariable(name: "length", arg: 4, scope: !314, file: !28, line: 21, type: !88)
!327 = !DILocalVariable(name: "mask_size", scope: !314, file: !28, line: 22, type: !88)
!328 = !DILocalVariable(name: "all_one", scope: !314, file: !28, line: 23, type: !31)
!329 = !{!330, !330, i64 0}
!330 = !{!"omnipotent char", !331, i64 0}
!331 = !{!"Simple C++ TBAA"}
!332 = !DILocation(line: 19, column: 49, scope: !314)
!333 = !DILocation(line: 20, column: 45, scope: !314)
!334 = !DILocation(line: 20, column: 61, scope: !314)
!335 = !DILocation(line: 21, column: 37, scope: !314)
!336 = !DILocation(line: 22, column: 10, scope: !314)
!337 = !DILocation(line: 23, column: 12, scope: !314)
!338 = !DILocation(line: 24, column: 3, scope: !314)
!339 = !DILocation(line: 26, column: 10, scope: !314)
!340 = !DILocation(line: 26, column: 3, scope: !314)
!341 = distinct !DISubprogram(name: "map_op_adapter", scope: !5, file: !5, line: 27, type: !342, scopeLine: 31, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !4, retainedNodes: !354)
!342 = !DISubroutineType(types: !343)
!343 = !{!344, !347, !350, !7, !320, !88, !320, !31, !320, !88, !88}
!344 = !DIDerivedType(tag: DW_TAG_typedef, name: "int32_t", file: !345, line: 26, baseType: !346)
!345 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/stdint-intn.h", directory: "")
!346 = !DIDerivedType(tag: DW_TAG_typedef, name: "__int32_t", file: !25, line: 40, baseType: !39)
!347 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !348, size: 64)
!348 = !DIDerivedType(tag: DW_TAG_typedef, name: "nanotube_context_t", file: !8, line: 71, baseType: !349)
!349 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "nanotube_context", file: !8, line: 71, flags: DIFlagFwdDecl, identifier: "_ZTS16nanotube_context")
!350 = !DIDerivedType(tag: DW_TAG_typedef, name: "nanotube_map_id_t", file: !8, line: 68, baseType: !351)
!351 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint16_t", file: !23, line: 25, baseType: !352)
!352 = !DIDerivedType(tag: DW_TAG_typedef, name: "__uint16_t", file: !25, line: 39, baseType: !353)
!353 = !DIBasicType(name: "unsigned short", size: 16, encoding: DW_ATE_unsigned)
!354 = !{!355, !356, !357, !358, !359, !360, !361, !362, !363, !364, !365}
!355 = !DILocalVariable(name: "ctx", arg: 1, scope: !341, file: !5, line: 27, type: !347)
!356 = !DILocalVariable(name: "map_id", arg: 2, scope: !341, file: !5, line: 27, type: !350)
!357 = !DILocalVariable(name: "type", arg: 3, scope: !341, file: !5, line: 28, type: !7)
!358 = !DILocalVariable(name: "key", arg: 4, scope: !341, file: !5, line: 28, type: !320)
!359 = !DILocalVariable(name: "key_length", arg: 5, scope: !341, file: !5, line: 29, type: !88)
!360 = !DILocalVariable(name: "data_in", arg: 6, scope: !341, file: !5, line: 29, type: !320)
!361 = !DILocalVariable(name: "data_out", arg: 7, scope: !341, file: !5, line: 30, type: !31)
!362 = !DILocalVariable(name: "mask", arg: 8, scope: !341, file: !5, line: 30, type: !320)
!363 = !DILocalVariable(name: "offset", arg: 9, scope: !341, file: !5, line: 31, type: !88)
!364 = !DILocalVariable(name: "data_length", arg: 10, scope: !341, file: !5, line: 31, type: !88)
!365 = !DILocalVariable(name: "ret", scope: !341, file: !5, line: 33, type: !88)
!366 = !DILocation(line: 27, column: 44, scope: !341)
!367 = !DILocation(line: 27, column: 67, scope: !341)
!368 = !DILocation(line: 28, column: 42, scope: !341)
!369 = !DILocation(line: 28, column: 63, scope: !341)
!370 = !DILocation(line: 29, column: 31, scope: !341)
!371 = !DILocation(line: 29, column: 58, scope: !341)
!372 = !DILocation(line: 30, column: 33, scope: !341)
!373 = !DILocation(line: 30, column: 58, scope: !341)
!374 = !DILocation(line: 31, column: 31, scope: !341)
!375 = !DILocation(line: 31, column: 46, scope: !341)
!376 = !DILocation(line: 33, column: 16, scope: !341)
!377 = !DILocation(line: 33, column: 10, scope: !341)
!378 = !DILocation(line: 36, column: 11, scope: !379)
!379 = distinct !DILexicalBlock(scope: !341, file: !5, line: 36, column: 7)
!380 = !DILocation(line: 0, scope: !341)
!381 = !DILocation(line: 42, column: 1, scope: !341)
!382 = distinct !DISubprogram(name: "packet_adjust_head_adapter", scope: !5, file: !5, line: 47, type: !383, scopeLine: 47, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !4, retainedNodes: !385)
!383 = !DISubroutineType(types: !384)
!384 = !{!344, !317, !344}
!385 = !{!386, !387, !388}
!386 = !DILocalVariable(name: "p", arg: 1, scope: !382, file: !5, line: 47, type: !317)
!387 = !DILocalVariable(name: "offset", arg: 2, scope: !382, file: !5, line: 47, type: !344)
!388 = !DILocalVariable(name: "success", scope: !382, file: !5, line: 48, type: !39)
!389 = !DILocation(line: 47, column: 55, scope: !382)
!390 = !DILocation(line: 47, column: 66, scope: !382)
!391 = !DILocation(line: 48, column: 18, scope: !382)
!392 = !DILocation(line: 48, column: 8, scope: !382)
!393 = !DILocation(line: 50, column: 10, scope: !382)
!394 = !DILocation(line: 50, column: 3, scope: !382)
!395 = distinct !DISubprogram(name: "packet_handle_xdp_result", scope: !5, file: !5, line: 56, type: !396, scopeLine: 56, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !4, retainedNodes: !398)
!396 = !DISubroutineType(types: !397)
!397 = !{null, !317, !39}
!398 = !{!399, !400}
!399 = !DILocalVariable(name: "packet", arg: 1, scope: !395, file: !5, line: 56, type: !317)
!400 = !DILocalVariable(name: "xdp_ret", arg: 2, scope: !395, file: !5, line: 56, type: !39)
!401 = !DILocation(line: 56, column: 50, scope: !395)
!402 = !DILocation(line: 56, column: 62, scope: !395)
!403 = !DILocation(line: 57, column: 3, scope: !395)
!404 = !DILocation(line: 59, column: 7, scope: !405)
!405 = distinct !DILexicalBlock(scope: !395, file: !5, line: 57, column: 21)
!406 = !DILocation(line: 60, column: 7, scope: !405)
!407 = !DILocation(line: 64, column: 7, scope: !405)
!408 = !DILocation(line: 65, column: 3, scope: !405)
!409 = !DILocation(line: 66, column: 1, scope: !395)
!410 = !DILocation(line: 22, column: 30, scope: !314)
!411 = !DILocation(line: 22, column: 35, scope: !314)
!412 = distinct !DISubprogram(name: "nanotube_merge_data_mask", scope: !28, file: !28, line: 33, type: !413, scopeLine: 35, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !27, retainedNodes: !415)
!413 = !DISubroutineType(types: !414)
!414 = !{null, !31, !31, !320, !320, !88, !88}
!415 = !{!416, !417, !418, !419, !420, !421, !422, !424, !427, !428, !429, !431, !434}
!416 = !DILocalVariable(name: "inout_data", arg: 1, scope: !412, file: !28, line: 33, type: !31)
!417 = !DILocalVariable(name: "inout_mask", arg: 2, scope: !412, file: !28, line: 33, type: !31)
!418 = !DILocalVariable(name: "in_data", arg: 3, scope: !412, file: !28, line: 34, type: !320)
!419 = !DILocalVariable(name: "in_mask", arg: 4, scope: !412, file: !28, line: 34, type: !320)
!420 = !DILocalVariable(name: "offset", arg: 5, scope: !412, file: !28, line: 35, type: !88)
!421 = !DILocalVariable(name: "data_length", arg: 6, scope: !412, file: !28, line: 35, type: !88)
!422 = !DILocalVariable(name: "i", scope: !423, file: !28, line: 36, type: !88)
!423 = distinct !DILexicalBlock(scope: !412, file: !28, line: 36, column: 3)
!424 = !DILocalVariable(name: "maskidx", scope: !425, file: !28, line: 37, type: !88)
!425 = distinct !DILexicalBlock(scope: !426, file: !28, line: 36, column: 45)
!426 = distinct !DILexicalBlock(scope: !423, file: !28, line: 36, column: 3)
!427 = !DILocalVariable(name: "bitidx", scope: !425, file: !28, line: 38, type: !22)
!428 = !DILocalVariable(name: "out_pos", scope: !425, file: !28, line: 39, type: !88)
!429 = !DILocalVariable(name: "update", scope: !425, file: !28, line: 40, type: !430)
!430 = !DIBasicType(name: "bool", size: 8, encoding: DW_ATE_boolean)
!431 = !DILocalVariable(name: "out_maskidx", scope: !432, file: !28, line: 45, type: !88)
!432 = distinct !DILexicalBlock(scope: !433, file: !28, line: 42, column: 18)
!433 = distinct !DILexicalBlock(scope: !425, file: !28, line: 42, column: 9)
!434 = !DILocalVariable(name: "out_bitidx", scope: !432, file: !28, line: 46, type: !22)
!435 = !DILocation(line: 33, column: 35, scope: !412)
!436 = !DILocation(line: 33, column: 56, scope: !412)
!437 = !DILocation(line: 34, column: 41, scope: !412)
!438 = !DILocation(line: 34, column: 65, scope: !412)
!439 = !DILocation(line: 35, column: 33, scope: !412)
!440 = !DILocation(line: 35, column: 48, scope: !412)
!441 = !DILocation(line: 36, column: 15, scope: !423)
!442 = !DILocation(line: 36, column: 24, scope: !426)
!443 = !DILocation(line: 36, column: 3, scope: !423)
!444 = !DILocation(line: 50, column: 1, scope: !412)
!445 = !DILocation(line: 37, column: 24, scope: !425)
!446 = !DILocation(line: 37, column: 12, scope: !425)
!447 = !DILocation(line: 38, column: 22, scope: !425)
!448 = !DILocation(line: 39, column: 29, scope: !425)
!449 = !DILocation(line: 39, column: 12, scope: !425)
!450 = !DILocation(line: 40, column: 19, scope: !425)
!451 = !DILocation(line: 40, column: 41, scope: !425)
!452 = !DILocation(line: 40, column: 36, scope: !425)
!453 = !DILocation(line: 42, column: 9, scope: !425)
!454 = !DILocation(line: 44, column: 29, scope: !432)
!455 = !DILocation(line: 44, column: 7, scope: !432)
!456 = !DILocation(line: 44, column: 27, scope: !432)
!457 = !DILocation(line: 45, column: 36, scope: !432)
!458 = !DILocation(line: 45, column: 14, scope: !432)
!459 = !DILocation(line: 46, column: 28, scope: !432)
!460 = !DILocation(line: 46, column: 15, scope: !432)
!461 = !DILocation(line: 47, column: 37, scope: !432)
!462 = !DILocation(line: 47, column: 7, scope: !432)
!463 = !DILocation(line: 47, column: 31, scope: !432)
!464 = !DILocation(line: 48, column: 5, scope: !432)
!465 = !DILocation(line: 36, column: 40, scope: !426)
!466 = distinct !{!466, !443, !467}
!467 = !DILocation(line: 49, column: 3, scope: !423)
!468 = distinct !DISubprogram(name: "nanotube_map_read", scope: !279, file: !279, line: 19, type: !469, scopeLine: 22, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !278, retainedNodes: !471)
!469 = !DISubroutineType(types: !470)
!470 = !{!88, !347, !350, !320, !88, !31, !88, !88}
!471 = !{!472, !473, !474, !475, !476, !477, !478}
!472 = !DILocalVariable(name: "ctx", arg: 1, scope: !468, file: !279, line: 19, type: !347)
!473 = !DILocalVariable(name: "id", arg: 2, scope: !468, file: !279, line: 19, type: !350)
!474 = !DILocalVariable(name: "key", arg: 3, scope: !468, file: !279, line: 20, type: !320)
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
!490 = !{!88, !347, !350, !320, !88, !320, !88, !88}
!491 = !{!492, !493, !494, !495, !496, !497, !498, !499, !500}
!492 = !DILocalVariable(name: "ctx", arg: 1, scope: !488, file: !279, line: 31, type: !347)
!493 = !DILocalVariable(name: "id", arg: 2, scope: !488, file: !279, line: 31, type: !350)
!494 = !DILocalVariable(name: "key", arg: 3, scope: !488, file: !279, line: 32, type: !320)
!495 = !DILocalVariable(name: "key_length", arg: 4, scope: !488, file: !279, line: 32, type: !88)
!496 = !DILocalVariable(name: "data_in", arg: 5, scope: !488, file: !279, line: 33, type: !320)
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
