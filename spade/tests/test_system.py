import getpass
import importlib
import os
import re
import string
import typing
import inspect
import pathlib
import platform
import tempfile
import subprocess
from unittest.mock import Mock, patch

import pytest

from spade import _settings

MODULE_NAME = pathlib.Path(__file__).stem
PACKAGE_PARENT_PATH = _settings._project_root_abspath.parent


# TODO: Remove user check when Windows CI server Gitlab-Runner account has access to the Abaqus license server
# https://re-git.lanl.gov/aea/python-projects/waves/-/issues/984
def check_ci_user() -> bool:
    user = getpass.getuser().lower()
    return "pn2606796" in user or user == "gitlab-runner"


test_check_ci_user_cases = {
    "windows ci user": ("PN2606796$", True),
    "windows ci user without trailing service account character": ("PN2606796", True),
    "macos ci user": ("gitlab-runner", True),
    "player character": ("roppenheimer", False),
}


@pytest.mark.parametrize(
    ("mock_user", "expected"),
    test_check_ci_user_cases.values(),
    ids=test_check_ci_user_cases.keys(),
)
def test_check_ci_user(mock_user: str, expected: bool) -> None:
    with patch("getpass.getuser", return_value=mock_user):
        testing_ci_user = check_ci_user()
    assert testing_ci_user is expected


def check_installed(package_name: str = "spade") -> bool:
    try:
        importlib.metadata.version(package_name)
        return True
    except importlib.metadata.PackageNotFoundError:
        return False


def test_check_installed() -> None:
    with patch("importlib.metadata.version", return_value="0.0.0"):
        assert check_installed()
    with patch("importlib.metadata.version", side_effect=importlib.metadata.PackageNotFoundError):
        assert not check_installed()


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


def create_test_prefix(request: pytest.FixtureRequest, module_name: str = MODULE_NAME) -> str:
    test_id = request.node.callspec.id
    test_identifier = create_valid_identifier(test_id)
    return f"{module_name}.{test_identifier}."


test_create_test_prefix_cases = {
    "test_identifier": ({}, "test_system.test_identifier."),
    "test identifier": ({}, "test_system.test_identifier."),
    "another_test_identifier": ({}, "test_system.another_test_identifier."),
    "override_module_name": ({"module_name": "another_module_name"}, "another_module_name.override_module_name."),
}


@pytest.mark.parametrize(
    ("kwargs", "expected"),
    test_create_test_prefix_cases.values(),
    ids=test_create_test_prefix_cases.keys(),
)
def test_create_test_prefix(kwargs: dict[str, str], expected: str, request: pytest.FixtureRequest) -> None:
    assert create_test_prefix(request, **kwargs) == expected


def return_temporary_directory_kwargs(
    system_test_directory: pathlib.Path | None,
    keep_system_tests: bool,
) -> dict:
    kwargs = {}
    temporary_directory_inspection = inspect.getfullargspec(tempfile.TemporaryDirectory)
    temporary_directory_arguments = temporary_directory_inspection.args + temporary_directory_inspection.kwonlyargs
    if "ignore_cleanup_errors" in temporary_directory_arguments and system_test_directory is not None:
        kwargs.update({"ignore_cleanup_errors": True})
    if keep_system_tests and "delete" in temporary_directory_arguments:
        kwargs.update({"delete": False})
    return kwargs


test_return_temporary_directory_kwargs_cases = {
    "no arguments": (None, False, [], [], {}),
    "directory": (pathlib.Path("dummy/path"), False, [], [], {}),
    "directory and keep": (pathlib.Path("dummy/path"), True, [], [], {}),
    "directory: matching directory kwarg": (
        pathlib.Path("dummy/path"),
        False,
        ["ignore_cleanup_errors"],
        [],
        {"ignore_cleanup_errors": True},
    ),
    "directory and keep: matching keep kwarg": (pathlib.Path("dummy/path"), True, [], ["delete"], {"delete": False}),
    "directory and keep: matching both kwargs": (
        pathlib.Path("dummy/path"),
        True,
        ["ignore_cleanup_errors"],
        ["delete"],
        {"ignore_cleanup_errors": True, "delete": False},
    ),
}


@pytest.mark.parametrize(
    ("system_test_directory", "keep_system_tests", "available_args", "available_kwargs", "expected"),
    test_return_temporary_directory_kwargs_cases.values(),
    ids=test_return_temporary_directory_kwargs_cases.keys(),
)
def test_return_temporary_directory_kwargs(
    system_test_directory: pathlib.Path,
    keep_system_tests: bool,
    available_args: list[str],
    available_kwargs: list[str],
    expected: dict[str, bool],
) -> None:
    mock_inspection = Mock()
    mock_inspection.args = available_args
    mock_inspection.kwonlyargs = available_kwargs
    with patch("inspect.getfullargspec", return_value=mock_inspection):
        kwargs = return_temporary_directory_kwargs(system_test_directory, keep_system_tests)
        assert kwargs == expected


env = os.environ.copy()
spade_command = "spade"
system = platform.system().lower()
testing_windows = True if system == "windows" else False
testing_macos = True if system == "darwin" else False
testing_ci_user = check_ci_user()
installed = check_installed()
if not installed:
    spade_command = "python -m spade._main"
    key = "PYTHONPATH"
    if key in env:
        env[key] = f"{package_parent_path}{os.pathsep}{env[key]}"
    else:
        env[key] = f"{PACKAGE_PARENT_PATH}"


# System tests that only require current project package
system_tests = [
    # CLI sign-of-life and help/usage
    [string.Template("${spade_command} --help")],
    [string.Template("${spade_command} docs --help")],
    [string.Template("${spade_command} extract --help")],
    pytest.param(
        [string.Template("${spade_command} docs --print-local-path")],
        marks=[pytest.mark.skipif(not installed, reason="The HTML docs path only exists in the as-installed packages")],
    ),
]


@pytest.mark.systemtest
@pytest.mark.parametrize("commands", system_tests)
def test_system(
    system_test_directory: pathlib.Path | None,
    keep_system_tests: bool,
    request: pytest.FixtureRequest,
    commands: typing.Iterable[str],
) -> None:
    run_system_test(system_test_directory, keep_system_tests, request, commands)


# System tests that require third-party software, e.g. Abaqus.
# TODO: add tutorials
# https://re-git.lanl.gov/aea/python-projects/spade/-/issues/22
odb_files = ["viewer_tutorial.odb", "w-reactor_global.odb"]
inp_files = [
    "beamgap",
    "selfcontact_gask",
    "erode_proj_and_plate",
]
spade_options = "--recompile --force-overwrite --verbose --debug"
system_tests_require_third_party = []
for odb_file in odb_files:
    system_tests_require_third_party.append(
        pytest.param(
            [
                string.Template(f"${{abaqus_command}} fetch -job {odb_file}"),
                string.Template(
                    f"${{spade_command}} extract {odb_file} --abaqus-commands ${{abaqus_command}} ${{spade_options}}"
                ),
            ],
            marks=[
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
    system_tests_require_third_party.append(
        pytest.param(
            [
                string.Template(f'${{abaqus_command}} fetch -job "{fetch_string}*"'),
                string.Template(f"${{abaqus_command}} -job {inp_file} -interactive -ask_delete no"),
                string.Template(
                    f"${{spade_command}} extract {inp_file}.odb"
                    f" --abaqus-commands ${{abaqus_command}} ${{spade_options}}"
                ),
            ],
            marks=[
                pytest.mark.skipif(testing_macos, reason="Abaqus does not install on macOS"),
            ],
            id=inp_file,
        )
    )


# TODO: Remove user check when Windows CI Gitlab-Runner account can access the Abaqus license server
# https://re-git.lanl.gov/aea/python-projects/waves/-/issues/984
@pytest.mark.skipif(
    testing_windows and testing_ci_user,
    reason="Windows CI server Gitlab-Runner user does not have access to Abaqus license server",
)
@pytest.mark.systemtest
@pytest.mark.require_third_party
@pytest.mark.parametrize("commands", system_tests_require_third_party)
def test_system_require_third_party(
    system_test_directory: typing.Optional[pathlib.Path],
    keep_system_tests: bool,
    request: pytest.FixtureRequest,
    commands: typing.Iterable[str],
    abaqus_command: str | None,
) -> None:
    run_system_test(system_test_directory, keep_system_tests, request, commands, abaqus_command=abaqus_command)


def run_system_test(
    system_test_directory: pathlib.Path | None,
    keep_system_tests: bool,
    request: pytest.FixtureRequest,
    commands: typing.Iterable[str | string.Template],
    abaqus_command: str | None = None,
) -> None:
    """Run shell commands as system tests in a temporary directory.

    Test directory name is constructed from test ID string, with character replacements to create a valid Python
    identifier as a conservative estimate of a valid directory name. Failed tests persist on disk.

    Accepts a custom pytest CLI option to re-direct the temporary system test root directory away from ``$TMPDIR`` as

    .. code-block::

       pytest --system-test-dir=/my/systemtest/output

    :param system_test_directory: custom pytest decorator defined in conftest.py
    :param keep_system_tests: custom pytest decorator defined in conftest.py
    :param request: pytest decorator with test case meta data
    :param commands: command string or list of strings for the system test
    :param abaqus_command: custom pytest fixture defined in conftest.py
    """
    if system_test_directory is not None:
        system_test_directory.mkdir(parents=True, exist_ok=True)

    # TODO: Move to common test utility VVV
    # Naive move to waves/_tests/common.py resulted in every test failing with FileNotFoundError.
    # Probably tempfile is handling some scope existence that works when inside the function but not when it's outside.
    temporary_directory = tempfile.TemporaryDirectory(
        dir=system_test_directory,
        prefix=create_test_prefix(request),
        **return_temporary_directory_kwargs(system_test_directory, keep_system_tests),
    )
    temporary_path = pathlib.Path(temporary_directory.name)
    temporary_path.mkdir(parents=True, exist_ok=True)
    # Move to common test utility ^^^

    template_substitution = {
        "spade_command": spade_command,
        "spade_options": spade_options,
        "abaqus_command": abaqus_command,
        "temporary_directory": temporary_path,
    }
    try:
        for command in commands:
            if isinstance(command, string.Template):
                command = command.substitute(template_substitution)
            subprocess.check_output(command, env=env, cwd=temporary_path, text=True, shell=True)
    except Exception as err:
        raise err
    else:
        if not keep_system_tests:
            temporary_directory.cleanup()
