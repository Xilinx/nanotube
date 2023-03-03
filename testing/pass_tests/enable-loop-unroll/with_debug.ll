;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ModuleID = 'foo.c'
source_filename = "foo.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local i32 @foo(i32 %n) local_unnamed_addr #0 !dbg !7 {
entry:
  call void @llvm.dbg.value(metadata i32 %n, metadata !12, metadata !DIExpression()), !dbg !16
  call void @llvm.dbg.value(metadata i32 0, metadata !13, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !14, metadata !DIExpression()), !dbg !18
  %cmp6 = icmp sgt i32 %n, 0, !dbg !19
  br i1 %cmp6, label %for.body, label %for.cond.cleanup, !dbg !21

for.cond.cleanup:                                 ; preds = %for.body, %entry
  %r.0.lcssa = phi i32 [ 0, %entry ], [ %add, %for.body ], !dbg !22
  call void @llvm.dbg.value(metadata i32 %r.0.lcssa, metadata !13, metadata !DIExpression()), !dbg !17
  ret i32 %r.0.lcssa, !dbg !23

for.body:                                         ; preds = %entry, %for.body
  %i.08 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %r.07 = phi i32 [ %add, %for.body ], [ 0, %entry ]
  call void @llvm.dbg.value(metadata i32 %i.08, metadata !14, metadata !DIExpression()), !dbg !18
  call void @llvm.dbg.value(metadata i32 %r.07, metadata !13, metadata !DIExpression()), !dbg !17
  %call = tail call i32 @bar(i32 %i.08) #3, !dbg !24
  %add = add nsw i32 %call, %r.07, !dbg !26
  %inc = add nuw nsw i32 %i.08, 1, !dbg !27
  call void @llvm.dbg.value(metadata i32 %inc, metadata !14, metadata !DIExpression()), !dbg !18
  call void @llvm.dbg.value(metadata i32 %add, metadata !13, metadata !DIExpression()), !dbg !17
  %exitcond = icmp eq i32 %inc, %n, !dbg !19
  br i1 %exitcond, label %for.cond.cleanup, label %for.body, !dbg !21, !llvm.loop !28
}

declare dso_local i32 @bar(i32) local_unnamed_addr #1

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone speculatable }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 8.0.0 ", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, nameTableKind: None)
!1 = !DIFile(filename: "foo.c", directory: "/home/neilt/project/nanotube")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"clang version 8.0.0 "}
!7 = distinct !DISubprogram(name: "foo", scope: !1, file: !1, line: 3, type: !8, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!8 = !DISubroutineType(types: !9)
!9 = !{!10, !10}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !{!12, !13, !14}
!12 = !DILocalVariable(name: "n", arg: 1, scope: !7, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "r", scope: !7, file: !1, line: 5, type: !10)
!14 = !DILocalVariable(name: "i", scope: !15, file: !1, line: 7, type: !10)
!15 = distinct !DILexicalBlock(scope: !7, file: !1, line: 7, column: 3)
!16 = !DILocation(line: 3, column: 13, scope: !7)
!17 = !DILocation(line: 5, column: 7, scope: !7)
!18 = !DILocation(line: 7, column: 12, scope: !15)
!19 = !DILocation(line: 7, column: 18, scope: !20)
!20 = distinct !DILexicalBlock(scope: !15, file: !1, line: 7, column: 3)
!21 = !DILocation(line: 7, column: 3, scope: !15)
!22 = !DILocation(line: 0, scope: !7)
!23 = !DILocation(line: 10, column: 3, scope: !7)
!24 = !DILocation(line: 8, column: 10, scope: !25)
!25 = distinct !DILexicalBlock(scope: !20, file: !1, line: 7, column: 27)
!26 = !DILocation(line: 8, column: 7, scope: !25)
!27 = !DILocation(line: 7, column: 23, scope: !20)
!28 = distinct !{!28, !21, !29, !30}
!29 = !DILocation(line: 9, column: 3, scope: !15)
!30 = !{!"llvm.loop.unroll.disable"}
