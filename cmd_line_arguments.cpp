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
    int digit_optind = 0;
    this->full_line = "";
    this->unexpected_args = "";
    this->exit = false;
    this->verboseoutput = false;
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
        int this_option_optind = optind ? optind : 1;
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
                printf("option %s", long_options[option_index].name);
                string option_name =  string(long_options[option_index].name);
                this->full_line += " --" + option_name;
                if (optarg)
                    printf(" with arg %s", optarg);
                    string option_arg = string(optarg);
                    this->full_line += " " + option_arg;
                    this->command_line_args[option_name] = option_arg;
                printf("\n");
                break;
            }

            case 'h': {
                printf("option h\n");
                this->full_line += " -h";
                this->help = true;
                this->exit = true;
                break;
            }

            case 'v': {
                printf("option v\n");
                this->full_line += " -v";
                this->verboseoutput = true;
                break;
            }

            case 'o': {
                printf("option o with value '%s'\n", optarg);
                string option_arg = string(optarg);
                this->full_line += " -o " + option_arg;
                this->command_line_args["output-file"] = optarg;
                break;
            }

            case 't': {
                printf("option t with value '%s'\n", optarg);
                string option_arg = string(optarg);
                this->full_line += " -t " + option_arg;
                this->command_line_args["output-file-type"] = optarg;
                break;
            }

            case '?': {
                break;
            }

            default: {
                printf("?? getopt returned character code 0%o ??\n", c);
            }
        }
    }

    if (optind < argc) {
         string first_arg = string(argv[optind++]);
         if (endsWith(first_arg, ".odb")) this->command_line_args["odb-file"] = first_arg;
         else this->unexpected_args += first_arg;
         while (optind < argc) this->unexpected_args += string(argv[optind++]) + " ";
     }

     if (!this->unexpected_args.empty()) cout << "Unexpected arguments: " + this->unexpected_args + "\n";

     // Check for odb-file and if it exists
     if (this->command_line_args["odb-file"].empty()) {
             cerr << "ODB file not provided on command line." << endl;
             perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
     }
     std::ifstream ifile_thermal(this->command_line_args["odb-file"].c_str());
     if (!ifile_thermal) {
         cerr << this->command_line_args["odb-file"] << " does not exist." << endl;
         perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
    }

    // Check for output file and if it exists
    if (this->command_line_args["output-file"].empty()) {
        string baseFileName = this->command_line_args["odb-file"];
        string baseFileName = baseFileName.substr(0,baseFileName.size()-4);  //remove '.odb'
        this->command_line_args["output-file"] = baseFileName + ".h5"; 
    }
    std::ifstream ifile_thermal(this->command_line_args["output-file"].c_str());
    if (ifile_thermal) {
        cerr << this->command_line_args["output-file"] << " already exists. Please provide a different name for the output file." << endl;
        perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
    }



}

bool CmdLineArguments::endsWith (std::string const &full_string, std::string const &ending) {
    if (full_string.length() >= ending.length()) {
        return (0 == full_string.compare (full_string.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::string CmdLineArguments::printCmdLineArguments () {
    std::string detailedString;
    char floatBuff[CHAR_BUFF]; 

    detailedString += "\n";
    detailedString += "input file: " + this->command_line_args["odb-file"] + "\n";
    detailedString += "output file: " + this->command_line_args["output-file"] + "\n";
    // Add more here
    detailedString += "\n";

    return detailedString;
}

// Getter
std::string CmdLineArguments::operator[](std::string const &str) const {
    std::string blank = "";
    std::map<std::string, std::string>::const_iterator iter;
    iter = command_line_args.find(str);
    if (iter != command_line_args.end()) { return iter->second; }
    else { return blank; }
}

const std::string& CmdLineArguments::cmdLine() const { return this->full_line; }
bool CmdLineArguments::verbose() const { return this->verboseoutput; }