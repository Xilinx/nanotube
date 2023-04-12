# \author  Neil Turton <neilt@amd.com>
#  \brief  The Nanotube project
#   \date  2020-06-10
#
###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################

import os
import os.path
import re
import subprocess
import sys

top_dir = os.getcwd()
sys.path.append(os.path.join(top_dir, 'py-lib'))

# This is to work around a bug in Python.  The project_defs file used
# to be in site_scons, but was moved to py-lib.  If the .pyc exists in
# site_scons then it will be used in preference.  Delete it to avoid
# the module becoming stale.
_project_defs_pyc = os.path.join(top_dir, "site_scons/project_defs.pyc")
if os.path.exists(_project_defs_pyc):
    os.unlink(_project_defs_pyc)

import project_defs

EnsurePythonVersion(2,7)
EnsureSConsVersion(2,5,1)
SConscriptChdir(0)

# Check the compiler used to build LLVM matches the configured
# compiler.
def check_llvm_cc(env):
    if env.get('LLVM_CONFIG') == None:
        return

    llvm_obj_root = env['LLVM_OBJ_ROOT']
    cc_re = re.compile(r'CMAKE_C_COMPILER:(FILEPATH|STRING)=')
    cxx_re = re.compile(r'CMAKE_CXX_COMPILER:(FILEPATH|STRING)=')

    cmake_cache = "%s/CMakeCache.txt" % llvm_obj_root
    if not os.path.exists(cmake_cache):
        print("WARNING: Could not determine the C++ compiler used to build LLVM.")
        print("  Looked in: %s" % (llvm_obj_root,))
        return

    f = open(cmake_cache, "r")

    llvm_cc = None
    llvm_cxx = None
    for line in f.readlines():
        m = cc_re.match(line)
        if m:
            llvm_cc = line[m.end():].rstrip()
        m = cxx_re.match(line)
        if m:
            llvm_cxx = line[m.end():].rstrip()
    f.close()

    if llvm_cc == None:
        sys.stderr.write("Could not determine C compiler used to build LLVM.\n")
        sys.exit(1)

    if llvm_cxx == None:
        sys.stderr.write("Could not determine C++ compiler used to build LLVM.\n")
        sys.exit(1)

    if llvm_cc != env['CC']:
        sys.stderr.write("C compiler used to build LLVM does not match configured compiler.\n")
        sys.stderr.write("  LLVM:       %s\n" % (llvm_cc,))
        sys.stderr.write("  Configured: %s\n" % (env['CC'],))
        sys.exit(1)

    if llvm_cxx != env['CXX']:
        sys.stderr.write("C++ compiler used to build LLVM does not match configured compiler.\n")
        sys.stderr.write("  LLVM:       %s\n" % (llvm_cxx,))
        sys.stderr.write("  Configured: %s\n" % (env['CXX'],))
        sys.exit(1)

def check_config(env):
    check_llvm_cc(env)

def process_config(env):
    c = config(str(env['BUILD_TOP']), ARGUMENTS)

    for var,val in c.items():
        env[var] = val

    get_llvm_info(env)

    if GetOption('clean'):
        return

    check_config(env)

    if c.need_save():
        c.save()

def capture_output(args):
    try:
        out = subprocess.check_output(args, universal_newlines=True)
    except subprocess.CalledProcessError as e:
        sys.stderr.write("Command %s returned exit code %s\n" %
                         (args, e.returncode))
        sys.exit(1)
    except Exception as e:
        sys.stderr.write("Error running %s: %s\n" %
                         (args, e))
        sys.exit(1)
    return out.rstrip('\n')
Export('capture_output')

def get_llvm_info(env):
    llvm_config = env.get('LLVM_CONFIG')
    if llvm_config == None:
        return

    if not os.path.exists(llvm_config):
        sys.stderr.write("ERROR: Could not find llvm-config at '%s'.\n" %
                         (llvm_config,))
        sys.exit(1)

    env['LLVM_VERSION'] = capture_output([llvm_config, "--version"])
    env['LLVM_SRC_ROOT'] = capture_output([llvm_config, "--src-root"])
    env['LLVM_OBJ_ROOT'] = capture_output([llvm_config, "--obj-root"])
    env['LLVM_BIN_DIR'] = capture_output([llvm_config, "--bindir"])
    env['LLVM_LIB_DIR'] = capture_output([llvm_config, "--libdir"])
    env['LLVM_CXX_FLAGS'] = capture_output([llvm_config, "--cxxflags"])
    env['LLVM_LD_FLAGS'] = capture_output([llvm_config, "--ldflags"])

    env['CLANG'] = '$LLVM_BIN_DIR/clang'
    env['LLC'] = '$LLVM_BIN_DIR/llc'
    env['LLVM_AS'] = '$LLVM_BIN_DIR/llvm-as'
    env['LLVM_DIS'] = '$LLVM_BIN_DIR/llvm-dis'
    env['LLVM_LINK'] = '$LLVM_BIN_DIR/llvm-link'

    for var in ('LLVM_CONFIG', 'LLVM_VERSION', 'LLVM_SRC_ROOT',):
        print("%s = %s" % (var, env[var]))

def add_subdir(old_env, subdir):
    env = old_env.Clone()
    env['SOURCE_DIR'] = old_env['SOURCE_DIR'].Dir(subdir)
    env['BUILD_DIR'] = old_env['BUILD_DIR'].Dir(subdir)
    env.SConscript(env['BUILD_DIR'].File('SConscript'),
                   exports=['env'])

def add_libs(env, lib_name):
    for t, n in project_defs.lib_defs[lib_name]:
        if t == 'file':
            env.Append(LIBS=[env['BUILD_TOP'].File(n)])
        else:
            assert t == 'lib'
            env.Append(LIBS=[n])

def lib_deps(env, lib_name):
    deps = []
    for t, n in project_defs.lib_defs[lib_name]:
        if t == 'file':
            deps.append(env['BUILD_TOP'].File(n))
    return deps

def add_libnanotube(env):
    add_libs(env, 'nanotube')

def build_all(env):
    build_top = env['BUILD_TOP']

    # When cleaning the build directory, remove the build directory
    # itself.
    Clean(build_top, build_top)

    # Build the subdirectories.
    subdirs = (
        'back_end',
        'front_end',
        'libnt',
        'testing',
    )
    for subdir in subdirs:
        env.VariantDir(env['BUILD_TOP'].Dir(subdir),
                       env['SOURCE_TOP'].Dir(subdir),
                       duplicate=0)
        env.add_subdir(subdir)
    env.Alias('all', '$BUILD_TOP')
    Default('all')

def run_tests(env):
    opts = ""
    tests = "testing"
    extra = env.get('EXTRA_TESTS')
    if extra:
        opts  =  "-D NT_DIR=" + top_dir
        tests += " " + os.path.join(extra, "testing")

    target = env.Command(
        'run_tests',
        [
            'all',
        ],
        [
            ( 'scripts/locate_tool --run llvm-lit ' + opts + ' -v ' + tests )
        ]
    )
    AlwaysBuild(target)

def set_pie(env, pie):
    if pie:
        # Enable PIC for llc builds
        # NB: On Ubuntu, the linker (GCC) tries to build PIE by default.  Instruct
        # llc to do the same for the .o files it generates
        env['LLCFLAGS']  = ['-relocation-model=pic']
        env['LINKFLAGS'] = ['-pie']
    else:
        # Disable PIC / PIE in the linker
        # NB: On Ubuntu, using GCC as the linker does PIE by default, but clang
        # does not generate PIC / PIE object files => disable in the linking step
        env['LLCFLAGS']  = ['-relocation-model=static']
        env['LINKFLAGS'] = ['-no-pie']

def main():
    source_top = Dir(".")
    build_top = Dir("build")

    env = Environment(tools = [
        'default',
        tool_bitfile,
        tool_bitfile_obj,
        tool_build_kernel_test,
        tool_llvm_asm,
        tool_nanotube_be,
        tool_nanotube_opt,
        tool_llvm_link,
    ])
    env['SOURCE_TOP'] = source_top
    env['SOURCE_DIR'] = source_top
    env['BUILD_TOP'] = build_top
    env['BUILD_TOP_ABS'] = build_top.abspath
    env['BUILD_DIR'] = build_top
    env['CPPPATH'] = [source_top.Dir('include')]

    # Set the C/C++ compiler flags:
    #   CFLAGS - Used for C only
    #   CCFLAGS - Used for C and C++
    #   CXXFLAGS - Used for C++ only
    env['CFLAGS'] = project_defs.CFLAGS
    env['CCFLAGS'] = project_defs.CCFLAGS
    env['CXXFLAGS'] = project_defs.CXXFLAGS
    env['GCC_CCFLAGS'] = project_defs.GCC_CCFLAGS

    set_pie(env, False);

    env['NANOTUBE_OPT'] = build_top.File('nanotube_opt')
    env['NANOTUBE_BE'] = build_top.File('nanotube_back_end')
    env['LIB_NANOTUBE'] = build_top.File(project_defs.LIB_NANOTUBE)
    env['BUILD_KERNEL_TEST'] = source_top.File('testing/scripts/build_kernel_test')

    sconsign_file = str(build_top.File(".sconsign.dblite"))
    if not GetOption('clean') or os.path.exists(sconsign_file):
        SConsignFile(sconsign_file)

    process_config(env)
    env.AddMethod(add_subdir, 'add_subdir')
    env.AddMethod(add_libs, 'add_libs')
    env.AddMethod(lib_deps, 'lib_deps')
    env.AddMethod(add_libnanotube, 'add_libnanotube')
    build_all(env)
    run_tests(env)

main()
