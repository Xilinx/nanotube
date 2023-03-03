;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "foo.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.entry = type { i8, i8 }
%struct.nanotube_context = type opaque

@buffer = dso_local global [4 x %struct.entry] zeroinitializer, align 1
@x = dso_local local_unnamed_addr global i8 0, align 1
@y = dso_local local_unnamed_addr global i8 0, align 1
@pointer_0 = common dso_local local_unnamed_addr global %struct.entry* null, align 8
@pointer_1 = common dso_local global %struct.entry* null, align 8
@.str = private unnamed_addr constant [8 x i8] c"Stage 0\00", align 1
@result = common dso_local local_unnamed_addr global [8 x i8] zeroinitializer, align 1

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  store i8 10, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 0, i32 0), align 1, !tbaa !2
  store i8 11, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 0, i32 1), align 1, !tbaa !6
  store i8 12, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 1, i32 0), align 1, !tbaa !2
  store i8 13, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 1, i32 1), align 1, !tbaa !6
  store i8 14, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 2, i32 0), align 1, !tbaa !2
  store i8 15, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 2, i32 1), align 1, !tbaa !6
  store i8 16, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 3, i32 0), align 1, !tbaa !2
  store i8 17, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 3, i32 1), align 1, !tbaa !6
  store %struct.entry* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 0), %struct.entry** @pointer_0, align 8, !tbaa !7
  store volatile %struct.entry* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 1), %struct.entry** @pointer_1, align 8, !tbaa !7
  %call = tail call %struct.nanotube_context* @nanotube_context_create() #2
  tail call void @nanotube_thread_create(%struct.nanotube_context* %call, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), void (%struct.nanotube_context*, i8*)* nonnull @stage_0, i8* null, i64 0) #2
  ret void
}

declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr #1

declare dso_local void @nanotube_thread_create(%struct.nanotube_context*, i8*, void (%struct.nanotube_context*, i8*)*, i8*, i64) local_unnamed_addr #1

; Function Attrs: nounwind uwtable
define dso_local void @stage_0(%struct.nanotube_context* nocapture readnone %ctxt, i8* nocapture readnone %data) #0 {
entry:
  %0 = load %struct.entry*, %struct.entry** @pointer_0, align 8, !tbaa !7
  %1 = load i8, i8* @x, align 1, !tbaa !9
  %tobool = icmp eq i8 %1, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %2 = load volatile %struct.entry*, %struct.entry** @pointer_1, align 8, !tbaa !7
  br label %if.end

if.end:                                           ; preds = %entry, %if.then
  %p.0 = phi %struct.entry* [ %2, %if.then ], [ %0, %entry ]
  %3 = load i8, i8* @y, align 1, !tbaa !9
  %idxprom = sext i8 %3 to i64
  %a = getelementptr inbounds %struct.entry, %struct.entry* %p.0, i64 %idxprom, i32 0
  store i8 42, i8* %a, align 1, !tbaa !2
  %4 = load i8, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 0, i32 0), align 1, !tbaa !2
  store i8 %4, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @result, i64 0, i64 0), align 1, !tbaa !9
  %5 = load i8, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 0, i32 1), align 1, !tbaa !6
  store i8 %5, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @result, i64 0, i64 1), align 1, !tbaa !9
  %6 = load i8, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 1, i32 0), align 1, !tbaa !2
  store i8 %6, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @result, i64 0, i64 2), align 1, !tbaa !9
  %7 = load i8, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 1, i32 1), align 1, !tbaa !6
  store i8 %7, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @result, i64 0, i64 3), align 1, !tbaa !9
  %8 = load i8, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 2, i32 0), align 1, !tbaa !2
  store i8 %8, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @result, i64 0, i64 4), align 1, !tbaa !9
  %9 = load i8, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 2, i32 1), align 1, !tbaa !6
  store i8 %9, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @result, i64 0, i64 5), align 1, !tbaa !9
  %10 = load i8, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 3, i32 0), align 1, !tbaa !2
  store i8 %10, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @result, i64 0, i64 6), align 1, !tbaa !9
  %11 = load i8, i8* getelementptr inbounds ([4 x %struct.entry], [4 x %struct.entry]* @buffer, i64 0, i64 3, i32 1), align 1, !tbaa !6
  store i8 %11, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @result, i64 0, i64 7), align 1, !tbaa !9
  tail call void @nanotube_thread_wait() #2
  ret void
}

declare dso_local void @nanotube_thread_wait() local_unnamed_addr #1

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
!2 = !{!3, !4, i64 0}
!3 = !{!"entry", !4, i64 0, !4, i64 1}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!3, !4, i64 1}
!7 = !{!8, !8, i64 0}
!8 = !{!"any pointer", !4, i64 0}
!9 = !{!4, !4, i64 0}
