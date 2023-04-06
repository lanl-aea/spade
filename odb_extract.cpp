//////////////////////////////////////////////////////////////////////////
// Extract data from an Abaqus odb file and write the data to an hdf5 file
//
//

#if (defined(HP) && (! defined(HKS_HPUXI)))
#include <iostream.h>
#else
#include <iostream>

#ifdef _WIN32
    #include <direct.h>
    #define getcwd _getcwd // MSFT "deprecation" warning
#else
    #include <unistd.h>
#endif

#endif


#include <string>
#include <map>
#include <set>
#include <fstream>
#include <vector>

// Begin local includes
#include "cmd_line_arguments.h"
#include "logging.h"
// End local includes

using namespace std;

int ABQmain(int argc, char **argv)
//int main(int argc, char **argv)
{
    CmdLineArguments command_line_arguments(argc, argv);
    if (command_line_arguments.help()) { cout<<command_line_arguments.helpMessage(); return 0; }  // If help option used, print help and exit
    Logging log_file(command_line_arguments["log-file"], command_line_arguments.verbose(), command_line_arguments.debug());
    log_file.log("\nCommand line used:\n"+ command_line_arguments.commandLine() + "\n");
    log_file.logVerbose("Arguments given:\n" + command_line_arguments.verboseArguments());
    log_file.logDebug("Debug logging turned on\n");

    return 0;
}
