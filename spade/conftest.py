import pathlib

import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--system-test-dir",
        action="store",
        type=pathlib.Path,
        default=None,
        help="system test build directory root",
    )
    parser.addoption(
        "--abaqus-command",
        action="store",
        type=pathlib.Path,
        default=None,
        help="Abaqus command for system test CLI pass through",
    )


@pytest.fixture
def system_test_directory(request):
    return request.config.getoption("--system-test-dir")


@pytest.fixture
def abaqus_command(request):
    return request.config.getoption("--abaqus-command")
