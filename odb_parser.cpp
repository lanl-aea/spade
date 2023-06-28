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

    try {  // Since the odb object isn't recognized outside the scope of the try/except, block the processing has to be done within the try block
        odb_Odb& odb = openOdb(file_name, true);  // Open as read only
        process_odb(odb, log_file);
        odb.close();
        log_file.logDebug("Odb Parser object successfully created\n");
    }
    catch(odb_BaseException& exc) {
        string error_message = exc.UserReport().CStr();
        log_file.logErrorAndExit("odbBaseException caught. Abaqus error message: " + error_message + "\n");
    }
    catch(...) {
        log_file.logErrorAndExit("Unkown exception when attempting to open odb file.\n");
    }

}

void OdbParser::process_odb(odb_Odb const &odb, Logging &log_file) {
    this->name = odb.name().CStr();
    this->analysisTitle = odb.analysisTitle().CStr();
    this->description = odb.description().CStr();
    this->path = odb.path().CStr();
    this->isReadOnly = odb.isReadOnly();

    odb_JobData jobData = odb.jobData();
    this->job_data.analysisCode = jobData.analysisCode();
    this->job_data.creationTime = jobData.creationTime().CStr();
    this->job_data.machineName = jobData.machineName().CStr();
    this->job_data.modificationTime = jobData.modificationTime().CStr();
    this->job_data.name = jobData.name().CStr();
    this->job_data.precision = jobData.precision();
    odb_SequenceProductAddOn add_ons = jobData.productAddOns();
    for (int i=0; i<add_ons.size(); i++) {
//        string add_on << add_ons.get(i);
//        std::istringstream add_on(add_ons.get(i));
        //TODO: figure out how to convert odb_Enum to string
        string add_on;
        this->job_data.productAddOns.push_back(add_on);
    }
    this->job_data.version = jobData.version().CStr();
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

//map<string, string> OdbParser::jobData() { return this->job_data; }
job_data_type OdbParser::jobData() const { return this->job_data; }

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