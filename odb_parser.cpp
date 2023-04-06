#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <cstring>
#include <cstdlib>

using namespace std;

#include <odb_API.h>
#include <odb_Coupling.h>
#include <odb_MPC.h>
#include <odb_ShellSolidCoupling.h>

#include <odb_parser.h>

OdbParser::OdbParser (CmdLineArguments const &command_line_arguments, Logging &log_file) {
    odb_String file_name = command_line_arguments["odb-file"].c_str();
    log_file.logDebug("Operating on file:" + command_line_arguments["odb-file"] + "\n");
    odb_Odb& odb = openOdb(file_name);
//    odbE_printFullOdb(odb, printPath);    

    log_file.logDebug("Odb Parser object successfully created\n");
}