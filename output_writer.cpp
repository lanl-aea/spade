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
#include <math.h>

#include <cmd_line_arguments.h>
#include <logging.h>
#include <output_writer.h>
#include "H5Cpp.h"
using namespace H5;

#define CHAR_BUFF 100 // Size of buffer for holding string and int/double/float/etc.

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

OutputWriter::OutputWriter (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser &odb_parser) {

    log_file.logVerbose("Starting to write output file: " + command_line_arguments.getTimeStamp(false) + "\n");

    if (command_line_arguments["output-file-type"] == "h5") this->write_h5(command_line_arguments, log_file, odb_parser);
    else if (command_line_arguments["output-file-type"] == "json") this->write_json(command_line_arguments, log_file, odb_parser);
    else if (command_line_arguments["output-file-type"] == "yaml") this->write_yaml(command_line_arguments, log_file, odb_parser);

    log_file.logVerbose("Finished writing output file: " + command_line_arguments.getTimeStamp(false) + "\n");
    
}

void OutputWriter::write_h5 (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser &odb_parser) {
// Write out data to hdf5 file

    // Open file for writing
    std::ifstream hdf5File (command_line_arguments["output-file"].c_str());
    log_file.logDebug("Creating hdf5 file " + command_line_arguments["output-file"] + "\n");
    const H5std_string FILE_NAME(command_line_arguments["output-file"]);
    H5File file(FILE_NAME, H5F_ACC_TRUNC);

//        hid_t fileId = file.getId();
//        Group fileGroup = H5Gopen(fileId, SLASH_GROUP.c_str(), H5P_DEFAULT);
//        Group unstructuredMeshGroup = H5Gopen(fileGroup.getId(), TOP_LEVEL_GROUP.c_str(), H5P_DEFAULT);

//    H5::Group odb_group = file.createGroup(string("/odb").c_str());
    log_file.logDebug("Creating odb group for meta-data " + command_line_arguments["output-file"] + "\n");
    create_top_level_groups(file, log_file, odb_parser);
//    string job_data_group_name = "/odb/jobData";

    StrType str_type(0, H5T_VARIABLE);
    DataSpace att_space(H5S_SCALAR);
    DataSpace str_space(H5S_SCALAR);
    map<string, string> job_data = odb_parser.jobData();

    H5::DataSet name_ds = this->odb_group.createDataSet( "name", str_type, str_space ); name_ds.write( odb_parser["name"], str_type );

    H5::Attribute name_attribute = this->odb_group.createAttribute( "name", str_type, att_space ); name_attribute.write( str_type, odb_parser["name"] );
    H5::Attribute analysisTitle_attribute = this->odb_group.createAttribute( "analysisTitle", str_type, att_space ); analysisTitle_attribute.write( str_type, odb_parser["analysisTitle"] );
    H5::Attribute description_attribute = this->odb_group.createAttribute( "description", str_type, att_space ); description_attribute.write( str_type, odb_parser["description"] );
    H5::Attribute path_attribute = this->odb_group.createAttribute( "path", str_type, att_space ); path_attribute.write( str_type, odb_parser["path"] );
//    std::stringstream bool_stream; bool_stream << std::boolalpha << odb_info["isReadOnly"]; string bool_string = bool_stream.str();
//    cout << bool_string << endl;
//    H5::Attribute isReadOnly_attribute = info_group.createAttribute( "isReadOnly", str_type, att_space ); isReadOnly_attribute.write( str_type, bool_string );

    /*
    H5::Exception::dontPrint();                             // suppress error messages
    string odb_group_name = "/test";
    try         {
      H5::Group group = file.openGroup  (group_name.c_str());
      std::cerr<<" TEST: opened group\n";                   // for debugging
    } catch (...) {
      std::cerr<<" TEST: caught something\n";               // for debugging
      H5::Group group = file.createGroup(group_name.c_str());
      std::cerr<<" TEST: created group\n";                  // for debugging
    }
    H5::Group group = file.openGroup  (group_name.c_str()); // for debugging
    std::cerr<<" TEST: opened group\n";                     // for debugging
    */

    file.close();  // Close the hdf5 file
}

void OutputWriter::create_top_level_groups (H5File &h5_file, Logging &log_file, OdbParser &odb_parser) {
//    for(const string &group : groups)
    this->odb_group = h5_file.createGroup(string("/odb").c_str());
    // TODO: potentially add amplitudes group
    this->contraints_group = h5_file.createGroup(string("/odb/constraints").c_str());
    // TODO: potentially add filters group
    this->interactions_group = h5_file.createGroup(string("/odb/interactions").c_str());
    this->job_data_group = h5_file.createGroup(string("/odb/jobData").c_str());
    // TODO: potentially add materials group
    this->parts_group = h5_file.createGroup(string("/odb/parts").c_str());
    this->root_assembly_group = h5_file.createGroup(string("/odb/rootAssembly").c_str());
    this->section_categories_group = h5_file.createGroup(string("/odb/sectionCategories").c_str());
    // TODO: add conditional to check that odb_parser has sector definition before adding the group
    this->sector_definition_group = h5_file.createGroup(string("/odb/sectorDefinition").c_str());
    this->steps_group = h5_file.createGroup(string("/odb/steps").c_str());
    this->user_data_group = h5_file.createGroup(string("/odb/userData").c_str());
}

void OutputWriter::write_yaml (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser &odb_parser) {
}

void OutputWriter::write_json (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser &odb_parser) {
}