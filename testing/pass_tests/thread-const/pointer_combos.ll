;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "bar.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque

@buffer_0 = common dso_local global i8 0, align 1
@pointer_0 = common dso_local local_unnamed_addr global i8* null, align 8
@pointer_1 = common dso_local local_unnamed_addr global i8* null, align 8
@buffer_1 = common dso_local global i32 0, align 4
@pointer_2 = common dso_local local_unnamed_addr global i8* null, align 8
@pointer_3 = common dso_local local_unnamed_addr global i8* null, align 8
@pointer_4 = common dso_local local_unnamed_addr global i32* null, align 8
@pointer_5 = common dso_local local_unnamed_addr global i32* null, align 8
@pointer_6 = common dso_local local_unnamed_addr global i32* null, align 8
@pointer_7 = common dso_local local_unnamed_addr global i32* null, align 8
@.str = private unnamed_addr constant [8 x i8] c"Stage 0\00", align 1
@result_0 = common dso_local local_unnamed_addr global i8* null, align 8
@result_1 = common dso_local local_unnamed_addr global i8* null, align 8
@result_2 = common dso_local local_unnamed_addr global i8* null, align 8
@result_3 = common dso_local local_unnamed_addr global i8* null, align 8
@result_4 = common dso_local local_unnamed_addr global i32* null, align 8
@result_5 = common dso_local local_unnamed_addr global i32* null, align 8
@result_6 = common dso_local local_unnamed_addr global i32* null, align 8
@result_7 = common dso_local local_unnamed_addr global i32* null, align 8

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  store i8* @buffer_0, i8** @pointer_0, align 8, !tbaa !2
  store i8* getelementptr inbounds (i8, i8* @buffer_0, i64 1), i8** @pointer_1, align 8, !tbaa !2
  store i8* bitcast (i32* @buffer_1 to i8*), i8** @pointer_2, align 8, !tbaa !2
  store i8* bitcast (i32* getelementptr inbounds (i32, i32* @buffer_1, i64 1) to i8*), i8** @pointer_3, align 8, !tbaa !2
  store i32* bitcast (i8* @buffer_0 to i32*), i32** @pointer_4, align 8, !tbaa !2
  store i32* bitcast (i8* getelementptr inbounds (i8, i8* @buffer_0, i64 1) to i32*), i32** @pointer_5, align 8, !tbaa !2
  store i32* @buffer_1, i32** @pointer_6, align 8, !tbaa !2
  store i32* getelementptr inbounds (i32, i32* @buffer_1, i64 1), i32** @pointer_7, align 8, !tbaa !2
  %call = tail call %struct.nanotube_context* @nanotube_context_create() #3
  tail call void @nanotube_thread_create(%struct.nanotube_context* %call, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), void (%struct.nanotube_context*, i8*)* nonnull @stage_0, i8* null, i64 0) #3
  ret void
}

declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr #1

declare dso_local void @nanotube_thread_create(%struct.nanotube_context*, i8*, void (%struct.nanotube_context*, i8*)*, i8*, i64) local_unnamed_addr #1

; Function Attrs: norecurse nounwind uwtable
define dso_local void @stage_0(%struct.nanotube_context* nocapture readnone %ctxt, i8* nocapture readnone %data) #2 {
entry:
  %0 = load i8*, i8** @pointer_0, align 8, !tbaa !2
  %add.ptr = getelementptr inbounds i8, i8* %0, i64 1
  store i8* %add.ptr, i8** @result_0, align 8, !tbaa !2
  %1 = load i8*, i8** @pointer_1, align 8, !tbaa !2
  %add.ptr1 = getelementptr inbounds i8, i8* %1, i64 1
  store i8* %add.ptr1, i8** @result_1, align 8, !tbaa !2
  %2 = load i8*, i8** @pointer_2, align 8, !tbaa !2
  %add.ptr2 = getelementptr inbounds i8, i8* %2, i64 1
  store i8* %add.ptr2, i8** @result_2, align 8, !tbaa !2
  %3 = load i8*, i8** @pointer_3, align 8, !tbaa !2
  %add.ptr3 = getelementptr inbounds i8, i8* %3, i64 1
  store i8* %add.ptr3, i8** @result_3, align 8, !tbaa !2
  %4 = load i32*, i32** @pointer_4, align 8, !tbaa !2
  %add.ptr4 = getelementptr inbounds i32, i32* %4, i64 1
  store i32* %add.ptr4, i32** @result_4, align 8, !tbaa !2
  %5 = load i32*, i32** @pointer_5, align 8, !tbaa !2
  %add.ptr5 = getelementptr inbounds i32, i32* %5, i64 1
  store i32* %add.ptr5, i32** @result_5, align 8, !tbaa !2
  %6 = load i32*, i32** @pointer_6, align 8, !tbaa !2
  %add.ptr6 = getelementptr inbounds i32, i32* %6, i64 1
  store i32* %add.ptr6, i32** @result_6, align 8, !tbaa !2
  %7 = load i32*, i32** @pointer_7, align 8, !tbaa !2
  %add.ptr7 = getelementptr inbounds i32, i32* %7, i64 1
  store i32* %add.ptr7, i32** @result_7, align 8, !tbaa !2
  ret void
}

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
!2 = !{!3, !3, i64 0}
!3 = !{!"any pointer", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
