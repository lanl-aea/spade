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
        default=[],
        help="Abaqus command for system test CLI pass through",
    )


@pytest.fixture
def system_test_directory(request: pytest.FixtureRequest) -> pathlib.Path:
    """Return the argument of custom pytest ``--system-test-dir`` command-line option."""
    return request.config.getoption("--system-test-dir")


def pytest_generate_tests(metafunc):
    """Parametrize systemt tests one per abaqus command"""
    if not metafunc.function.__name__ == "test_system":
        return
    else:
        abaqus_commands = metafunc.config.getoption("abaqus_command")
        if not abaqus_commands:
            abaqus_commands = ["abaqus"]
        metafunc.parametrize("abaqus_command", abaqus_commands)
