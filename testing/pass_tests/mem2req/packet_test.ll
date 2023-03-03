;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'build/testing/pass_tests/mem2req/packet_test.bc'
source_filename = "testing/pass_tests/mem2req/packet_test.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque

; Function Attrs: uwtable
define dso_local i32 @_Z11memset_testP16nanotube_contextP15nanotube_packet(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) local_unnamed_addr #0 {
entry:
  %call = tail call i8* @nanotube_packet_data(%struct.nanotube_packet* %packet)
  %call1 = tail call i8* @nanotube_packet_end(%struct.nanotube_packet* %packet)
  %sub.ptr.lhs.cast = ptrtoint i8* %call1 to i64
  %sub.ptr.rhs.cast = ptrtoint i8* %call to i64
  %sub.ptr.sub = sub i64 %sub.ptr.lhs.cast, %sub.ptr.rhs.cast
  %cmp = icmp slt i64 %sub.ptr.sub, 42
  br i1 %cmp, label %cleanup, label %if.end

if.end:                                           ; preds = %entry
  tail call void @llvm.memset.p0i8.i64(i8* align 1 %call, i8 0, i64 21, i1 false)
  %add.ptr = getelementptr inbounds i8, i8* %call, i64 21
  tail call void @llvm.memset.p0i8.i64(i8* nonnull align 1 %add.ptr, i8 15, i64 21, i1 false)
  br label %cleanup

cleanup:                                          ; preds = %if.end, %entry
  %retval.0 = phi i32 [ 0, %if.end ], [ -1, %entry ]
  ret i32 %retval.0
}

declare dso_local i8* @nanotube_packet_data(%struct.nanotube_packet*) local_unnamed_addr #1

declare dso_local i8* @nanotube_packet_end(%struct.nanotube_packet*) local_unnamed_addr #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1) #2

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #3

attributes #0 = { uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { argmemonly nounwind }
attributes #3 = { nounwind readnone speculatable }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = !{!"clang version 8.0.0 "}
