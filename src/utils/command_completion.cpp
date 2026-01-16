#include "command_completion.hpp"

#ifndef _WIN32
#include "command_generator.hpp"
#include <readline/readline.h>

char** command_completion(const char* text, int start, int end){
    rl_attempted_completion_over = 1; 
    
    if(start == 0){
        return rl_completion_matches(text, command_generator);
    }
    
    return nullptr;
}
#endif
