//! An object for holding the data from the odb

#include "cmd_line_arguments.h"
#include "logging.h"

#ifndef __ODB_PARSER_H_INCLUDED__
#define __ODB_PARSER_H_INCLUDED__

using namespace std;

/*!
   This class holds all the data from the odb
*/
class OdbParser {
    public:
        //! The constructor.
        /*!
          The constructor opens the odb file and stores and organizes the data from within
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
        */
        OdbParser (CmdLineArguments const &command_line_arguments, Logging &log_file);

    private:
        string odb_name;
        string analysis_title;
        string path;
        bool is_read_only;
};
#endif  // __ODB_PARSER_H_INCLUDED__
