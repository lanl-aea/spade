//! A class for holding command line arguments

#ifndef __CMD_LINE_ARGUMENTS_H_INCLUDED__
#define __CMD_LINE_ARGUMENTS_H_INCLUDED__

/*!
   This class collects and verifies command line options and creates the help menu.
*/
class CmdLineArguments {
    public:
        //! The constructor.
        /*!
          The constructor uses getopt to get the command line options. Then it verifies user input, sets defaults, and stores the values. It quits execution if there are fatal errors. 
          \param argc command line argument count
          \param argv array of command line arguments
        */
        CmdLineArguments (int &argc, char **argv);
        //! String of verbose arguments.
        /*!
          This function returns a string listing the values of all the command line options.
          \return The string of command line options
        */
        std::string verboseArguments ();
        //! String with help message.
        /*!
          This function returns a string with a POSIX-like help message for the user.
          \return The help message
          \sa CmdLineArguments()
        */
        std::string helpMessage ();
        //! Check if string ends with substring.
        /*!
          This function takes in two strings and determines whether one string contains the other string as a substring at the end.
          \param full_string full string to check
          \param ending substring to check
          \return Boolean with answer
        */
        bool endsWith (std::string const &full_string, std::string const &ending);

        // Getter Functions
        // \sa is for "see also"

        //! Operator which allows class to be used like a map data structure.
        /*!
          This function a command line option corresponding to the provided string.
          \param str command line option
          \return Value of command line option
          \sa CmdLineArguments()
        */
        std::string operator[](std::string const &str) const;
        //! Return string with complete command line given.
        /*!
          Return the complete command line used by user.
          \return String with all command line options given
        */
        const std::string& commandLine() const;
        //! Return the name of the command given.
        /*!
          This returns the value of the command typed on the command line, should be the name of the compiled code where main resides (i.e. argv[0]).
          \return Name of command
        */
        const std::string& commandName() const;
        //! Return the value of the verbose flag.
        /*!
          If the user specifies verbose this function will return true, otherwise it will return false. This is a getter method.
          \return Boolean indicating whether the verbose option is used
        */
        bool verbose() const;
        //! Return the value of the help flag.
        /*!
          If the user requests the help menu, a flag is set, this function returns the value of that flag. This is a getter method.
          \return Boolean indicating whether the help option is used
        */
        bool help() const;

    private:
        std::map<std::string, std::string> command_line_args;
        std::string command_line;
        std::string unexpected_args;
        std::string command_name;

        bool help_command;
        bool verbose_output;

};
#endif // __CMD_LINE_ARGUMENTS_H_INCLUDED__
