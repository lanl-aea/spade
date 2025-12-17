"""Define the project's custom pytest options and CLI extensions."""

import pathlib

import pytest


def pytest_addoption(parser: pytest.Parser) -> None:
    """Add the custom pytest options to the pytest command-line parser."""
    parser.addoption(
        "--system-test-dir",
        action="store",
        type=pathlib.Path,
        default=None,
        help="system test build directory root",
    )
    parser.addoption(
        "--keep-system-tests",
        action="store_true",
        default=False,
        help="flag to skip system test directory cleanup",
    )
    parser.addoption(
        "--abaqus-command",
        action="append",
        type=str,
        default=None,
        help=(
            "Abaqus command for system test CLI pass through."
            " Repeat to test against more than one Abaqus command (default ['abaqus'])"
        ),
    )


@pytest.fixture
def system_test_directory(request: pytest.FixtureRequest) -> pathlib.Path:
    """Return the argument of custom pytest ``--system-test-dir`` command-line option."""
    return request.config.getoption("--system-test-dir")


@pytest.fixture
def keep_system_tests(request: pytest.FixtureRequest) -> bool:
    """Return the argument of custom pytest ``--keep-system-tests`` command-line option."""
    return request.config.getoption("--keep-system-tests")


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    """Parametrize systemt tests one per abaqus command."""
    if metafunc.function.__name__ != "test_system_require_third_party":
        return
    else:
        abaqus_commands = metafunc.config.getoption("abaqus_command")
        if abaqus_commands is None:
            abaqus_commands = ["abaqus"]
        metafunc.parametrize("abaqus_command", abaqus_commands)
