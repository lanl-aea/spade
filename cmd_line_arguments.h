//cmd_line_arguments.h

#ifndef __CMD_LINE_ARGUMENTS_H_INCLUDED__
#define __CMD_LINE_ARGUMENTS_H_INCLUDED__

class CmdLineArguments {
    public:
        CmdLineArguments (int &argc, char **argv);
        std::string printCmdLineArguments ();
        bool endsWith (std::string const &full_string, std::string const &ending);

        // Getter
        std::string operator[](std::string const &str) const;
        const std::string& cmdLine() const;
        bool verbose() const;

    private:
        std::map<std::string, std::string> command_line_args;
        std::string full_line;
        std::string unexpected_args;

        bool help;
        bool verboseoutput;
        bool exit;

};
#endif // __CMD_LINE_ARGUMENTS_H_INCLUDED__
