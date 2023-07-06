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

using namespace std;
#endif

#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <functional>
#include <ctime>
#include <cctype>
#include <locale>
#include <vector>
#include <iterator>
#include <regex>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include <odb_API.h>
#include <odb_Coupling.h>
#include <odb_MPC.h>
#include <odb_ShellSolidCoupling.h>
#include <odb_MaterialTypes.h>
#include <odb_SectionTypes.h>

#include "H5Cpp.h"
using namespace H5;

#include <odb_extract_object.h>

OdbExtractObject::OdbExtractObject (CmdLineArguments &command_line_arguments, Logging &log_file) {
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


    log_file.logVerbose("Starting to write output file: " + command_line_arguments.getTimeStamp(false) + "\n");

    if (command_line_arguments["output-file-type"] == "h5") this->write_h5(command_line_arguments, log_file);
    else if (command_line_arguments["output-file-type"] == "json") this->write_json(command_line_arguments, log_file);
    else if (command_line_arguments["output-file-type"] == "yaml") this->write_yaml(command_line_arguments, log_file);

    log_file.logVerbose("Finished writing output file: " + command_line_arguments.getTimeStamp(false) + "\n");

}

void OdbExtractObject::process_odb(odb_Odb &odb, Logging &log_file) {

    log_file.logVerbose("Reading top level attributes of odb.\n");
    this->name = odb.name().CStr();
    this->analysisTitle = odb.analysisTitle().CStr();
    this->description = odb.description().CStr();
    this->path = odb.path().CStr();
    this->isReadOnly = odb.isReadOnly();

    log_file.logVerbose("Reading odb jobData.\n");
    odb_JobData jobData = odb.jobData();
    static const char * analysis_code_enum_strings[] = { "Abaqus Explicit", "Abaqus Standard", "Unknown Analysis Code" };
    this->job_data.analysisCode = analysis_code_enum_strings[jobData.analysisCode()];
    this->job_data.creationTime = jobData.creationTime().CStr();
    this->job_data.machineName = jobData.machineName().CStr();
    this->job_data.modificationTime = jobData.modificationTime().CStr();
    this->job_data.name = jobData.name().CStr();
    static const char * precision_enum_strings[] = { "Single Precision", "Double Precision" };
    this->job_data.precision = precision_enum_strings[jobData.precision()];
    odb_SequenceProductAddOn add_ons = jobData.productAddOns();
    static const char * add_on_enum_strings[] = { "aqua", "design", "biorid", "cel", "soliter", "cavparallel" };
    // Values gotten from: https://help.3ds.com/2023/English/DSSIMULIA_Established/SIMACAEKERRefMap/simaker-c-jobdatacpp.htm?contextscope=all
    for (int i=0; i<add_ons.size(); i++) {
        this->job_data.productAddOns.push_back(add_on_enum_strings[add_ons.constGet(i)]);
    }
    this->job_data.version = jobData.version().CStr();

    if (odb.hasSectorDefinition())
    {
        log_file.logVerbose("Reading odb sector definition.\n");
	    odb_SectorDefinition sd = odb.sectorDefinition();
        this->sector_definition.numSectors = sd.numSectors();
	    odb_SequenceSequenceFloat symAx = sd.symmetryAxis();
        this->sector_definition.start_point[0] = symAx[0][0];
        this->sector_definition.start_point[1] = symAx[0][1];
        this->sector_definition.start_point[2] = symAx[0][2];
        this->sector_definition.end_point[0] = symAx[1][0];
        this->sector_definition.end_point[1] = symAx[1][1];
        this->sector_definition.end_point[2] = symAx[1][2];
    }

    odb_PartRepository& parts = odb.parts();
    odb_PartRepositoryIT iter(parts);    
    for (iter.first(); !iter.isDone(); iter.next()) {
        log_file.logVerbose("Starting to parse part: " + string(iter.currentKey().CStr()) + "\n");
        odb_Part& part = parts[iter.currentKey()];
    }


}





void OdbExtractObject::write_h5 (CmdLineArguments &command_line_arguments, Logging &log_file) {
// Write out data to hdf5 file

    // Open file for writing
    std::ifstream hdf5File (command_line_arguments["output-file"].c_str());
    log_file.logDebug("Creating hdf5 file " + command_line_arguments["output-file"] + "\n");
    const H5std_string FILE_NAME(command_line_arguments["output-file"]);
    H5File h5_file(FILE_NAME, H5F_ACC_TRUNC);

//    H5::Group odb_group = file.createGroup(string("/odb").c_str());
    log_file.logDebug("Creating odb group for meta-data " + command_line_arguments["output-file"] + "\n");

//    write_string_dataset(this->odb_group, "name", this->name);
    log_file.logVerbose("Writing top level attributes to odb group.\n");
    this->odb_group = h5_file.createGroup(string("/odb").c_str());
    write_attribute(this->odb_group, "name", this->name);
    write_attribute(this->odb_group, "analysisTitle", this->analysisTitle);
    write_attribute(this->odb_group, "description", this->description);
    write_attribute(this->odb_group, "path", this->path);
    stringstream bool_stream; bool_stream << std::boolalpha << this->isReadOnly; string bool_string = bool_stream.str();
    write_attribute(this->odb_group, "isReadOnly", bool_string);

    log_file.logVerbose("Writing odb jobData.\n");
    this->job_data_group = h5_file.createGroup(string("/odb/jobData").c_str());
    write_attribute(this->job_data_group, "analysisCode", this->job_data.analysisCode);
    write_attribute(this->job_data_group, "creationTime", this->job_data.creationTime);
    write_attribute(this->job_data_group, "machineName", this->job_data.machineName);
    write_attribute(this->job_data_group, "modificationTime", this->job_data.modificationTime);
    write_attribute(this->job_data_group, "name", this->job_data.name);
    write_attribute(this->job_data_group, "precision", this->job_data.precision);
    write_vector_string_dataset(this->job_data_group, "productAddOns", this->job_data.productAddOns);
    write_attribute(this->job_data_group, "version", this->job_data.version);

    this->sector_definition_group = h5_file.createGroup(string("/odb/sectorDefinition").c_str());
    if (this->sector_definition.numSectors > 0) {
        /*
        write_string_dataset(this->sector_definition_group, "numSectors", this->sector_definition.numSectors);
        H5::Group symmetry_axis_group = h5_file.createGroup(string("/odb/sectorDefinition/symmetryAxis").c_str());
        write_string_dataset(this->sector_definition_group, "numSectors", this->sector_definition.numSectors);
        */
    }

    this->contraints_group = h5_file.createGroup(string("/odb/constraints").c_str());
    this->interactions_group = h5_file.createGroup(string("/odb/interactions").c_str());
    this->parts_group = h5_file.createGroup(string("/odb/parts").c_str());
    this->root_assembly_group = h5_file.createGroup(string("/odb/rootAssembly").c_str());
    this->section_categories_group = h5_file.createGroup(string("/odb/sectionCategories").c_str());
    this->steps_group = h5_file.createGroup(string("/odb/steps").c_str());
    this->user_data_group = h5_file.createGroup(string("/odb/userData").c_str());

//    vector<string> temp_string = { "testing", "this", "vector" };
//    std::vector<const char*> array_of_c_string = { "testing", "this", "vector" };
//    for(const string &group : groups)

    // TODO: potentially add amplitudes group
    // TODO: potentially add filters group
    // TODO: potentially add materials group

    h5_file.close();  // Close the hdf5 file
}

void OdbExtractObject::write_attribute(const H5::Group& group, const string & attribute_name, const string & string_value) {
    H5::DataSpace attribute_space(H5S_SCALAR);
    int string_size = string_value.size();
    if (string_size == 0) { string_size++; }  // If the string is empty, make the string size equal to one, as StrType must have a positive size
    H5::StrType string_type (0, string_size);  // Use the length of the string or 1 if string is blank
    H5::Attribute attribute = group.createAttribute(attribute_name, string_type, attribute_space);
    attribute.write(string_type, string_value);
}

void OdbExtractObject::write_string_dataset(const H5::Group& group, const string & dataset_name, const string & string_value) {
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one string
    int string_size = string_value.size();
    if (string_size == 0) { string_size++; }  // If the string is empty, make the string size equal to one, as StrType must have a positive size
    H5::StrType string_type (0, string_size);
    H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
    dataset.write(&string_value[0], string_type);
}

void OdbExtractObject::write_vector_string_dataset(const H5::Group& group, const string & dataset_name, const vector<const char*> & string_values) {
//    hsize_t dimensions[] = {1};
    hsize_t dimensions[1] = {hsize_t(string_values.size())};
    H5::DataSpace dataspace(1, dimensions);  // Create a space for as many strings as are in the vector
//    H5::StrType string_type(0, H5T_VARIABLE);
    H5::StrType string_type(H5::PredType::C_S1, H5T_VARIABLE);
    H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
    dataset.write(string_values.data(), string_type);
}


void OdbExtractObject::write_yaml (CmdLineArguments &command_line_arguments, Logging &log_file) {
}

void OdbExtractObject::write_json (CmdLineArguments &command_line_arguments, Logging &log_file) {
}