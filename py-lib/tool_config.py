# \author  Neil Turton <neilt@amd.com>
#  \brief  Project Nanotube tool configuration.
#   \date  2020-10-26
#
###########################################################################
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
###########################################################################

from distutils.spawn import find_executable
import json
import os
import os.path

# Detect the location of host C/C++ compilers.
def _default_host_tools_dir(conf):
    tools_dir = None

    cc = conf.get('CC')
    if cc != None:
        tools_dir = os.path.dirname(cc)

    cxx = conf.get('CXX')
    if cxx != None:
        cxx_tools_dir = os.path.dirname(cxx)
        if tools_dir != None and tools_dir != cxx_tools_dir:
            raise RuntimeError("Mismatch between CC directory (%s) and"
                               " CXX directory (%s)" %
                               (tools_dir, cxx_tools_dir))
        tools_dir = cxx_tools_dir

    if tools_dir != None:
        return tools_dir

    return "/usr/bin"

# Detect the location of the LLVM binary directory.
def _default_llvm_bin_dir(conf):
    llvm_config = conf.get("LLVM_CONFIG")
    if llvm_config != None:
        return os.path.dirname(llvm_config)

    llvm_bin_dir = "../nanotube-llvm/build/bin"
    if os.path.exists(llvm_bin_dir):
        return llvm_bin_dir

    ebpf_user = conf.get("EBPF_USER")
    if ebpf_user != None:
        user_llvm_dir = ( "/scratch/%s/nanotube-llvm/build/bin"
                          % ebpf_user )
        if os.path.exists(user_llvm_dir):
            return user_llvm_dir

        user_llvm_dir = ( "/scratch/%s/llvm-project/build/bin"
                          % ebpf_user )
        if os.path.exists(user_llvm_dir):
            return user_llvm_dir

    return None

# Information about tools and installs.
#
# Each item can have the following attributes:
#   command  - The name of command which can be run using locate_tool.
#   var      - The name of the variable to set.
#   save     - A flag indicating whether the variable is added to the file by default.
#   default  - A function to return the default value.
#   install  - The variable holding the install directory.
#   rel_path - The location relative to the install directory.
#   is_path  - True if the variable is a path.
#   paths    - A list of paths to search.
#   env_vars - A list of variables to set when running the command.
_infos = [
    {
        'var': 'BSUB_CORE_OPTIONS',
        'default': '-R "rusage[mem=22G]" -R osdistro=rhel -R osver=ws7',
    },
    {
        'var': 'BSUB_XWIN_OPTIONS',
        'default': '-IX',
    },
    {
        'var': 'BSUB_BATCH_OPTIONS',
        'default': '-Is',
    },
    {
        'var': 'BSUB_EXTRA_OPTIONS',
        'default': '',
    },
    {
        'var': 'HOST_TOOLS_DIR',
        'is_path': True,
        'default': _default_host_tools_dir,
    },
    {
        'var': 'CC',
        'is_path': True,
        'default': (lambda conf: find_executable('cc')),
        'save': True,
    },
    {
        'var': 'CXX',
        'is_path': True,
        'default': (lambda conf: find_executable('c++')),
        'save': True,
    },
    {
        'var': 'EBPF_USER',
        'default': (lambda conf: os.environ.get('USER')),
        'save': True,
    },
    {
        'var': 'LLVM_BIN_DIR',
        'is_path': True,
        'default': _default_llvm_bin_dir,
    },
    {
        'command': 'clang',
        'install': 'LLVM_BIN_DIR',
        'rel_path': 'clang',
    },
    {
        'command': 'clang++',
        'install': 'LLVM_BIN_DIR',
        'rel_path': 'clang++',
    },
    {
        'command': 'llc',
        'install': 'LLVM_BIN_DIR',
        'rel_path': 'llc',
    },
    {
        'command': 'llvm-config',
        'var': 'LLVM_CONFIG',
        'install': 'LLVM_BIN_DIR',
        'rel_path': 'llvm-config',
        'save': True,
    },
    {
        'command': 'llvm-dis',
        'var': 'LLVM_DIS',
        'install': 'LLVM_BIN_DIR',
        'rel_path': 'llvm-dis',
    },
    {
        'command': 'llvm-link',
        'var': 'LLVM_LINK',
        'install': 'LLVM_BIN_DIR',
        'rel_path': 'llvm-link',
    },
    {
        'command': 'llvm-lit',
        'var': 'LLVM_LIT',
        'install': 'LLVM_BIN_DIR',
        'rel_path': 'llvm-lit',
    },
    {
        'command': 'opt',
        'var': 'LLVM_OPT',
        'install': 'LLVM_BIN_DIR',
        'rel_path': 'opt',
    },
    {
        'var': 'XILINX_VITIS_HLS',
        'paths': [
            '/proj/xbuilds/2021.1_released/installs/lin64/Vitis_HLS/2021.1'
        ],
    },
    {
        'command': 'vitis_hls',
        'install': 'XILINX_VITIS_HLS',
        'rel_path': 'bin/vitis_hls',
        'env_vars': ['XILINX_VIVADO'],
    },
    {
        'var': 'XILINX_VIVADO',
        'paths': [
            '/proj/xbuilds/2021.1_released/installs/lin64/Vivado/2021.1',
        ],
    },
    {
        'command': 'vivado',
        'install': 'XILINX_VIVADO',
        'rel_path': 'bin/vivado',
    },
    {
        'var' : 'EXTRA_TESTS',
        'is_path' : True,
    },
]

class config(object):
    """This class represents the build configuration."""
    def __init__(self, build_dir, options):
        """Create a config object.

        Parameters:
          build_dir(string): The path to the build directory.
          options(dict): The user supplied options.
        """
        self.__build_dir = build_dir
        self.__path = os.path.join(build_dir, "config.json")
        self._load_config()
        self.__new_config = {}
        self._update(options)
        self.__config_vars = dict(self.__new_config)
        self._set_defaults()

    def _load_config(self):
        """Load the config file."""
        self.__have_config = os.path.exists(self.__path)

        if self.__have_config:
            f = open(self.__path, "r")
            self.__old_config = json.load(f)
            f.close()
        else:
            self.__old_config = {}

    def _update(self, options):
        """Update the config from the user supplied options.

        Parameters:
          options(dict): The user supplied options.
        """
        # Update the variables from the options and the environment.
        new_config = self.__new_config
        for info in _infos:
            if 'var' not in info:
                continue

            var = info['var']
            if var in options:
                new_config[var] = options[var]
            elif var in self.__old_config:
                new_config[var] = self.__old_config[var]
            elif var in os.environ:
                new_config[var] = os.environ[var]

    def _set_defaults(self):
        """Set the default config values."""
        # Set the defaults.  This is done as a separate step so that
        # the computing the defaults can reference any variable set by
        # the user.
        new_config = self.__new_config
        config_vars = self.__config_vars
        for info in _infos:
            # Ignore items which have no memory.
            if 'var' not in info:
                continue

            # If it has not been set then determine the default value
            # and set it.
            var = info['var']
            val = config_vars.get(var)
            if val == None:
                val = self._get_default(info)

                # Do nothing if there is no default.
                if val == None:
                    continue

            # Convert relative paths to absolute paths if necessary.
            is_path = ( ('rel_path' in info) or
                        ('paths' in info) )
            if info.get('is_path', is_path):
                val = os.path.abspath(val)

            config_vars[var] = val

            # Set the value in the config file if necessary.
            if info.get('save'):
                new_config[var] = val

    def _get_default(self, info):
        """Get the default value of a variable.

        Parameters:
          info(dict): The entry for the variable in _infos.
        """
        if 'default' in info:
            val = info['default']
            if type(val) != str:
                val = val(self)
            if val != None:
                return val

        if 'install' in info:
            install = info['install']
            rel_path = info['rel_path']
            base = self.__config_vars.get(install)
            if base != None:
                return os.path.join(base, rel_path)

        if 'paths' in info:
            for p in info['paths']:
                if not os.path.exists(p):
                    continue

                return p

    def items(self):
        """Iterate over the configuration variable-value pairs."""
        return self.__config_vars.items()

    def get(self, key):
        """Get the value of a variable.

        Parameters:
          key(string): The variable name.

        Returns:
          The value of the variable or None if not found.
        """
        return self.__config_vars.get(key)

    def delete(self, key):
        """Delete a variable from the configuration.

        Parameters:
          key(string): The name of the variable to delete.
        """
        if key in self.__config_vars:
            del self.__config_vars[key]
        if key in self.__new_config:
            del self.__new_config[key]

    def have_config(self):
        """Determine whether the configuration file exists.

        Returns:
          A boolean value which is true if the file exists.
        """
        return self.__have_config

    def need_save(self):
        """Determine whether the configuration file needs to be saved.

        Returns:
          A boolean value which is true if the file should be saved.
        """
        return self.__old_config != self.__new_config

    def save(self):
        """Save the configuration file."""
        if not os.path.exists(self.__build_dir):
            os.mkdir(self.__build_dir)

        tmp = self.__path + ".tmp"
        f = open(tmp, "w")
        json.dump(self.__new_config, f, indent=2)
        f.write("\n")
        f.close()
        os.rename(tmp, self.__path)

    def find_command(self, name):
        """Find the specified command.

        Parameters:
          name(string): The name of the command.

        Returns:

          A dictionary containing information about the command or
          None if the tool is not known.  The dictionary is a copy of
          the _infos entry with an additional "path" member containing
          the path to the executable.  The "path" value is None if the
          executable was not found.
        """
        for info in _infos:
            if info.get('command') != name:
                continue

            if 'var' in info:
                path = self.__config_vars.get(info['var'])
            else:
                base = self.__config_vars.get(info['install'])
                if base != None:
                    path = os.path.join(base, info['rel_path'])
                else:
                    path = None

            result = dict(info)
            result['path'] = path
            return result

        return None
