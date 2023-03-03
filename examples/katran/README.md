# Overview

Katran is a high-performance layer 4 network load balancer created by Facebook.
Its eBPF code can be used to illustrate the function of the Nanotube compiler.

Katran is released under GPL v2 and copyrighted by Facebook.

# Preparing Katran

After building Nanotube, the Katran load-balancer can be converted to a packet
processing pipeline.  We have tested this with a specific version of Katran and
provide a small patch on top.  The following steps check out Katran from
upstream, patch it, and process it through the Nanotube compiler (assuming you
are in the examples/katran directory):

    git clone https://github.com/facebookincubator/katran.git ../../external/katran

You can checkout the right version and apply a small patch as follows:

    git -C ../../external/katran checkout 6fd5b8
    patch -d ../../external/katran -p1 < katran_nanotube_patch.diff

# Compiling Katran with Nanotube

Make sure the Nanotube compiler framework is built correctly (see the main
README.md), and you have prepared the source code as described in the previous
section.  Invoking the various compilation steps can then be done by:

    ./nanotube_steps.sh

The resulting packet processing pipeline can be found in a subdirectory:

    ls balancer_kern.*.hls

And various intermediate compilation steps are created as LLVM IR files:

    ls *.bc

