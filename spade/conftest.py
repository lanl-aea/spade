"""Define the project's custom pytest options and CLI extensions."""

import pathlib

import pytest


def pytest_addoption(parser):
    """Add the custom pytest options to the pytest command-line parser."""
    parser.addoption(
        "--system-test-dir",
        action="store",
        type=pathlib.Path,
        default=None,
        help="system test build directory root",
    )
    parser.addoption(
        "--abaqus-command",
        action="append",
        default=["abaqus"],
        help=(
            "Abaqus command for system test CLI pass through."
            " Repeat to test against more than one Abaqus command (default %(default)s)"
        ),
    )


@pytest.fixture
def system_test_directory(request: pytest.FixtureRequest) -> pathlib.Path:
    """Return the argument of custom pytest ``--system-test-dir`` command-line option."""
    return request.config.getoption("--system-test-dir")


def pytest_generate_tests(metafunc):
    """Parametrize systemt tests one per abaqus command"""
    if not metafunc.function.__name__ == "test_system_require_third_party":
        return
    else:
        metafunc.parametrize("abaqus_command", metafunc.config.getoption("abaqus_command"))
