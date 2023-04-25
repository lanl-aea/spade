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

OdbParser::OdbParser (CmdLineArguments &command_line_arguments, Logging &log_file) {
    log_file.logVerbose("Starting to parse odb file: " + command_line_arguments.getTimeStamp(false) + "\n");
    odb_String file_name = command_line_arguments["odb-file"].c_str();
    log_file.logDebug("Operating on file:" + command_line_arguments["odb-file"] + "\n");
    odb_Odb& odb = openOdb(file_name);
//    odbE_printFullOdb(odb, printPath);    
    this->odb_info["name"] = odb.name().CStr();
    this->odb_info["analysisTitle"] = odb.analysisTitle().CStr();
    this->odb_info["description"] = odb.description().CStr();
    this->odb_info["path"] = odb.path().CStr();

    log_file.logDebug("Odb Parser object successfully created\n");
}

map<string, string> OdbParser::odbInfo() { return this->odb_info; }