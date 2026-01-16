#ifndef COMMAND_COMPLETION_HPP
#define COMMAND_COMPLETION_HPP

#ifndef _WIN32
char** command_completion(const char* text, int start, int end);
#endif

#endif // COMMAND_COMPLETION_HPP
