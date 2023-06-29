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

abaqus_paths = [
    "/apps/abaqus/Commands/abq2023",
     "/usr/projects/ea/DassaultSystemes/SIMULIA/Commands/abq2023",
     "abq2023"
]
env["abaqus"] = env.Detect(abaqus_paths)
abaqus_paths = utilities.find_abaqus_paths(env["abaqus"])

odb_flags = "-c -fPIC -w -Wno-deprecated -DTYPENAME=typename -D_LINUX_SOURCE ", \
	    "-DABQ_LINUX -DABQ_LNX86_64 -DSMA_GNUC -DFOR_TRAIL -DHAS_BOOL ", \
	    "-DASSERT_ENABLED -D_BSD_TYPES -D_BSD_SOURCE -D_GNU_SOURCE ", \
	    "-D_POSIX_SOURCE -D_XOPEN_SOURCE_EXTENDED -D_XOPEN_SOURCE ", \
	    "-DHAVE_OPENGL -DHKS_OPEN_GL -DGL_GLEXT_PROTOTYPES ", \
	    "-DMULTI_THREADING_ENABLED -D_REENTRANT -DABQ_MPI_SUPPORT -DBIT64 ", \
	    "-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 ", \
	    "-I$(ABQ_PATH)/linux_a64/code/include -I$(ABQ_PATH)/ ", \
	    "-I$(H5_PATH)/include/ -I. -I$(H5_PATH)/lib -static-libstdc++"
abaqus_link_flags = "-fPIC -Wl,-Bdynamic -Wl,--add-needed -o %J %F %M %L %B %O -lhdf5 -lhdf5_cpp -L$(GPP_PATH)/lib -lstdc++ -Wl,-rpath,$(ABQ_PATH)/linux_a64/code/bin/,-rpath,$(GPP_PATH)/lib"

env.MergeFlags(odb_flags)
env.MergeFlags(abaqus_link_flags)

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
