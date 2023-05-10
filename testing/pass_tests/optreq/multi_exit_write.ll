;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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
  %call = tail call i32 %kernel(%struct.nanotube_context* null, %struct.nanotube_packet* null) #3
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
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.lr.ph
  %i.016 = phi i64 [ 0, %for.body.lr.ph ], [ %inc5, %for.body ]
  %offset.addr.015 = phi i64 [ %offset, %for.body.lr.ph ], [ %inc, %for.body ]
  %shr = lshr i64 %offset.addr.015, 8
  %xor = xor i64 %shr, %offset.addr.015
  %add = add i64 %xor, %conv313
  %conv4 = trunc i64 %add to i8
  %arrayidx = getelementptr inbounds i8, i8* %data_out, i64 %i.016
  store i8 %conv4, i8* %arrayidx, align 1, !tbaa !6
  %inc = add i64 %offset.addr.015, 1
  %inc5 = add nuw i64 %i.016, 1
  %exitcond = icmp eq i64 %inc5, %length
  br i1 %exitcond, label %for.end, label %for.body

for.end:                                          ; preds = %for.body, %entry
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

; Function Attrs: nounwind uwtable
define weak dso_local i32 @process_packet(%struct.nanotube_context* %nt_ctx, %struct.nanotube_packet* %packet) #0 {
entry:
  %buf1 = alloca [8 x i8], align 1
  %mask = alloca [1 x i8], align 1
  %0 = getelementptr inbounds [8 x i8], [8 x i8]* %buf1, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 8, i8* nonnull %0) #3
  %1 = getelementptr inbounds [1 x i8], [1 x i8]* %mask, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %1) #3
  store i8 -1, i8* %1, align 1
  %call = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 0, i64 8)
  %2 = load i8, i8* %0, align 1, !tbaa !6
  %cmp = icmp eq i8 %2, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %call4 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* nonnull %0, i8* nonnull %1, i64 4, i64 8)
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %1) #3
  call void @llvm.lifetime.end.p0i8(i64 8, i8* nonnull %0) #3
  ret i32 0

if.else:                                          ; preds = %entry
  %call7 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* nonnull %0, i8* nonnull %1, i64 0, i64 8)
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %1) #3
  call void @llvm.lifetime.end.p0i8(i64 8, i8* nonnull %0) #3
  ret i32 1
}

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  tail call void @nanotube_add_plain_packet_kernel(i8* undef, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* nonnull @process_packet, i32 undef, i32 undef)
  ret void
}

; Function Attrs: nounwind uwtable
define dso_local i32 @main() local_unnamed_addr #0 {
entry:
  tail call void @nanotube_setup()
  ret i32 0
}

; Function Attrs: nounwind
declare i32 @putchar(i32) local_unnamed_addr #3

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
!2 = !{!3, !3, i64 0}
!3 = !{!"int", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!4, !4, i64 0}
