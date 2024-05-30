import os
import shlex
import pathlib
import argparse
import platform
import tempfile
import subprocess

from spade import _settings
from spade import _utilities


_exclude_from_namespace = set(globals().keys())


# TODO: full API
def main(args: argparse.Namespace) -> None:
    """Main parser behavior when no subcommand is specified

    :param args: argument namespace

    :raises RuntimeError: If any subprocess returns a non-zero exit code or an Abaqus ODB file is not provided.
    """
    # Setup environment
    try:
        abaqus_command = _utilities.find_command(args.abaqus_commands)
    except FileNotFoundError as err:
        raise RuntimeError(str(err))
    _, abaqus_bin, _ = _utilities.return_abaqus_code_paths(abaqus_command)
    source_directory = pathlib.Path(__file__).parent
    abaqus_version = _utilities.abaqus_official_version(abaqus_command)
    platform_string = "_".join(f"{platform.system()} {platform.release()}".split())
    build_directory = _settings._project_root_abspath / f"build-{abaqus_version}-{platform_string}"
    current_env = os.environ.copy()

    with tempfile.TemporaryDirectory() as temporary_directory:

        temporary_path = pathlib.Path(temporary_directory)

        # Compile c++ executable
        spade_executable = cpp_compile(
            build_directory=temporary_path,
            abaqus_command=abaqus_command,
            environment=current_env,
            working_directory=source_directory,
            recompile=args.recompile,
            debug=args.debug
        )

        # Run c++ executable
        cpp_execute(
            spade_executable=spade_executable,
            abaqus_bin=abaqus_bin,
            args=args,
            environment=current_env,
            debug=args.debug
        )


def get_parser() -> argparse.ArgumentParser:
    """Return a 'no-help' parser for the extract subcommand

    :return: parser
    """
    parser = argparse.ArgumentParser(add_help=False)

    # File inputs
    parser.add_argument(
        "ODB_FILE",
        type=str,
        help="ODB file from which to extract data",
        metavar="ODB_FILE.odb"
    )
    parser.add_argument("-e", "--extracted-file", type=str,
                        help="Name of extracted file. (default: <ODB file name>.h5)")
    parser.add_argument("-l", "--log-file", type=str,
                        help=f"Name of log file. (default: <ODB file name>.{_settings._project_name_short}.log)")

    parser.add_argument("--frame", type=str, default="all",
                        help="Get information from the specified frame (default: %(default)s)")
    parser.add_argument("--frame-value", type=str, default="all",
                        help="Get information from the specified frame value (default: %(default)s)")
    parser.add_argument("--step", type=str, default="all",
                        help="Get information from the specified step (default: %(default)s)")
    parser.add_argument("--field", type=str, default="all",
                        help="Get information from the specified field (default: %(default)s)")
    parser.add_argument("--history", type=str, default="all",
                        help="Get information from the specified history value (default: %(default)s)")
    parser.add_argument("--history-region", type=str, default="all",
                        help="Get information from the specified history region (default: %(default)s)")
    parser.add_argument("--instance", type=str, default="all",
                        help="Get information from the specified instance (default: %(default)s)")
    parser.add_argument(
        "-a", "--abaqus-commands",
        nargs="+",
        type=pathlib.Path,
        default=_settings._default_abaqus_commands,
        help="Ordered list of Abaqus executable paths. Use first found (default: %(default)s)"
    )

    # True or false inputs
    parser.add_argument("-v", "--verbose", action="store_true", default=False,
                        help="Turn on verbose logging")
    parser.add_argument("-f", "--force-overwrite", action="store_true", default=False,
                        help="Overwrite the extracted and log file(s)")
    parser.add_argument("-d", "--debug", action="store_true", default=False, help=argparse.SUPPRESS)
    parser.add_argument("--recompile", action="store_true", default=False, help=argparse.SUPPRESS)
    return parser


def cpp_compile(
    build_directory: pathlib.Path = pathlib.Path("build"),
    abaqus_command: pathlib.Path = pathlib.Path("abaqus"),
    environment: dict = dict(),
    working_directory: pathlib.Path = pathlib.Path("."),
    recompile: bool = False,
    debug: bool = False
) -> pathlib.Path:
    """Compile the SPADE c++ executable

    :param build_directory: Absolute or relative path for the SCons build directory
    :param abaqus_command: Abaqus executable path
    :param environment: compilation environment for the ``subprocess.run`` shell call
    :param working_directory: working directory for the ``subprocess.run`` shell call
    :param recompile: Force recompile the executable
    :param debug: Show verbose SCons output

    :returns: spade executable path

    :raises RuntimeError: If the SCons command raises a ``subprocess.CalledProcessError``
    """
    spade_executable = build_directory / _settings._project_name_short
    spade_executable = spade_executable.resolve()
    if not spade_executable.exists() or recompile:
        project_options = f"--build-dir={build_directory} --abaqus-command={abaqus_command} "
        if recompile:
            project_options += " --recompile"
        scons_command = shlex.split(f"scons {project_options}", posix=(os.name == "posix"))
        if debug:
            scons_stdout = None
        else:
            scons_stdout = subprocess.PIPE
        try:
            scons_output = subprocess.run(
                scons_command,
                env=environment,
                cwd=working_directory,
                check=True,
                stdout=scons_stdout
            )
        except subprocess.CalledProcessError as err:
            message = "Could not compile with Abaqus command '{abaqus_command}'"
            if debug:
                message += f": {str(err)}"
            raise RuntimeError(message)
    return spade_executable


def cpp_execute(
    spade_executable: pathlib.Path,
    abaqus_bin: pathlib.Path,
    args: argparse.Namespace,
    environment: dict = dict(),
    debug: bool = False
) -> None:
    """Run the SPADE c++ executable

    :param spade_executable: Spade c++ executable path
    :param abaqus_bin: Abaqus bin path
    :param args: The Spade Python CLI namespace
    :param environment: compilation environment for the ``subprocess.run`` shell call
    :param debug: Show verbose SCons output

    :raises RuntimeError: If the spade c++ command raises a ``subprocess.CalledProcessError``
    """
    full_command_line_arguments = f"{spade_executable.resolve()}" + cpp_wrapper(args)
    try:
        environment["LD_LIBRARY_PATH"] = f"{abaqus_bin}:{environment['LD_LIBRARY_PATH']}"
    except KeyError:
        environment["LD_LIBRARY_PATH"] = f"{abaqus_bin}"
    command_line_arguments = shlex.split(full_command_line_arguments, posix=(os.name == "posix"))
    try:
        subprocess.run(command_line_arguments, env=environment, check=True)
    except subprocess.CalledProcessError as err:
        message = f"{_settings._project_name_short} extract failed in Abaqus ODB application"
        if debug:
            message += f": {str(err)}"
        raise RuntimeError(message)


def cpp_wrapper(args) -> str:
    """Reconstruct the c++ executable CLI from the Python CLI wrapper

    :returns: c++ CLI arguments
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

    return full_command_line_arguments


# Limit help() and 'from module import *' behavior to the module's public API
_module_objects = set(globals().keys()) - _exclude_from_namespace
__all__ = [name for name in _module_objects if not name.startswith("_")]
