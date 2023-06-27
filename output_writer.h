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
        OutputWriter (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser &odb_parser);
        //! Write output to a YAML file.
        /*!
          Write the parsed odb data to a YAML formatted file
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \param odb_parser OdbParser object with the parsed data from the odb
          \sa OutputWriter()
        */
        void write_yaml (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser &odb_parser);
        //! Write output to a JSON file.
        /*!
          Write the parsed odb data to a JSON formatted file
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \param odb_parser OdbParser object with the parsed data from the odb
          \sa OutputWriter()
        */
        void write_json (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser &odb_parser);
        //! Write output to an HDF5 file.
        /*!
          Write the parsed odb data to an HDF5 formatted file
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \param odb_parser OdbParser object with the parsed data from the odb
          \sa OutputWriter()
        */
        void write_h5 (CmdLineArguments &command_line_arguments, Logging &log_file, OdbParser &odb_parser);
        //! Write Top level groups in hdf5 file
        /*!
          Create all top level data members of the odb inside of the hdf5 file
          \param h5_file HDF5 file in which to create gtop level groups
          \param log_file Logging object for writing log messages
          \param odb_parser OdbParser object with the parsed data from the odb
          \sa write_h5()
        */
        void create_top_level_groups (H5File &h5_file, Logging &log_file, OdbParser &odb_parser);

    private:
         H5::Group odb_group;
         // TODO: potentially add amplitudes group
         H5::Group contraints_group;
         // TODO: potentially add filters group
         H5::Group interactions_group;
         H5::Group job_data_group;
         // TODO: potentially add materials group
         H5::Group parts_group;
         H5::Group root_assembly_group;
         H5::Group section_categories_group;
         // TODO: add conditional to check that odb_parser has sector definition before adding the group
         H5::Group sector_definition_group;
         H5::Group steps_group;
         H5::Group user_data_group;

};
#endif  // __OUTPUT_WRITER_H_INCLUDED__
