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

#include <cmd_line_arguments.h>

using namespace std;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
static const string slash="\\";
#else
static const string slash="/";
#endif

#define CHAR_BUFF 100 // Size of buffer for holding string and int/double/float/etc.


CmdLineArguments::CmdLineArguments (int &argc, char **argv) {

    int c;
    this->command_line = "";
    this->unexpected_args = "";
    this->verbose_output = false;
    this->help_command = false;
    this->command_line_args["odb-file"] = "";
    this->command_line_args["output-file"] = "";
    this->command_line_args["output-file-type"] = "";
    this->command_line_args["step"] = "";
    this->command_line_args["frame"] = "";
    this->command_line_args["frame-value"] = "";
    this->command_line_args["field"] = "";
    this->command_line_args["history"] = "";
    this->command_line_args["history-region"] = "";
    this->command_line_args["instance"] = "";
    this->command_line_args["log-file"] = "";

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help",             no_argument,       0,  'h'},
            {"output-file",      required_argument, 0,  'o'},
            {"output-file-type", required_argument, 0,  't'},
            {"verbose",          no_argument,       0,  'v'},
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

        c = getopt_long(argc, argv, "ho:t:v", long_options, &option_index);
        if (c == -1) break;

        switch (c) {
            case 0: {
                string option_name =  string(long_options[option_index].name);
                if (optarg)
                    this->command_line_args[option_name] = string(optarg);
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

            case 'o': {
                string option_arg = string(optarg);
                this->command_line_args["output-file"] = optarg;
                break;
            }

            case 't': {
                string option_arg = string(optarg);
                this->command_line_args["output-file-type"] = optarg;
                break;
            }

            case '?': {
                break;
            }

        }
    }

    if (optind < argc) {
        string first_arg = string(argv[optind++]);
        if (endsWith(first_arg, ".odb")) this->command_line_args["odb-file"] = first_arg;
        else this->unexpected_args += first_arg;
        while (optind < argc) this->unexpected_args += string(argv[optind++]) + " ";
    }
    this->command_name = string(argv[0]);
    this->command_name.erase(this->command_name.find("./"), 2);  // Find and erase './' if present in command name



    if (!this->help_command) {
        // Check for odb-file and if it exists
        if (this->command_line_args["odb-file"].empty()) {
            cerr << "ODB file not provided on command line.\n";
            perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
        }
        ifstream odb_file(this->command_line_args["odb-file"].c_str());
        if (!odb_file) {
            cerr << this->command_line_args["odb-file"] << " does not exist.\n";
            perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
        }

        string base_file_name = this->command_line_args["odb-file"];
        base_file_name = base_file_name.substr(0,base_file_name.size()-4);  //remove '.odb'
        // Check for output file and if it exists
        if (this->command_line_args["output-file"].empty()) {
            this->command_line_args["output-file"] = base_file_name + ".h5"; 
        }
        // Check if output file already exists
        ifstream output_file(this->command_line_args["output-file"].c_str());
        if (output_file) {
            cerr << this->command_line_args["output-file"] << " already exists. Please provide a different name for the output file.\n";
            perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
        }
        // Create log file name if not provided
        if (this->command_line_args["log-file"].empty()) {
            this->command_line_args["log-file"] = base_file_name + ".odb_extract.log"; 
        }
        // Check if log file already exists
        ifstream log_file(this->command_line_args["log-file"].c_str());
        if (log_file) {
            cerr << this->command_line_args["log-file"] << " already exists. This file will be overwritten.\n";
            perror(""); // print error
            // TODO: decide if overwriting the log is the functionality we want
        }

        // TODO: fix default values
        // Set default values
        if (this->command_line_args["odb-file-type"].empty()) { this->command_line_args["odb-file-type"] = "h5"; }
        if (this->command_line_args["step"].empty()) { this->command_line_args["step"] = "all"; }
        if (this->command_line_args["frame"].empty()) { this->command_line_args["frame"] = "all"; }
        if (this->command_line_args["frame-value"].empty()) { this->command_line_args["frame-value"] = ""; }
        if (this->command_line_args["field"].empty()) { this->command_line_args["field"] = "all"; }
        if (this->command_line_args["history"].empty()) { this->command_line_args["history"] = "all"; }
        if (this->command_line_args["history-region"].empty()) { this->command_line_args["history-region"] = "all"; }
        if (this->command_line_args["instance"].empty()) { this->command_line_args["instance"] = "all"; }

        if (this->command_line_args["odb-file-type"].empty()) { this->command_line_args["odb-file-type"] = "h5"; }

        this->command_line = this->command_name + " ";
        for (int i=1; i<argc; ++i) { this->command_line += string(argv[i]) + " "; }  // concatenate options into single string

        if ((!this->unexpected_args.empty()) && (!this->help_command)) cout << "Unexpected arguments: " + this->unexpected_args + "\n";
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
    char floatBuff[CHAR_BUFF]; 

    arguments += "\n";
    arguments += "\tinput file: " + this->command_line_args["odb-file"] + "\n";
    arguments += "\toutput file: " + this->command_line_args["output-file"] + "\n";
    arguments += "\toutput file type: " + this->command_line_args["output-file-type"] + "\n";
    if (this->verbose_output) { arguments += "\tverbose: True\n"; } else { arguments += "\tverbose: False\n"; }   
    arguments += "\tstep: " + this->command_line_args["step"] + "\n";
    arguments += "\tframe: " + this->command_line_args["frame"] + "\n";
    arguments += "\tframe value: " + this->command_line_args["frame-value"] + "\n";
    arguments += "\tfield: " + this->command_line_args["field"] + "\n";
    arguments += "\thistory: " + this->command_line_args["history"] + "\n";
    arguments += "\thistory region: " + this->command_line_args["history-region"] + "\n";
    arguments += "\tinstance: " + this->command_line_args["instance"] + "\n";
    arguments += "\tlog file: " + this->command_line_args["log-file"] + "\n";
    // TODO: Add more here
    arguments += "\n";

    return arguments;
}

string CmdLineArguments::helpMessage () {
    string help_message;

    help_message += "usage: " + this->command_name + " [-h] [-v] [-o output_file_name.h5] [-t h5] [--step all] [--frame 0] [--frame-value frame_value] [--field field_name] [--history all] [--history-region all] [--instance instance_name] odb_file.odb\n\n";
    help_message += "Extract data from an Abaqus odb file and store it in an hdf5 file\n";
    help_message += "\npositional arguments:\n\todb_file.odb\tAbaqus odb file\n";
    help_message += "\noptional arguments:\n";
    help_message += "\t-h,\t--help\tshow this help message and exit\n";
    help_message += "\t-v,\t--verbose\tturn on verbose logging\n";
    help_message += "\t-o,\t--output-file\tname of output file (default: <odb file name>.h5)\n";
    help_message += "\t-t,\t--output-file-type\ttype of file to store output (default: h5)\n";
    help_message += "\t--step\tget information from specified step (default: all)\n";
    help_message += "\t--frame\tget information from specified frame (default: all)\n";
    help_message += "\t--frame-value\tget information from specified frame (default: all)\n";
    help_message += "\t--field\tget information from specified field (default: all)\n";
    help_message += "\t--history\tget information from specified history value (default: all)\n";
    help_message += "\t--history-region\tget information from specified history region (default: all)\n";
    help_message += "\t--instance\tget information from specified instance (default: all)\n";
    // TODO: Add more here
    help_message += "\nExample: " + this->command_name + " odb_file.odb\n";
    help_message += "\n";

    return help_message;
}

// Getter
string CmdLineArguments::operator[](string const &str) const {
    string blank = "";
    map<string, string>::const_iterator iter;
    iter = command_line_args.find(str);
    if (iter != command_line_args.end()) { return iter->second; }
    else { return blank; }
}

const string& CmdLineArguments::commandLine() const { return this->command_line; }
const string& CmdLineArguments::commandName() const { return this->command_name; }
bool CmdLineArguments::verbose() const { return this->verbose_output; }
bool CmdLineArguments::help() const { return this->help_command; }