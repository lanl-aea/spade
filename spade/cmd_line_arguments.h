//! An object for holding command line arguments

#include <map>

#ifndef __CMD_LINE_ARGUMENTS_H_INCLUDED__
#define __CMD_LINE_ARGUMENTS_H_INCLUDED__

using namespace std;

/*!
   This class collects and verifies command line options and creates the help menu.
*/
class CmdLineArguments {
    public:
        //! The constructor.
        /*!
          The constructor uses getopt to get the command line options. Then it verifies user input, sets defaults, and stores the values. It quits execution if there are fatal errors. 
          \param argc integer containing count of command line arguments
          \param argv array containing command line arguments
        */
        CmdLineArguments (int &argc, char **argv);
        //! String of verbose arguments.
        /*!
          This function returns a string listing the values of all the command line options.
          \return string containing the command line options
        */
        string verboseArguments ();
        //! String with help message.
        /*!
          This function returns a string with a POSIX-like help message for the user.
          \return string with the help message
          \sa CmdLineArguments()
        */
        string helpMessage ();
        //! Check if string ends with substring.
        /*!
          This function takes in two strings and determines whether one string contains the other string as a substring at the end.
          \param full_string full string to check
          \param ending substring to check
          \return boolean with answer
        */
        bool endsWith (string const &full_string, string const &ending);
        //! Return a timestamp as a string.
        /*!
          This function returns a string with a time stamp.
          \param for_file boolean indicating whether the string should be in a format for a file name
          \return string with the time stamp
        */
        string getTimeStamp (bool for_file);

        // Getter Functions
        // \sa is for "see also"

        //! Operator which allows class to be used like a map data structure.
        /*!
          This function returns a command line option corresponding to the provided string.
          \param str string containing the command line option
          \return value of command line option
          \sa CmdLineArguments()
        */
        string operator[](string const &str) const;
        //! Getter if operator can't be used
        /*!
          This function returns a command line option corresponding to the provided string.
          \param str string containing the command line option
          \return value of command line option
          \sa CmdLineArguments()
        */
        string get(string const &str) const;
        //! Return string with complete command line given.
        /*!
          Return the complete command line used by user. This is a getter method.
          \return string with all command line options given
        */
        const string& commandLine() const;
        //! Return the name of the command given.
        /*!
          This returns the value of the command typed on the command line, should be the name of the compiled code where main resides (i.e. argv[0]). This is a getter method.
          \return string containing the name of command
        */
        const string& commandName() const;
        //! Return a string with the start time of the execution of this process
        /*!
          Return a string containing the start time of the execution of this process. This is a getter method.
          \return string containing the start time
        */
        const string& startTime() const;
        //! Return the value of the verbose flag.
        /*!
          If the user specifies verbose this function will return true, otherwise it will return false. This is a getter method.
          \return boolean indicating whether the verbose option is used
        */
        bool verbose() const;
        //! Return the value of the help flag.
        /*!
          If the user requests the help menu, a flag is set, this function returns the value of that flag. This is a getter method.
          \return boolean indicating whether the help option is used
        */
        bool help() const;
        //! Return the value of the force flag.
        /*!
          If the user gives the force option, a flag is set, this function returns the value of that flag. This is a getter method.
          \return boolean indicating whether the force option is used
        */
        bool force() const;
        bool debug() const; // docstring not given, more for developer use

    private:
        map<string, string> command_line_arguments;
        string command_line;
        string unexpected_args;
        string command_name;
        string start_time;

        bool help_command;
        bool verbose_output;
        bool debug_output;
        bool force_overwrite;

};
#endif // __CMD_LINE_ARGUMENTS_H_INCLUDED__
