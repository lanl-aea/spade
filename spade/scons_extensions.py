"""
.. warning::

   The CLI design is in flux, so the SCons extensions API is also in flux. The API is subject to change without warning.
   Treat as experimental.
"""

import typing
import pathlib
import argparse

import SCons.Builder

from spade import _settings
from spade import _utilities


_exclude_from_namespace = set(globals().keys())


# TODO: Decide if SPADE should be downstream or upstream of WAVES. If downstream, it's ok to add WAVES as a run time
# dependence and use the WAVES function
def _first_target_emitter(
    target: list,
    source: list,
    env,
    suffixes: typing.Iterable[str] = [],
    appending_suffixes: typing.Iterable[str] = [],
    stdout_extension: str = _settings._stdout_extension,
) -> typing.Tuple[list, list]:
    """Appends the target list with the builder managed targets

    Searches for a file ending in the stdout extension. If none is found, creates a target by appending the stdout
    extension to the first target in the ``target`` list. The associated Builder requires at least one target for this
    reason. The stdout file is always placed at the end of the returned target list.

    The suffixes list are replacement operations on the first target's suffix. The appending suffixes list are appending
    operations on the first target's suffix.

    The emitter will assume all emitted targets build in the current build directory. If the target(s) must be built in
    a build subdirectory, e.g. in a parameterized target build, then the first target must be provided with the build
    subdirectory, e.g. ``parameter_set1/target.ext``. When in doubt, provide a STDOUT redirect file with the ``.stdout``
    extension as a target, e.g. ``target.stdout``.

    :param target: The target file list of strings
    :param source: The source file list of SCons.Node.FS.File objects
    :param SCons.Script.SConscript.SConsEnvironment env: The builder's SCons construction environment object
    :param suffixes: Suffixes which should replace the first target's extension
    :param appending_suffixes: Suffixes which should append the first target's extension

    :return: target, source
    """
    string_targets = [str(target_file) for target_file in target]
    first_target = pathlib.Path(string_targets[0])

    # Search for a user specified stdout file. Fall back to first target with appended stdout extension
    stdout_target = next(
        (target_file for target_file in string_targets if target_file.endswith(stdout_extension)),
        f"{first_target}{stdout_extension}",
    )

    replacing_targets = [str(first_target.with_suffix(suffix)) for suffix in suffixes]
    appending_targets = [f"{first_target}{suffix}" for suffix in appending_suffixes]
    string_targets = string_targets + replacing_targets + appending_targets

    # Get a list of unique targets, less the stdout target. Preserve the target list order.
    string_targets = [target_file for target_file in string_targets if target_file != stdout_target]
    string_targets = list(dict.fromkeys(string_targets))

    # Always append the stdout target for easier use in the action string
    string_targets.append(stdout_target)

    return string_targets, source


def cli_builder(
    program: str = "spade",
    subcommand: str = "",
    required: str = "",
    options: str = "",
    abaqus_commands: typing.List[str] = _settings._default_abaqus_commands,
) -> SCons.Builder.Builder:
    """Return a generic SPADE CLI builder.

    This builder provides a template action for the SPADE CLI. The default behavior will not do anything unless
    the ``subcommand`` argument is updated to one of the SPADE CLI :ref:`cli_subcommands`.

    At least one target must be specified. The first target determines the working directory for the builder's action.
    The action changes the working directory to the first target's parent directory prior to execution.

    The emitter will assume all emitted targets build in the current build directory. If the target(s) must be built in
    a build subdirectory, e.g. in a parameterized target build, then the first target must be provided with the build
    subdirectory, e.g. ``parameter_set1/my_target.ext``. When in doubt, provide a STDOUT redirect file as a target, e.g.
    ``target.stdout``.

    This builder and any builders created from this template will be most useful if the ``options`` argument places
    SCons substitution variables in the action string, e.g. ``--argument ${argument}``, such that the task definitions
    can modify the options on a per-task basis. Any option set in this manner *must* be provided by the task definition.

    *Builder/Task keyword arguments*

    * ``program``: The SPADE command line executable absolute or relative path
    * ``subcommand``: A SPADE subcommand
    * ``required``: A space delimited string of subcommand required arguments
    * ``options``: A space delimited string of subcommand optional arguments
    * ``abaqus_commands``: The Abaqus command line executable absolute or relative path. When provided as a task
      keyword argument, this must be a space delimited string, not a list.
    * ``cd_action_prefix``: Advanced behavior. Most users should accept the defaults.
    * ``redirect_action_postfix``: Advanced behavior. Most users should accept the defaults.

    .. code-block::
       :caption: action string construction

       ${cd_action_prefix} ${program} ${subcommand} ${required} ${options} --abaqus-commands ${abaqus_commands} ${redirect_action_postfix}

    .. code-block::
       :caption: SConstruct

       import spade
       env = Environment()
       env.Append(BUILDERS={
           "SpadeCLIBuilder": spade.scons_extensions.cli_builder(
               subcommand="extract",
               required="${SOURCE.abspath} --extracted-file ${TARGET.abspath} --force-overwrite"
           )
       })
       env.SpadeCLIBuilder(
           target=["target.cae"],
           source=["source.csv"],
       )

    :param program: The SPADE command line executable absolute or relative path
    :param subcommand: A SPADE subcommand
    :param required: A space delimited string of subcommand required arguments
    :param options: A space delimited string of subcommand optional arguments
    :param abaqus_commands: The Abaqus command line executable absolute or relative path options

    :returns: SCons SPADE CLI builder
    """  # noqa: E501
    action = [
        "${cd_action_prefix} ${program} ${subcommand} ${required} ${options} "
        "--abaqus-commands ${abaqus_commands} "
        "${redirect_action_postfix}"
    ]
    builder = SCons.Builder.Builder(
        action=action,
        emitter=_first_target_emitter,
        cd_action_prefix=_settings._cd_action_prefix,
        redirect_action_postfix=_settings._redirect_action_postfix,
        program=program,
        subcommand=subcommand,
        required=required,
        options=options,
        abaqus_commands=_utilities.character_delimited_list(abaqus_commands),
    )
    return builder


def extract(
    program: str = "spade",
    subcommand: str = "extract",
    required: str = "${SOURCE.abspath} --extracted-file ${TARGET.abspath} --force-overwrite",
    options: str = "",
    abaqus_commands: typing.List[str] = _settings._default_abaqus_commands,
) -> SCons.Builder.Builder:
    """Return a SPADE extract subcommand CLI builder.

    See the :ref:`extract_cli` CLI documentation for detailed subcommand usage and options. Builds subcommand specific
    options for the :meth:`spade.scons_extensions.cli_builder` function.

    At least one target must be specified. The first target determines the working directory for the builder's action.
    The action changes the working directory to the first target's parent directory prior to execution.

    The emitter will assume all emitted targets build in the current build directory. If the target(s) must be built in
    a build subdirectory, e.g. in a parameterized target build, then the first target must be provided with the build
    subdirectory, e.g. ``parameter_set1/my_target.ext``. When in doubt, provide a STDOUT redirect file as a target, e.g.
    ``target.stdout``.

    This builder and any builders created from this template will be most useful if the ``options`` argument places
    SCons substitution variables in the action string, e.g. ``--argument ${argument}``, such that the task definitions
    can modify the options on a per-task basis. Any option set in this manner *must* be provided by the task definition.

    *Builder/Task keyword arguments*

    * ``program``: The SPADE command line executable absolute or relative path
    * ``subcommand``: A SPADE subcommand
    * ``required``: A space delimited string of subcommand required arguments
    * ``options``: A space delimited string of subcommand optional arguments
    * ``abaqus_commands``: The Abaqus command line executable absolute or relative path. When provided as a task
      keyword argument, this must be a space delimited string, not a list.
    * ``cd_action_prefix``: Advanced behavior. Most users should accept the defaults.
    * ``redirect_action_postfix``: Advanced behavior. Most users should accept the defaults.

    .. code-block::
       :caption: action string construction

       ${cd_action_prefix} ${program} ${subcommand} ${required} ${options} --abaqus-commands ${abaqus_commands} ${redirect_action_postfix}

    .. code-block::
       :caption: SConstruct

       import spade
       env = Environment()
       env.Append(BUILDERS={
           "SpadeExtract": spade.scons_extensions.extract()
       })
       env.SpadeExtract(
           target=["target.h5"],
           source=["source.odb"],
       )

    :param program: The SPADE command line executable absolute or relative path
    :param subcommand: A SPADE subcommand
    :param required: A space delimited string of subcommand required arguments
    :param options: A space delimited string of subcommand optional arguments
    :param abaqus_commands: The Abaqus command line executable absolute or relative path options

    :returns: SCons SPADE extract CLI builder
    """  # noqa: E501
    return cli_builder(
        program=program,
        subcommand=subcommand,
        required=required,
        options=options,
        abaqus_commands=abaqus_commands,
    )


_module_objects = set(globals().keys()) - _exclude_from_namespace
__all__ = [name for name in _module_objects if not name.startswith("_")]
