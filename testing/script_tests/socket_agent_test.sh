#!/bin/bash
###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################
set -eu
SRC_TOP="$1"
SRC_TESTING_DIR="$2"
BUILD_TESTING_DIR="$3"

TEST_NAME="test_ip_tunnel"
TEST_GOLDEN_DIR="$SRC_TESTING_DIR/kernel_tests/golden"
TEST_EXE_DIR="$BUILD_TESTING_DIR/kernel_tests"
TEST_EXE="$TEST_EXE_DIR/$TEST_NAME"

"$SRC_TOP/scripts/socket_test" -t 1s -c \
  -i "$TEST_GOLDEN_DIR/$TEST_NAME.packets.IN" \
  -e "$TEST_GOLDEN_DIR/$TEST_NAME.packets.OUT" \
  "$TEST_EXE"
exit 0
