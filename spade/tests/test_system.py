import os
import shlex
import string
import typing
import inspect
import pathlib
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

# System tests that only require current project package
system_tests = [
    # CLI sign-of-life and help/usage
    [string.Template("${spade_command} --help")],
    [string.Template("${spade_command} docs --help")],
    [string.Template("${spade_command} extract --help")],
]
if installed:
    system_tests.append(
        # The HTML docs path doesn't exist in the repository. Can only system test from an installed package.
        [string.Template("${spade_command} docs --print-local-path")]
    )

# System tests that require third-party software. These should be marked "pytest.mark.require_third_party".
# TODO: Pass through abaqus command in pytest CLI
# https://re-git.lanl.gov/aea/python-projects/spade/-/issues/54
# TODO: add tutorials
# https://re-git.lanl.gov/aea/python-projects/spade/-/issues/22
spade_options = "--abaqus-commands /apps/abaqus/Commands/abq2023 --recompile --force-overwrite"
for odb_file in odb_files:
    system_tests.append(
        pytest.param(
            [
                f"/apps/abaqus/Commands/abq2023 fetch -job {odb_file}",
                string.Template(f"${{spade_command}} extract {odb_file} ${{spade_options}}"),
            ],
            marks=pytest.mark.require_third_party
        )
    )
for inp_file in inp_files:
    system_tests.append(
        pytest.param(
            [
                f"/apps/abaqus/Commands/abq2023 fetch -job '{inp_file}*'",
                f"/apps/abaqus/Commands/abq2023 -job {inp_file} -interactive -ask_delete no",
                string.Template(f"${{spade_command}} extract {inp_file}.odb  ${{spade_options}}"),
            ],
            marks=pytest.mark.require_third_party
        )
    )


@pytest.mark.systemtest
@pytest.mark.parametrize("commands", system_tests)
def test_system(
    system_test_directory,
    request,
    commands: typing.Iterable[str]
) -> None:
    """Run the system tests in a temporary directory

    Accepts a custom pytest CLI option to re-direct the temporary system test root directory away from ``$TMPDIR`` as

    .. code-block::

       pytest --system-test-dir=/my/systemtest/output

    :param system_test_directory: custom pytest decorator defined in conftest.py
    :param request: pytest decorator with test case meta data
    :param commands: command string or list of strings for the system test
    """
    # Attempt to construct a valid directory prefix from the test ID string printed by pytest
    # Works best if there is only one test function in this module so there are no duplicate ids
    test_id = request.node.callspec.id
    test_prefix = f"{test_id}." if " " not in test_id else None

    if system_test_directory is not None:
        system_test_directory.mkdir(parents=True, exist_ok=True)

    kwargs = {}
    temporary_directory_arguments = inspect.getfullargspec(tempfile.TemporaryDirectory).args
    if "ignore_cleanup_errors" in temporary_directory_arguments and system_test_directory is not None:
        kwargs.update({"ignore_cleanup_errors": True})
    temp_directory = tempfile.TemporaryDirectory(dir=system_test_directory, prefix=test_prefix, **kwargs)
    temp_path = pathlib.Path(temp_directory.name)
    temp_path.mkdir(parents=True, exist_ok=True)
    template_substitution = {
        "spade_command": spade_command,
        "spade_options": spade_options,
        "temp_directory": temp_directory,
    }
    try:
        for command in commands:
            if isinstance(command, string.Template):
                command = command.substitute(template_substitution)
            command = shlex.split(command)
            subprocess.check_output(command, env=env, cwd=temp_path).decode('utf-8')
    except Exception as err:
        raise Exception
    else:
        temp_directory.cleanup()
