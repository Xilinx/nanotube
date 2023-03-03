;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque

@buffer = common dso_local global [64 x i32] zeroinitializer, align 16
@pointer = common dso_local local_unnamed_addr global i32* null, align 8
@.str = private unnamed_addr constant [8 x i8] c"Stage 0\00", align 1
@result_int = common dso_local local_unnamed_addr global i32 0, align 4
@result_ptr = common dso_local local_unnamed_addr global i32* null, align 8

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  store i32* getelementptr inbounds ([64 x i32], [64 x i32]* @buffer, i64 0, i64 11), i32** @pointer, align 8, !tbaa !2
  store i32 42, i32* getelementptr inbounds ([64 x i32], [64 x i32]* @buffer, i64 0, i64 11), align 4, !tbaa !6
  %call = tail call %struct.nanotube_context* @nanotube_context_create() #2
  tail call void @nanotube_thread_create(%struct.nanotube_context* %call, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), void (%struct.nanotube_context*, i8*)* nonnull @stage_0, i8* null, i64 0) #2
  ret void
}

declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr #1

declare dso_local void @nanotube_thread_create(%struct.nanotube_context*, i8*, void (%struct.nanotube_context*, i8*)*, i8*, i64) local_unnamed_addr #1

; Function Attrs: nounwind uwtable
define dso_local void @stage_0(%struct.nanotube_context* nocapture readnone %ctxt, i8* nocapture readnone %data) #0 {
entry:
  %0 = load i32*, i32** @pointer, align 8, !tbaa !2
  %1 = load i32, i32* %0, align 4, !tbaa !6
  store i32 %1, i32* @result_int, align 4, !tbaa !6
  store i32* %0, i32** @result_ptr, align 8, !tbaa !2
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
!2 = !{!3, !3, i64 0}
!3 = !{!"any pointer", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!7, !7, i64 0}
!7 = !{!"int", !4, i64 0}
