import pathlib

# semi-private project meta variables for package internal use to avoid hard-coded duplication
_project_root_abspath = pathlib.Path(__file__).parent.resolve()
_project_name = "Serialized Proprietary Abaqus Data Extractor"
_project_description = (
    "Tool for extracting data from an Abaqus output database (odb) file into a hierarchical data format (hdf5) file"
)
_project_name_short = "spade"
_project_name_cpp = "spade"
_project_source = "src"
_default_abaqus_commands = [pathlib.Path("abaqus"), pathlib.Path("abq2025")]
_compiler_flags_gcc = (
    "-c -fPIC -w -Wno-deprecated -DTYPENAME=typename -D_LINUX_SOURCE "
    "-DABQ_LINUX -DABQ_LNX86_64 -DSMA_GNUC -DFOR_TRAIL -DHAS_BOOL "
    "-DASSERT_ENABLED -D_BSD_TYPES -D_BSD_SOURCE -D_GNU_SOURCE "
    "-D_POSIX_SOURCE -D_XOPEN_SOURCE_EXTENDED -D_XOPEN_SOURCE "
    "-DHAVE_OPENGL -DHKS_OPEN_GL -DGL_GLEXT_PROTOTYPES "
    "-DMULTI_THREADING_ENABLED -D_REENTRANT -DABQ_MPI_SUPPORT -DBIT64 "
    "-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -static-libstdc++ "
)
_link_flags_gcc = "-fPIC -Wl,-Bdynamic -Wl,--add-needed -o %J %F %M %L %B %O -lhdf5 -lhdf5_cpp -lstdc++ -lhdf5_hl "
_compiler_flags_msvc = (
    "/c /W0 /MD /TP /EHsc /DNDEBUG /DWIN32 /DTP_IP /D_CONSOLE /DNTI /DFLT_LIC /DOL_DOC /D__LIB__ /DHKS_NT "
    "/D_WINDOWS_SOURCE /DFAR= /D_WINDOWS /DABQ_WIN86_64 "
)
_link_flags_msvc = (
    "/nologo /INCREMENTAL:NO /subsystem:console /machine:AMD64 /STACK:20000000 /NODEFAULTLIB:LIBC.LIB "
    "/NODEFAULTLIB:LIBCMT.LIB /DEFAULTLIB:OLDNAMES.LIB /DEFAULTLIB:LIBIFCOREMD.LIB /DEFAULTLIB:LIBIFPORTMD.LIB "
    "/DEFAULTLIB:LIBMMD.LIB /DEFAULTLIB:kernel32.lib /DEFAULTLIB:user32.lib /DEFAULTLIB:advapi32.lib /FIXED:NO "
    "/LARGEADDRESSAWARE /Fe:%J %F %M %L %B %O oldnames.lib user32.lib ws2_32.lib netapi32.lib advapi32.lib "
    "msvcrt.lib vcruntime.lib ucrt.lib hdf5.lib hdf5_cpp.lib hdf5_hl.lib "
)

# Docs subcommand
_installed_docs_index = _project_root_abspath / "docs/index.html"

# SCons extensions
_cd_action_prefix = "cd ${TARGET.dir.abspath} &&"
_redirect_action_postfix = "> ${TARGETS[-1].abspath} 2>&1"
_stdout_extension = ".stdout"
