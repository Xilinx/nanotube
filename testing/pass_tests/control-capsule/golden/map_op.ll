;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/control-capsule/map_op.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_map = type opaque
%struct.nanotube_packet = type opaque

@.str = private unnamed_addr constant [7 x i8] c"simple\00", align 1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #0

declare dso_local i64 @nanotube_map_op(%struct.nanotube_context*, i16 zeroext, i32, i8*, i64, i8*, i8*, i8*, i64, i64) local_unnamed_addr #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #0

; Function Attrs: uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #2 {
entry:
  %call = tail call %struct.nanotube_map* @nanotube_map_create(i16 zeroext 0, i32 0, i64 4, i64 8)
  %call1 = tail call %struct.nanotube_context* @nanotube_context_create()
  tail call void @nanotube_context_add_map(%struct.nanotube_context* %call1, %struct.nanotube_map* %call)
  tail call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i64 0, i64 0), void (%struct.nanotube_context*, %struct.nanotube_packet*)* nonnull @simple, i32 0, i32 1)
  ret void
}

declare dso_local %struct.nanotube_map* @nanotube_map_create(i16 zeroext, i32, i64, i64) local_unnamed_addr #1

declare dso_local %struct.nanotube_context* @nanotube_context_create() local_unnamed_addr #1

declare dso_local void @nanotube_context_add_map(%struct.nanotube_context*, %struct.nanotube_map*) local_unnamed_addr #1

declare dso_local void @nanotube_add_plain_packet_kernel(i8*, void (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32) local_unnamed_addr #1

; Function Attrs: uwtable
define private void @simple(%struct.nanotube_context* %context, %struct.nanotube_packet* nocapture readnone %packet) #2 {
entry:
  %control_capsule_buffer = alloca i8, i64 20
  %capsule_class = call i32 @nanotube_capsule_classify_sb(%struct.nanotube_packet* %packet)
  switch i32 %capsule_class, label %pass_through_capsule [
    i32 1, label %process_net_packet
    i32 2, label %control_capsule_read
  ]

pass_through_capsule:                             ; preds = %entry
  ret void

control_capsule_read:                             ; preds = %entry
  %0 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* %control_capsule_buffer, i64 0, i64 20)
  %cc.res_id0.get = getelementptr inbounds i8, i8* %control_capsule_buffer, i64 2
  %cc.res_id0.load = load i8, i8* %cc.res_id0.get
  %cc.res_id0.zext = zext i8 %cc.res_id0.load to i16
  %cc.res_id1.gep = getelementptr inbounds i8, i8* %control_capsule_buffer, i64 3
  %cc.res_id1.load = load i8, i8* %cc.res_id1.gep
  %cc.res_id1.zext = zext i8 %cc.res_id1.load to i16
  %cc.res_id1.shl = shl i16 %cc.res_id1.zext, 8
  %cc.res_id = or i16 %cc.res_id0.zext, %cc.res_id1.shl
  switch i16 %cc.res_id, label %control_capsule_bad_resource [
    i16 0, label %control_capsule_map0_bb
  ]

control_capsule_map0_bb:                          ; preds = %control_capsule_read
  call void @nanotube_map_process_capsule(%struct.nanotube_context* %context, i16 0, i8* %control_capsule_buffer, i64 4, i64 8)
  br label %control_capsule_write

control_capsule_bad_resource:                     ; preds = %control_capsule_read
  %cc.rc0.gep = getelementptr inbounds i8, i8* %control_capsule_buffer, i64 4
  store i8 2, i8* %cc.rc0.gep
  %cc.rc1.gep = getelementptr inbounds i8, i8* %control_capsule_buffer, i64 5
  store i8 0, i8* %cc.rc1.gep
  br label %control_capsule_write

control_capsule_write:                            ; preds = %control_capsule_map0_bb, %control_capsule_bad_resource
  %1 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* %control_capsule_buffer, i64 0, i64 20)
  ret void

process_net_packet:                               ; preds = %entry
  %key = alloca i32, align 4
  %data = alloca [1 x i8], align 1
  %mask = alloca [1 x i8], align 1
  %2 = bitcast i32* %key to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %2) #3
  store i32 67305985, i32* %key, align 4
  %3 = getelementptr inbounds [1 x i8], [1 x i8]* %data, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %3) #3
  %4 = getelementptr inbounds [1 x i8], [1 x i8]* %mask, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %4) #3
  %call = call i64 @nanotube_map_op(%struct.nanotube_context* %context, i16 zeroext 0, i32 0, i8* nonnull %2, i64 4, i8* null, i8* nonnull %3, i8* null, i64 0, i64 1)
  %.promoted = load i8, i8* %3, align 1, !tbaa !2
  %5 = add i8 %.promoted, 4
  store i8 %5, i8* %3, align 1, !tbaa !2
  store i8 -1, i8* %4, align 1
  %call7 = call i64 @nanotube_map_op(%struct.nanotube_context* %context, i16 zeroext 0, i32 0, i8* nonnull %2, i64 4, i8* nonnull %3, i8* null, i8* nonnull %4, i64 0, i64 1)
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %4) #3
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %3) #3
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %2) #3
  ret void
}

declare i64 @nanotube_packet_write(%struct.nanotube_packet*, i8*, i64, i64)

declare i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64)

declare void @nanotube_map_process_capsule(%struct.nanotube_context*, i16, i8*, i64, i64)

declare i32 @nanotube_capsule_classify_sb(%struct.nanotube_packet*)

attributes #0 = { argmemonly nounwind }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
!2 = !{!3, !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C++ TBAA"}
