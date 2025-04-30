/**
  ******************************************************************************
  * \file spade.cpp
  ******************************************************************************
  * Extract data from an Abaqus odb file and write the data to an hdf5 file
  ******************************************************************************
  */


#include <iostream>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <vector>

#include "cmd_line_arguments.h"
#include "logging.h"
#include "spade_object.h"

using namespace std;

//! Main function called when code is compiled
/*!
  This function mostly sets up the various objects required to extract the data from the odb. Most of the work is then
  done by those objects.

  \param argc integer containing count of command line arguments
  \param argv array containing command line arguments
*/
int ABQmain(int argc, char **argv)
{
    try {
        CmdLineArguments command_line_arguments(argc, argv);
        // If help option used, print help and exit
        if (command_line_arguments.help()) { cout<<command_line_arguments.helpMessage(); return 0; }
        Logging log_file(
            command_line_arguments["log-file"],
            command_line_arguments.verbose(),
            command_line_arguments.debug()
        );
        log_file.log("Command line used: "+ command_line_arguments.commandLine());
        log_file.logVerbose("Arguments given:" + command_line_arguments.verboseArguments());
        log_file.logDebug("Debug logging turned on");
        SpadeObject spade_object(command_line_arguments, log_file);
        log_file.logVerbose("Successful completion of " + command_line_arguments.commandName());
    } catch (const std::runtime_error &err) {
        cerr << err.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
