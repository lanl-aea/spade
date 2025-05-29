#! /usr/bin/env python

import os
import inspect
import pathlib
import platform
import warnings

import setuptools_scm


warnings.filterwarnings(action="ignore", message="tag", category=UserWarning, module="setuptools_scm")

# Set project meta variables
project_name = "spade"
version = setuptools_scm.get_version()
project_configuration = pathlib.Path(inspect.getfile(lambda: None))
project_directory = project_configuration.parent
distribution_name = project_name.replace("-", "_")
package_specification = f"{distribution_name}-{version}"
package_directory = project_directory / distribution_name
project_variables = {
    "name": project_name,
    "version": version,
    "project_directory": project_directory,
    "package_directory": package_directory,
}
project_variables_substitution = dict()
for key, value in project_variables.items():
    project_variables_substitution[f"@{key}@"] = value

AddOption(
    "--build-dir",
    dest="build_directory",
    default="build",
    nargs=1,
    type="string",
    action="store",
    metavar="DIR",
    help="SCons build (variant) root directory. Relative or absolute path. (default: '%default')",
)
AddOption(
    "--prefix",
    dest="prefix",
    default="install",
    nargs=1,
    type="string",
    action="store",
    metavar="DIR",
    help="SCons installation pip prefix ``--prefix``. Relative or absolute path. (default: '%default')",
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
    prefix=pathlib.Path(GetOption("prefix")),
    abaqus_command=GetOption("abaqus_command"),
)
for key, value in project_variables.items():
    env[key] = value
env["abaqus_command"] = env["abaqus_command"] if env["abaqus_command"] is not None else [default_abaqus_command]
env["ENV"]["PYTHONDONTWRITEBYTECODE"] = 1

build_directory = pathlib.Path(GetOption("build_directory"))

# Handle OS-aware tee output
system = platform.system().lower()
if system == "windows":  # Assume PowerShell
    env["tee_suffix"] = "$(| Tee-Object -FilePath ${TARGETS[-1].abspath}$)"
else:  # *Nix style tee
    env["tee_suffix"] = "$(2>&1 | tee ${TARGETS[-1].abspath}$)"

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

# Build
installed_documentation = package_directory / "docs"
packages = env.Command(
    target=[
        build_directory / f"dist/{package_specification}.tar.gz",
        build_directory / f"dist/{package_specification}-py3-none-any.whl",
    ],
    source=["pyproject.toml"],
    action=[
        Copy(package_directory / "README.rst", "README.rst"),
        Copy(package_directory / "pyproject.toml", "pyproject.toml"),
        Delete(Dir(installed_documentation)),
        Copy(Dir(installed_documentation), Dir(build_directory / "docs/html")),
        Delete(Dir(installed_documentation / ".doctrees")),
        Delete(installed_documentation / ".buildinfo"),
        Delete(installed_documentation / ".buildinfo.bak"),
        Copy(Dir(installed_documentation), build_directory / f"docs/man/{project_name}.1"),
        "python -m build --verbose --outdir=${TARGET.dir.abspath} --no-isolation .",
        Delete(Dir(package_specification)),
        Delete(Dir(f"{distribution_name}.egg-info")),
        Delete(Dir(installed_documentation)),
        Delete(package_directory / "README.rst"),
        Delete(package_directory / "pyproject.toml"),
    ],
)
env.Depends(packages, [Alias("html"), Alias("man")])
env.AlwaysBuild(packages)
env.Alias("build", packages)
env.Clean("build", Dir(build_directory / "dist"))

# Install
install = []
install.extend(
    env.Command(
        target=[build_directory / "install.log"],
        source=[packages[0]],
        action=[
            (
                "python -m pip install ${SOURCE.abspath} --prefix ${prefix} --log ${TARGET.abspath} "
                "--verbose --no-input --no-cache-dir --disable-pip-version-check --no-deps --ignore-installed "
                "--no-build-isolation --no-warn-script-location --no-index"
            ),
        ],
        prefix=env["prefix"],
    )
)
install.extend(
    env.Install(
        target=[env["prefix"] / "man/man1", env["prefix"] / "share/man/man1"],
        source=[build_directory / f"docs/man/{project_name}.1"],
    )
)
env.AlwaysBuild(install)
env.Alias("install", install)

# Documentation
SConscript(
    dirs="docs",
    variant_dir=build_directory / "docs",
    exports=["env", "project_variables_substitution"],
)

# Add pytests, style checks, and static type checking
workflow_configurations = ["pytest", "style"]
for workflow in workflow_configurations:
    SConscript(
        workflow,
        variant_dir=build_directory / workflow,
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
