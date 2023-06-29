#! /usr/bin/env python

import re
import sys
import pathlib
import subprocess

def compiler_help(env):
    compiler_help_message = "\nEnvironment:\n"
    keys = [
        "CC", "CCVERSION", "CCFLAGS", "SHCCFLAGS", "CPPFLAGS", "CPPPATH", "CCCOM", "SHCCOM",
        "CXX", "CXXVERSION", "CXXFLAGS", "SHCXXFLAGS", "CXXCOM", "SHCXXCOM",
        "LINKFLAGS", "SHLINKFLAGS", "LIBPREFIX", "SHLIBSUFFIX", "LIBPATH", "SHOBJSUFFIX", "LIBS",
        "STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME"
    ]
    for key in keys:
        if key in env:
            compiler_help_message += f"    {key}: {env[key]}\n"
    return compiler_help_message

def find_abaqus_paths(abaqus_program):
    subprocess_command = [abaqus_program, "information=environment"]
    abaqus_environment = subprocess.check_output(subprocess_command).decode("utf-8")
    abaqus_paths_regex = r"Abaqus is located in the directory(.*)"
    abaqus_paths_match = re.search(abaqus_paths_regex, abaqus_environment)
    if abaqus_paths_match:
        abaqus_paths = abaqus_paths_match.groups()[0].strip().split(" ")
        abaqus_paths = [pathlib.Path(path) for path in abaqus_paths]
        abaqus_paths.sort()
    else:
        abaqus_paths = None
    return abaqus_paths

def return_abaqus_code_paths(abaqus_program, code_directory="code"):
    abaqus_paths = find_abaqus_paths(abaqus_program)

    if abaqus_paths:
        abaqus_installation = abaqus_paths[0]
    else:
        print(f"Could not find Abaqus installation directory", file=sys.stderr)
        abaqus_installation = None

    try:
        abaqus_code_path = next(path for path in abaqus_paths if path.name == code_directory)
    except StopIteration as err:
        print(f"Could not find Abaqus '{code_directory}' directory", file=sys.stderr)
        abaqus_code_bin = None
        abaqus_code_include = None
    else:
        abaqus_code_bin = abaqus_code_path / "bin"
        abaqus_code_include = abaqus_code_path / "include"

    return abaqus_installation, abaqus_code_bin, abaqus_code_include
