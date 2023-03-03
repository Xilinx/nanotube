#! /bin/bash
###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################
set -u

for C in 1 2 ;
do
  echo "Running case $C"
  "$1"/test_array_maps -c "$C" 2>&1
  echo "Exit code $?"
done
exit 0

