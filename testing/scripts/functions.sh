###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################
init_test() {
    TEST_PREFIX="$1"

    local SCRIPT_DIR_REL=$(dirname "$0")
    local TESTING_SCRIPT_DIR=$(readlink -f "${SCRIPT_DIR_REL}")
    SRC_TESTING=$(dirname "${TESTING_SCRIPT_DIR}")
    SRC_TOP=$(dirname "${SRC_TESTING}")

    BUILD_TOP="${SRC_TOP}/build"
    BUILD_TESTING="${BUILD_TOP}/testing"

    TEST_BASE=$(basename "${TEST_PREFIX}")
    local TEST_SRC_DIR_REL=$(dirname "${TEST_PREFIX}")
    TEST_SRC_DIR=$(readlink -f "${TEST_SRC_DIR_REL}")
    TEST_DIR_REL="${TEST_SRC_DIR#${SRC_TESTING}/}"
    TEST_BUILD_DIR="${BUILD_TESTING}/${TEST_DIR_REL}"
    TEST_GOLDEN_DIR="${TEST_SRC_DIR}/golden"
    TEST_GOLDEN_BUILD_DIR="${TEST_BUILD_DIR}/golden"
    TEST_OUTPUT_DIR="${TEST_BUILD_DIR}/output"
    TEST_GOLDEN_BUILD_DIR="${TEST_BUILD_DIR}/golden"

    mkdir -p "${TEST_OUTPUT_DIR}"

    GOLDEN_PREFIX="${TEST_GOLDEN_DIR}/${TEST_BASE}"
    GOLDEN_BUILD_PREFIX="${TEST_GOLDEN_BUILD_DIR}/${TEST_BASE}"
    OUTPUT_PREFIX="${TEST_OUTPUT_DIR}/${TEST_BASE}"
}

check_files() {
    local M=""
    for F in $@; do
        if ! [[ -e $F ]]; then
            echo -n "Missing input / output file"
            echo " $F"
            M="x"
        fi
    done
    [[ $M == "" ]]
}

# check_output <golden_output> <actual_output> [<filter>]
#
# Checks <golden_output> against <actual_output> and if different, prints out a
# diff between them, and exits with non-zero status.  If they are equal, prints
# "PASS" and returns with zero exit.  <filter> is an optional expression that
# can be provided to remove lines matching it before the comparison, for
# example comments.
check_output() {
    local GO=$1
    local O=$2
    local GOLDEN_TEMP=$GO
    local OUTPUT_TEMP=$O

    # If optional third paramter is given, filter away lines
    if [[ $# -ge 3 ]]; then
        local FILTER=$3
        local GOLDEN_TEMP=$(mktemp)
        local OUTPUT_TEMP=$(mktemp)
        grep -v "$FILTER" < $GO > $GOLDEN_TEMP
        grep -v "$FILTER" < $O  > $OUTPUT_TEMP
    fi

    echo -n "  Checking output $GO.. "
    local RET=0
    if diff -q $GOLDEN_TEMP $OUTPUT_TEMP > /dev/null; then
        echo "PASS."
    else
        echo "FAIL. Diff:"
        diff -u $GOLDEN_TEMP $OUTPUT_TEMP | sed 's/^/  |  /'
        echo "  Output: $O"
        RET=1
    fi
    if [[ $GO != $GOLDEN_TEMP ]]; then
        rm $GOLDEN_TEMP $OUTPUT_TEMP
    fi
    return $RET
}
