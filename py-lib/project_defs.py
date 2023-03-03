###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################
import os.path

LIB_NANOTUBE = 'libnt/libnanotube.a'

# This is ugly and needs to be cleaned up, but right now I'm trying to
# get things working.
LIB_BOOST_SYSTEM = ()
if ( os.path.exists("/usr/lib64/libboost_system.so") or
     os.path.exists("/usr/lib/x86_64-linux-gnu/libboost_system.so") ):
    LIB_BOOST_SYSTEM = (
        ('lib', 'boost_system'),
    )

_lib_def_tuple = (
    (
        'nanotube', (
            ('file', LIB_NANOTUBE),
            ('lib',  'pcap'),
            ('lib',  'pthread'),
        ),
    ),

    # Although the test harness is not technically a library, it is
    # compiled once and used in many executables.  As such it makes
    # sense to define it as a library so that the dependencies can be
    # managed centrally.
    (
        'test_harness', (
            ('file', 'testing/harness/test_harness_main.o'),
            ('file', 'testing/harness/libtest_harness.a'),
            ('lib', 'boost_program_options'),
            ('lib', 'stdc++'),
            ('lib', 'm'),
            ('ref', 'nanotube'),
        ) + LIB_BOOST_SYSTEM,
    ),
)
lib_defs = {}

for lib_name, lib_deps in _lib_def_tuple:
    # Expand libraries which have already been defined.
    expanded_deps = []
    for dep_item in lib_deps:
        dep_type, dep_name = dep_item
        if dep_type == 'ref':
            expanded_deps.extend(lib_defs[dep_name])
        else:
            expanded_deps.append(dep_item)

    # Remove duplicates.
    unique_deps = []
    dep_set = set()
    for dep in expanded_deps:
        if dep not in dep_set:
            unique_deps.append(dep)
            dep_set.add(dep)

    # Insert into the dictionary.
    lib_defs[lib_name] = unique_deps

# Set the C/C++ compiler flags:
#   CFLAGS - Used for C only
#   CCFLAGS - Used for C and C++
#   CXXFLAGS - Used for C++ only
#
#   GCC_CCFLAGS - same as above, but only when compiling with GCC
CFLAGS = []
CCFLAGS = ['-Wall', '-Werror', '-O2', '-g']
# Work around "buggy" code in LLVM that uses ASCII art and confuses the comment checker
CCFLAGS.append('-Wno-comment')
CXXFLAGS = []
# Work around code in LLVM DenseMaps that GCC thinks might be uninitialised
GCC_CCFLAGS = ["-Wno-maybe-uninitialized"]
