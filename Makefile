#!/usr/bin/make -f

# Variables:
# GNU make recommends using lower case for variables that serve internal 
# purposes in the makefile and upper case for parameters that control implicit
# rules or for parameters that the user should override with command options
# use make --just-print to see what commands are to be run without running them

SHELL:=/bin/bash

include_local_objects = cmd_line_arguments.o
include_local_sources = cmd_line_arguments.cpp

include_h5_objects = h5_writer.o
include_h5_sources = h5_writer.cpp

include_odb_objects = odb_parser.o
include_odb_sources = odb_parser.cpp

#objects = $(include_local_objects) $(include_h5_objects) $(include_odb_objects)
objects = $(include_local_objects) $(include_h5_objects)
#link_exe_obj = $(objects) -static-libstdc++
link_exe_obj = $(objects)
abq_env = abaqus_v6.env
tool = odb_extract

ifneq ($(wildcard /usr/projects/ea/SIMULIA/EstProducts/*),) # returns all entries in directory or empty string
ABQ_BASE_PATH = /usr/projects/ea/SIMULIA/EstProducts
ABQ_CMD_PATH = /usr/projects/ea/DassaultSystemes/SIMULIA/Commands
GPP=$(shell module load gcc; which g++)
H5_PATH=$(shell module load gcc; module load hdf5-serial; dirname $$(dirname $$(which h5c++)))
else
ABQ_BASE_PATH = /apps/SIMULIA/EstProducts
ABQ_CMD_PATH = /apps/abaqus/Commands
H5_PATH=$(shell dirname $$(dirname $$(which h5c++)))
GPP=$(shell which g++)
endif

GPP_PATH=$(shell dirname $$(dirname $(GPP)))
LATEST_ABAQUS = $(shell ls $(ABQ_BASE_PATH) | tail -n 1)
ABQ_PATH = $(ABQ_BASE_PATH)/$(LATEST_ABAQUS)
H5_FlAGS = -fPIC -I$(H5_PATH)/include/  -I. -I$(H5_PATH)/lib
CC=$(GPP)

ODB_FLAGS = -c -fPIC -w -Wno-deprecated -DTYPENAME=typename -D_LINUX_SOURCE \
	      -DABQ_LINUX -DABQ_LNX86_64 -DSMA_GNUC -DFOR_TRAIL -DHAS_BOOL \
	      -DASSERT_ENABLED -D_BSD_TYPES -D_BSD_SOURCE -D_GNU_SOURCE \
	      -D_POSIX_SOURCE -D_XOPEN_SOURCE_EXTENDED -D_XOPEN_SOURCE \
	      -DHAVE_OPENGL -DHKS_OPEN_GL -DGL_GLEXT_PROTOTYPES \
	      -DMULTI_THREADING_ENABLED -D_REENTRANT -DABQ_MPI_SUPPORT -DBIT64 \
	      -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	      -I$(ABQ_PATH)/linux_a64/code/include -I$(ABQ_PATH)/ \
	      -I$(H5_PATH)/include/ -I. -I$(H5_PATH)/lib -static-libstdc++
gpp_odb = $(CC) $(ODB_FLAGS)
link_exe = $(GPP) -fPIC -Wl,-Bdynamic -Wl,--add-needed -o %J %F %M %L %B %O -lhdf5 -lhdf5_cpp -L$(GPP_PATH)/lib64 -L$(GPP_PATH)/lib -L$(H5_PATH)/lib -lstdc++

.DEFAULT_GOAL := $(tool)  # Not strictly necessary since the target is first
# Compiling the main code requires all the object files as well as the env file
#$(tool): $(include_local_objects) $(include_h5_objects) $(include_odb_objects) $(abq_env)
$(tool): $(include_local_objects) $(include_h5_objects) $(abq_env)
	$(ABQ_CMD_PATH)/abq$(LATEST_ABAQUS) make job=$(tool).cpp

$(include_local_objects): $(include_local_sources)
	$(GPP) -I. -c $*.cpp -o $@

$(include_h5_objects): $(include_h5_sources)
	$(GPP) $(H5_FlAGS) -c $*.cpp -o $@

$(include_odb_objects): $(include_odb_sources)
	$(GPP) $(ODB_FLAGS) -c $*.cpp -o $@

.PHONY : $(abq_env)  # Even though a file is the target, I want it updated every time regardless
# Two steps to create abaqus_v6.env, first move existing file, then create file
# If no existing file ignore mv error (indicated by dash in front) 
$(abq_env):
	-mv $(abq_env) $(abq_env).old
	echo "compile_cpp='$(gpp_odb)'" >> $(abq_env)
	echo "link_exe='$(link_exe) $(link_exe_obj)'" >> $(abq_env)

.PHONY : clean  # This will cause 'clean' to work even if a file is named clean
clean : # A dash is used to ignore any errors caused by rm
	-rm -f $(tool) $(objects) $(abq_env) $(tool).o
