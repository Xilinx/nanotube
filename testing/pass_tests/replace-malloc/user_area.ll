;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'bar.c'
source_filename = "bar.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque

@.str = private unnamed_addr constant [8 x i8] c"Stage 0\00", align 1
@result = common dso_local local_unnamed_addr global i32 0, align 4

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  %arg = alloca i32, align 4
  %0 = bitcast i32* %arg to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %0) #3
  store i32 42, i32* %arg, align 4, !tbaa !2
  %call = tail call %struct.nanotube_context* @nanotube_context_create() #3
  call void @nanotube_thread_create(%struct.nanotube_context* %call, i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), void (%struct.nanotube_context*, i8*)* nonnull @stage_0, i8* nonnull %0, i64 4) #3
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %0) #3
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #1

declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr #2

declare dso_local void @nanotube_thread_create(%struct.nanotube_context*, i8*, void (%struct.nanotube_context*, i8*)*, i8*, i64) local_unnamed_addr #2

; Function Attrs: nounwind uwtable
define dso_local void @stage_0(%struct.nanotube_context* nocapture readnone %ctxt, i8* nocapture readonly %data) #0 {
entry:
  %0 = bitcast i8* %data to i32*
  %1 = load i32, i32* %0, align 4, !tbaa !2
  store i32 %1, i32* @result, align 4, !tbaa !2
  tail call void @nanotube_thread_wait() #3
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #1

declare dso_local void @nanotube_thread_wait() local_unnamed_addr #2

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
!2 = !{!3, !3, i64 0}
!3 = !{!"int", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
