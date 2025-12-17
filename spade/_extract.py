import argparse
import errno
import os
import pathlib
import platform
import shlex
import subprocess
import sys
import tempfile

from spade import _settings, _utilities

_exclude_from_namespace = set(globals().keys())

# TODO: Find a better way to define optional print functions when using API instead of CLI/main function
# https://re-git.lanl.gov/aea/python-projects/spade/-/issues/40
print_verbose = lambda *a, **k: None  # noqa: E731
print_debug = lambda *a, **k: None  # noqa: E731


# TODO: full API
def main(args: argparse.Namespace) -> None:
    """Main parser behavior when no subcommand is specified.

    :param args: argument namespace

    :raises RuntimeError: If any subprocess returns a non-zero exit code or an Abaqus ODB file is not provided.
    """
    if args.debug:
        args.verbose = True

    # TODO: Find a better way to define optional print functions when using API instead of CLI/main function
    # https://re-git.lanl.gov/aea/python-projects/spade/-/issues/40
    global print_verbose
    global print_debug
    print_verbose = print if args.verbose else lambda *a, **k: None
    print_debug = print if args.debug else lambda *a, **k: None
    current_env = os.environ.copy()

    # Find Abaqus
    try:
        abaqus_command = _utilities.find_command(args.abaqus_commands)
    except FileNotFoundError as err:
        raise RuntimeError(str(err))
    print_verbose(f"Found Abaqus command: {abaqus_command}")
    abaqus_version = _utilities.abaqus_official_version(abaqus_command)
    print_verbose(f"Found Abaqus version: {abaqus_version}")
    _, abaqus_bin, _ = _utilities.return_abaqus_code_paths(abaqus_command)
    print_verbose(f"Found Abaqus bin: {abaqus_bin}")

    try:
        with tempfile.TemporaryDirectory(dir=".", prefix=f"{_settings._project_name_short}.") as temporary_directory:
            temporary_path = pathlib.Path(temporary_directory).resolve()

            # Compile c++ executable
            print_verbose(f"Compiling and linking against Abaqus {abaqus_version}")
            spade_executable = cpp_compile(
                build_directory=temporary_path,
                abaqus_command=abaqus_command,
                environment=current_env,
                working_directory=_settings._project_root_abspath,
                recompile=args.recompile,
                debug=args.debug,
            )

            # Run c++ executable
            print_verbose(f"Running extract for file: {args.ODB_FILE}")
            cpp_execute(
                spade_executable=spade_executable,
                abaqus_bin=abaqus_bin,
                args=args,
                environment=current_env,
            )
    except OSError as err:
        if err.errno == errno.ENOTEMPTY:
            print(
                f"Failed to clean up temporary directory. You can safely remove this directory.\n{err}",
                file=sys.stderr,
            )
        else:
            raise err


def get_parser() -> argparse.ArgumentParser:
    """Return a 'no-help' parser for the extract subcommand.

    :return: parser
    """
    parser = argparse.ArgumentParser(add_help=False)

    # File inputs
    parser.add_argument(
        "ODB_FILE",
        type=str,
        help="ODB file from which to extract data",
        metavar="ODB_FILE.odb",
    )
    parser.add_argument(
        "-e",
        "--extracted-file",
        type=str,
        help="Name of extracted file. (default: <ODB file name>.h5)",
    )
    parser.add_argument(
        "-l",
        "--log-file",
        type=str,
        help=f"Name of log file. (default: <ODB file name>.{_settings._project_name_short}.log)",
    )

    parser.add_argument(
        "--frame",
        nargs="+",
        default="all",
        help="Get information from the specified frame(s) (default: %(default)s)",
    )  # Lambda used to convert input into a string with double quotes on either side
    parser.add_argument(
        "--frame-value",
        nargs="+",
        default="all",
        help="Get information from the specified frame value(s) (default: %(default)s)",
    )
    parser.add_argument(
        "--step",
        nargs="+",
        default="all",
        help="Get information from the specified step(s) (default: %(default)s)",
    )
    parser.add_argument(
        "--field",
        nargs="+",
        default="all",
        help="Get information from the specified field(s) (default: %(default)s)",
    )
    parser.add_argument(
        "--history",
        nargs="+",
        default="all",
        help="Get information from the specified history value(s) (default: %(default)s)",
    )
    parser.add_argument(
        "--history-region",
        nargs="+",
        default="all",
        help="Get information from the specified history region(s) (default: %(default)s)",
    )
    parser.add_argument(
        "--instance",
        nargs="+",
        default="all",
        help="Get information from the specified instance(s) (default: %(default)s)",
    )
    parser.add_argument(
        "-a",
        "--abaqus-commands",
        nargs="+",
        type=pathlib.Path,
        default=_settings._default_abaqus_commands,
        help=(
            "Ordered list of Abaqus executable paths. Use first found "
            f"(default: {_utilities.character_delimited_list(_settings._default_abaqus_commands)})"
        ),
    )
    parser.add_argument(
        "--format",
        type=str,
        choices=["extract", "odb", "vtk"],
        default="extract",
        help="Specify the format of the data in the output file",
    )

    # True or false inputs
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Turn on verbose logging",
    )
    parser.add_argument(
        "-f",
        "--force-overwrite",
        action="store_true",
        default=False,
        help="Overwrite the extracted and log file(s)",
    )
    parser.add_argument(
        "-d",
        "--debug",
        action="store_true",
        default=False,
        help=argparse.SUPPRESS,
    )
    parser.add_argument(
        "--recompile",
        action="store_true",
        default=False,
        help=argparse.SUPPRESS,
    )
    return parser


def cpp_compile(
    build_directory: pathlib.Path = pathlib.Path("build"),
    abaqus_command: pathlib.Path = pathlib.Path("abaqus"),
    environment: dict | None = None,
    working_directory: pathlib.Path = pathlib.Path(),
    recompile: bool = False,
    debug: bool = False,
) -> pathlib.Path:
    """Compile the SPADE c++ executable.

    :param build_directory: Absolute or relative path for the SCons build directory
    :param abaqus_command: Abaqus executable path
    :param environment: compilation environment for the ``subprocess.run`` shell call
    :param working_directory: working directory for the ``subprocess.run`` shell call
    :param recompile: Force recompile the executable
    :param debug: Show verbose SCons output

    :returns: spade executable path

    :raises RuntimeError: If the SCons command raises a ``subprocess.CalledProcessError``
    """
    if environment is None:
        environment = {}
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
        print_debug(f"Compiling {_settings._project_name_short} with command {scons_command}")
        try:
            subprocess.run(
                scons_command,
                env=environment,
                cwd=working_directory,
                check=True,
                stdout=scons_stdout,
            )
        except subprocess.CalledProcessError as err:
            message = f"Could not compile with Abaqus command '{abaqus_command}': {err!s}"
            raise RuntimeError(message)
    return spade_executable


def cpp_execute(
    spade_executable: pathlib.Path,
    abaqus_bin: pathlib.Path,
    args: argparse.Namespace,
    environment: dict | None = None,
    working_directory: pathlib.Path = pathlib.Path(),
) -> None:
    """Run the SPADE c++ executable.

    :param spade_executable: Spade c++ executable path
    :param abaqus_bin: Abaqus bin path
    :param args: The Spade Python CLI namespace
    :param environment: compilation environment for the ``subprocess.run`` shell call

    :raises RuntimeError: If the spade c++ command raises a ``subprocess.CalledProcessError``
    """
    if environment is None:
        environment = {}
    full_command_line_arguments = f"{spade_executable.resolve()}" + cpp_wrapper(args)
    if platform.system().lower() == "windows":
        environment["PATH"] = f"{abaqus_bin};{abaqus_bin}32;{environment['PATH']}"
    else:
        try:
            environment["LD_LIBRARY_PATH"] = f"{abaqus_bin}:{environment['LD_LIBRARY_PATH']}"
        except KeyError:
            environment["LD_LIBRARY_PATH"] = f"{abaqus_bin}"
    command_line_arguments = shlex.split(full_command_line_arguments, posix=(os.name == "posix"))
    print_debug(f"Running {_settings._project_name_short} with command {command_line_arguments}")
    try:
        subprocess.run(command_line_arguments, env=environment, cwd=working_directory, check=True)
    except subprocess.CalledProcessError as err:
        message = f"{_settings._project_name_short} extract failed in Abaqus ODB application: {err!s}"
        raise RuntimeError(message)


def cpp_wrapper(args: argparse.Namespace) -> str:
    """Reconstruct the c++ executable CLI from the Python CLI wrapper.

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
        full_command_line_arguments += f" --log-file {args.log_file}"

    # String inputs
    if args.frame:
        full_command_line_arguments += f" --frame {_utilities.quoted_string(args.frame)}"
    if args.frame_value:
        full_command_line_arguments += f" --frame-value {_utilities.quoted_string(args.frame_value)}"
    if args.step:
        full_command_line_arguments += f" --step {_utilities.quoted_string(args.step)}"
    if args.field:
        full_command_line_arguments += f" --field {_utilities.quoted_string(args.field)}"
    if args.history:
        full_command_line_arguments += f" --history {_utilities.quoted_string(args.history)}"
    if args.history_region:
        full_command_line_arguments += f" --history-region {_utilities.quoted_string(args.history_region)}"
    if args.instance:
        full_command_line_arguments += f" --instance {_utilities.quoted_string(args.instance)}"
    if args.format:
        full_command_line_arguments += f" --format {args.format}"

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
