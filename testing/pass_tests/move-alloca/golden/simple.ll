;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/move-alloca/simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define void @foo() {
entry:
  %vla = alloca i32, i64 16, align 16
  %call = tail call i32 @baz()
  %tobool = icmp eq i32 %call, 0
  br i1 %tobool, label %while.body, label %L1

while.body:                                       ; preds = %if.else, %if.then7, %entry
  %alloc0 = alloca i32, i64 1, align 16
  %call1 = call i32 @baz()
  br label %L1

L1:                                               ; preds = %while.body, %entry
  %call2 = call i32 @baz()
  %cmp = icmp eq i32 %call2, 0
  br i1 %cmp, label %while.end.critedge, label %if.end4

if.end4:                                          ; preds = %L1
  %call5 = call i32 @baz()
  %cmp6 = icmp eq i32 %call2, 1
  %0 = zext i32 %call5 to i64
  %1 = call i8* @llvm.stacksave()
  br i1 %cmp6, label %if.then7, label %if.else

if.then7:                                         ; preds = %if.end4
  %2 = bitcast i32* %vla to i8*
  %call8 = call i32 @bar(i8* nonnull %2)
  call void @llvm.stackrestore(i8* %1)
  br label %while.body

if.else:                                          ; preds = %if.end4
  %vla10 = alloca i8, i64 %0, align 16
  %call11 = call i32 @bar(i8* nonnull %vla10)
  call void @llvm.stackrestore(i8* %1)
  br label %while.body

while.end.critedge:                               ; preds = %L1
  ret void
}

; Function Attrs: nounwind
declare void @llvm.stackrestore(i8*) #0

; Function Attrs: nounwind
declare i8* @llvm.stacksave() #0

declare i32 @baz()

declare i32 @bar(i8*)

attributes #0 = { nounwind }
