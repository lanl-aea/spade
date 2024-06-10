#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <cstring>
#include <cstdlib>

#include <logging.h>

using namespace std;

Logging::Logging (string const &log_file_name, bool const &log_verbose, bool const &log_debug) {

    this->log_file.open(log_file_name.c_str());
    if (!this->log_file.is_open()) {
        cerr << "Couldn't open: " << log_file_name << " Log messages will be printed to stdout." << endl;
        this->output_stream = &std::cout;
    } else { this->output_stream = &this->log_file; }
    time_t now = time(0); // current date/time based on current system
    char* dt = ctime(&now); // convert now to string form
    *this->output_stream << "Started at: " << dt;
    this->log_debug = log_debug;
    this->log_verbose = log_verbose;
}
Logging::~Logging () {

    time_t now = time(0);
    char* dt = ctime(&now);
    *this->output_stream << "Finished at: " << dt << endl;
    if (this->log_file.is_open()) { this->log_file.close(); }
}

void Logging::log (string const &output) { *this->output_stream << output << endl; }
void Logging::logVerbose (string const &output) { if (this->log_verbose) { *this->output_stream << output << endl; } }
void Logging::logWarning (string const &output) { *this->output_stream << "WARNING: " << output << endl;  }
void Logging::logDebug (string const &output) { if (this->log_debug) { *this->output_stream << output << endl; } }

void Logging::logErrorAndExit (string const &output) {
    *this->output_stream << output << endl;
    time_t now = time(0);
    char* dt = ctime(&now);
    *this->output_stream << "Exited with error at: " << dt << endl;
    if (this->log_file.is_open()) { this->log_file.close(); }
    throw std::runtime_error(output);
}
