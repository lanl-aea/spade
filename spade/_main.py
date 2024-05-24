import os
import argparse
import sys
import shlex
import subprocess
import pathlib
import platform

from spade import _settings
from spade import __version__


def main():
    """This is the main function that performs actions based on command line arguments.

    :returns: return code
    """
    parser = get_parser()
    args, unknown = parser.parse_known_args()
    full_command_line_arguments = ""

    # File name inputs
    if args.odb_file:
        full_command_line_arguments += f" {args.odb_file}"
    else:
        print("Abaqus output database (odb) file not specified.", file=sys.stderr)
        return 1
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

    abaqus_prefix = ""
    for prefix in _settings._abaqus_prefix:
        if prefix.exists():
            abaqus_prefix = prefix
    abaqus_prefix = abaqus_prefix / _settings._additional_abaqus_prefix
    abaqus_versions = [_.name for _ in abaqus_prefix.glob('*') if _.is_dir()]
    if args.abaqus_version not in abaqus_versions:
        args.abaqus_version = max(abaqus_versions)
    abaqus_prefix = abaqus_prefix / args.abaqus_version
    abaqus_path = abaqus_prefix / _settings._abaqus_suffix
    source_directory = pathlib.Path(__file__).parent
    platform_string = "_".join(f"{platform.system()} {platform.release()}".split())
    spade_version = source_directory / f"{_settings._project_name_short}_{args.abaqus_version}_{platform_string}"
    current_env = os.environ.copy()
    if not spade_version.exists():
        # Compile necessary version
        scons_command = shlex.split(f"scons abaqus_version={args.abaqus_version} platform={platform_string} "
                                   f"--directory={source_directory.resolve()}",
                                   posix=(os.name == 'posix'))
        sub_process = subprocess.Popen(scons_command, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, env=current_env, cwd=f"{source_directory}")
        out, error_code = sub_process.communicate()
        if error_code:
            print(error_code.decode("utf-8"), file=sys.stderr)
            print("Could not compile with specified Abaqus version")
            sys.exit(sub_process.returncode)
    full_command_line_arguments = str(spade_version) + full_command_line_arguments

    try:
        current_env['LD_LIBRARY_PATH'] = f"{abaqus_path}:{current_env['LD_LIBRARY_PATH']}"
    except KeyError:
        current_env['LD_LIBRARY_PATH'] = f"{abaqus_path}"
    command_line_arguments = shlex.split(full_command_line_arguments, posix=(os.name == 'posix'))
    sub_process = subprocess.Popen(command_line_arguments, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, env=current_env)
    out, error_code = sub_process.communicate()
    if out:
        print(out.decode("utf-8"))
    if error_code:
        print(error_code.decode("utf-8"), file=sys.stderr)
    return_code = sub_process.returncode

    if return_code:
        return return_code
    else:
        return 0


def get_parser():
    """Get parser object for command line options

    :return: parser
    :rtype: ArgumentParser
    """
    main_parser = argparse.ArgumentParser(description=_settings._project_description,
                                          prog=f"{_settings._project_name_short}")

    main_parser.add_argument('-v', '--version', action='version',
                             version=f'{_settings._project_name_short} {__version__}')

    # File inputs
    main_parser.add_argument(nargs='?', dest='odb_file', type=str,
                    help='odb file from which to extract data',
                    metavar='ODB_FILE.odb')
    main_parser.add_argument('-e', '--extracted-file', type=str,
                             help='Name of extracted file. (default: <odb file name>.h5)')
    main_parser.add_argument('-l', '--log-file', type=str,
                             help=f'Name of log file. (default: <odb file name>.{_settings._project_name_short}.h5)')

    # String inputs
    # main_parser.add_argument('-t', '--extracted-file-type', type=str, default="hdf5",
                            #  help='Type of file for storing extracted output (default: %(default)s)')
    main_parser.add_argument('--frame', type=str, default="all",
                             help='Get information from the specified frame (default: %(default)s)')
    main_parser.add_argument('--frame-value', type=str, default="all",
                             help='Get information from the specified frame value (default: %(default)s)')
    main_parser.add_argument('--step', type=str, default="all",
                             help='Get information from the specified step (default: %(default)s)')
    main_parser.add_argument('--field', type=str, default="all",
                             help='Get information from the specified field (default: %(default)s)')
    main_parser.add_argument('--history', type=str, default="all",
                             help='Get information from the specified history value (default: %(default)s)')
    main_parser.add_argument('--history-region', type=str, default="all",
                             help='Get information from the specified history region (default: %(default)s)')
    main_parser.add_argument('--instance', type=str, default="all",
                             help='Get information from the specified instance (default: %(default)s)')
    main_parser.add_argument('-a', '--abaqus-version', type=str, default=_settings._default_abaqus_version,
                             help='Get information from the specified instance (default: %(default)s)')

    # True or false inputs
    main_parser.add_argument('-V', '--verbose', action='store_true', default=False,
                             help='Turn on verbose logging')
    main_parser.add_argument('-f', '--force-overwrite', action='store_true', default=False,
                             help='Force the overwrite of the hdf5 file if it already exists')
    main_parser.add_argument('-d', '--debug', action='store_true', default=False, help=argparse.SUPPRESS)

    return main_parser


if __name__ == "__main__":
    sys.exit(main())  # pragma: no cover
