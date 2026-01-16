#include "run_external.hpp"
#include "to_char_array.hpp"
#include "StdoutRedirect.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #define pid_t intptr_t
#else
    #include <sys/wait.h>
#endif

void run_external(std::vector<std::string>& args, bool redirect_out, const std::string& outfile, bool append, bool redirect_err, const std::string& errfile, bool err_append) {
#ifdef _WIN32
    std::string command_line;
    for(const auto& arg : args) command_line += arg + " ";
    
    // Simple execution for Windows
    intptr_t ret = _spawnvp(_P_WAIT, args[0].c_str(), to_char_array(args).data());
#else
    pid_t pid = fork();
    if (pid == 0) {
        // Child
        StdoutRedirect r(redirect_out, outfile, append, redirect_err, errfile, err_append);
        std::vector<char*> c_args = to_char_array(args); // Create a non-const copy for execvp
        execvp(c_args[0], c_args.data());
        std::cout << c_args[0] << ": command not found" << std::endl;
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
    }
#endif
}
