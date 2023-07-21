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
        std::ostringstream ss;
	    ss << "[" << symAx[0][0]<<","<<symAx[0][1]<<","<<symAx[0][2]<<"]";
        this->sector_definition.start_point  = ss.str();
        ss.str(""); ss.clear();  // First line clears the ostringstream, second line clears any error flags
	    ss <<"[" << symAx[1][0]<<","<<symAx[1][1]<<","<<symAx[1][2]<<"]";
        this->sector_definition.end_point  = ss.str();
    } else {
        this->sector_definition.numSectors = 0;
        this->sector_definition.start_point  = "";
        this->sector_definition.end_point  = "";
    }

    log_file.logVerbose("Reading odb section category data.\n");
    odb_SectionCategoryRepositoryIT section_category_iter(odb.sectionCategories());
    for (section_category_iter.first(); !section_category_iter.isDone(); section_category_iter.next()) {
        odb_SectionCategory section_category = section_category_iter.currentValue();
        section_category_type category;
        category.name = section_category.name().CStr();
        category.description = section_category.description().CStr();
        int section_point_size = section_category.sectionPoints().size();
        for (int i=0; i<section_point_size; i++) {
            odb_SectionPoint section_point = section_category.sectionPoints(i);
            section_point_type point;
            point.number = to_string(section_point.number());
            point.description = section_point.description().CStr();
            category.sectionPoints.push_back(point);
        }
        this->section_categories.push_back(category);
    }

    log_file.logVerbose("Reading odb user data.\n");
    odb_UserXYDataRepositoryIT user_xy_data_iter(odb.userData().xyDataObjects());
    for (user_xy_data_iter.first(); !user_xy_data_iter.isDone(); user_xy_data_iter.next()) {
        odb_UserXYData userXYData = user_xy_data_iter.currentValue();
        user_xy_data_type user_xy_data;
        user_xy_data.name = userXYData.name().CStr();
        user_xy_data.sourceDescription = userXYData.sourceDescription().CStr();
        user_xy_data.contentDescription = userXYData.contentDescription().CStr();
        user_xy_data.positionDescription = userXYData.positionDescription().CStr();
        user_xy_data.xAxisLabel = userXYData.xAxisLabel().CStr();
        user_xy_data.yAxisLabel = userXYData.yAxisLabel().CStr();
        user_xy_data.legendLabel = userXYData.legendLabel().CStr();
        user_xy_data.description = userXYData.description().CStr();
        odb_SequenceSequenceFloat user_xy_data_data;
        userXYData.getData(user_xy_data_data);
        int column_number = 0;
        for (int i=0; i<user_xy_data_data.size(); i++) {
            odb_SequenceFloat user_xy_data_data_dimension1 = user_xy_data_data[i];
            if (user_xy_data_data_dimension1.size() > column_number) { column_number = user_xy_data_data_dimension1.size(); } // use the maximum for the number of columns
            vector<float> dimension1;
            for (int j=0; j<user_xy_data_data_dimension1.size(); j++) {
                dimension1.push_back(user_xy_data_data_dimension1[j]);
            }
            user_xy_data.data.push_back(dimension1);
        }
        user_xy_data.max_column_size = column_number;
        this->user_xy_data.push_back(user_xy_data);
    }

    odb_PartRepository& parts = odb.parts();
    odb_PartRepositoryIT parts_iter(parts);    
    for (parts_iter.first(); !parts_iter.isDone(); parts_iter.next()) {
        log_file.logVerbose("Starting to parse part: " + string(parts_iter.currentKey().CStr()) + "\n");
        odb_Part& part = parts[parts_iter.currentKey()];
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
    H5::Group odb_group = h5_file.createGroup(string("/odb").c_str());
    write_attribute(odb_group, "name", this->name);
    write_attribute(odb_group, "analysisTitle", this->analysisTitle);
    write_attribute(odb_group, "description", this->description);
    write_attribute(odb_group, "path", this->path);
    stringstream bool_stream; bool_stream << std::boolalpha << this->isReadOnly; string bool_string = bool_stream.str();
    write_attribute(odb_group, "isReadOnly", bool_string);

    log_file.logVerbose("Writing odb jobData.\n");
    H5::Group job_data_group = h5_file.createGroup(string("/odb/jobData").c_str());
    write_attribute(job_data_group, "analysisCode", this->job_data.analysisCode);
    write_attribute(job_data_group, "creationTime", this->job_data.creationTime);
    write_attribute(job_data_group, "machineName", this->job_data.machineName);
    write_attribute(job_data_group, "modificationTime", this->job_data.modificationTime);
    write_attribute(job_data_group, "name", this->job_data.name);
    write_attribute(job_data_group, "precision", this->job_data.precision);
    write_vector_string_dataset(job_data_group, "productAddOns", this->job_data.productAddOns);
    write_attribute(job_data_group, "version", this->job_data.version);

    log_file.logVerbose("Writing odb sector definition.\n");
    H5::Group sector_definition_group = h5_file.createGroup(string("/odb/sectorDefinition").c_str());
    write_integer_dataset(sector_definition_group, "numSectors", this->sector_definition.numSectors);
    H5::Group symmetry_axis_group = h5_file.createGroup(string("/odb/sectorDefinition/symmetryAxis").c_str());
    write_string_dataset(symmetry_axis_group, "StartPoint", this->sector_definition.start_point);
    write_string_dataset(symmetry_axis_group, "EndPoint", this->sector_definition.end_point);

    log_file.logVerbose("Writing odb section categories.\n");
    H5::Group section_categories_group = h5_file.createGroup(string("/odb/sectionCategories").c_str());
    for (int i=0; i<this->section_categories.size(); i++) {
        string category_group_name = "/odb/sectionCategories/" + this->section_categories[i].name;
        H5::Group section_category_group = h5_file.createGroup(category_group_name.c_str());
        write_string_dataset(section_category_group, "description", this->section_categories[i].description);
        for (int j=0; j<this->section_categories[i].sectionPoints.size(); j++) {
            string point_group_name = "/odb/sectionCategories/" + this->section_categories[i].name + "/" + this->section_categories[i].sectionPoints[j].number;
            H5::Group section_point_group = h5_file.createGroup(point_group_name.c_str());
            write_string_dataset(section_category_group, "description", this->section_categories[i].sectionPoints[j].description);
        }
    }

    log_file.logVerbose("Writing odb user data.\n");
    H5::Group user_data_group = h5_file.createGroup(string("/odb/userData").c_str());
    for (int i=0; i<this->user_xy_data.size(); i++) {
        string user_xy_data_name = "/odb/userData/" + this->user_xy_data[i].name;
        log_file.logDebug("User data name:" + this->user_xy_data[i].name);
        H5::Group user_xy_data_group = h5_file.createGroup(user_xy_data_name.c_str());
        write_string_dataset(user_xy_data_group, "sourceDescription", this->user_xy_data[i].sourceDescription);
        write_string_dataset(user_xy_data_group, "contentDescription", this->user_xy_data[i].contentDescription);
        write_string_dataset(user_xy_data_group, "positionDescription", this->user_xy_data[i].positionDescription);
        write_string_dataset(user_xy_data_group, "xAxisLabel", this->user_xy_data[i].xAxisLabel);
        write_string_dataset(user_xy_data_group, "yAxisLabel", this->user_xy_data[i].yAxisLabel);
        write_string_dataset(user_xy_data_group, "legendLabel", this->user_xy_data[i].legendLabel);
        write_string_dataset(user_xy_data_group, "description", this->user_xy_data[i].description);
        write_2D_float(user_xy_data_group, "data", this->user_xy_data[i].max_column_size, this->user_xy_data[i].data);
    }

    this->contraints_group = h5_file.createGroup(string("/odb/constraints").c_str());
    this->interactions_group = h5_file.createGroup(string("/odb/interactions").c_str());
    this->parts_group = h5_file.createGroup(string("/odb/parts").c_str());
    this->root_assembly_group = h5_file.createGroup(string("/odb/rootAssembly").c_str());
    this->steps_group = h5_file.createGroup(string("/odb/steps").c_str());

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
    attribute_space.close();
}

void OdbExtractObject::write_string_dataset(const H5::Group& group, const string & dataset_name, const string & string_value) {
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one string
    int string_size = string_value.size();
    if (string_size == 0) { string_size++; }  // If the string is empty, make the string size equal to one, as StrType must have a positive size
    H5::StrType string_type (0, string_size);
    H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
    dataset.write(&string_value[0], string_type);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_vector_string_dataset(const H5::Group& group, const string & dataset_name, const vector<const char*> & string_values) {
//    hsize_t dimensions[] = {1};
    hsize_t dimensions[1] = {hsize_t(string_values.size())};
    H5::DataSpace dataspace(1, dimensions);  // Create a space for as many strings as are in the vector
//    H5::StrType string_type(0, H5T_VARIABLE);
    H5::StrType string_type(H5::PredType::C_S1, H5T_VARIABLE);
    H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
    dataset.write(string_values.data(), string_type);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_integer_dataset(const H5::Group& group, const string & dataset_name, const int & int_value) {
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one integer
//    int integer_size = integer_value.size();
//    if (integer_size == 0) { integer_size++; }  // If the integer is empty, make the integer size equal to one, as StrType must have a positive size
//    H5::IntType integer_type (0, integer_size);
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::STD_I32BE, dataspace);
    dataset.write(&int_value, H5::PredType::NATIVE_INT);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_2D_float(const H5::Group& group, const string & dataset_name, int & max_column_size, vector<vector<float>> & data_array) {
    float float_array[data_array.size()][max_column_size]; // Need to convert vector to array with contiguous memory for H5 to process
    for( int i = 0; i<data_array.size(); ++i) {
        for( int j = 0; j<data_array[i].size(); ++j) {
            float_array[i][j] = data_array[i][j];
        }
    }
    hsize_t dimensions[] = {data_array.size(), max_column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
    dataset.write(float_array, H5::PredType::NATIVE_FLOAT);
    dataset.close();
    dataspace.close();
}


void OdbExtractObject::write_yaml (CmdLineArguments &command_line_arguments, Logging &log_file) {
}

void OdbExtractObject::write_json (CmdLineArguments &command_line_arguments, Logging &log_file) {
}