;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'foo.c'
source_filename = "foo.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque

@.str = private unnamed_addr constant [7 x i8] c"kernel\00", align 1

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  tail call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i64 0, i64 0), void (%struct.nanotube_context*, %struct.nanotube_packet*)* nonnull @kernel, i32 -1, i32 0) #3
  ret void
}

declare dso_local void @nanotube_add_plain_packet_kernel(i8*, void (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32) local_unnamed_addr #1

; Function Attrs: nounwind uwtable
define dso_local void @kernel(%struct.nanotube_context* nocapture readnone %context, %struct.nanotube_packet* %packet) #0 {
entry:
  %buffer = alloca [14 x i8], align 1
  %0 = getelementptr inbounds [14 x i8], [14 x i8]* %buffer, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 14, i8* nonnull %0) #3
  %call = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 0, i64 14) #3
  %call2 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 0, i64 14) #3
  call void @llvm.lifetime.end.p0i8(i64 14, i8* nonnull %0) #3
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #2

declare dso_local i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64) local_unnamed_addr #1

declare dso_local i64 @nanotube_packet_write(%struct.nanotube_packet*, i8*, i64, i64) local_unnamed_addr #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #2

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { argmemonly nounwind }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
