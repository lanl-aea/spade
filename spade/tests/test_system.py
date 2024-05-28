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
# TODO: Chase down possible race condition before re-enabling system tests
# https://re-git.lanl.gov/aea/python-projects/spade/-/issues/19
#    "w-reactor_global.odb"
]

# If executing in repository, add package to PYTHONPATH
try:
    version("spade")
    installed = True
except PackageNotFoundError:
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
    system_tests.append(
        [f"/apps/abaqus/Commands/abq2023 fetch -job {odb_file}",
         f"{spade_command} extract {odb_file} --abaqus-commands /apps/abaqus/Commands/abq2023 abq2023 --recompile"]
    )
if installed:
    system_tests.append(
        # The HTML docs path doesn't exist in the repository. Can only system test from an installed package.
        f"{spade_command} docs --print-local-path"
    )


@pytest.mark.systemtest
@pytest.mark.parametrize("commands", system_tests)
def test_run_tutorial(commands: typing.Union[str, typing.Iterable[str]]) -> None:
    """Run the system tests in a temporary directory

    :param commands: command string or list of strings for the system test
    """
    if isinstance(commands, str):
        commands = [commands]
    with tempfile.TemporaryDirectory() as temp_directory:
        for command in commands:
            command = shlex.split(command)
            subprocess.check_output(command, env=env, cwd=temp_directory).decode("utf-8")
