import os
import re
import sys
import shlex
import shutil
import typing
import pathlib
import argparse
import platform
import subprocess

from spade import _settings
from spade import _utilities


_exclude_from_namespace = set(globals().keys())


# Comes from WAVES internal utilities. Probably worth keeping a project specific version here.
def search_commands(options: typing.Iterable[str]) -> typing.Optional[str]:
    """Return the first found command in the list of options. Return None if none are found.

    :param list options: executable path(s) to test

    :returns: command absolute path
    """
    command_search = (shutil.which(command) for command in options)
    command_abspath = next((command for command in command_search if command is not None), None)
    return command_abspath


# Comes from WAVES internal utilities. Probably worth keeping a project specific version here.
def find_command(options: typing.Iterable[str]) -> str:
    """Return first found command in list of options.

    :param options: alternate command options

    :returns: command absolute path

    :raises RuntimeError: If no matching command is found
    """
    command_abspath = search_commands(options)
    if command_abspath is None:
        raise RuntimeError(f"Could not find any executable on PATH in: {_utilities.character_delimited_string(options)}")
    return command_abspath


def abaqus_official_version(abaqus_command: pathlib.Path) -> str:
    """Return 'official version' string from abaqus_command 'information=version"

    :param str abaqus_command: string value used to call Abaqus via subprocess

    :return: Abaqus official version string
    """
    try:
        abaqus_version_check = subprocess.check_output([abaqus_command, 'information=version']).decode('utf-8')

    except FileNotFoundError:
        raise RuntimeError(f"Abaqus command not found at: '{abaqus_command}'")

    except OSError:
        raise RuntimeError(f"Abaqus command failed. Command used: '{abaqus_command}'")

    # TODO: Figure out what exceptions are likely to raise and convert to RuntimeError
    official_version_regex = r"(?i)official\s+version:\s+abaqus\s+\b(20)\d{2}\b"
    official_version_match = re.search(official_version_regex, abaqus_version_check)
    official_version = int(official_version_match[0].split(' ')[-1])

    return official_version


# TODO: full API
def main(args: argparse.ArgumentParser) -> None:
    """Main parser behavior when no subcommand is specified

    :param args: argument namespace

    :raises RuntimeError: If any subprocess returns a non-zero exit code or an Abaqus ODB file is not provided.
    """
    full_command_line_arguments = ""
    # File name inputs
    if args.ODB_FILE:
        full_command_line_arguments += f" {args.ODB_FILE}"
    else:
        raise RuntimeError("Abaqus output database (ODB) file not specified.")
    if args.extracted_file:
        full_command_line_arguments += f" --extracted-file {args.extracted_file}"
    if args.log_file:
        full_command_line_arguments += f" --log-file {args.extracted_file}"

    # String inputs
    # if args.extracted_file_type:
    #    full_command_line_arguments += f" --extracted-file-type {args.extracted_file_type}"
    if args.frame:
        full_command_line_arguments += f" --frame {args.frame}"
    if args.frame_value:
        full_command_line_arguments += f" --frame-value {args.frame_value}"
    if args.step:
        full_command_line_arguments += f" --step {args.step}"
    if args.field:
        full_command_line_arguments += f" --field {args.field}"
    if args.history:
        full_command_line_arguments += f" --history {args.history}"
    if args.history_region:
        full_command_line_arguments += f" --history-region {args.history_region}"
    if args.instance:
        full_command_line_arguments += f" --instance {args.instance}"

    # True or False inputs
    if args.verbose:
        full_command_line_arguments += " --verbose"
    if args.force_overwrite:
        full_command_line_arguments += " --force-overwrite"
    if args.debug:
        full_command_line_arguments += " --debug"

    abaqus_command = find_command(args.abaqus_commands)
    abaqus_version = abaqus_official_version(abaqus_command)
    _, abaqus_bin, _ = _utilities.return_abaqus_code_paths(abaqus_command)
    source_directory = pathlib.Path(__file__).parent
    platform_string = "_".join(f"{platform.system()} {platform.release()}".split())
    spade_version = source_directory / f"{_settings._project_name_short}_{abaqus_version}_{platform_string}"
    current_env = os.environ.copy()
    if not spade_version.exists():
        # Compile necessary version
        scons_command = shlex.split(f"scons abaqus_version={abaqus_version} platform={platform_string} "
                                   f"--directory={source_directory.resolve()}",
                                   posix=(os.name == 'posix'))
        sub_process = subprocess.Popen(scons_command, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, env=current_env, cwd=f"{source_directory}")
        out, error_code = sub_process.communicate()
        if error_code:
            print(error_code.decode("utf-8"), file=sys.stderr)
            raise RuntimeError("Could not compile with specified Abaqus version")
    full_command_line_arguments = str(spade_version) + full_command_line_arguments

    try:
        current_env['LD_LIBRARY_PATH'] = f"{abaqus_bin}:{current_env['LD_LIBRARY_PATH']}"
    except KeyError:
        current_env['LD_LIBRARY_PATH'] = f"{abaqus_bin}"
    command_line_arguments = shlex.split(full_command_line_arguments, posix=(os.name == 'posix'))
    sub_process = subprocess.Popen(command_line_arguments, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, env=current_env)
    out, error_code = sub_process.communicate()
    if out:
        print(out.decode("utf-8"))
    if error_code:
        print(error_code.decode("utf-8"), file=sys.stderr)
    return_code = sub_process.returncode

    # TODO: Sort out which error message return value should be put in the message
    if return_code != 0:
        raise RuntimeError(return_code)


def get_parser() -> argparse.ArgumentParser:
    """Return a 'no-help' parser for the extract subcommand

    :return: parser
    """
    parser = argparse.ArgumentParser(add_help=False)

    # File inputs
    parser.add_argument(
        "ODB_FILE",
        type=str,
        help='ODB file from which to extract data',
        metavar='ODB_FILE.odb'
    )
    parser.add_argument('-e', '--extracted-file', type=str,
                        help='Name of extracted file. (default: <ODB file name>.h5)')
    parser.add_argument('-l', '--log-file', type=str,
                        help=f'Name of log file. (default: <ODB file name>.{_settings._project_name_short}.h5)')

    parser.add_argument('--frame', type=str, default="all",
                        help='Get information from the specified frame (default: %(default)s)')
    parser.add_argument('--frame-value', type=str, default="all",
                        help='Get information from the specified frame value (default: %(default)s)')
    parser.add_argument('--step', type=str, default="all",
                        help='Get information from the specified step (default: %(default)s)')
    parser.add_argument('--field', type=str, default="all",
                        help='Get information from the specified field (default: %(default)s)')
    parser.add_argument('--history', type=str, default="all",
                        help='Get information from the specified history value (default: %(default)s)')
    parser.add_argument('--history-region', type=str, default="all",
                        help='Get information from the specified history region (default: %(default)s)')
    parser.add_argument('--instance', type=str, default="all",
                        help='Get information from the specified instance (default: %(default)s)')
    parser.add_argument('-a', '--abaqus-commands', nargs="+", type=pathlib.Path, default=_settings._default_abaqus_commands,
                        help='Ordered list of Abaqus executable paths. Use first found (default: %(default)s)')

    # True or false inputs
    parser.add_argument('-v', '--verbose', action='store_true', default=False,
                        help='Turn on verbose logging')
    parser.add_argument('-f', '--force-overwrite', action='store_true', default=False,
                        help='Force the overwrite of the hdf5 file if it already exists')
    parser.add_argument('-d', '--debug', action='store_true', default=False, help=argparse.SUPPRESS)
    return parser


# Limit help() and 'from module import *' behavior to the module's public API
_module_objects = set(globals().keys()) - _exclude_from_namespace
__all__ = [name for name in _module_objects if not name.startswith("_")]
