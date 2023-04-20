//! An object for writing the data to the hdf5 file

#include "cmd_line_arguments.h"
#include "logging.h"
#include "odb_parser.h"
#include "H5Cpp.h"
using namespace H5;

#ifndef __OUTPUT_WRITER_H_INCLUDED__
#define __OUTPUT_WRITER_H_INCLUDED__


/*!
   This class handles the writing of the hdf5 file
*/
class OutputWriter {
    public:
        //! The constructor.
        /*!
          The constructor creates the object and calls the writer function
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \param odb_parser OdbParser object with the parsed data from the odb
        */
        OutputWriter (CmdLineArguments const &command_line_arguments, Logging &log_file, OdbParser const &odb_parser);

};
#endif  // __OUTPUT_WRITER_H_INCLUDED__
