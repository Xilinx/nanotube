;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'simple.cc'
source_filename = "simple.cc"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%"class.std::ios_base::Init" = type { i8 }
%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque

@_ZStL8__ioinit = internal global %"class.std::ios_base::Init" zeroinitializer, align 1
@__dso_handle = external hidden global i8
@.str = private unnamed_addr constant [11 x i8] c"simple_mem\00", align 1
@.str.1 = private unnamed_addr constant [9 x i8] c"test_war\00", align 1
@.str.2 = private unnamed_addr constant [12 x i8] c"mem_compute\00", align 1
@llvm.global_ctors = appending global [1 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* @_GLOBAL__sub_I_simple.cc, i8* null }]

; Function Attrs: uwtable
define internal fastcc void @__cxx_global_var_init() unnamed_addr #0 section ".text.startup" {
entry:
  tail call void @_ZNSt8ios_base4InitC1Ev(%"class.std::ios_base::Init"* nonnull @_ZStL8__ioinit)
  %0 = tail call i32 @__cxa_atexit(void (i8*)* bitcast (void (%"class.std::ios_base::Init"*)* @_ZNSt8ios_base4InitD1Ev to void (i8*)*), i8* getelementptr inbounds (%"class.std::ios_base::Init", %"class.std::ios_base::Init"* @_ZStL8__ioinit, i64 0, i32 0), i8* nonnull @__dso_handle) #3
  ret void
}

declare dso_local void @_ZNSt8ios_base4InitC1Ev(%"class.std::ios_base::Init"*) unnamed_addr #1

; Function Attrs: nounwind
declare dso_local void @_ZNSt8ios_base4InitD1Ev(%"class.std::ios_base::Init"*) unnamed_addr #2

; Function Attrs: nounwind
declare dso_local i32 @__cxa_atexit(void (i8*)*, i8*, i8*) local_unnamed_addr #3

; Function Attrs: uwtable
define dso_local void @_Z10simple_memP16nanotube_contextP15nanotube_packet(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) #0 {
entry:
  %direct = alloca [10 x i8], align 1
  %read_buf = alloca [10 x i8], align 1
  %write_buf = alloca [10 x i8], align 1
  %pre_init = alloca [10 x i8], align 1
  %read_ldst = alloca [10 x i8], align 8
  %write_ldst = alloca [10 x i8], align 8
  %0 = getelementptr inbounds [10 x i8], [10 x i8]* %direct, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %0) #3
  %call = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 1, i64 10)
  %1 = getelementptr inbounds [10 x i8], [10 x i8]* %read_buf, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %1) #3
  %2 = getelementptr inbounds [10 x i8], [10 x i8]* %write_buf, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %2) #3
  %call2 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %1, i64 11, i64 10)
  %3 = getelementptr inbounds [10 x i8], [10 x i8]* %pre_init, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %3) #3
  call void @llvm.memset.p0i8.i64(i8* nonnull align 1 %3, i8 42, i64 10, i1 false)
  %4 = getelementptr inbounds [10 x i8], [10 x i8]* %read_ldst, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %4) #3
  %5 = getelementptr inbounds [10 x i8], [10 x i8]* %write_ldst, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %5) #3
  %call5 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %4, i64 21, i64 10)
  %call7 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 31, i64 10)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* nonnull align 1 %2, i8* nonnull align 1 %1, i64 10, i1 false)
  %call11 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %2, i64 41, i64 10)
  %call13 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %3, i64 51, i64 10)
  %6 = bitcast [10 x i8]* %read_ldst to i64*
  %7 = load i64, i64* %6, align 8, !tbaa !2
  %8 = bitcast [10 x i8]* %write_ldst to i64*
  store i64 %7, i64* %8, align 8, !tbaa !2
  %arrayidx18 = getelementptr inbounds [10 x i8], [10 x i8]* %read_ldst, i64 0, i64 8
  %9 = bitcast i8* %arrayidx18 to i16*
  %10 = load i16, i16* %9, align 8, !tbaa !6
  %arrayidx20 = getelementptr inbounds [10 x i8], [10 x i8]* %write_ldst, i64 0, i64 8
  %11 = bitcast i8* %arrayidx20 to i16*
  store i16 %10, i16* %11, align 8, !tbaa !6
  %call22 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %5, i64 61, i64 10)
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %5) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %4) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %3) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %2) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %1) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %0) #3
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #4

declare dso_local i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64) local_unnamed_addr #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1) #4

declare dso_local i64 @nanotube_packet_write(%struct.nanotube_packet*, i8*, i64, i64) local_unnamed_addr #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #4

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #4

; Function Attrs: uwtable
define dso_local void @_Z8test_warP16nanotube_contextP15nanotube_packet(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) #0 {
entry:
  %buf = alloca [10 x i8], align 1
  %rd = alloca i32, align 4
  %buf6 = alloca [10 x i8], align 1
  %rd12 = alloca i32, align 4
  %buf18 = alloca [10 x i8], align 1
  %rd24 = alloca i32, align 4
  %buf30 = alloca [10 x i8], align 1
  %rd36 = alloca i8, align 1
  %0 = getelementptr inbounds [10 x i8], [10 x i8]* %buf, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %0) #3
  %call = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 1, i64 10)
  %add.ptr = getelementptr inbounds [10 x i8], [10 x i8]* %buf, i64 0, i64 4
  %1 = bitcast i8* %add.ptr to i32*
  %2 = bitcast i32* %rd to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %2) #3
  %3 = load i32, i32* %1, align 4, !tbaa !8
  store i32 %3, i32* %rd, align 4, !tbaa !8
  store i32 42, i32* %1, align 4, !tbaa !8
  %call4 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 11, i64 10)
  %call5 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %2, i64 21, i64 4)
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %2) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %0) #3
  %4 = getelementptr inbounds [10 x i8], [10 x i8]* %buf6, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %4) #3
  %call8 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %4, i64 31, i64 10)
  %add.ptr11 = getelementptr inbounds [10 x i8], [10 x i8]* %buf6, i64 0, i64 4
  %5 = bitcast i8* %add.ptr11 to i32*
  %6 = bitcast i32* %rd12 to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %6) #3
  %7 = load i32, i32* %5, align 4, !tbaa !8
  store i32 %7, i32* %rd12, align 4, !tbaa !8
  %arrayidx14 = getelementptr inbounds [10 x i8], [10 x i8]* %buf6, i64 0, i64 8
  %8 = bitcast i8* %arrayidx14 to i32*
  store i32 42, i32* %8, align 4, !tbaa !8
  %call16 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %4, i64 41, i64 10)
  %call17 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %6, i64 51, i64 4)
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %6) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %4) #3
  %9 = getelementptr inbounds [10 x i8], [10 x i8]* %buf18, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %9) #3
  %call20 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %9, i64 61, i64 10)
  %add.ptr23 = getelementptr inbounds [10 x i8], [10 x i8]* %buf18, i64 0, i64 4
  %10 = bitcast i8* %add.ptr23 to i32*
  %11 = bitcast i32* %rd24 to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %11) #3
  %12 = load i32, i32* %10, align 4, !tbaa !8
  store i32 %12, i32* %rd24, align 4, !tbaa !8
  %arrayidx26 = getelementptr inbounds [10 x i8], [10 x i8]* %buf18, i64 0, i64 5
  store i8 42, i8* %arrayidx26, align 1, !tbaa !10
  %call28 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %9, i64 71, i64 10)
  %call29 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %11, i64 81, i64 4)
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %11) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %9) #3
  %13 = getelementptr inbounds [10 x i8], [10 x i8]* %buf30, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %13) #3
  %call32 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %13, i64 91, i64 10)
  %add.ptr35 = getelementptr inbounds [10 x i8], [10 x i8]* %buf30, i64 0, i64 4
  %14 = bitcast i8* %add.ptr35 to i32*
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %rd36) #3
  %arrayidx37 = getelementptr inbounds [10 x i8], [10 x i8]* %buf30, i64 0, i64 5
  %15 = load i8, i8* %arrayidx37, align 1, !tbaa !10
  store i8 %15, i8* %rd36, align 1, !tbaa !10
  store i32 42, i32* %14, align 4, !tbaa !8
  %call40 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %13, i64 101, i64 10)
  %call41 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %rd36, i64 111, i64 1)
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %rd36) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %13) #3
  ret void
}

; Function Attrs: uwtable
define dso_local void @_Z11mem_computeP16nanotube_contextP15nanotube_packet(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) #0 {
entry:
  %pre_init = alloca [10 x i8], align 2
  %packet_data = alloca [10 x i8], align 1
  %swizzled = alloca [10 x i8], align 1
  %0 = getelementptr inbounds [10 x i8], [10 x i8]* %pre_init, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %0) #3
  %1 = getelementptr inbounds [10 x i8], [10 x i8]* %packet_data, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %1) #3
  call void @llvm.memset.p0i8.i64(i8* nonnull align 2 %0, i8 42, i64 10, i1 false)
  %call = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %1, i64 1, i64 10)
  %arrayidx = getelementptr inbounds [10 x i8], [10 x i8]* %packet_data, i64 0, i64 1
  %2 = load i8, i8* %arrayidx, align 1, !tbaa !10
  %arrayidx2 = getelementptr inbounds [10 x i8], [10 x i8]* %packet_data, i64 0, i64 2
  %3 = load i8, i8* %arrayidx2, align 1, !tbaa !10
  %sub = sub i8 %2, %3
  %4 = load i8, i8* %1, align 1, !tbaa !10
  %add = add i8 %sub, %4
  store i8 %add, i8* %1, align 1, !tbaa !10
  %call8 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %1, i64 1, i64 10)
  %call10 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %0, i64 11, i64 10)
  %5 = getelementptr inbounds [10 x i8], [10 x i8]* %swizzled, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 10, i8* nonnull %5) #3
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* nonnull align 1 %5, i8* nonnull align 1 %1, i64 10, i1 false)
  %arrayidx13 = getelementptr inbounds [10 x i8], [10 x i8]* %swizzled, i64 0, i64 4
  store i8 1, i8* %arrayidx13, align 1, !tbaa !10
  %arrayidx14 = getelementptr inbounds [10 x i8], [10 x i8]* %swizzled, i64 0, i64 5
  store i8 2, i8* %arrayidx14, align 1, !tbaa !10
  %add.ptr = getelementptr inbounds [10 x i8], [10 x i8]* %swizzled, i64 0, i64 6
  %6 = bitcast [10 x i8]* %pre_init to i16*
  %7 = bitcast i8* %add.ptr to i16*
  %8 = load i16, i16* %6, align 2
  store i16 %8, i16* %7, align 1
  %call18 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %5, i64 21, i64 10)
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %5) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %1) #3
  call void @llvm.lifetime.end.p0i8(i64 10, i8* nonnull %0) #3
  ret void
}

; Function Attrs: uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 {
entry:
  tail call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str, i64 0, i64 0), void (%struct.nanotube_context*, %struct.nanotube_packet*)* nonnull @_Z10simple_memP16nanotube_contextP15nanotube_packet, i32 0, i32 1)
  tail call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.1, i64 0, i64 0), void (%struct.nanotube_context*, %struct.nanotube_packet*)* nonnull @_Z8test_warP16nanotube_contextP15nanotube_packet, i32 0, i32 1)
  tail call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @.str.2, i64 0, i64 0), void (%struct.nanotube_context*, %struct.nanotube_packet*)* nonnull @_Z11mem_computeP16nanotube_contextP15nanotube_packet, i32 0, i32 1)
  ret void
}

declare dso_local void @nanotube_add_plain_packet_kernel(i8*, void (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32) local_unnamed_addr #1

; Function Attrs: uwtable
define internal void @_GLOBAL__sub_I_simple.cc() #0 section ".text.startup" {
entry:
  tail call fastcc void @__cxx_global_var_init()
  ret void
}

attributes #0 = { uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }
attributes #4 = { argmemonly nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 "}
!2 = !{!3, !3, i64 0}
!3 = !{!"long", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C++ TBAA"}
!6 = !{!7, !7, i64 0}
!7 = !{!"short", !4, i64 0}
!8 = !{!9, !9, i64 0}
!9 = !{!"int", !4, i64 0}
!10 = !{!4, !4, i64 0}
