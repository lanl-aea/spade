//! A class for holding command line arguments

#ifndef __CMD_LINE_ARGUMENTS_H_INCLUDED__
#define __CMD_LINE_ARGUMENTS_H_INCLUDED__

/*!
   This class collects and verifies command line options and creates the help menu
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
          This function returns a string listing the values of all the command line options
          \return The string of command line options
        */
        std::string verboseArguments ();
        //! String with help message.
        /*!
          This function returns a POSIX-like string with the help message for the user
          \sa CmdLineArguments()
          \return The help message
        */
        std::string helpMessage ();
        //! Check if string ends with substring
        /*!
          This function takes in string and a potential substring and says whether the string ends with the provided substring
          \param full_string full string to check
          \param ending substring to check
          \return Boolean with answer
        */
        bool endsWith (std::string const &full_string, std::string const &ending);

        // Getter Functions

        //! Operator which allows clas to be used like a map (dictionary)
        /*!
          This function a command line option corresponding to the provided string
          \param str command line option
          \return Value of command line option
        */
        std::string operator[](std::string const &str) const;
        const std::string& commandLine() const;
        const std::string& commandName() const;
        bool verbose() const;
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
