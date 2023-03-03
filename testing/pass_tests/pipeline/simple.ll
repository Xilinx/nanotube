;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/pipeline/simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque

@.str = private unnamed_addr constant [7 x i8] c"simple\00", align 1

; Function Attrs: uwtable
define dso_local i32 @simple(%struct.nanotube_context* nocapture readnone %context, %struct.nanotube_packet* %packet) #0 {
entry:
  %buffer = alloca [4 x i8], align 1
  %mask = alloca i8, align 1
  %0 = getelementptr inbounds [4 x i8], [4 x i8]* %buffer, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %0) #3
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %mask) #3
  store i8 -1, i8* %mask, align 1, !tbaa !2
  %call = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 0, i64 4)
  %call2 = call i64 @nanotube_packet_write_masked(%struct.nanotube_packet* %packet, i8* nonnull %0, i8* nonnull %mask, i64 0, i64 4)
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %mask) #3
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %0) #3
  ret i32 0
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #1

declare dso_local i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64) local_unnamed_addr #2

declare dso_local i64 @nanotube_packet_write_masked(%struct.nanotube_packet*, i8*, i8*, i64, i64) local_unnamed_addr #2

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #1

; Function Attrs: uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  tail call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i64 0, i64 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* nonnull @simple, i32 0, i32 1)
  ret void
}

declare dso_local void @nanotube_add_plain_packet_kernel(i8*, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32) local_unnamed_addr #2

attributes #0 = { uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
!2 = !{!3, !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C++ TBAA"}
