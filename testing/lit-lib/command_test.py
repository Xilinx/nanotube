###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################
from lit.formats import FileBasedTest
from lit.ShUtil import Command, Pipeline
from lit.util import mkdir_p
from lit import Test, TestRunner
import os
import os.path
import subprocess

try:
  from shlex import quote
except ImportError:
  from pipes import quote

class command_test(FileBasedTest):
  """command_test is a format which invokes an external command on each
  test file.

  It is used by setting config.test_format to a command_test object in
  lit.local.cfg.

  The constructor is called with a string containing a path to the
  executable to run or a tuple containing the initial arguments of the
  command to run.

  If the current source directory contains a file called gen_test_list
  then it is executed to produce a list of test names.  Otherwise, the
  normal lit file search procedure (getTestsInDirectory) is used.
  """

  def __init__(self, prefix):
    try:
      if type(prefix) in (str, unicode):
        prefix = [prefix,]
    except NameError:
      # Python 3 does not have unicode defined anymore; instead, all strings
      # are unicode by default
      if isinstance(prefix, str):
        prefix = [prefix,]
    # For easier readability, make the script path relative, but
    # prefix an explicit "./" because LIT will otherwise not resolve
    # the path
    prefix[0] = "./" + os.path.relpath(prefix[0])
    self.__prefix = tuple(prefix)

  def execute(self, test, lit_config):
    source_path = test.getSourcePath()
    exec_path = test.getExecPath()
    source_path = os.path.relpath(source_path)
    exec_path   = os.path.relpath(exec_path)

    cmdline = ' '.join(quote(x) for x in (self.__prefix + (source_path,)))
    exec_dir = os.path.dirname(exec_path)
    commands = [cmdline]
    mkdir_p(exec_dir)
    res = TestRunner.executeScriptInternal(test, lit_config, exec_path,
                                           commands, os.getcwd())
    out, err, exit_code, timeout_info = res
    if exit_code == 0:
      status = Test.PASS
    else:
      if timeout_info is None:
        status = Test.FAIL
      else:
        status = Test.TIMEOUT
    output = "Command: %s\nExit code: %d\n" % (cmdline, exit_code)
    if timeout_info is not None:
      output += "Timeout: %s\n" % (timeout_info,)
    output += ( "Begin output:\n" +
                out +
                "End output:\n" )
    output += ( "Begin errors:\n" +
                err +
                "End errors:\n" )
    return Test.Result(status, output)

  def getTestsInDirectory(self, test_suite, path_in_suite, lit_config,
                          local_config):
    source_path = test_suite.getSourcePath(path_in_suite)
    test_list_exe = os.path.join(source_path, "gen_test_list")
    if os.path.exists(test_list_exe):
      return self.get_tests_from_exe(test_suite, path_in_suite,
                                     test_list_exe, lit_config,
                                     local_config)

    super_self = super(command_test, self)
    return super_self.getTestsInDirectory(test_suite, path_in_suite,
                                          lit_config, local_config)

  def get_tests_from_exe(self, test_suite, path_in_suite, test_list_exe,
                         lit_config, local_config):
    # The output from the generating script is a sequence of test
    # names, one per line.
    proc = subprocess.Popen([test_list_exe], stdout=subprocess.PIPE)
    for line in proc.stdout.readlines():
      test_name = line.rstrip()
      yield Test.Test(test_suite, path_in_suite + (test_name,),
                      local_config)
