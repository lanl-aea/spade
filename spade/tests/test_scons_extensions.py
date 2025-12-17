"""Test spade SCons builders and support functions."""

import pytest
import SCons
import SCons.Environment

from spade import _settings, scons_extensions


def check_nodes(
    nodes: SCons.Node.NodeList,
    node_count: int,
    action_count: int,
    expected_string: str,
    expected_env_kwargs: dict,
) -> None:
    """Verify the expected action string against a builder's target nodes.

    :param nodes: Target node list returned by a builder
    :param node_count: expected length of ``nodes``
    :param action_count: expected length of action list for each node
    :param expected_string: the builder's action string.
    :param expected_env_kwargs: the builder's expected environment keyword arguments

    .. note::

       The method of interrogating a node's action list results in a newline separated string instead of a list of
       actions. The ``expected_string`` should contain all elements of the expected action list as a single, newline
       separated string. The ``action_count`` should be set to ``1`` until this method is updated to search for the
       finalized action list.
    """
    assert len(nodes) == node_count
    for node in nodes:
        node.get_executor()
        assert len(node.executor.action_list) == action_count
        assert str(node.executor.action_list[0]) == expected_string
        for key, value in expected_env_kwargs.items():
            assert node.env[key] == value


# TODO: Figure out how to cleanly reset the construction environment between parameter sets
test_cli_builder = {
    "cli_builder": (
        "cli_builder",
        {},
        1,
        1,
        ["cli_builder.txt"],
        ["cli_builder.txt.stdout"],
        {
            "program": "spade",
            "subcommand": "",
            "abaqus_commands": " ".join(str(path) for path in _settings._default_abaqus_commands),
        },
    ),
    "cli_builder with subcommand": (
        "cli_builder",
        {"subcommand": "subcommand"},
        1,
        1,
        ["cli_builder.txt"],
        ["cli_builder.txt.stdout"],
        {
            "program": "spade",
            "subcommand": "subcommand",
            "abaqus_commands": " ".join(str(path) for path in _settings._default_abaqus_commands),
        },
    ),
}


@pytest.mark.parametrize(
    ("builder", "kwargs", "node_count", "action_count", "source_list", "target_list", "expected_env_kwargs"),
    test_cli_builder.values(),
    ids=test_cli_builder.keys(),
)
def test_cli_builder(
    builder: str,
    kwargs: dict,
    node_count: int,
    action_count: int,
    source_list: list[str],
    target_list: list[str],
    expected_env_kwargs: dict,
) -> None:
    env = SCons.Environment.Environment()
    expected_string = (
        "${cd_action_prefix} ${program} ${subcommand} ${required} ${options} "
        "--abaqus-commands ${abaqus_commands} "
        "${redirect_action_postfix}"
    )

    env.Append(BUILDERS={builder: scons_extensions.cli_builder(**kwargs)})
    nodes = env["BUILDERS"][builder](env, target=target_list, source=source_list)
    check_nodes(nodes, node_count, action_count, expected_string, expected_env_kwargs)


test_builders = {
    "extract": (
        "extract",
        {},
        1,
        1,
        ["target.odb"],
        ["target.odb.stdout"],
        {
            "program": "spade",
            "subcommand": "extract",
            "abaqus_commands": " ".join(str(path) for path in _settings._default_abaqus_commands),
            "required": "${SOURCE.abspath} --extracted-file ${TARGET.abspath} --force-overwrite",
        },
    ),
}


@pytest.mark.parametrize(
    ("builder", "kwargs", "node_count", "action_count", "source_list", "target_list", "expected_env_kwargs"),
    test_builders.values(),
    ids=test_builders.keys(),
)
def test_builders(
    builder: str,
    kwargs: dict,
    node_count: int,
    action_count: int,
    source_list: list[str],
    target_list: list[str],
    expected_env_kwargs: dict,
) -> None:
    env = SCons.Environment.Environment()
    expected_string = (
        "${cd_action_prefix} ${program} ${subcommand} ${required} ${options} "
        "--abaqus-commands ${abaqus_commands} "
        "${redirect_action_postfix}"
    )

    builder_function = getattr(scons_extensions, builder)
    env.Append(BUILDERS={builder: builder_function(**kwargs)})
    nodes = env["BUILDERS"][builder](env, target=target_list, source=source_list)
    check_nodes(nodes, node_count, action_count, expected_string, expected_env_kwargs)
