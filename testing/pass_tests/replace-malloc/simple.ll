; \author  Neil Turton <neilt@amd.com>
;  \brief  A test for replacing calls to nanotube_malloc.
;   \date  2021-07-19
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: MIT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
source_filename = "testing/pass_tests/replace-malloc/simple.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@buf0 = local_unnamed_addr global i8* null, align 8
@buf1 = local_unnamed_addr global i8* null, align 8

define void @nanotube_setup() {
entry:
  %buf0 = tail call i8* @nanotube_malloc(i64 32)
  store i8* %buf0, i8** @buf0, align 8
  %buf1 = tail call i8* @nanotube_malloc(i64 48)
  store i8* %buf1, i8** @buf1, align 8
  ret void
}

declare i8* @nanotube_malloc(i64)
