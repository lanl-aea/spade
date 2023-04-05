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
//#include "logging.h"
// End local includes

using namespace std;

int ABQmain(int argc, char **argv)
//int main(int argc, char **argv)
{
    CmdLineArguments command_line_args(argc, argv);
//    Logging logFile(cmdLineArgs["logfilename"], cmdLineArgs.logDebug());
//    logFile.log("Command Line used: "+cmdLineArgs.cmdLine() + "\n");
    if (command_line_args.help()) { cout<<command_line_args.helpMessage(); return 0; }
    if (command_line_args.verbose()) {
        cout << "\nCommand line used:\n\t"+ command_line_args.commandLine() + "\n";
        cout << "Arguments given:\n" + command_line_args.verboseArguments();
    }

    return 0;
}
