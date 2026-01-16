#include "StdoutRedirect.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

#ifdef _WIN32
    #include <io.h>
    #define dup _dup
    #define dup2 _dup2
    #define close _close
    #define STDOUT_FILENO 1
    #define STDERR_FILENO 2
    #define O_WRONLY _O_WRONLY
    #define O_CREAT _O_CREAT
    #define O_APPEND _O_APPEND
    #define O_TRUNC _O_TRUNC
#endif

StdoutRedirect::StdoutRedirect(bool redirect_out, const std::string& outfile, bool append, 
               bool redirect_err, const std::string& errfile, bool err_append) 
    : original_stdout(-1), original_stderr(-1), redirecting_out(false), redirecting_err(false) {
    
    if (redirect_out) {
        original_stdout = dup(STDOUT_FILENO);
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        int fd = open(outfile.c_str(), flags, 0644);
        if (fd >= 0) {
            dup2(fd, STDOUT_FILENO);
            close(fd);
            redirecting_out = true;
        }
    }

    if (redirect_err) {
        original_stderr = dup(STDERR_FILENO);
        int flags = O_WRONLY | O_CREAT | (err_append ? O_APPEND : O_TRUNC);
        int fd = open(errfile.c_str(), flags, 0644);
        if (fd >= 0) {
            dup2(fd, STDERR_FILENO);
            close(fd);
            redirecting_err = true;
        }
    }
}

StdoutRedirect::~StdoutRedirect() {
    if (redirecting_out && original_stdout != -1) {
        dup2(original_stdout, STDOUT_FILENO);
        close(original_stdout);
    }
    if (redirecting_err && original_stderr != -1) {
        dup2(original_stderr, STDERR_FILENO);
        close(original_stderr);
    }
}
