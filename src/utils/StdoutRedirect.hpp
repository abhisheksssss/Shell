#ifndef STDOUTREDIRECT_HPP
#define STDOUTREDIRECT_HPP

#include <string>

class StdoutRedirect {
    int original_stdout;
    int original_stderr;
    bool redirecting_out;
    bool redirecting_err;

public:
    StdoutRedirect(bool redirect_out, const std::string& outfile, bool append, 
                   bool redirect_err = false, const std::string& errfile = "", bool err_append = false);
    ~StdoutRedirect();
};

#endif // STDOUTREDIRECT_HPP
