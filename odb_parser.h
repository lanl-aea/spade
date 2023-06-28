//! An object for holding the data from the odb

#include <vector>

#include "cmd_line_arguments.h"
#include "logging.h"

#include <odb_API.h>


#ifndef __ODB_PARSER_H_INCLUDED__
#define __ODB_PARSER_H_INCLUDED__

using namespace std;

struct root_assembly {
};

struct job_data_type {
    string analysisCode;
    string creationTime;
    string machineName;
    string modificationTime;
    string name;
    string precision;
    vector<string> productAddOns;
    string version;
};

struct user_data_type {
  string name;
};

/*!
   This class holds all the data from the odb
*/
class OdbParser {
    public:
        //! The constructor.
        /*!
          The constructor checks to see if the odb file needs to be upgraded, upgrades if necessary, then opens it and calls the function to process the odb
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \sa process_odb()
        */
        OdbParser (CmdLineArguments &command_line_arguments, Logging &log_file);
        //! Parse the open odb file
        /*!
          After the odb has been opened this function will do the parsing, including calling of other functions needed for parsing
          \param odb odb_Odb an open odb object
          \param log_file Logging object for writing log messages
        */
        void process_odb (odb_Odb const &odb, Logging &log_file);

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
//        map<string, string> jobData();
        job_data_type jobData() const;

    private:
        string name;
        string analysisTitle;
        string description;
        string path;
        string isReadOnly;
        job_data_type job_data;
        /*
        odb_Assembly& rootAssembly;
        odb_PartRepository& parts;
        odb_StepRepository& steps;
        odb_SectionCategoryRepository& sectionCategories;
        odb_SectorDefinition& sectorDefinition;
        odb_InteractionRepository& interactions;
        odb_InteractionPropertyRepository& interactionProperties;
        */
        user_data_type user_data;
};
#endif  // __ODB_PARSER_H_INCLUDED__
