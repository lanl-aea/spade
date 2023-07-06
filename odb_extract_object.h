//! An object for holding the data from the odb

#include <vector>

#include "H5Cpp.h"
using namespace H5;
#include <odb_API.h>

#include "cmd_line_arguments.h"
#include "logging.h"


#ifndef __ODB_EXTRACT_OBJECT_H_INCLUDED__
#define __ODB_EXTRACT_OBJECT_H_INCLUDED__

using namespace std;

struct job_data_type {
    string analysisCode;
    string creationTime;
    string machineName;
    string modificationTime;
    string name;
    string precision;
    vector<const char*> productAddOns;
    string version;
};

struct sector_definition_type {
    int numSectors;
    string start_point;
    string end_point;
};

struct section_point_type {
    string number;
    string description;
};

struct section_category_type {
    string name;
    string description;
    vector<section_point_type> sectionPoints;
};

struct user_xy_data_type {
    string name;
    string sourceDescription;
    string contentDescription;
    string positionDescription;
    string xAxisLabel;
    string yAxisLabel;
    string legendLabel;
    string description;
    vector<vector<float>> data;
};

struct quantity_type {
    string label;
    string type;
};

struct user_data_type {
    string name;
    string sourceDescription;
    string contentDescription;
    string positionDescription;
    string xValuesLabel;
    string yValuesLabel;
    quantity_type axis1QuantityType;
    quantity_type axis2QuantityType;
    string legendLabel;
    vector<user_xy_data_type> xyDataObjects;
};

struct part {
    string name;
};

struct root_assembly {
};

/*!
   This class holds all the data from the odb
*/
class OdbExtractObject {
    public:
        //! The constructor.
        /*!
          The constructor checks to see if the odb file needs to be upgraded, upgrades if necessary, then opens it and calls the function to process the odb
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \sa process_odb()
        */
        OdbExtractObject (CmdLineArguments &command_line_arguments, Logging &log_file);
        //! Parse the open odb file
        /*!
          After the odb has been opened this function will do the parsing, including calling of other functions needed for parsing
          \param odb odb_Odb an open odb object
          \param log_file Logging object for writing log messages
        */
        void process_odb (odb_Odb &odb, Logging &log_file);


        //Functions for writing out the data

        //! Write output to a YAML file.
        /*!
          Write the parsed odb data to a YAML formatted file
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \sa OdbExtractObject()
        */
        void write_yaml (CmdLineArguments &command_line_arguments, Logging &log_file);
        //! Write output to a JSON file.
        /*!
          Write the parsed odb data to a JSON formatted file
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \sa OdbExtractObject()
        */
        void write_json (CmdLineArguments &command_line_arguments, Logging &log_file);
        //! Write output to an HDF5 file.
        /*!
          Write the parsed odb data to an HDF5 formatted file
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \sa OdbExtractObject()
        */
        void write_h5 (CmdLineArguments &command_line_arguments, Logging &log_file);
        //! Write a string as an attribute
        /*!
          Create an attribute with a string using the passed-in values
          \param group name of HDF5 group in which to write the new attribute
          \param attribute_name Name of the new attribute where a string is to be written
          \param string_value The string that should be written in the new attribute
        */
        void write_attribute(const H5::Group& group, const string & attribute_name, const string & string_value);
        //! Write a string as a dataset
        /*!
          Create a dataset with a single string using the passed-in values
          \param group name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a string is to be written
          \param string_value The string that should be written in the new dataset
        */
        void write_string_dataset(const H5::Group& group, const string & dataset_name, const string & string_value);
        //! Write a vector of strings as a dataset
        /*!
          Create a dataset with a vector of strings using the passed-in values
          \param group name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a string is to be written
          \param string_values The vector of strings that should be written in the new dataset
        */
//        void write_vector_string_dataset(const H5::Group& group, const string & dataset_name, const vector<string> & string_values);
        void write_vector_string_dataset(const H5::Group& group, const string & dataset_name, const vector<const char*> & string_values);
        void write_integer_dataset(const H5::Group& group, const string & dataset_name, const int & int_value);

    private:
        string name;
        string analysisTitle;
        string description;
        string path;
        bool isReadOnly;
        job_data_type job_data;
        vector<part> parts;
        sector_definition_type sector_definition;
        vector<section_category_type> section_categories;
        user_data_type user_data;
        /*
        odb_Assembly& rootAssembly;
        odb_PartRepository& parts;
        odb_StepRepository& steps;
        odb_SectionCategoryRepository& sectionCategories;
        odb_SectorDefinition& sectorDefinition;
        odb_InteractionRepository& interactions;
        odb_InteractionPropertyRepository& interactionProperties;
        */

        // TODO: potentially add amplitudes group
        H5::Group contraints_group;
        // TODO: potentially add filters group
        H5::Group interactions_group;
        // TODO: potentially add materials group
        H5::Group parts_group;
        H5::Group root_assembly_group;
        // TODO: add conditional to check that odb_parser has sector definition before adding the group
        H5::Group steps_group;
        H5::Group user_data_group;
};
#endif  // __ODB_EXTRACT_OBJECT_H_INCLUDED__
