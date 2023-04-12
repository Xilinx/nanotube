###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################

# Extract paths from the script being called and the provided test files
# Assumptions:
#   - the build directory lives at a fixed offset from the calling script
#   - golden data that was built and the test executable will live in a
#     subdirectory of the build directory
#   - the source directory is given with the test name
#   - golden data that does not need building will live in a fixed subdirectory
#     of the source directory
# Input: relative path to test source file and compiler passes
# Output:
#   TEST_PREFIX .. the relative path to the test source and compiler passes
#   SRC_TESTING & SRC_TOP .. point to the Nanotube repo testing & root dir
#   BUILD_TESTING & BUILD_TOP .. point to the build directory testing & root
#   TEST_BASE .. the test name
#   TEST_SRC_DIR .. (relative) path to the source directory of the test
#   TEST_DIR_REL .. relative location of the test to the testing/ directory
#   TEST_BUILD_DIR .. (relative) path to the build subdir for the specific test
#   TEST_GOLDEN_BUILD_DIR, TEST_OUTPUT_DIR .. per-test build subdirectories for
#                                             output and built golden data
#   TEST_GOLDEN_DIR .. directory containing the unbuilt golden files
#   GOLDEN_PREFIX .. prefix (path and name) for unbuilt per-test golden data
#   GOLDEN_BUILD_PREFIX .. prefix (path and name) for built per-test golden
#                          data
#   OUTPUT_PREFIX .. prefix (path and name) for per-test output
init_test() {
    TEST_PREFIX="$1"

    # the calling script lives in <repo>/testing/scripts
    # derive from that the build file location
    local SCRIPT_DIR_REL=$(dirname "$0")
    SRC_TESTING=$(dirname "${SCRIPT_DIR_REL}")
    SRC_TOP=$(dirname "${SRC_TESTING}")
    local BUILD_TOP_DOT="${SRC_TOP}/build"
    BUILD_TOP="${BUILD_TOP_DOT##./}"
    BUILD_TESTING="${BUILD_TOP}/testing"

    # the test source livs in <test_repo>/testing/*_tests/**/<test_source>
    # and TEST_PREFIX points to that; extract the location of the source
    TEST_BASE=$(basename "${TEST_PREFIX}")
    TEST_SRC_DIR="$(dirname ${TEST_PREFIX})"
    # strip either "testing/" from the start, or the longest match up to and
    # including /testing/"
    TEST_DIR_REL="$(echo $TEST_SRC_DIR | sed 's#^\(\|.*/\)testing/##')"

    # The test executable, built golden files, and the output working directory
    # live relative from the build directory
    TEST_BUILD_DIR="${BUILD_TESTING}/${TEST_DIR_REL}"
    TEST_GOLDEN_BUILD_DIR="${TEST_BUILD_DIR}/golden"
    TEST_OUTPUT_DIR="${TEST_BUILD_DIR}/output"

    # golden files that didn't need building live in the source directory
    TEST_GOLDEN_DIR="${TEST_SRC_DIR}/golden"

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
