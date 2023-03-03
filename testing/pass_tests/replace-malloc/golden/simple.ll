;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/replace-malloc/simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@buf0 = local_unnamed_addr global i8* null, align 8
@buf1 = local_unnamed_addr global i8* null, align 8
@nanotube_malloc_buf = global [32 x i8] zeroinitializer
@nanotube_malloc_buf.1 = global [48 x i8] zeroinitializer

define void @nanotube_setup() {
entry:
  store i8* getelementptr inbounds ([32 x i8], [32 x i8]* @nanotube_malloc_buf, i8 0, i8 0), i8** @buf0, align 8
  store i8* getelementptr inbounds ([48 x i8], [48 x i8]* @nanotube_malloc_buf.1, i8 0, i8 0), i8** @buf1, align 8
  ret void
}

declare i8* @nanotube_malloc(i64)
