#!/bin/bash
# Execute Nanotube and LLVM steps to move code from an EBPF interface through
# the full Nanotube comiler flow.  For now, this is executable docuemntation,
# and should over time be merged into the respective tools.
#
###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################

set -eu

NT_BUILD_DIR=../../build
NT_OPT=${NT_BUILD_DIR}/nanotube_opt
NT_BE=${NT_BUILD_DIR}/nanotube_back_end
LOCATE_TOOL=../../scripts/locate_tool
KATRAN=../../external/katran
BALANCER_KERN=${KATRAN}/katran/lib/bpf/balancer_kern.c

expect() {
  THING=$1
  BASE=$(basename $THING)
  CODE=$2
  if [[ -z $THING || ! -e $THING ]]; then
    echo "Expecting $BASE in $THING but not finding it.  Please edit "\
         "the script or put $BASE in the right place."
    exit $CODE
  fi
}

expect $NT_OPT 2
expect $LOCATE_TOOL 3
expect $KATRAN 4
expect $BALANCER_KERN 5


LLVM_LINK="${LOCATE_TOOL} --run llvm-link"
CLANG="${LOCATE_TOOL} --run clang"
INFILE="balancer_kern.O3.bc"

# Build Katran packet kernel
$CLANG  -O3 -I $KATRAN/katran/lib/linux_includes \
            -Wno-unused-value -Wno-pointer-sign \
            -Wno-compare-distinct-pointer-types \
            -fno-vectorize -fno-slp-vectorize \
            -D NANOTUBE_SIMPLE \
            $BALANCER_KERN \
            -c -emit-llvm \
            -o $INFILE

# Front-end: Translate from EBPF to the high-level Nanotube interface
FEFILE=${INFILE/.bc/.nt.bc}
echo "Front-end: Converting EBPF to Nanotube; $INFILE to $FEFILE"
$NT_OPT -ebpf2nanotube $INFILE -o $FEFILE

F=$FEFILE
NEWF=${F/.bc/.req.bc}
echo "Back-end: Converting loads / stores to Nanotube requests; $F to $NEWF"
$NT_OPT -mem2req $F -o $NEWF

F=$NEWF
NEWF=${F/.bc/.lower.bc}
echo "back-end: Lowering the Nanotube interface closer to hardware; "\
     "$F to $NEWF"
$LLVM_LINK -o $NEWF $F \
           ${NT_BUILD_DIR}/libnt/nanotube_high_level.bc

F=$NEWF
NEWF=${F/.bc/.inline.bc}
echo "Front-end: Inlining adapter functions; $F to $NEWF"
$NT_OPT -always-inline -constprop $F -o $NEWF

F=$NEWF
NEWF=${F/.bc/.platform.bc}
echo "Front-end: Performing platform conversion; $F to $NEWF"
$NT_OPT -platform -always-inline -constprop $F -o $NEWF

F=$NEWF
NEWF=${F/.bc/.optreq.bc}
echo "back-end: Optimising packet accesses; $F to $NEWF"
$NT_OPT -nt-attributes -O2 -optreq -enable-loop-unroll -always-inline -constprop -loop-unroll -simplifycfg $F -o $NEWF

F=$NEWF
NEWF=${F/.bc/.converge.bc}
echo "back-end: Converging the control flow; $F to $NEWF"
$NT_OPT -compact-geps -converge_mapa $F -o $NEWF

F=$NEWF
NEWF=${F/.bc/.pipeline.bc}
echo "back-end: Splitting code into pipeline stages; $F to $NEWF"
$NT_OPT -compact-geps -basicaa -tbaa -nanotube-aa -pipeline $F -o $NEWF

F=$NEWF
NEWF=${F/.bc/.link_taps.bc}
echo "back-end: Link the map/packet taps; $F to $NEWF"
$LLVM_LINK -o $NEWF $F \
           ${NT_BUILD_DIR}/libnt/nanotube_low_level.bc

F=$NEWF
NEWF=${F/.bc/.inline_opt.bc}
echo "back-end: Inline the map/packet taps; $F to $NEWF"
$NT_OPT $F -o $NEWF \
         -always-inline \
         -rewrite-setup -replace-malloc \
         -thread-const -constprop \
         -enable-loop-unroll -loop-unroll \
         -move-alloca -simplifycfg -instcombine \
         -thread-const -constprop -simplifycfg

F=$NEWF
NEWF=${F/.bc/.hls}
echo "back-end: Writing out HLS C; $F to $NEWF"
$NT_BE $F -o $NEWF --overwrite
