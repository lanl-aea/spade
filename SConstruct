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

env = Environment(
    ENV=os.environ.copy(),
    variant_dir_base=GetOption("variant_dir_base")
)
for key, value in project_variables.items():
    env[key] = value

# Hardcoded from WAVES find_program/add_program
conf = env.Configure()
abaqus_paths = ["/apps/abaqus/Commands/abq2023", "abq2023"]
program_paths = [conf.CheckProg(name) for name in abaqus_paths]
conf.Finish()
first_found_path = next((path for path in program_paths if path is not None), None)
if first_found_path:
    env.AppendENVPath("PATH", first_found_path, delete_existing=False)

variant_dir_base = pathlib.Path(env["variant_dir_base"])
build_dir = variant_dir_base / "docs"
SConscript(dirs="docs", variant_dir=pathlib.Path(build_dir), exports=["env", "project_variables_substitution"])

# Add pytests, style checks, and static type checking
workflow_configurations = ["pytest", "flake8"]
for workflow in workflow_configurations:
    build_dir = variant_dir_base / workflow
    SConscript(build_dir.name, variant_dir=build_dir, exports='env', duplicate=False)

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
