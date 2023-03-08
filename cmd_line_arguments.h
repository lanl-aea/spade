//cmd_line_arguments.h

#ifndef __CMD_LINE_ARGUMENTS_H_INCLUDED__
#define __CMD_LINE_ARGUMENTS_H_INCLUDED__

class CmdLineArguments {
    public:
        CmdLineArguments (int &argc, char **argv);
        std::string verboseArguments ();
        std::string helpMessage ();
        bool endsWith (std::string const &full_string, std::string const &ending);

        // Getter
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
