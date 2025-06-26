#! /usr/bin/env python
import re
import sys
import shutil
import typing
import pathlib
import subprocess


def compiler_help(env):
    compiler_help_message = "\nEnvironment:\n"
    keys = [
        "CC",
        "CCVERSION",
        "CCFLAGS",
        "SHCCFLAGS",
        "CPPFLAGS",
        "CPPPATH",
        "CCCOM",
        "SHCCOM",
        "CXX",
        "CXXVERSION",
        "CXXFLAGS",
        "SHCXXFLAGS",
        "CXXCOM",
        "SHCXXCOM",
        "LINKFLAGS",
        "SHLINKFLAGS",
        "LIBPREFIX",
        "SHLIBSUFFIX",
        "LIBPATH",
        "SHOBJSUFFIX",
        "LIBS",
        "STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME",
    ]
    for key in keys:
        if key in env:
            compiler_help_message += f"    {key}: {env[key]}\n"
    return compiler_help_message


def find_abaqus_paths(abaqus_program):
    subprocess_command = [abaqus_program, "information=environment"]
    abaqus_environment = subprocess.check_output(subprocess_command, text=True)
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
        print("Could not find Abaqus installation directory", file=sys.stderr)
        abaqus_installation = None

    try:
        abaqus_code_path = next(path for path in abaqus_paths if path.name == code_directory)
    except StopIteration:
        print(f"Could not find Abaqus '{code_directory}' directory", file=sys.stderr)
        abaqus_code_bin = None
        abaqus_code_include = None
    else:
        abaqus_code_bin = abaqus_code_path / "bin"
        abaqus_code_include = abaqus_code_path / "include"

    return abaqus_installation, abaqus_code_bin, abaqus_code_include


def abaqus_official_version(abaqus_command: pathlib.Path) -> str:
    """Return 'official version' string from abaqus_command 'information=version"

    :param str abaqus_command: string value used to call Abaqus via subprocess

    :return: Abaqus official version string
    """
    try:
        abaqus_version = subprocess.check_output([abaqus_command, "information=version"], text=True)

    except FileNotFoundError:
        raise RuntimeError(f"Abaqus command not found at: '{abaqus_command}'")

    except OSError:
        raise RuntimeError(f"Abaqus command failed. Command used: '{abaqus_command}'")

    # TODO: Figure out what exceptions are likely to raise and convert to RuntimeError
    official_version_regex = r"(?i)official\s+version:\s+abaqus\s+(.*)"
    official_version_match = re.search(official_version_regex, abaqus_version)
    if official_version_match:
        official_version = official_version_match.groups()[0]
    else:
        raise RuntimeError("Could not find Abaqus official version")

    return official_version


# Ripped from Turbo-Turtle. Probably worth keeping a project specific version.
def character_delimited_list(sequence: typing.Iterable, character: str = " ") -> str:
    """Map a list of non-strings to a character delimited string


    :param character: Character(s) to use when joining sequence elements

    :returns: string delimited by specified character
    """
    return character.join(map(str, sequence))


def quoted_string(list_or_string) -> str:
    """Make a string with double quotes on either side from a list or string


    :param list_or_string: list or string to be returned as a string with double quotes on either side

    :returns: string with quotes on either side
    """
    return (
        '"' + character_delimited_list(list_or_string, ",") + '"'
        if isinstance(list_or_string, list)
        else '"' + str(list_or_string) + '"'
    )


# Comes from WAVES scons extensions. Keep a SPADE specific version here because this project *must* be upstream of WAVES
def search_commands(options: typing.Iterable[str]) -> typing.Optional[str]:
    """Return the first found command in the list of options. Return None if none are found.

    :param list options: executable path(s) to test

    :returns: command absolute path
    """
    command_search = (shutil.which(command) for command in options)
    command_abspath = next((command for command in command_search if command is not None), None)
    return command_abspath


# Comes from WAVES scons extensions. Keep a SPADE specific version here because this project *must* be upstream of WAVES
def find_command(options: typing.Iterable[str]) -> str:
    """Return first found command in list of options.

    :param options: alternate command options

    :returns: command absolute path

    :raises FileNotFoundError: If no matching command is found
    """
    command_abspath = search_commands(options)
    if command_abspath is None:
        raise FileNotFoundError(f"Could not find any executable on PATH in: {character_delimited_list(options)}")
    return command_abspath
