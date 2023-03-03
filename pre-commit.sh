#!/bin/bash
#
###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################
#
# HOWTO: ln -s ../../pre-commit.sh .git/hooks/pre-commit
#        or simply call this from the existing pre-commit hook

echo "GIT Pre-Commit Hook.  Running tests..."
git stash -q --keep-index
make run_tests
RES=$?
git stash pop -q
exit $RES
