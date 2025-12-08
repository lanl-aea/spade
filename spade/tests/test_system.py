import os
import re
import shlex
import string
import typing
import inspect
import pathlib
import platform
import tempfile
import subprocess
from importlib.metadata import version, PackageNotFoundError

import pytest

from spade import _settings


env = os.environ.copy()
spade_command = "spade"
odb_files = ["viewer_tutorial.odb", "w-reactor_global.odb"]
inp_files = [
    "beamgap",
    "selfcontact_gask",
    "erode_proj_and_plate",
]
testing_macos = True if platform.system().lower() == "darwin" else False

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
# TODO: add tutorials
# https://re-git.lanl.gov/aea/python-projects/spade/-/issues/22
spade_options = "--recompile --force-overwrite --verbose --debug"
for odb_file in odb_files:
    system_tests.append(
        pytest.param(
            [
                string.Template(f"${{abaqus_command}} fetch -job {odb_file}"),
                string.Template(
                    f"${{spade_command}} extract {odb_file} --abaqus-commands ${{abaqus_command}} ${{spade_options}}"
                ),
            ],
            marks=[
                pytest.mark.require_third_party,
                pytest.mark.skipif(testing_macos, reason="Abaqus does not install on macOS"),
            ],
            id=odb_file,
        )
    )
for inp_file in inp_files:
    if inp_file.startswith("erode"):
        fetch_string = "erode_"
    else:
        fetch_string = inp_file
    system_tests.append(
        pytest.param(
            [
                string.Template(f"${{abaqus_command}} fetch -job '{fetch_string}*'"),
                string.Template(f"${{abaqus_command}} -job {inp_file} -interactive -ask_delete no"),
                string.Template(
                    f"${{spade_command}} extract {inp_file}.odb --abaqus-commands ${{abaqus_command}} ${{spade_options}}"
                ),
            ],
            marks=[
                pytest.mark.require_third_party,
                pytest.mark.skipif(testing_macos, reason="Abaqus does not install on macOS"),
            ],
            id=inp_file,
        )
    )


@pytest.mark.systemtest
@pytest.mark.parametrize("commands", system_tests)
def test_system(
    system_test_directory,
    abaqus_command,
    request,
    commands: typing.Iterable[str],
) -> None:
    """Run the system tests in a temporary directory

    Accepts a custom pytest CLI option to re-direct the temporary system test root directory away from ``$TMPDIR`` as

    .. code-block::

       pytest --system-test-dir=/my/systemtest/output

    :param system_test_directory: custom pytest decorator defined in conftest.py
    :param abaqus_command: custom pytest decorator defined in conftest.py
    :param request: pytest decorator with test case meta data
    :param commands: command string or list of strings for the system test
    """
    module_name = pathlib.Path(__file__).stem
    test_id = request.node.callspec.id
    test_prefix = create_valid_identifier(test_id)
    test_prefix = f"{module_name}.{test_prefix}."

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
        "abaqus_command": abaqus_command,
        "temp_directory": temp_directory,
    }
    try:
        for command in commands:
            if isinstance(command, string.Template):
                command = command.substitute(template_substitution)
            command = shlex.split(command)
            subprocess.check_output(command, env=env, cwd=temp_path).decode("utf-8")
    except Exception as err:
        raise err
    else:
        temp_directory.cleanup()


def create_valid_identifier(identifier: str) -> None:
    """Create a valid Python identifier from an arbitray string by replacing invalid characters with underscores

    :param identifier: String to convert to valid Python identifier
    """
    return re.sub(r"\W|^(?=\d)", "_", identifier)


create_valid_identifier_tests = {
    "leading digit": ("1word", "_1word"),
    "leading space": (" word", "_word"),
    "replace slashes and spaces": ("w o /rd", "w_o__rd"),
    "replace special characters": ("w%o @rd", "w_o__rd"),
}


@pytest.mark.parametrize(
    "identifier, expected",
    create_valid_identifier_tests.values(),
    ids=create_valid_identifier_tests.keys(),
)
def test_create_valid_identifier(identifier, expected) -> None:
    returned = create_valid_identifier(identifier)
    assert returned == expected
