;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'testing/pass_tests/optreq/insert_at_moved.c'
source_filename = "testing/pass_tests/optreq/insert_at_moved.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque

@.str = private unnamed_addr constant [14 x i8] c"read(%d, %d)\0A\00", align 1
@write_count = internal unnamed_addr global i32 0, align 4
@.str.1 = private unnamed_addr constant [14 x i8] c"write(%d, %d)\00", align 1
@.str.2 = private unnamed_addr constant [8 x i8] c"%s %02x\00", align 1
@.str.3 = private unnamed_addr constant [3 x i8] c"\0A \00", align 1
@.str.4 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@.str.5 = private unnamed_addr constant [7 x i8] c"\0AMask:\00", align 1

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_add_plain_packet_kernel(i8* nocapture readnone %name, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* nocapture %kernel, i32 %bus_type, i32 %capsules) local_unnamed_addr #0 {
entry:
  %call = tail call i32 %kernel(%struct.nanotube_context* null, %struct.nanotube_packet* null) #4
  ret void
}

; Function Attrs: nounwind uwtable
define weak dso_local i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %data_out, i64 %offset, i64 %length) local_unnamed_addr #0 {
entry:
  %conv = trunc i64 %offset to i32
  %conv1 = trunc i64 %length to i32
  %call = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str, i64 0, i64 0), i32 %conv, i32 %conv1)
  %cmp14 = icmp eq i64 %length, 0
  br i1 %cmp14, label %for.end, label %for.body.lr.ph

for.body.lr.ph:                                   ; preds = %entry
  %0 = load i32, i32* @write_count, align 4, !tbaa !2
  %conv313 = zext i32 %0 to i64
  %min.iters.check = icmp ult i64 %length, 16
  br i1 %min.iters.check, label %for.body.preheader, label %vector.ph

vector.ph:                                        ; preds = %for.body.lr.ph
  %n.vec = and i64 %length, -16
  %ind.end = add i64 %n.vec, %offset
  %.splatinsert = insertelement <16 x i64> undef, i64 %offset, i32 0
  %.splat = shufflevector <16 x i64> %.splatinsert, <16 x i64> undef, <16 x i32> zeroinitializer
  %induction18 = add <16 x i64> %.splat, <i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14, i64 15>
  %broadcast.splatinsert19 = insertelement <16 x i64> undef, i64 %conv313, i32 0
  %broadcast.splat20 = shufflevector <16 x i64> %broadcast.splatinsert19, <16 x i64> undef, <16 x i32> zeroinitializer
  %1 = add i64 %n.vec, -16
  %2 = lshr exact i64 %1, 4
  %3 = add nuw nsw i64 %2, 1
  %xtraiter = and i64 %3, 1
  %4 = icmp eq i64 %1, 0
  br i1 %4, label %middle.block.unr-lcssa, label %vector.ph.new

vector.ph.new:                                    ; preds = %vector.ph
  %unroll_iter = sub nsw i64 %3, %xtraiter
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph.new
  %index = phi i64 [ 0, %vector.ph.new ], [ %index.next.1, %vector.body ]
  %vec.ind = phi <16 x i64> [ %induction18, %vector.ph.new ], [ %vec.ind.next.1, %vector.body ]
  %niter = phi i64 [ %unroll_iter, %vector.ph.new ], [ %niter.nsub.1, %vector.body ]
  %5 = lshr <16 x i64> %vec.ind, <i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8>
  %6 = xor <16 x i64> %5, %vec.ind
  %7 = add <16 x i64> %6, %broadcast.splat20
  %8 = trunc <16 x i64> %7 to <16 x i8>
  %9 = getelementptr inbounds i8, i8* %data_out, i64 %index
  %10 = bitcast i8* %9 to <16 x i8>*
  store <16 x i8> %8, <16 x i8>* %10, align 1, !tbaa !6
  %index.next = or i64 %index, 16
  %vec.ind.next = add <16 x i64> %vec.ind, <i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16, i64 16>
  %11 = lshr <16 x i64> %vec.ind.next, <i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8>
  %12 = xor <16 x i64> %11, %vec.ind.next
  %13 = add <16 x i64> %12, %broadcast.splat20
  %14 = trunc <16 x i64> %13 to <16 x i8>
  %15 = getelementptr inbounds i8, i8* %data_out, i64 %index.next
  %16 = bitcast i8* %15 to <16 x i8>*
  store <16 x i8> %14, <16 x i8>* %16, align 1, !tbaa !6
  %index.next.1 = add i64 %index, 32
  %vec.ind.next.1 = add <16 x i64> %vec.ind, <i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32, i64 32>
  %niter.nsub.1 = add i64 %niter, -2
  %niter.ncmp.1 = icmp eq i64 %niter.nsub.1, 0
  br i1 %niter.ncmp.1, label %middle.block.unr-lcssa, label %vector.body, !llvm.loop !7

middle.block.unr-lcssa:                           ; preds = %vector.body, %vector.ph
  %index.unr = phi i64 [ 0, %vector.ph ], [ %index.next.1, %vector.body ]
  %vec.ind.unr = phi <16 x i64> [ %induction18, %vector.ph ], [ %vec.ind.next.1, %vector.body ]
  %lcmp.mod = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod, label %middle.block, label %vector.body.epil

vector.body.epil:                                 ; preds = %middle.block.unr-lcssa
  %17 = lshr <16 x i64> %vec.ind.unr, <i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8>
  %18 = xor <16 x i64> %17, %vec.ind.unr
  %19 = add <16 x i64> %18, %broadcast.splat20
  %20 = trunc <16 x i64> %19 to <16 x i8>
  %21 = getelementptr inbounds i8, i8* %data_out, i64 %index.unr
  %22 = bitcast i8* %21 to <16 x i8>*
  store <16 x i8> %20, <16 x i8>* %22, align 1, !tbaa !6
  br label %middle.block

middle.block:                                     ; preds = %middle.block.unr-lcssa, %vector.body.epil
  %cmp.n = icmp eq i64 %n.vec, %length
  br i1 %cmp.n, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %middle.block, %for.body.lr.ph
  %i.016.ph = phi i64 [ 0, %for.body.lr.ph ], [ %n.vec, %middle.block ]
  %offset.addr.015.ph = phi i64 [ %offset, %for.body.lr.ph ], [ %ind.end, %middle.block ]
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %i.016 = phi i64 [ %inc5, %for.body ], [ %i.016.ph, %for.body.preheader ]
  %offset.addr.015 = phi i64 [ %inc, %for.body ], [ %offset.addr.015.ph, %for.body.preheader ]
  %shr = lshr i64 %offset.addr.015, 8
  %xor = xor i64 %shr, %offset.addr.015
  %add = add i64 %xor, %conv313
  %conv4 = trunc i64 %add to i8
  %arrayidx = getelementptr inbounds i8, i8* %data_out, i64 %i.016
  store i8 %conv4, i8* %arrayidx, align 1, !tbaa !6
  %inc = add i64 %offset.addr.015, 1
  %inc5 = add nuw i64 %i.016, 1
  %exitcond = icmp eq i64 %inc5, %length
  br i1 %exitcond, label %for.end, label %for.body, !llvm.loop !9

for.end:                                          ; preds = %for.body, %middle.block, %entry
  ret i64 0
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #1

; Function Attrs: nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #2

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #1

; Function Attrs: nounwind uwtable
define weak dso_local i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* %data_in, i8* %mask, i64 %offset, i64 %length) local_unnamed_addr #0 {
entry:
  %0 = load i32, i32* @write_count, align 4, !tbaa !2
  %inc = add nsw i32 %0, 1
  store i32 %inc, i32* @write_count, align 4, !tbaa !2
  %conv = trunc i64 %offset to i32
  %conv1 = trunc i64 %length to i32
  %call = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str.1, i64 0, i64 0), i32 %conv, i32 %conv1)
  %cmp37 = icmp eq i64 %length, 0
  br i1 %cmp37, label %for.end, label %for.body

for.body:                                         ; preds = %entry, %for.body
  %i.038 = phi i64 [ %inc7, %for.body ], [ 0, %entry ]
  %rem = and i64 %i.038, 7
  %cmp3 = icmp eq i64 %rem, 0
  %cond = select i1 %cmp3, i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.3, i64 0, i64 0), i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str.4, i64 0, i64 0)
  %arrayidx = getelementptr inbounds i8, i8* %data_in, i64 %i.038
  %1 = load i8, i8* %arrayidx, align 1, !tbaa !6
  %conv5 = zext i8 %1 to i32
  %call6 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str.2, i64 0, i64 0), i8* %cond, i32 %conv5)
  %inc7 = add nuw i64 %i.038, 1
  %exitcond39 = icmp eq i64 %inc7, %length
  br i1 %exitcond39, label %for.end, label %for.body

for.end:                                          ; preds = %for.body, %entry
  %call8 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str.5, i64 0, i64 0))
  %add = add i64 %length, 7
  %div = lshr i64 %add, 3
  %cmp1035 = icmp eq i64 %div, 0
  br i1 %cmp1035, label %for.end22, label %for.body12

for.body12:                                       ; preds = %for.end, %for.body12
  %i.136 = phi i64 [ %inc21, %for.body12 ], [ 0, %for.end ]
  %rem13 = and i64 %i.136, 7
  %cmp14 = icmp eq i64 %rem13, 0
  %cond16 = select i1 %cmp14, i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.3, i64 0, i64 0), i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str.4, i64 0, i64 0)
  %arrayidx17 = getelementptr inbounds i8, i8* %mask, i64 %i.136
  %2 = load i8, i8* %arrayidx17, align 1, !tbaa !6
  %conv18 = zext i8 %2 to i32
  %call19 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str.2, i64 0, i64 0), i8* %cond16, i32 %conv18)
  %inc21 = add nuw nsw i64 %i.136, 1
  %exitcond = icmp eq i64 %inc21, %div
  br i1 %exitcond, label %for.end22, label %for.body12

for.end22:                                        ; preds = %for.body12, %for.end
  %putchar = tail call i32 @putchar(i32 10)
  ret i64 %length
}

; Function Attrs: norecurse nounwind uwtable
define weak dso_local void @nanotube_merge_data_mask(i8* %inout_data, i8* %inout_mask, i8* %in_data, i8* %in_mask, i64 %offset, i64 %data_length) local_unnamed_addr #3 {
entry:
  %cmp25 = icmp eq i64 %data_length, 0
  br i1 %cmp25, label %for.cond.cleanup, label %for.body

for.cond.cleanup:                                 ; preds = %if.end, %entry
  ret void

for.body:                                         ; preds = %entry, %if.end
  %i.026 = phi i64 [ %inc, %if.end ], [ 0, %entry ]
  %div = lshr i64 %i.026, 3
  %0 = trunc i64 %i.026 to i32
  %conv = and i32 %0, 7
  %add = add i64 %i.026, %offset
  %arrayidx = getelementptr inbounds i8, i8* %in_mask, i64 %div
  %1 = load i8, i8* %arrayidx, align 1, !tbaa !6
  %conv1 = zext i8 %1 to i32
  %shl = shl i32 1, %conv
  %and = and i32 %shl, %conv1
  %tobool = icmp eq i32 %and, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %for.body
  %arrayidx3 = getelementptr inbounds i8, i8* %in_data, i64 %i.026
  %2 = load i8, i8* %arrayidx3, align 1, !tbaa !6
  %arrayidx4 = getelementptr inbounds i8, i8* %inout_data, i64 %add
  store i8 %2, i8* %arrayidx4, align 1, !tbaa !6
  %div5 = lshr i64 %add, 3
  %3 = trunc i64 %add to i32
  %conv7 = and i32 %3, 7
  %shl9 = shl i32 1, %conv7
  %arrayidx10 = getelementptr inbounds i8, i8* %inout_mask, i64 %div5
  %4 = load i8, i8* %arrayidx10, align 1, !tbaa !6
  %5 = trunc i32 %shl9 to i8
  %conv12 = or i8 %4, %5
  store i8 %conv12, i8* %arrayidx10, align 1, !tbaa !6
  br label %if.end

if.end:                                           ; preds = %for.body, %if.then
  %inc = add nuw i64 %i.026, 1
  %exitcond = icmp eq i64 %inc, %data_length
  br i1 %exitcond, label %for.cond.cleanup, label %for.body
}

; Function Attrs: nounwind uwtable
define weak dso_local i32 @process_packet(%struct.nanotube_context* %nt_ctx, %struct.nanotube_packet* %packet) local_unnamed_addr #0 {
entry:
  %buf1 = alloca [8 x i8], align 1
  %mask = alloca [1 x i8], align 1
  %0 = getelementptr inbounds [8 x i8], [8 x i8]* %buf1, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 8, i8* nonnull %0) #4
  %1 = getelementptr inbounds [1 x i8], [1 x i8]* %mask, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %1) #4
  store i8 -1, i8* %1, align 1
  %call = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 0, i64 8)
  %arrayidx = getelementptr inbounds [8 x i8], [8 x i8]* %buf1, i64 0, i64 2
  %2 = load i8, i8* %arrayidx, align 1, !tbaa !6
  %conv = zext i8 %2 to i64
  %3 = load i8, i8* %0, align 1, !tbaa !6
  %cmp = icmp eq i8 %3, 0
  br i1 %cmp, label %if.then, label %if.else18

if.then:                                          ; preds = %entry
  %arrayidx4 = getelementptr inbounds [8 x i8], [8 x i8]* %buf1, i64 0, i64 1
  %4 = load i8, i8* %arrayidx4, align 1, !tbaa !6
  %cmp6 = icmp eq i8 %4, 0
  br i1 %cmp6, label %if.then8, label %if.else

if.then8:                                         ; preds = %if.then
  %call11 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* nonnull %0, i8* nonnull %1, i64 32, i64 4)
  br label %if.end

if.else:                                          ; preds = %if.then
  %call14 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* nonnull %0, i8* nonnull %1, i64 32, i64 8)
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then8
  %call17 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* nonnull %0, i8* nonnull %1, i64 %conv, i64 4)
  br label %if.end22

if.else18:                                        ; preds = %entry
  %call21 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* nonnull %0, i8* nonnull %1, i64 %conv, i64 8)
  br label %if.end22

if.end22:                                         ; preds = %if.else18, %if.end
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %1) #4
  call void @llvm.lifetime.end.p0i8(i64 8, i8* nonnull %0) #4
  ret i32 0
}

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  %call.i = tail call i32 @process_packet(%struct.nanotube_context* null, %struct.nanotube_packet* null) #4
  ret void
}

; Function Attrs: nounwind uwtable
define dso_local i32 @main() local_unnamed_addr #0 {
entry:
  %call.i.i = tail call i32 @process_packet(%struct.nanotube_context* null, %struct.nanotube_packet* null) #4
  ret i32 0
}

; Function Attrs: nounwind
declare i32 @putchar(i32) local_unnamed_addr #4

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
!2 = !{!3, !3, i64 0}
!3 = !{!"int", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!4, !4, i64 0}
!7 = distinct !{!7, !8}
!8 = !{!"llvm.loop.isvectorized", i32 1}
!9 = distinct !{!9, !10, !8}
!10 = !{!"llvm.loop.unroll.runtime.disable"}
