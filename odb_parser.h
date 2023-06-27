//! An object for holding the data from the odb

#include "cmd_line_arguments.h"
#include "logging.h"

#ifndef __ODB_PARSER_H_INCLUDED__
#define __ODB_PARSER_H_INCLUDED__

using namespace std;

struct root_assembly {
};

struct job_data {
};

struct user_data {
  string name;
};

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
        OdbParser (CmdLineArguments &command_line_arguments, Logging &log_file);

        //Getters

        //! Operator which allows class to be used like a map data structure.
        /*!
          This function returns the data found under the passed in string
          \param key string indicating where to find odb data
          \return odb data located at specified string
        */
        string operator[](string const &key) const;
        //! Returns job data from the odb
        /*!
          The odb job data contains several strings which are returned by this function
          \return map of odb job data
        */
        map<string, string> jobData();

    private:
        string name;
        string analysisTitle;
        string description;
        string path;
        string isReadOnly;
        map<string, string> job_data;
        /*
        odb_Assembly& rootAssembly;
        odb_PartRepository& parts;
        odb_StepRepository& steps;
        odb_SectionCategoryRepository& sectionCategories;
        odb_SectorDefinition& sectorDefinition;
        odb_InteractionRepository& interactions;
        odb_InteractionPropertyRepository& interactionProperties;
        */
        user_data userData;
};
#endif  // __ODB_PARSER_H_INCLUDED__
