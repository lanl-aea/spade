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

OutputWriter::OutputWriter (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser const &odb_parser) {

    log_file.logVerbose("Starting to write output file: " + command_line_arguments.getTimeStamp(false) + "\n");

    if (command_line_arguments["output-file-type"] == "h5") this->write_h5(command_line_arguments, log_file, odb_parser);
    else if (command_line_arguments["output-file-type"] == "json") this->write_json(command_line_arguments, log_file, odb_parser);
    else if (command_line_arguments["output-file-type"] == "yaml") this->write_yaml(command_line_arguments, log_file, odb_parser);

    
}

void OutputWriter::write_h5 (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser const &odb_parser) {
// Write out data to hdf5 file

    std::ifstream hdf5File (command_line_arguments["output-file"].c_str());
    log_file.logDebug("Creating hdf5 file " + command_line_arguments["output-file"] + "\n");
    if (hdf5File) {
        log_file.logErrorAndExit(command_line_arguments["output-file"] + " already exists.");
    } else {
        const H5std_string FILE_NAME(command_line_arguments["output-file"]);
//        H5File file(FILE_NAME, H5F_ACC);
//        H5File file(FILE_NAME, H5F_ACC_EXCL|H5F_ACC_TRUNC);
        H5File file(FILE_NAME, H5F_ACC_TRUNC);
//        hid_t fileId = file.getId();
//        Group fileGroup = H5Gopen(fileId, SLASH_GROUP.c_str(), H5P_DEFAULT);
//        Group unstructuredMeshGroup = H5Gopen(fileGroup.getId(), TOP_LEVEL_GROUP.c_str(), H5P_DEFAULT);
//        if (command_line_arguments.detailedOutput()) { time_t now = time(0); dt = ctime(&now); log_file.log("Finished parsing input file: " + std::string(dt)); }
    }
}

void OutputWriter::write_yaml (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser const &odb_parser) {
}

void OutputWriter::write_json (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser const &odb_parser) {
}