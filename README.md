# Copyright and License

Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: MIT

# The Nanotube Compiler and Framework

Nanotube is a collection of compiler passes, libraries, and an API to
facilitate execution of EBPF XDP and similar networking code on an FPGA in a
SmartNIC.  The compiler takes EBPF XDP C code as input and outputs a packet
processing pipeline in HLS C++.  This HLS C++ code can then be synthesised
using Vitis HLS and placed on an FPGA.

The compiler performs various transformations on the program; starting with a
translation of EBPF calls to calls to similar Nanotube API functions.  It then
performs multiple stages of transforming the code structurally and to different
API levels:

- mem2req: Converts C style pointer accesses (loads & stores) to explicit
  accesses to map and packet data
- optreq: combines adjacent map / packet accesses into fewer wider accesses
- converge: straightens the control-flow graph around Nanotube API calls
- pipeline: splits the single packet processing function into multiple
  coarse-grained pipeline stages, and change the application logic to process
  packet words flowing through, rather than a flat packet representation in
  memory
- hls: creates HLS C++ code from LLVM IR for synthesis with Vitis HLS

The Nanotube library implements packet accesses and maps in an implementation
which is synthesis friendly, meaning that it will be placed in the application
and will create efficient hardware in high-level synthesis.

# Getting started

Before compiling Nanotube, you need to make sure all the dependencies
are available.  Many of them are available as packages in common Linux
distributions.  The others are LLVM and Vitis-HLS.

You will need enough free storage space for Nanotube and its
dependencies.  Using local storage will be faster, but network storage
can be used if preferred.

Building Nanotube itself requires only the headers from Vitis-HLS and
that is enough to test the HLS output of Nanotube.  Generating Verilog
from the HLS output and building bitfiles requires full installations
of Vitis-HLS and Vivado.  The Nanotube build system will attempt to
locate installations of Vitis-HLS and Vivado on the legacy Xilinx
network.  If this fails, it will be necessary to provide the path to a
directory which contains either a full Vitis-HLS install or just
include sub-directory.

Approximate sizes for a minimal installation are:

      LLVM              24.2G
      Vitis-HLS headers  5.6M
      Nanotube           1.9G

A full installation will require the following instead of the
Vitis-HLS headers:

      Vitis-HLS full     6.5G
      Vivado            81.8G

## Installing Linux packages

Every Linux distribution has a different set of available packages and
package names vary between distributions.  To make things easier,
instructions are given here for some common distributions.  It may be
possible to compile Nanotube on other distributions using similar
instructions, but there may be missing packages or other problems.

### Installing packages on Ubuntu 20.04

To install the require packages, run these commands as root:

      apt-get install -y cmake g++ m4 libpcap-dev wireshark scons libelf-dev libpci-dev
      apt-get install -y libboost-dev libboost-doc libboost-program-options-dev
      apt-get install -y python-is-python2 libxml2-dev libboost-system-dev
      apt-get install -y python3-yaml

Note that the python-is-python2 package changes the /usr/bin/python
symlink to point to Python2, so it may affect other programs or
projects.  It is required for LLVM to work correctly.

### Installing packages on Ubuntu 18.04

To install the require packages, run these commands as root:

      apt-get install -y cmake g++ m4 libpcap-dev wireshark scons libelf-dev libpci-dev
      apt-get install -y libboost-dev libboost-doc libboost-program-options-dev
      apt-get install -y libxml2-dev libboost-system-dev
      apt-get install -y python3-yaml

### Installing packages on CentOS 7

To install the required packages, run these commands as root:

      yum install -y https://cbs.centos.org/kojifiles/packages/scons/2.5.1/4.el7/noarch/scons-2.5.1-4.el7.noarch.rpm
      yum install -y devtoolset-6 libpcap-devel wireshark
      yum install -y boost-devel pciutils-devel
      yum install -y python36-PyYAML

Note that devtoolset-{5,6,7,8} should be fine, but the instructions
below will need to be changed.

## Checking out the Nanotube repository

To checkout the Nanotube repository, run this command:

      git clone git@github.com:Xilinx/nanotube.git

## Downloading and building LLVM

You will need to build LLVM before building Nanotube.  The build_llvm
script makes this process easier and selects LLVM build options which
are known to work.  Nanotube is built against LLVM 8.0.0 built from source,
but we are planning to a more recent version of LLVM and explore the use of
distribution packages.

The instructions for building LLVM are slightly different for CentOS 7
because the default g++ is not new enough.  The devtoolset-6 package
is used instead, but requires modified steps.

The script build_llvm accepts -j<N> like make does.

### Downloading LLVM

Choose a directory where LLVM will be built.  The instructions below
use /scratch/$USER/nanotube-llvm, but a different directory can be
used if preferred.  Just make sure the correct path is given in
later commands.

      mkdir -p /scratch/$USER
      git clone -b llvmorg-8.0.0 https://github.com/llvm/llvm-project.git --depth 1  /scratch/$USER/nanotube-llvm

### Building LLVM

To build LLVM on CentOS 7, run this command:

      scl enable devtoolset-6 'scripts/build_llvm -j4 /scratch/$USER/nanotube-llvm'

To build LLVM on other systems, run this command:

      scripts/build_llvm -j4 /scratch/$USER/nanotube-llvm

## Installing Vitis-HLS and Vivado

The Nanotube compiler compiles from EBPF XDP C to C++ with HLS annotations,
converting the program in multiple steps.  The output of each step can be
executed for testing, including the produced HLS C++ code.  For that, we need
Vitis-HLS headers; for synthesising the design onto an FPGA, a full
installation of Vitis is required.

Vitis can be obtained from [its product page](https://www.xilinx.com/products/design-tools/vitis/vitis-platform.html).

A minimal installation uses only the install directory from a normal
Vitis-HLS installation.  First find an existing installation of
Vitis-HLS or install a new one.  Then create a destination directory
and copy the include directory into it.  For example:

      mkdir  /scratch/$USER/Vitis_HLS_2022.2
      rsync -a <vitis_install_dir>/Vitis_HLS/2022.2/include/ /scratch/$USER/Vitis_HLS_2022.2/include/

## Building Nanotube

The Nanotube build system is based on scons.  When building the
project for the first time, it may be necessary to provide the
locations of dependencies on the scons command line.  The locations
specified are stored in the build configuration (build/config.json) so
they do not need to be provided when rebuilding the project.

By default, scons will build all the files in the build directory.
The scons command accepts extra command line arguments to control the
build process:

      LLVM_CONFIG=<path>      Specify the location of the llvm-config executable.
      XILINX_VITIS_HLS=<path> Specify the location of the Vitis-HLS installation.
      -j<N>                   Run <N> jobs in parallel.
      run_tests               Build the project and run the tests.
      <path-to-file>          Build the specified file.
      <path-to-directory>     Build all the files in the specified directory.

On CentOS 7, the first build of Nanotube must use a different command
to make sure the correct C++ compiler is used.  Distributions with a
more recent C++ compiler do not need this modification.

      scl enable devtoolset-6 'scons -j4 [arguments]'

The path to the llvm-config executable is specified using the
LLVM_CONFIG variable on the scons command line and the path to the Vitis-HLS
installation is specified using the XILINX_VITIS_HLS variable:

      scons LLVM_CONFIG=/scratch/$USER/llvm-project/build/bin/llvm-config XILINX_VITIS_HLS=/scratch/$USER/Vitis_HLS_2022.2 [args...]

(NOTE: When building on a machine inside the AMD / Xilinx network, we use an
existing Vitis-HLS install and XILINX_VITIS_HLS is auto-detected and can be
omitted.)

Once the build directory has been created using the commands above,
the LLVM_CONFIG and XILINX_VITIS_HLS options can be omitted, so the
tests can be re-run using this shorter command:

      scons -j4 run_tests

To compile the bpf-compile compiler (eBBPF -> LLVM-IR) do the following:

      scons build/bpf-compile

To run the resulting tool, use

      build/bpf-compile examples/build/xdp_tx_iptunnel_kern.o

## Perform an HLS build

When scons has finished building the compiler and tests, the following
command will perform an HLS build of one of the test programs.

      scripts/hls_build -- \
        build/testing/kernel_tests/tap_packet_read.link_taps.inline_opt.hls \
        tmp/tap_packet_read \
        --pcap-in $(pwd)/build/testing/kernel_tests/golden/test_tap_packet_read.pcap.IN \
        --pcap-exp $(pwd)/build/testing/kernel_tests/golden/test_tap_packet_read.pcap.OUT

The HLS build benefits from machines with fast CPUs and lots of memory.  If
your environment has access to an LSF cluster, you can use "--bsub" after
"scripts/hls_build" to run the compilation there.  Note that this may need
adaptation depending on your specific LSF setup.  Add "-j <N>" to run multiple
jobs in parallel.  Add "--clean most" to remove the simulation directories
after completing each synthesis.

The utilisation and performance can be viewed with the following command:

      scripts/report_hls_synth tmp/tap_packet_read

The -s option will give just the aggregate values.

# Example

Please see examples/katran/README.md for an example using Facebook's Katran L4
load balancer.

The tests (especially those in testing/kernel_tests) also provide insight into
Nanotube functionality.

# Releases

Nanotube currently does not have a formal release process.  That means we do
not provide formal releases, stable branches, etc.  Instead, the main branch
will regularly receive new features, bug fixes, and occasionally bigger
changes such as API changes.

# Contributing

Contributing to the Nanotube compiler is possible via GitHub pull requests.
External contributions will be reviewed by core Nanotube developers.  If the
contribution passes the review and fits the project, we will merge it; if not,
we will provide comments on the proposed changes.

Here are a few rules for contributing:

* Contributed code will be released under the project's license; make sure
  that this is okay (no lifting of code from elsewhere, make sure your employer
  knows, etc)
* Each commit must have a descriptive commit message describing a high-level
  of the change and why it is needed
* Each commit must compile and pass the tests in the repository
* New contributed features should provide tests; that way, we can be more
  confident they do not break in the future
* Commits should be incremental and split logically for easier code review

# Reporting bugs

We use GitHub issues for tracking bugs in the compiler.

  https://github.com/Xilinx/nanotube/issues

When filing a bug, the summary should include a short clear
description of the bug.

## Information to include in the description.

The description should provide enough details for someone else to
reproduce the problem.  The following pieces of information will make
that easier.

### What commands you typed.

If you provide the exact commands you typed then the person who is
trying to reproduce the problem can copy them into their terminal.
Without the exact commands, they are likely to type a different
command which might not show the problem.  It's important to provide
the full commands, even if you think some parts are not relevant.

### What you are trying to do.

Sometimes the desired effect of a command is not obvious.  If you
describe what you are trying to do, the person handling the bug can
confirm that the commands you typed are expected to have the effect
you want.

### What output you saw.

The person trying to reproduce the problem will want to know whether
their attempt has been successful.  An easy way to do that is to copy
the output you saw into the bug report.  The output may also allow
them to diagnose the problem without reproducing it.

### What output did you expected to see.

It's useful to be clear about what you expect to see, even if you
think that is obvious.  For example, if the program crashed, you might
say that you expect it to generate an error message or that you expect
it to successfully generate the output file.  This is to make sure the
bug gets fixed in a way which is acceptable to you.

### The commit ID of the Nanotube repository.

Some bugs only occur with specific versions of the Nanotube source
code.  To make sure the person trying to reproduce the problem is
using the same version, provide the commit ID in your description.
This can be obtained by using the following command:

  git log -1

### The version of the operating system being run.

Some bugs only occur with a specific operating system or version of an
operating system.  To handle these cases, it's useful to provide the
operating system version being used in the bug description.  The
following command will report the operating system version on most
Linux distributions:

  lsb_release -a
