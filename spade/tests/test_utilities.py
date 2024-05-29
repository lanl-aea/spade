from unittest.mock import patch
from contextlib import nullcontext as does_not_raise
import subprocess

import pytest

from spade import _utilities


def test_search_commands():
    """Test :meth:`spade._utilities.search_command`"""
    with patch("shutil.which", return_value=None) as shutil_which:
        command_abspath = _utilities.search_commands(["notfound"])
        assert command_abspath is None

    with patch("shutil.which", return_value="found") as shutil_which:
        command_abspath = _utilities.search_commands(["found"])
        assert command_abspath == "found"


find_command = {
    "first": (
        ["first", "second"], "first", does_not_raise()
    ),
    "second": (
        ["first", "second"], "second", does_not_raise()
    ),
    "none": (
        ["first", "second"], None, pytest.raises(FileNotFoundError)
    ),
}


@pytest.mark.parametrize("options, found, outcome",
                         find_command.values(),
                         ids=find_command.keys())
def test_find_command(options, found, outcome):
    """Test :meth:`spade._utilities.find_command`"""
    with patch("spade._utilities.search_commands", return_value=found), outcome:
        try:
            command_abspath = _utilities.find_command(options)
            assert command_abspath == found
        finally:
            pass


character_delimited_list = {
    "int": (
        [1, 2, 3],
        " ",
        "1 2 3"
    ),
    "int: comma": (
        [1, 2, 3],
        ",",
        "1,2,3"
    ),
    "float": (
        [1., 2., 3., 4., 5.],
        " ",
        "1.0 2.0 3.0 4.0 5.0"
    ),
    "float: multi-character": (
        [1., 2., 3., 4., 5.],
        "\n\t",
        "1.0\n\t2.0\n\t3.0\n\t4.0\n\t5.0"
    ),
    "string": (
        ["one", "two"],
        " ",
        "one two"
    ),
    "string: one": (
        ["one"],
        " ",
        "one"
    )
}


@pytest.mark.parametrize("sequence, character, expected",
                         character_delimited_list.values(),
                         ids=character_delimited_list.keys())
def test_character_delimited_list(sequence, character, expected):
    string = _utilities.character_delimited_list(sequence, character=character)
    assert string == expected
