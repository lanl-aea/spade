//! An object for handling the logging

#ifndef __LOGGING_H_INCLUDED__
#define __LOGGING_H_INCLUDED__

using namespace std;

/*!
   This class handles the log file. The constructor creates the log file, the destructor closes it, and other functions write to it.
*/
class Logging {
    public:
        //! The constructor.
        /*!
          The constructor opens the log file using the provided log file name, and writes an initial message to it. If the log file can't be opened stdout will be used for messages.
          \param log_file_name string containing the name of the log file
          \param log_verbose boolean saying whether verbose logging is turned on
        */
        Logging (string const &log_file_name, bool const &log_verbose, bool const &log_debug);
        //! The destructor.
        /*!
          The destructor prints the time to the end of the log file and closes it.
        */
        ~Logging ();
        //! The log function.
        /*!
          This function logs the provided message.
          \param output string containing the log message
        */
        void log (string const &output);
        //! Function for logging verbose messages
        /*!
          If the verbose option is used, then the message provided will be logged.
          \param output string containing the log message
        */
        void logVerbose (string const &output);
        //! Function for logging a warning message
        /*!
          If something unextpected occurs, but execution may continue, the word 'WARNING' will be prepended to the log message
          \param output string containing the log message
        */
        void logWarning (string const &output);
        // logDebug is for debugging and therefore there is no API documentation for it
        void logDebug (string const &output);
        //! Function for logging an error message and exiting the code.
        /*!
          If a fatal error is reached, i.e. code execution cannot successfully continue, this function will be used to log a message and exit the execution of the program.
          \param output string containing the log message
        */
        void logErrorAndExit (string const &output);

    private:
        bool log_verbose;
        bool log_debug;
        std::ofstream log_file;
        std::ostream* output_stream;
};
#endif  // __LOGGING_H_INCLUDED__
