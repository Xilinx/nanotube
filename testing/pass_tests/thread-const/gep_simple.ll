;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "foo.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque

@buffer = dso_local global [16 x i8] zeroinitializer, align 16
@pointer_0 = common dso_local global i32* null, align 8
@pointer_1 = common dso_local local_unnamed_addr global i32** null, align 8
@.str = private unnamed_addr constant [8 x i8] c"Stage 0\00", align 1
@result = common dso_local local_unnamed_addr global i8 0, align 1

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  store i32* bitcast ([16 x i8]* @buffer to i32*), i32** @pointer_0, align 8, !tbaa !2
  store i32** @pointer_0, i32*** @pointer_1, align 8, !tbaa !2
  store i8 42, i8* getelementptr inbounds ([16 x i8], [16 x i8]* @buffer, i64 0, i64 0), align 16, !tbaa !6
  store i8 85, i8* getelementptr inbounds ([16 x i8], [16 x i8]* @buffer, i64 0, i64 4), align 4, !tbaa !6
  %call = tail call %struct.nanotube_context* @nanotube_context_create() #2
  tail call void @nanotube_thread_create(%struct.nanotube_context* %call, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), void (%struct.nanotube_context*, i8*)* nonnull @stage_0, i8* null, i64 0) #2
  ret void
}

declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr #1

declare dso_local void @nanotube_thread_create(%struct.nanotube_context*, i8*, void (%struct.nanotube_context*, i8*)*, i8*, i64) local_unnamed_addr #1

; Function Attrs: nounwind uwtable
define dso_local void @stage_0(%struct.nanotube_context* nocapture readnone %ctxt, i8* nocapture readnone %data) #0 {
entry:
  %0 = load i32**, i32*** @pointer_1, align 8, !tbaa !2
  %1 = load i32*, i32** %0, align 8, !tbaa !2
  %add.ptr = getelementptr inbounds i32, i32* %1, i64 1
  %2 = bitcast i32* %add.ptr to i8*
  %3 = load i8, i8* %2, align 1, !tbaa !6
  store i8 %3, i8* @result, align 1, !tbaa !6
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
!6 = !{!4, !4, i64 0}
