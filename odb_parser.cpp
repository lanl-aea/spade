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
#include <odb_MaterialTypes.h>
#include <odb_SectionTypes.h>

#include <odb_parser.h>


OdbParser::OdbParser (CmdLineArguments &command_line_arguments, Logging &log_file) {
    log_file.logVerbose("Starting to parse odb file: " + command_line_arguments.getTimeStamp(false) + "\n");
    odb_String file_name = command_line_arguments["odb-file"].c_str();
    log_file.logDebug("Operating on file:" + command_line_arguments["odb-file"] + "\n");

    if (isUpgradeRequiredForOdb(file_name)) {
        log_file.logDebug("Upgrade to odb required.\n");
        odb_String upgraded_file_name = string("upgraded_" + command_line_arguments["odb-file"]).c_str();
        log_file.log("Upgrading file:" + command_line_arguments["odb-file"] + "\n");
        upgradeOdb(file_name, upgraded_file_name);
        file_name = upgraded_file_name;
    }

    odb_Odb& odb = openOdb(file_name, true);  // Open as read only
//    odbE_printFullOdb(odb, printPath);    

    this->name = odb.name().CStr();
    this->analysisTitle = odb.analysisTitle().CStr();
    this->description = odb.description().CStr();
    this->path = odb.path().CStr();
    this->isReadOnly = odb.isReadOnly();

    odb.close();
    log_file.logDebug("Odb Parser object successfully created\n");
}

// Getters
string OdbParser::operator[](string const &key) const {
    if (key == "name") { return this->name; }
    else if (key == "analysisTitle") { return this->analysisTitle; }
    else if (key == "description") { return this->description; }
    else if (key == "path") { return this->path; }
    else if (key == "isReadOnly") { return this->isReadOnly; }
    else { return ""; }
}

map<string, string> OdbParser::jobData() { return this->job_data; }

        /*
        odb_Assembly& rootAssembly;
        odb_JobData jobData;
        odb_PartRepository& parts;
        odb_StepRepository& steps;
        odb_SectionCategoryRepository& sectionCategories;
        odb_SectorDefinition& sectorDefinition;
        odb_InteractionRepository& interactions;
        odb_InteractionPropertyRepository& interactionProperties;
        odb_ConstraintRepository& constraints;
        user_data userData;
        */