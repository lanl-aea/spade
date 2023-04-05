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
static const std::string slash="\\";
#else
static const std::string slash="/";
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
        std::ifstream odb_file(this->command_line_args["odb-file"].c_str());
        if (!odb_file) {
            cerr << this->command_line_args["odb-file"] << " does not exist.\n";
            perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
        }

        // Check for output file and if it exists
        if (this->command_line_args["output-file"].empty()) {
            string base_file_name = this->command_line_args["odb-file"];
            base_file_name = base_file_name.substr(0,base_file_name.size()-4);  //remove '.odb'
            this->command_line_args["output-file"] = base_file_name + ".h5"; 
        }
        std::ifstream output_file(this->command_line_args["output-file"].c_str());
        if (output_file) {
            cerr << this->command_line_args["output-file"] << " already exists. Please provide a different name for the output file.\n";
            perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
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

bool CmdLineArguments::endsWith (std::string const &full_string, std::string const &ending) {
    if (full_string.length() >= ending.length()) {
        return (0 == full_string.compare (full_string.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::string CmdLineArguments::verboseArguments () {
    std::string arguments;
    char floatBuff[CHAR_BUFF]; 

    arguments += "\n";
    arguments += "input file: " + this->command_line_args["odb-file"] + "\n";
    arguments += "output file: " + this->command_line_args["output-file"] + "\n";
    arguments += "output file type: " + this->command_line_args["output-file-type"] + "\n";
    if (this->verbose_output) { arguments += "verbose: True\n"; } else { arguments += "verbose: False\n"; }   
    arguments += "step: " + this->command_line_args["step"] + "\n";
    arguments += "frame: " + this->command_line_args["frame"] + "\n";
    arguments += "frame value: " + this->command_line_args["frame-value"] + "\n";
    arguments += "field: " + this->command_line_args["field"] + "\n";
    arguments += "history: " + this->command_line_args["history"] + "\n";
    arguments += "history region: " + this->command_line_args["history-region"] + "\n";
    arguments += "instance: " + this->command_line_args["instance"] + "\n";
    // TODO: Add more here
    arguments += "\n";

    return arguments;
}

std::string CmdLineArguments::helpMessage () {
    std::string help_message;

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
std::string CmdLineArguments::operator[](std::string const &str) const {
    std::string blank = "";
    std::map<std::string, std::string>::const_iterator iter;
    iter = command_line_args.find(str);
    if (iter != command_line_args.end()) { return iter->second; }
    else { return blank; }
}

const std::string& CmdLineArguments::commandLine() const { return this->command_line; }
const std::string& CmdLineArguments::commandName() const { return this->command_name; }
bool CmdLineArguments::verbose() const { return this->verbose_output; }
bool CmdLineArguments::help() const { return this->help_command; }