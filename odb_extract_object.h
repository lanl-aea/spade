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
    int max_column_size;
};


struct tangential_behavior_type {
    string formulation;
    string directionality;
    string slipRateDependency;  // Boolean
    string pressureDependency;  // Boolean
    string temperatureDependency;  // Boolean
    int dependencies;
    string exponentialDecayDefinition;
    vector<vector <double>> table;
    string shearStressLimit;  // String NONE or a double
    string maximumElasticSlip;
    double fraction;
    string absoluteDistance;  // String NONE or a double
    string elasticSlipStiffness;
    string nStateDependentVars;
    string useProperties;
};

struct odb_element_type {
    int label;
    string type;
    vector<int> connectivity;
    section_category_type sectionCategory;
    vector<string> instanceNames;
};

struct odb_node_type {
    int label;
    float coordinates[3];
};

struct odb_set_type {
    string name;
    string type;  // Enum [NODE_SET, ELEMENT_SET, SURFACE_SET]
    int size;
    vector<string> instanceNames;
    vector<odb_node_type> nodes;
    vector<odb_element_type> elements;
    vector<string> faces;
};

struct contact_standard_type {
    string sliding;  // Symbolic Constant [FINITE, SMALL]
    float smooth;
    float hcrit;
    string limitSlideDistance;
    string slideDistance;
    float extensionZone;
    string adjustMethod;  // Symbolic Constant [NONE, OVERCLOSED, TOLERANCE, SET]
    float adjustTolerance;
    string enforcement;  // Symbolic Constant [NODE_TO_SURFACE, SURFACE_TO_SURFACE]
    string thickness;  // Boolean
    string tied;  // Boolean
    string contactTracking;  // Symbolic Constant [ONE_CONFIG, TWO_CONFIG]
    string createStepName;

    tangential_behavior_type interactionProperty;
    odb_set_type main;
    odb_set_type secondary;
    odb_set_type adjust;
};

struct contact_explicit_type {
    string sliding;  // Symbolic Constant [FINITE, SMALL]
    string mainNoThick;
    string secondaryNoThick;
    string mechanicalConstraint;
    string weightingFactorType;
    float weightingFactor;
    string createStepName;
    string useReverseDatumAxis;  // Boolean
    string contactControls;
  
    tangential_behavior_type interactionProperty;
    odb_set_type main;
    odb_set_type secondary;
 
};

struct part_type {
    string name;
};

struct root_assembly_type {
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
        //! Process the open odb file and store the results
        /*!
          After the odb has been opened this function will do the parsing, including calling of other functions needed for parsing
          \param odb An open odb object
          \param log_file Logging object for writing log messages
        */
        void process_odb (odb_Odb &odb, Logging &log_file);
        //! Process odb_SectionCategory object from the odb file
        /*!
          Process odb_SectionCategory object and return the values in a section_category_type
          \param section_category An odb_SectionCategory object in the odb
          \param log_file Logging object for writing log messages
          \return odb_node_type with data stored from the odb
          \sa process_odb()
        */
        section_category_type process_section_category (const odb_SectionCategory &section_category, Logging &log_file);
        //! Process interaction property object from the odb file
        /*!
          Process interaction property object and return the values in a tangential_behavior_type
          \param interaction_property An odb_InteractionProperty object in the odb
          \param log_file Logging object for writing log messages
          \return tangential_behavior_type with data stored from the odb
          \sa process_interactions()
          \sa process_constraints()
          \sa process_odb()
        */
        tangential_behavior_type process_interaction_property (const odb_InteractionProperty &interaction_property, Logging &log_file);
        //! Process odb_Node object from the odb file
        /*!
          Process odb_Node object and return the values in an odb_node_type
          \param node An odb_Node object in the odb
          \param log_file Logging object for writing log messages
          \return odb_node_type with data stored from the odb
          \sa process_odb()
        */
        odb_node_type process_node (const odb_Node &node, Logging &log_file);
        //! Process odb_Element object from the odb file
        /*!
          Process odb_Element property object and return the values in an odb_element_type
          \param element An odb_Element object in the odb
          \param log_file Logging object for writing log messages
          \return odb_elment_type with data stored from the odb
          \sa process_odb()
        */
        odb_element_type process_element (const odb_Element &element, Logging &log_file);
        //! Process odb set object from the odb file
        /*!
          Process odb set object and return the values in an odb_set_type
          \param set An odb_Set object in the odb
          \param log_file Logging object for writing log messages
          \return odb_set_type with data stored from the odb
          \sa process_odb()
        */
        odb_set_type process_set (const odb_Set &set, Logging &log_file);
        //! Process interactions from the odb file
        /*!
          Process interactions and store the results
          \param interactions A repository of interactions in the odb
          \param odb An open odb object
          \param log_file Logging object for writing log messages
          \sa process_odb()
        */
        void process_interactions (const odb_InteractionRepository &interactions, odb_Odb &odb, Logging &log_file);
        //! Process constraints from the odb file
        /*!
          Process constraints and store the results
          \param constraints A repository of constraints in the odb
          \param odb An open odb object
          \param log_file Logging object for writing log messages
          \sa process_odb()
        */
        void process_constraints (const odb_ConstraintRepository &constraints, odb_Odb &odb, Logging &log_file);
        //! Process a part from the odb file
        /*!
          Process a part object and store the results
          \param part An odb part object
          \param odb An open odb object
          \param log_file Logging object for writing log messages
          \sa process_odb()
        */
        void process_part (const odb_Part &part, odb_Odb &odb, Logging &log_file);
        //! Process an instance from the odb file
        /*!
          Process an instance and store the results
          \param instance An odb instance object
          \param odb An open odb object
          \param log_file Logging object for writing log messages
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \sa process_odb()
        */
        void process_instance (const odb_Instance &instance, odb_Odb &odb, Logging &log_file, CmdLineArguments &command_line_arguments);
        //! Process the root assembly from the odb file
        /*!
          Process the root assembly and store the results
          \param instance An odb instance object
          \param odb An open odb object
          \param log_file Logging object for writing log messages
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \sa process_odb()
        */
        void process_root_assembly (const odb_Assembly &root_assembly, odb_Odb &odb, Logging &log_file, CmdLineArguments &command_line_arguments);
        //! Process a step from the odb file
        /*!
          Process a step object and store the results
          \param step An odb step object
          \param odb An open odb object
          \param log_file Logging object for writing log messages
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \sa process_odb()
        */
        void process_step (const odb_Step &step, odb_Odb &odb, Logging &log_file, CmdLineArguments &command_line_arguments);


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
          \param group Name of HDF5 group in which to write the new attribute
          \param attribute_name Name of the new attribute where a string is to be written
          \param string_value The string that should be written in the new attribute
        */
        void write_attribute(const H5::Group& group, const string & attribute_name, const string & string_value);
        //! Write a string as a dataset
        /*!
          Create a dataset with a single string using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a string is to be written
          \param string_value The string that should be written in the new dataset
        */
        void write_string_dataset(const H5::Group& group, const string & dataset_name, const string & string_value);
        //! Write a vector of strings as a dataset
        /*!
          Create a dataset with an array of strings using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where an array of strings is to be written
          \param string_values The vector of strings that should be written in the new dataset
        */
        void write_vector_string_dataset(const H5::Group& group, const string & dataset_name, const vector<const char*> & string_values);
        //! Write an integer as a dataset
        /*!
          Create a dataset with an integer using the passed-in value
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where an integer is to be written
          \param int_value The integer that should be written in the new dataset
        */
        void write_integer_dataset(const H5::Group& group, const string & dataset_name, const int & int_value);
        //! Write a vector of vectors of floats as a dataset
        /*!
          Create a dataset with a two-dimensional array of floats using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param max_column_size Integer indicating the column dimension, row dimension is determined by size of data_array
          \param data_array The vector of vectors of floats that should be written in the new dataset
        */
        void write_2D_float(const H5::Group& group, const string & dataset_name, int & max_column_size, vector<vector<float>> & data_array);


    private:
        string name;
        string analysisTitle;
        string description;
        string path;
        bool isReadOnly;
        job_data_type job_data;
        vector<part_type> parts;
        sector_definition_type sector_definition;
        vector<section_category_type> section_categories;
        vector<user_xy_data_type> user_xy_data;
        vector<contact_standard_type> standard_interactions;
        vector<contact_explicit_type> explicit_interactions;
        /*
        odb_Assembly& rootAssembly;
        odb_PartRepository& parts;
        odb_StepRepository& steps;
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
};
#endif  // __ODB_EXTRACT_OBJECT_H_INCLUDED__
