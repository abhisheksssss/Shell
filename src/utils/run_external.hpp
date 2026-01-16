#ifndef RUN_EXTERNAL_HPP
#define RUN_EXTERNAL_HPP

#include <vector>
#include <string>

void run_external(std::vector<std::string>& args, bool redirect_out, const std::string& outfile, bool append, bool redirect_err, const std::string& errfile, bool err_append);

#endif // RUN_EXTERNAL_HPP
