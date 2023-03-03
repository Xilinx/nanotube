# \author  Neil Turton <neilt@amd.com>
#  \brief  SCons tool definitions.
#   \date  2020/07/27
#
###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################

import json
import os.path
import sys

scons_dir = os.path.dirname(__file__)
top_dir = os.path.normpath(os.path.join(scons_dir, ".."))
py_lib_dir = os.path.join(top_dir, "py-lib")
sys.path.append(py_lib_dir)

from tool_config import config

def check_var(env, var, builder):
    assert env.get(var) != None, (
        "ERROR: Builder '%s' invoked when variable"
        " '%s' is not set.\n" % (builder, var)
    )

def tool_bitfile(env):
    """Invoke Clang to generate a bitfile from C or C++."""

    def generator(source, target, env, for_signature):
        prefix = '${CLANG} -o $TARGET -c -emit-llvm'
        suffix = '-fno-vectorize -fno-slp-vectorize'
        ext = os.path.splitext(str(source[0]))[1]

        if ext in (".c"):
            check_var(env, 'CLANG', 'BitFile')
            return ( prefix + " $CFLAGS $CCFLAGS $_CCCOMCOM $SOURCES " +
                     suffix )

        if ext in (".cc", ".cxx", ".cpp"):
            check_var(env, 'CLANG', 'BitFile')
            return ( prefix + " $CXXFLAGS $CCFLAGS $_CCCOMCOM $SOURCES " +
                     suffix )

        if ext in (".ll"):
            check_var(env, 'LLVM_AS', 'BitFile')
            return "${LLVM_AS} -o $TARGET $SOURCE"

        assert False, "Unknown extension '%s'" % ext

    bitfile = Builder(
        generator = generator,
        src_suffix = [ ".c", ".cc", ".cxx", ".cpp", ".ll" ],
        suffix = ".bc",
        single_source=True,
    )

    env.Append(BUILDERS = {'BitFile': bitfile})

def tool_llvm_asm(env):
    """Invoke Clang to generate an LLVM asm file from C or C++."""

    def generator(source, target, env, for_signature):
        prefix = '${CLANG} -o $TARGET -S -emit-llvm'
        ext = os.path.splitext(str(source[0]))[1]

        if ext in (".c"):
            check_var(env, 'CLANG', 'LlvmAsm')
            return prefix + " $CFLAGS $CCFLAGS $_CCCOMCOM $SOURCES"

        if ext in (".cc", ".cxx", ".cpp"):
            check_var(env, 'CLANG', 'LlvmAsm')
            return prefix + " $CXXFLAGS $CCFLAGS $_CCCOMCOM $SOURCES"

        if ext in (".ll"):
            check_var(env, 'LLVM_AS', 'Llvm_As')
            return "${LLVM_AS} -o $TARGET $SOURCE"

        assert False, "Unknown extension '%s'" % ext

    llvm_asm = Builder(
        generator = generator,
        src_suffix = [ ".c", ".cc", ".cxx", ".cpp", ".ll" ],
        suffix = ".ll",
        single_source=True,
    )

    env.Append(BUILDERS = {'LlvmAsm': llvm_asm})

def tool_bitfile_obj(env):
    """Invoke llc to generate an object file from a bitfile."""

    def generator(source, target, env, for_signature):
        check_var(env, 'LLC', 'BitFileObj')
        return '${LLC} $LLCFLAGS -o $TARGET $SOURCE -filetype=obj'

    the_builder = Builder(
        generator = generator,
        src_suffix = [ ".bc", ".ll" ],
        suffix = ".o",
        single_source=True,
    )

    env.Append(BUILDERS = {'BitFileObj': the_builder})

def tool_build_kernel_test(env):
    """Invoke build_kernel_test to create an executable from HLS source."""

    def json_scan(node, env, path):
        filename = str(node)
        if not os.path.exists(filename):
            return []

        try:
            info = json.load(open(filename))
        except Exception as e:
            raise RuntimeError("Error parsing JSON file '%s': %s" %
                               (filename, str(e)))

        try:
            thread_ids = [x['thread_id'] for x in info['stages']]
        except Exception as e:
            raise RuntimeError("Invalid contents of JSON file '%s'" %
                               (filename,))

        src_dir = os.path.dirname(filename)

        filenames = [ 'poll_thread.cc', 'stages.hh', ]
        filenames += [ 'stage_%s.cc' % x for x in thread_ids ]
        return [ env.File(os.path.join(src_dir, x)) for x in filenames ]

    scanner = Scanner(function=json_scan, skeys=".json")

    def generator(source, target, env, for_signature):
        check_var(env, 'BUILD_TOP', 'Build_Kernel_Test')
        check_var(env, 'BUILD_KERNEL_TEST', 'Build_Kernel_Test')
        env.Depends(target, "${BUILD_KERNEL_TEST}")
        env.Depends(target, env.lib_deps('test_harness'))
        env.Append(SCANNERS=scanner)
        return '${BUILD_KERNEL_TEST} -o $TARGET $SOURCE'

    the_builder = Builder(
        generator = generator,
        src_suffix = [".hls/pipeline.json"],
        suffix = "",
        single_source=True,
    )

    env.Append(BUILDERS = {'Build_Kernel_Test': the_builder})

def tool_nanotube_be(env):
    """Invoke nanotube_back_end to produce HLS output."""

    def generator(source, target, env, for_signature):
        check_var(env, 'BUILD_TOP', 'Nanotube_Be')
        check_var(env, 'NANOTUBE_BE', 'Nanotube_Be')
        check_var(env, 'BE_FLAGS', 'Nanotube_Be')
        env.Depends(target, "${NANOTUBE_BE}")
        env.Depends(target, "${BUILD_TOP}/front_end/libebpf_passes.so")
        env.Depends(target, "${BUILD_TOP}/back_end/libback_end_passes.so")
        return ( '${NANOTUBE_BE} --overwrite $SOURCE $BE_FLAGS -o ' +
                 os.path.dirname(str(target[0])) )

    env.SetDefault(BE_FLAGS='')

    the_builder = Builder(
        generator = generator,
        src_suffix = [ ".bc", ".ll" ],
        suffix = ".hls/pipeline.json",
        single_source=True,
    )

    env.Append(BUILDERS = {'Nanotube_Be': the_builder})

def tool_nanotube_opt(env):
    """Invoke nanotube_opt to convert bitfiles."""

    def generator(source, target, env, for_signature):
        check_var(env, 'BUILD_TOP', 'NanotubeOpt')
        check_var(env, 'NANOTUBE_OPT', 'NanotubeOpt')
        check_var(env, 'OPT_FLAGS', 'NanotubeOpt')
        env.Depends(target, "${NANOTUBE_OPT}")
        env.Depends(target, "${BUILD_TOP}/front_end/libebpf_passes.so")
        env.Depends(target, "${BUILD_TOP}/back_end/libback_end_passes.so")
        return '${NANOTUBE_OPT} -o $TARGET $SOURCE $OPT_FLAGS'

    the_builder = Builder(
        generator = generator,
        src_suffix = [ ".bc", ".ll" ],
        suffix = ".bc",
        single_source=True,
    )

    env.Append(BUILDERS = {'NanotubeOpt': the_builder})

def tool_llvm_link(env):
    """Invoke llvm-link to merge bitfiles."""

    def generator(source, target, env, for_signature):
        check_var(env, 'LLVM_LINK', 'LlvmLink')
        return '${LLVM_LINK} -o $TARGET $SOURCES $LLVM_OPTS'

    the_builder = Builder(
        generator = generator,
        src_suffix = [ ".bc", ".ll" ],
        suffix = ".bc",
    )

    env.Append(BUILDERS = {'LlvmLink': the_builder})
