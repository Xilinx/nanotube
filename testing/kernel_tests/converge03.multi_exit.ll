; ModuleID = 'build/testing/kernel_tests/converge03.bc'
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/kernel_tests/converge03.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.nanotube_context = type opaque
%struct.nanotube_packet = type opaque

@.str = private unnamed_addr constant [15 x i8] c"process_packet\00", align 1
@.str.1 = private unnamed_addr constant [11 x i8] c"multi_exit\00", align 1

; Function Attrs: nounwind uwtable
define dso_local i32 @process_packet(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) #0 !dbg !7 {
entry:
  %decision = alloca i8, align 1
  %data = alloca i8, align 1
  call void @llvm.dbg.value(metadata %struct.nanotube_context* undef, metadata !19, metadata !DIExpression()), !dbg !28
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !20, metadata !DIExpression()), !dbg !29
  call void @llvm.dbg.value(metadata i8* %decision, metadata !21, metadata !DIExpression(DW_OP_deref)), !dbg !30
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %decision) #4, !dbg !31
  call void @llvm.dbg.value(metadata i8* %data, metadata !27, metadata !DIExpression(DW_OP_deref)), !dbg !32
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %data) #4, !dbg !33
  call void @llvm.dbg.value(metadata i8* %decision, metadata !21, metadata !DIExpression(DW_OP_deref)), !dbg !30
  %call = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %decision, i64 0, i64 1) #4, !dbg !34
  call void @llvm.dbg.value(metadata i8* %data, metadata !27, metadata !DIExpression(DW_OP_deref)), !dbg !32
  %call1 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %data, i64 1, i64 1) #4, !dbg !35
  %0 = load i8, i8* %decision, align 1, !dbg !36, !tbaa !38
  call void @llvm.dbg.value(metadata i8 %0, metadata !21, metadata !DIExpression()), !dbg !30
  %cmp = icmp eq i8 %0, 127, !dbg !41
  br i1 %cmp, label %if.then, label %if.else, !dbg !42

if.then:                                          ; preds = %entry
  call void @llvm.dbg.value(metadata i8* %data, metadata !27, metadata !DIExpression(DW_OP_deref)), !dbg !32
  %call3 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %data, i64 2, i64 1) #4, !dbg !43
  %.pre = load i8, i8* %data, align 1, !dbg !45, !tbaa !38
  %phitmp = add i8 %.pre, 1, !dbg !46
  br label %if.end11, !dbg !46

if.else:                                          ; preds = %entry
  %conv = zext i8 %0 to i32, !dbg !36
  %1 = load i8, i8* %data, align 1, !dbg !47, !tbaa !38
  call void @llvm.dbg.value(metadata i8 %1, metadata !27, metadata !DIExpression()), !dbg !32
  %conv4 = zext i8 %1 to i32, !dbg !47
  %add = add nuw nsw i32 %conv4, %conv, !dbg !50
  %cmp6 = icmp eq i32 %add, 127, !dbg !51
  br i1 %cmp6, label %if.else9, label %if.then8, !dbg !52

if.then8:                                         ; preds = %if.else
  call void @llvm.dbg.value(metadata i8 -1, metadata !27, metadata !DIExpression()), !dbg !32
  store i8 -1, i8* %data, align 1, !dbg !53, !tbaa !38
  br label %if.end11

if.else9:                                         ; preds = %if.else
  call void @llvm.dbg.value(metadata i8* %data, metadata !27, metadata !DIExpression(DW_OP_deref)), !dbg !32
  %call10 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %data, i64 3, i64 1) #4, !dbg !55
  br label %D, !dbg !57

if.end11:                                         ; preds = %if.then8, %if.then
  %2 = phi i8 [ 0, %if.then8 ], [ %phitmp, %if.then ]
  call void @llvm.dbg.value(metadata i8 %2, metadata !27, metadata !DIExpression()), !dbg !32
  store i8 %2, i8* %data, align 1, !dbg !45, !tbaa !38
  br label %D, !dbg !58

D:                                                ; preds = %if.end11, %if.else9
  call void @llvm.dbg.value(metadata i8* %data, metadata !27, metadata !DIExpression(DW_OP_deref)), !dbg !32
  %call12 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %data, i64 4, i64 1) #4, !dbg !59
  call void @llvm.dbg.value(metadata i8* %data, metadata !27, metadata !DIExpression(DW_OP_deref)), !dbg !32
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %data) #4, !dbg !61
  call void @llvm.dbg.value(metadata i8* %decision, metadata !21, metadata !DIExpression(DW_OP_deref)), !dbg !30
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %decision) #4, !dbg !61
  ret i32 0, !dbg !62
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #1

declare dso_local i64 @nanotube_packet_read(%struct.nanotube_packet*, i8*, i64, i64) local_unnamed_addr #2

declare dso_local i64 @nanotube_packet_write(%struct.nanotube_packet*, i8*, i64, i64) local_unnamed_addr #2

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #1

; Function Attrs: nounwind uwtable
define dso_local i32 @multi_exit(%struct.nanotube_context* nocapture readnone %nt_ctx, %struct.nanotube_packet* %packet) #0 !dbg !63 {
entry:
  %decision = alloca i8, align 1
  %data = alloca i8, align 1
  call void @llvm.dbg.value(metadata %struct.nanotube_context* undef, metadata !65, metadata !DIExpression()), !dbg !69
  call void @llvm.dbg.value(metadata %struct.nanotube_packet* %packet, metadata !66, metadata !DIExpression()), !dbg !70
  call void @llvm.dbg.value(metadata i8* %decision, metadata !67, metadata !DIExpression(DW_OP_deref)), !dbg !71
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %decision) #4, !dbg !72
  call void @llvm.dbg.value(metadata i8* %data, metadata !68, metadata !DIExpression(DW_OP_deref)), !dbg !73
  call void @llvm.lifetime.start.p0i8(i64 1, i8* nonnull %data) #4, !dbg !74
  call void @llvm.dbg.value(metadata i8* %decision, metadata !67, metadata !DIExpression(DW_OP_deref)), !dbg !71
  %call = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %decision, i64 0, i64 1) #4, !dbg !75
  call void @llvm.dbg.value(metadata i8* %data, metadata !68, metadata !DIExpression(DW_OP_deref)), !dbg !73
  %call1 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %data, i64 1, i64 1) #4, !dbg !76
  %0 = load i8, i8* %decision, align 1, !dbg !77, !tbaa !38
  call void @llvm.dbg.value(metadata i8 %0, metadata !67, metadata !DIExpression()), !dbg !71
  %cmp = icmp eq i8 %0, 127, !dbg !79
  br i1 %cmp, label %if.then, label %if.end, !dbg !80

if.then:                                          ; preds = %entry
  call void @llvm.dbg.value(metadata i8* %data, metadata !68, metadata !DIExpression(DW_OP_deref)), !dbg !73
  %call3 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %data, i64 2, i64 1) #4, !dbg !81
  %1 = load i8, i8* %data, align 1, !dbg !83, !tbaa !38
  call void @llvm.dbg.value(metadata i8 %1, metadata !68, metadata !DIExpression()), !dbg !73
  %inc = add i8 %1, 1, !dbg !83
  call void @llvm.dbg.value(metadata i8 %inc, metadata !68, metadata !DIExpression()), !dbg !73
  store i8 %inc, i8* %data, align 1, !dbg !83, !tbaa !38
  call void @llvm.dbg.value(metadata i8* %data, metadata !68, metadata !DIExpression(DW_OP_deref)), !dbg !73
  %call4 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %data, i64 4, i64 1) #4, !dbg !84
  ret i32 1, !dbg !86

if.end:                                           ; preds = %entry
  %conv = zext i8 %0 to i32, !dbg !77
  %2 = load i8, i8* %data, align 1, !dbg !87, !tbaa !38
  call void @llvm.dbg.value(metadata i8 %2, metadata !68, metadata !DIExpression()), !dbg !73
  %conv5 = zext i8 %2 to i32, !dbg !87
  %add = add nuw nsw i32 %conv5, %conv, !dbg !89
  %cmp7 = icmp eq i32 %add, 127, !dbg !90
  br i1 %cmp7, label %if.else, label %if.then9, !dbg !91

if.then9:                                         ; preds = %if.end
  call void @llvm.dbg.value(metadata i8 0, metadata !68, metadata !DIExpression()), !dbg !73
  store i8 0, i8* %data, align 1, !dbg !92, !tbaa !38
  br label %D, !dbg !93

if.else:                                          ; preds = %if.end
  call void @llvm.dbg.value(metadata i8* %data, metadata !68, metadata !DIExpression(DW_OP_deref)), !dbg !73
  %call10 = call i64 @nanotube_packet_read(%struct.nanotube_packet* %packet, i8* nonnull %data, i64 3, i64 1) #4, !dbg !94
  br label %D, !dbg !96

D:                                                ; preds = %if.then9, %if.else
  call void @llvm.dbg.value(metadata i8* %data, metadata !68, metadata !DIExpression(DW_OP_deref)), !dbg !73
  %call13 = call i64 @nanotube_packet_write(%struct.nanotube_packet* %packet, i8* nonnull %data, i64 4, i64 1) #4, !dbg !97
  br label %cleanup, !dbg !99

cleanup:                                          ; preds = %D, %if.then
  call void @llvm.dbg.value(metadata i8* %data, metadata !68, metadata !DIExpression(DW_OP_deref)), !dbg !73
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %data) #4, !dbg !101
  call void @llvm.dbg.value(metadata i8* %decision, metadata !67, metadata !DIExpression(DW_OP_deref)), !dbg !71
  call void @llvm.lifetime.end.p0i8(i64 1, i8* nonnull %decision) #4, !dbg !101
  ret i32 0, !dbg !101
}

; Function Attrs: nounwind uwtable
define dso_local void @nanotube_setup() local_unnamed_addr #0 !dbg !102 {
entry:
  tail call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([15 x i8], [15 x i8]* @.str, i64 0, i64 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* nonnull @process_packet, i32 -1, i32 0) #4, !dbg !105
  tail call void @nanotube_add_plain_packet_kernel(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.1, i64 0, i64 0), i32 (%struct.nanotube_context*, %struct.nanotube_packet*)* nonnull @multi_exit, i32 -1, i32 0) #4, !dbg !106
  ret void, !dbg !107
}

declare dso_local void @nanotube_add_plain_packet_kernel(i8*, i32 (%struct.nanotube_context*, %struct.nanotube_packet*)*, i32, i32) local_unnamed_addr #2

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #3

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind readnone speculatable }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 8.0.0 ", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, nameTableKind: None)
!1 = !DIFile(filename: "testing/kernel_tests/converge03.c", directory: "/home/stephand/20-Projects/2019-12_eBPF_proto/nanotube")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"clang version 8.0.0 "}
!7 = distinct !DISubprogram(name: "process_packet", scope: !1, file: !1, line: 10, type: !8, scopeLine: 12, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !18)
!8 = !DISubroutineType(types: !9)
!9 = !{!10, !11, !15}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64)
!12 = !DIDerivedType(tag: DW_TAG_typedef, name: "nanotube_context_t", file: !13, line: 72, baseType: !14)
!13 = !DIFile(filename: "include/nanotube_api.h", directory: "/home/stephand/20-Projects/2019-12_eBPF_proto/nanotube")
!14 = !DICompositeType(tag: DW_TAG_structure_type, name: "nanotube_context", file: !13, line: 72, flags: DIFlagFwdDecl)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64)
!16 = !DIDerivedType(tag: DW_TAG_typedef, name: "nanotube_packet_t", file: !13, line: 74, baseType: !17)
!17 = !DICompositeType(tag: DW_TAG_structure_type, name: "nanotube_packet", file: !13, line: 74, flags: DIFlagFwdDecl)
!18 = !{!19, !20, !21, !27}
!19 = !DILocalVariable(name: "nt_ctx", arg: 1, scope: !7, file: !1, line: 10, type: !11)
!20 = !DILocalVariable(name: "packet", arg: 2, scope: !7, file: !1, line: 11, type: !15)
!21 = !DILocalVariable(name: "decision", scope: !7, file: !1, line: 28, type: !22)
!22 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint8_t", file: !23, line: 24, baseType: !24)
!23 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/stdint-uintn.h", directory: "")
!24 = !DIDerivedType(tag: DW_TAG_typedef, name: "__uint8_t", file: !25, line: 37, baseType: !26)
!25 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types.h", directory: "")
!26 = !DIBasicType(name: "unsigned char", size: 8, encoding: DW_ATE_unsigned_char)
!27 = !DILocalVariable(name: "data", scope: !7, file: !1, line: 29, type: !22)
!28 = !DILocation(line: 10, column: 40, scope: !7)
!29 = !DILocation(line: 11, column: 40, scope: !7)
!30 = !DILocation(line: 28, column: 13, scope: !7)
!31 = !DILocation(line: 28, column: 5, scope: !7)
!32 = !DILocation(line: 29, column: 13, scope: !7)
!33 = !DILocation(line: 29, column: 5, scope: !7)
!34 = !DILocation(line: 30, column: 5, scope: !7)
!35 = !DILocation(line: 31, column: 5, scope: !7)
!36 = !DILocation(line: 34, column: 9, scope: !37)
!37 = distinct !DILexicalBlock(scope: !7, file: !1, line: 34, column: 9)
!38 = !{!39, !39, i64 0}
!39 = !{!"omnipotent char", !40, i64 0}
!40 = !{!"Simple C/C++ TBAA"}
!41 = !DILocation(line: 34, column: 18, scope: !37)
!42 = !DILocation(line: 34, column: 9, scope: !7)
!43 = !DILocation(line: 36, column: 9, scope: !44)
!44 = distinct !DILexicalBlock(scope: !37, file: !1, line: 34, column: 27)
!45 = !DILocation(line: 49, column: 9, scope: !7)
!46 = !DILocation(line: 38, column: 5, scope: !44)
!47 = !DILocation(line: 40, column: 13, scope: !48)
!48 = distinct !DILexicalBlock(scope: !49, file: !1, line: 40, column: 13)
!49 = distinct !DILexicalBlock(scope: !37, file: !1, line: 38, column: 12)
!50 = !DILocation(line: 40, column: 18, scope: !48)
!51 = !DILocation(line: 40, column: 29, scope: !48)
!52 = !DILocation(line: 40, column: 13, scope: !49)
!53 = !DILocation(line: 41, column: 18, scope: !54)
!54 = distinct !DILexicalBlock(scope: !48, file: !1, line: 40, column: 38)
!55 = !DILocation(line: 44, column: 13, scope: !56)
!56 = distinct !DILexicalBlock(scope: !48, file: !1, line: 43, column: 16)
!57 = !DILocation(line: 45, column: 13, scope: !56)
!58 = !DILocation(line: 49, column: 5, scope: !7)
!59 = !DILocation(line: 51, column: 5, scope: !7)
!60 = !DILocation(line: 53, column: 5, scope: !7)
!61 = !DILocation(line: 55, column: 1, scope: !7)
!62 = !DILocation(line: 54, column: 5, scope: !7)
!63 = distinct !DISubprogram(name: "multi_exit", scope: !1, file: !1, line: 57, type: !8, scopeLine: 59, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !64)
!64 = !{!65, !66, !67, !68}
!65 = !DILocalVariable(name: "nt_ctx", arg: 1, scope: !63, file: !1, line: 57, type: !11)
!66 = !DILocalVariable(name: "packet", arg: 2, scope: !63, file: !1, line: 58, type: !15)
!67 = !DILocalVariable(name: "decision", scope: !63, file: !1, line: 61, type: !22)
!68 = !DILocalVariable(name: "data", scope: !63, file: !1, line: 62, type: !22)
!69 = !DILocation(line: 57, column: 36, scope: !63)
!70 = !DILocation(line: 58, column: 36, scope: !63)
!71 = !DILocation(line: 61, column: 13, scope: !63)
!72 = !DILocation(line: 61, column: 5, scope: !63)
!73 = !DILocation(line: 62, column: 13, scope: !63)
!74 = !DILocation(line: 62, column: 5, scope: !63)
!75 = !DILocation(line: 63, column: 5, scope: !63)
!76 = !DILocation(line: 64, column: 5, scope: !63)
!77 = !DILocation(line: 66, column: 9, scope: !78)
!78 = distinct !DILexicalBlock(scope: !63, file: !1, line: 66, column: 9)
!79 = !DILocation(line: 66, column: 18, scope: !78)
!80 = !DILocation(line: 66, column: 9, scope: !63)
!81 = !DILocation(line: 67, column: 9, scope: !82)
!82 = distinct !DILexicalBlock(scope: !78, file: !1, line: 66, column: 27)
!83 = !DILocation(line: 68, column: 13, scope: !82)
!84 = !DILocation(line: 69, column: 9, scope: !82)
!85 = !DILocation(line: 70, column: 9, scope: !82)
!86 = !DILocation(line: 71, column: 9, scope: !82)
!87 = !DILocation(line: 74, column: 9, scope: !88)
!88 = distinct !DILexicalBlock(scope: !63, file: !1, line: 74, column: 9)
!89 = !DILocation(line: 74, column: 14, scope: !88)
!90 = !DILocation(line: 74, column: 25, scope: !88)
!91 = !DILocation(line: 74, column: 9, scope: !63)
!92 = !DILocation(line: 80, column: 9, scope: !63)
!93 = !DILocation(line: 80, column: 5, scope: !63)
!94 = !DILocation(line: 77, column: 9, scope: !95)
!95 = distinct !DILexicalBlock(scope: !88, file: !1, line: 76, column: 12)
!96 = !DILocation(line: 78, column: 9, scope: !95)
!97 = !DILocation(line: 82, column: 5, scope: !63)
!98 = !DILocation(line: 83, column: 5, scope: !63)
!99 = !DILocation(line: 84, column: 5, scope: !63)
!100 = !DILocation(line: 0, scope: !63)
!101 = !DILocation(line: 85, column: 1, scope: !63)
!102 = distinct !DISubprogram(name: "nanotube_setup", scope: !1, file: !1, line: 87, type: !103, scopeLine: 87, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !2)
!103 = !DISubroutineType(types: !104)
!104 = !{null}
!105 = !DILocation(line: 88, column: 3, scope: !102)
!106 = !DILocation(line: 89, column: 3, scope: !102)
!107 = !DILocation(line: 90, column: 1, scope: !102)
