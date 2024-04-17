#include <iostream>

#ifdef _WIN32
    #include <direct.h>
    #define getcwd _getcwd // MSFT "deprecation" warning
#else
    #include <unistd.h>  // Needed for getopt and potentially other POSIX API
#endif

#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>     /* for printf */
#include <getopt.h>
#include <iomanip>
#include <ctime>
#include <filesystem>
//#include <cctype>
#include <algorithm>

#include <cmd_line_arguments.h>

using namespace std;

CmdLineArguments::CmdLineArguments (int &argc, char **argv) {

    int c;
    this->command_line = "";
    this->unexpected_args = "";
    this->verbose_output = false;
    this->debug_output = false;
    this->help_command = false;
    this->force_overwrite = false;
    this->command_line_arguments["odb-file"] = "";
    this->command_line_arguments["extracted-file"] = "";
    this->command_line_arguments["extracted-file-type"] = "";
    this->command_line_arguments["step"] = "";
    this->command_line_arguments["frame"] = "";
    this->command_line_arguments["frame-value"] = "";
    this->command_line_arguments["field"] = "";
    this->command_line_arguments["history"] = "";
    this->command_line_arguments["history-region"] = "";
    this->command_line_arguments["instance"] = "";
    this->command_line_arguments["log-file"] = "";
    this->start_time = this->getTimeStamp(true);

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help",             no_argument,       0,  'h'},
            {"extracted-file",      required_argument, 0,  'e'},
            {"extracted-file-type", required_argument, 0,  't'},
            {"verbose",          no_argument,       0,  'v'},
            {"debug",            no_argument,       0,  'd'},
            {"force-overwrite",  no_argument,       0,  'f'},
            {"step",             required_argument, 0,  0 },
            {"frame",            required_argument, 0,  0 },
            {"frame-value",      required_argument, 0,  0 },
            {"field",            required_argument, 0,  0 },
            {"history",          required_argument, 0,  0 },
            {"history-region",   required_argument, 0,  0 },
            {"instance",         required_argument, 0,  0 },
            {"log-file",         required_argument, 0,  0 },
            {0,0,0,0 }
        };

        c = getopt_long(argc, argv, "ho:t:vdf", long_options, &option_index);
        if (c == -1) break;

        switch (c) {
            case 0: {
                string option_name =  string(long_options[option_index].name);
                if (optarg)
                    this->command_line_arguments[option_name] = string(optarg);
                break;
            }

            case 'h': {
                this->help_command = true;
                break;
            }

            case 'v': {
                this->verbose_output = true;
                break;
            }

            case 'd': {
                this->debug_output = true;
                break;
            }

            case 'o': {
                string option_arg = string(optarg);
                this->command_line_arguments["extracted-file"] = optarg;
                break;
            }

            case 't': {
                string option_arg = string(optarg);
                this->command_line_arguments["extracted-file-type"] = optarg;
                break;
            }

            case 'f': {
                this->force_overwrite = true;
                break;
            }

            case '?': {
                break;
            }

        }
    }

    if (optind < argc) {
        string first_arg = string(argv[optind++]);
        if (endsWith(first_arg, ".odb")) this->command_line_arguments["odb-file"] = first_arg;
        else this->unexpected_args += first_arg;
        while (optind < argc) this->unexpected_args += string(argv[optind++]) + " ";
    }
    this->command_name = string(argv[0]);
    this->command_name = std::filesystem::path(this->command_name).filename().generic_string();


    if (!this->help_command) {
        // Check for odb-file and if it exists
        if (this->command_line_arguments["odb-file"].empty()) {
            cerr << "ODB file not provided on command line.\n";
            perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
        }
        if (!std::filesystem::exists(std::filesystem::path(this->command_line_arguments["odb-file"]))) {
            cerr << this->command_line_arguments["odb-file"] << " does not exist.\n";
            perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
        }

        // Handle extracted file type
        std::transform(this->command_line_arguments["extracted-file-type"].begin(), this->command_line_arguments["extracted-file-type"].end(), this->command_line_arguments["extracted-file-type"].begin(), ::tolower);
        if ((this->command_line_arguments["extracted-file-type"] != "json") && (this->command_line_arguments["extracted-file-type"] != "yaml")) this->command_line_arguments["extracted-file-type"] = "h5";

        string base_file_name = std::filesystem::path(this->command_line_arguments["odb-file"]).replace_extension("").generic_string();

        // Handle extracted file name
        if (this->command_line_arguments["extracted-file"].empty()) { this->command_line_arguments["extracted-file"] = base_file_name + "." + this->command_line_arguments["extracted-file-type"]; }
        std::filesystem::path file_path = this->command_line_arguments["extracted-file"];
        std::filesystem::perms directory_permissions = std::filesystem::status(file_path.parent_path()).permissions();
        if (std::filesystem::perms::none == (std::filesystem::perms::owner_write & directory_permissions)) {  // If parent path is not writable, exit with error
            cerr << "Do not have write permission for: " << file_path.parent_path() << '\n';
            perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
        }

        // Check if extracted file already exists
        if (std::filesystem::exists(file_path)) {
            if (!this->force_overwrite) {
                cerr << this->command_line_arguments["extracted-file"] << " already exists. Appending time stamp to extracted file.\n";
                this->command_line_arguments["extracted-file"] = base_file_name + "_" + this->start_time + "." + this->command_line_arguments["extracted-file-type"]; 
            } else {
                if ( remove(this->command_line_arguments["extracted-file"].c_str()) != 0 ) {
                    cerr << "Cannot delete: " << this->command_line_arguments["extracted-file"] << "\n";
                    perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
                }
            }
        } 
        // Create log file name if not provided
        if (this->command_line_arguments["log-file"].empty()) {
            this->command_line_arguments["log-file"] = base_file_name + ".spade.log"; 
        }
        // Check if log file already exists
        std::filesystem::path log_file = this->command_line_arguments["log-file"];
        if (std::filesystem::exists(log_file)) {
            cerr << this->command_line_arguments["log-file"] << " already exists. Appending time stamp to log file.\n";
            string log_extension = log_file.extension();
            string log_base_name = log_file.replace_extension("").generic_string();
            this->command_line_arguments["log-file"] = log_base_name + "_" + this->start_time + log_extension;
        }

        // TODO: fix default values
        // Set default values
        if (this->command_line_arguments["step"].empty()) { this->command_line_arguments["step"] = "all"; }
        if (this->command_line_arguments["frame"].empty()) { this->command_line_arguments["frame"] = "all"; }
        if (this->command_line_arguments["frame-value"].empty()) { this->command_line_arguments["frame-value"] = ""; }
        if (this->command_line_arguments["field"].empty()) { this->command_line_arguments["field"] = "all"; }
        if (this->command_line_arguments["history"].empty()) { this->command_line_arguments["history"] = "all"; }
        if (this->command_line_arguments["history-region"].empty()) { this->command_line_arguments["history-region"] = "all"; }
        if (this->command_line_arguments["instance"].empty()) { this->command_line_arguments["instance"] = "all"; }

        this->command_line = this->command_name + " ";
        for (int i=1; i<argc; ++i) { this->command_line += string(argv[i]) + " "; }  // concatenate options into single string

        if ((!this->unexpected_args.empty()) && (!this->help_command)) cerr << "Unexpected arguments: " + this->unexpected_args + "\n";
    }

}

bool CmdLineArguments::endsWith (string const &full_string, string const &ending) {
    if (full_string.length() >= ending.length()) {
        return (0 == full_string.compare (full_string.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

string CmdLineArguments::verboseArguments () {
    string arguments;

    arguments += "\n";
    arguments += "\tinput file: " + this->command_line_arguments["odb-file"] + "\n";
    arguments += "\textracted file: " + this->command_line_arguments["extracted-file"] + "\n";
    arguments += "\textracted file type: " + this->command_line_arguments["extracted-file-type"] + "\n";
    if (this->verbose_output) { arguments += "\tverbose: True\n"; } else { arguments += "\tverbose: False\n"; }   
    arguments += "\tstep: " + this->command_line_arguments["step"] + "\n";
    arguments += "\tframe: " + this->command_line_arguments["frame"] + "\n";
    arguments += "\tframe value: " + this->command_line_arguments["frame-value"] + "\n";
    arguments += "\tfield: " + this->command_line_arguments["field"] + "\n";
    arguments += "\thistory: " + this->command_line_arguments["history"] + "\n";
    arguments += "\thistory region: " + this->command_line_arguments["history-region"] + "\n";
    arguments += "\tinstance: " + this->command_line_arguments["instance"] + "\n";
    arguments += "\tlog file: " + this->command_line_arguments["log-file"] + "\n";
    // TODO: Add more here
    arguments += "\n";

    return arguments;
}

string CmdLineArguments::helpMessage () {
    string help_message;

    help_message += "usage: " + this->command_name + " [-h] [-v] [-o extracted_file_name.h5] [-t h5] [--step all] [--frame 0] [--frame-value frame_value] [--field field_name] [--history all] [--history-region all] [--instance instance_name] odb_file.odb\n\n";
    help_message += "Extract data from an Abaqus odb file and store it in an hdf5 file\n";
    help_message += "\npositional arguments:\n\todb_file.odb\tAbaqus odb file\n";
    help_message += "\noptional arguments:\n";
    help_message += "\t-h,\t--help\tshow this help message and exit\n";
    help_message += "\t-v,\t--verbose\tturn on verbose logging\n";
    help_message += "\t-e,\t--extracted-file\tname of extracted file (default: <odb file name>.h5)\n";
    help_message += "\t-t,\t--extracted-file-type\ttype of file to store extracted output (default: h5)\n";
    help_message += "\t-f,\t--force-overwrite\tif extracted file already exists, then over write the file\n";
    help_message += "\t--step\tget information from specified step (default: all)\n";
    help_message += "\t--frame\tget information from specified frame (default: all)\n";
    help_message += "\t--frame-value\tget information from specified frame (default: all)\n";
    help_message += "\t--field\tget information from specified field (default: all)\n";
    help_message += "\t--history\tget information from specified history value (default: all)\n";
    help_message += "\t--history-region\tget information from specified history region (default: all)\n";
    help_message += "\t--instance\tget information from specified instance (default: all)\n";
    help_message += "\t--log-file\tname of log file (default: <odb file name>.spade.log)\n";
    // TODO: Add more here
    help_message += "\nExample: " + this->command_name + " odb_file.odb\n";
    help_message += "\n";

    return help_message;
}

string CmdLineArguments::getTimeStamp(bool for_file) {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::stringstream time_buffer;
    if (for_file) {
        time_buffer << std::put_time(&tm, "%Y%m%d-%H%M%S");
    } else {
        time_buffer << std::put_time(&tm, "%a %b %d %H:%M:%S %Y");
    }
    return time_buffer.str();
}

// Getter
string CmdLineArguments::operator[](string const &str) const {
    string blank = "";
    map<string, string>::const_iterator iter;
    iter = command_line_arguments.find(str);
    if (iter != command_line_arguments.end()) { return iter->second; }
    else { return blank; }
}

const string& CmdLineArguments::commandLine() const { return this->command_line; }
const string& CmdLineArguments::commandName() const { return this->command_name; }
const string& CmdLineArguments::startTime() const { return this->start_time; }
bool CmdLineArguments::verbose() const { return this->verbose_output; }
bool CmdLineArguments::debug() const { return this->debug_output; }
bool CmdLineArguments::force() const { return this->force_overwrite; }
bool CmdLineArguments::help() const { return this->help_command; }