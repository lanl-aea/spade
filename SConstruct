#! /usr/bin/env python

import os
import pathlib

import waves

import utilities

user_env = os.environ.copy()
env = Environment(
    ENV=user_env,
    CC=user_env["CC"],
    CXX=user_env["CXX"]
)

gpp_path = pathlib.Path(env["CXX"]).resolve().parent
gpp_lib = gpp_path / "lib"
gpp_include = gpp_path / "include"

abaqus_paths = [
    "/apps/abaqus/Commands/abq2023",
     "/usr/projects/ea/DassaultSystemes/SIMULIA/Commands/abq2023",
     "abq2023"
]
env["abaqus"] = env.Detect(abaqus_paths)
abaqus_installation, abaqus_code_bin, abaqus_code_include = utilities.return_abaqus_code_paths(env["abaqus"])

odb_flags = "-c -fPIC -w -Wno-deprecated -DTYPENAME=typename -D_LINUX_SOURCE " \
	    "-DABQ_LINUX -DABQ_LNX86_64 -DSMA_GNUC -DFOR_TRAIL -DHAS_BOOL " \
	    "-DASSERT_ENABLED -D_BSD_TYPES -D_BSD_SOURCE -D_GNU_SOURCE " \
	    "-D_POSIX_SOURCE -D_XOPEN_SOURCE_EXTENDED -D_XOPEN_SOURCE " \
	    "-DHAVE_OPENGL -DHKS_OPEN_GL -DGL_GLEXT_PROTOTYPES " \
	    "-DMULTI_THREADING_ENABLED -D_REENTRANT -DABQ_MPI_SUPPORT -DBIT64 " \
	    "-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 " \
	    f"-I{abaqus_code_include} -I{abaqus_installation} " \
	    f"-I{gpp_include} -I. -I{gpp_lib} -static-libstdc++"
abaqus_link_flags = f"-fPIC -Wl,-Bdynamic -Wl,--add-needed -o %J %F %M %L %B %O " \
                    "-lhdf5 -lhdf5_cpp -L{gpp_lib} -lstdc++ -Wl,-rpath,{abaqus_code_bin},-rpath,{gpp_lib}"
h5_flags = "-fPIC -I{gpp_include} -I. -I{gpp_lib}"
local_flags = "-I."

# Build static objects
env.Object("cmd_line_arguments.cpp", CXXFLAGS=local_flags)
env.Object("logging.cpp", CXXFLAGS=local_flags)
env.Object("output_writer.cpp", CXXFLAGS=h5_flags + odb_flags)
env.Object("odb_parser.cpp", CXXFLAGS=odb_flags)

# Write build abaqus environment file
compile_cpp = f"{env['CXX']} {odb_flags}"
# TODO: decide how to handle object files. Prefer to grab them from Task target list(s).
link_exe = f"{env['CXX']} {abaqus_link_flags}"
env.Substfile(
    "abaqus_v6.env.in",
    SUBST_DICT={"@compile_cpp@": compile_cpp, "@link_exe@": link_exe}
)

# Set project meta variables
project_name = "odb_extract"
project_dir = pathlib.Path(Dir(".").abspath)
try:
    import setuptools_scm
    version = setuptools_scm.get_version()
except (ModuleNotFoundError, LookupError) as err:
    import warnings
    warnings.warn(f"Setting version to 'None'. {err}", RuntimeWarning)
    version = "None"


# Add aliases to help message so users know what build target options are available
# This must come *after* all expected Alias definitions and SConscript files.
waves.builders.project_help_message()
Help(utilities.compiler_help(env), append=True)
