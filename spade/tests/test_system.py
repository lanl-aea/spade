import os
import shlex
import typing
import tempfile
import subprocess
from importlib.metadata import version, PackageNotFoundError

import pytest

from spade import _settings


env = os.environ.copy()
spade_command = "spade"
odb_files = [
    "viewer_tutorial.odb",
    "w-reactor_global.odb"
]
inp_files = [
    "beamgap",
    "selfcontact_gask",
]

# If executing in repository, add package to PYTHONPATH
try:
    version("spade")
    installed = True
except PackageNotFoundError:
    # TODO: Recover from the SCons task definition?
    build_directory = _settings._project_root_abspath.parent / "build" / "systemtests"
    build_directory.mkdir(parents=True, exist_ok=True)
    installed = False

if not installed:
    spade_command = "python -m spade._main"
    package_parent_path = _settings._project_root_abspath.parent
    key = "PYTHONPATH"
    if key in env:
        env[key] = f"{package_parent_path}:{env[key]}"
    else:
        env[key] = f"{package_parent_path}"

system_tests = [
    # CLI sign-of-life and help/usage
    f"{spade_command} --help",
    f"{spade_command} docs --help",
    f"{spade_command} extract --help",
    # Tutorials
    # https://re-git.lanl.gov/aea/python-projects/spade/-/issues/22
]
# TODO: Move abaqus command to a search in SConstruct
for odb_file in odb_files:
    system_tests.append([
        f"/apps/abaqus/Commands/abq2023 fetch -job {odb_file}",
        f"{spade_command} extract {odb_file} --abaqus-commands /apps/abaqus/Commands/abq2023 abq2023 --recompile"
    ])
for inp_file in inp_files:
    system_tests.append([
        f"/apps/abaqus/Commands/abq2023 fetch -job '{inp_file}*'",
        f"/apps/abaqus/Commands/abq2023 -job {inp_file} -interactive -ask_delete no",
        f"{spade_command} extract {inp_file}.odb --abaqus-commands /apps/abaqus/Commands/abq2023 --recompile"
    ])
if installed:
    system_tests.append(
        # The HTML docs path doesn't exist in the repository. Can only system test from an installed package.
        f"{spade_command} docs --print-local-path"
    )


@pytest.mark.systemtest
@pytest.mark.parametrize("number, commands", enumerate(system_tests))
def test_run_tutorial(number: int, commands: typing.Union[str, typing.Iterable[str]]) -> None:
    """Run the system tests in a temporary directory

    :param int number: the command number. Used during local testing to separate command directories.
    :param commands: command string or list of strings for the system test
    """
    if isinstance(commands, str):
        commands = [commands]
    if installed:
        with tempfile.TemporaryDirectory() as temp_directory:
            run_commands(commands, temp_directory)
    else:
        command_directory = build_directory / f"commands{number}"
        command_directory.mkdir(parents=True, exist_ok=True)
        run_commands(commands, command_directory)


def run_commands(commands, build_directory):
    for command in commands:
        command = shlex.split(command)
        subprocess.check_output(command, env=env, cwd=build_directory).decode('utf-8')
