import os
import shlex
import string
import typing
import pathlib
import tempfile
import subprocess
from importlib.metadata import version, PackageNotFoundError

import pytest

from spade import _settings


env = os.environ.copy()
spade_command = "spade"
odb_extract_command = "odb_extract"

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
    [f"{spade_command} --help"],
    [f"{spade_command} docs --help"],
    # Tutorials
]
if installed:
    system_tests.append(
        # The HTML docs path doesn't exist in the repository. Can only system test from an installed package.
        [f"{spade_command} docs --print-local-path"]
    )


@pytest.mark.systemtest
@pytest.mark.parametrize("commands", system_tests)
def test_run_tutorial(commands: typing.Iterable[str]) -> None:
    """Fetch and run the tutorial configuration file(s) as system tests in a temporary directory

    Iterates on the command strings in the commands list. Performs string template substitution using keys:

    * ``spade_command``
    * ``temp_directory``

    :param commands: list of command strings for the system test
    """
    with tempfile.TemporaryDirectory() as temp_directory:
        template_substitution = {
            "spade_command": spade_command,
            "temp_directory": temp_directory
        }
        for command in commands:
            if isinstance(command, string.Template):
                command = command.substitute(template_substitution)
            command = command.split(" ")
            subprocess.check_output(command, env=env, cwd=temp_directory).decode("utf-8")
