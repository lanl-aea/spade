#! /usr/bin/env python

import os
import pathlib
import warnings

import setuptools_scm


warnings.filterwarnings(action="ignore", message="tag", category=UserWarning, module="setuptools_scm")

# Set project meta variables
project_dir = pathlib.Path(Dir(".").abspath)
package_source_dir = "spade"
project_variables = {
    "project_dir": project_dir,
    "package_dir": project_dir / package_source_dir,
    "version": setuptools_scm.get_version(),
}
project_variables_substitution = dict()
for key, value in project_variables.items():
    project_variables_substitution[f"@{key}@"] = value

AddOption(
    "--build-dir",
    dest="variant_dir_base",
    default="build",
    nargs=1,
    type="string",
    action="store",
    metavar="DIR",
    help="SCons build (variant) root directory. Relative or absolute path. (default: '%default')"
)
default_abaqus_command = "/apps/abaqus/Commands/abq2024"
AddOption(
    "--abaqus-command",
    dest="abaqus_command",
    nargs=1,
    type="string",
    action="append",
    metavar="COMMAND",
    help=f"Override for the Abaqus command. Repeat to specify more than one (default: '[{default_abaqus_command}]')",
)

env = Environment(
    ENV=os.environ.copy(),
    variant_dir_base=GetOption("variant_dir_base"),
    abaqus_command=GetOption("abaqus_command"),
)
for key, value in project_variables.items():
    env[key] = value
env["abaqus_command"] = env["abaqus_command"] if env["abaqus_command"] is not None else [default_abaqus_command]
env["ENV"]["PYTHONDONTWRITEBYTECODE"] = 1

# Find third-party software
abaqus_environments = dict()
for command in env["abaqus_command"]:
    # TODO: more robust version/name recovery without CI server assumptions
    version = pathlib.Path(command).name
    abaqus_environments[version] = env.Clone()
    conf = abaqus_environments[version].Configure()
    found_command = conf.CheckProg(command)
    conf.Finish()
    abaqus_environments[version]["abaqus"] = found_command
    if found_command is not None:
        abaqus_environments[version].AppendENVPath("PATH", pathlib.Path(found_command).parent, delete_existing=False)

variant_dir_base = pathlib.Path(env["variant_dir_base"])
build_dir = variant_dir_base / "docs"
SConscript(dirs="docs", variant_dir=pathlib.Path(build_dir), exports=["env", "project_variables_substitution"])

# Add pytests, style checks, and static type checking
workflow_configurations = ["pytest", "flake8"]
for workflow in workflow_configurations:
    build_dir = variant_dir_base / workflow
    SConscript(
        build_dir.name,
        variant_dir=build_dir,
        exports={"env": env, "abaqus_environments": abaqus_environments},
        duplicate=False,
    )

# Add aliases to help message so users know what build target options are available
# This must come *after* all expected Alias definitions and SConscript files.
from SCons.Node.Alias import default_ans
alias_help = "\nTarget Aliases:\n"
for alias in default_ans:
    alias_help += f"    {alias}\n"
try:
    Help(alias_help, append=True, keep_local=True)
except TypeError as err:
    Help(alias_help, append=True)
