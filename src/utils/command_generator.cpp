#include "command_generator.hpp"

#ifndef _WIN32
#include "split_path.hpp"
#include "is_executable.hpp"
#include "builtins.hpp"
#include <vector>
#include <string>
#include <cstring>
#include <dirent.h>
#include <cstdlib>

using namespace std;

char* command_generator(const char* text, int state) {
    static int list_index, len;
    static vector<string> path_dirs;
    static size_t path_index;
    static DIR* dir_handle;
    static string current_dir;
    const char* name;

    // First call: initialize
    if (!state) {
        list_index = 0;
        len = strlen(text);
        path_index = 0;
        dir_handle = nullptr;
        current_dir.clear();
        
        // Get PATH directories
        path_dirs.clear();
        char* path_env = getenv("PATH");
        if (path_dirs.empty() && path_env) {
            path_dirs = split_path(path_env);
        }
    }

    // First, check builtin commands
    while ((name = builtin_command[list_index])) {
        list_index++;
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    // Then search through PATH directories for executables
    while (path_index < path_dirs.size()) {
        // Open the next directory if needed
        if (dir_handle == nullptr) {
            current_dir = path_dirs[path_index];
            path_index++;
            
            // Skip non-existent directories
            dir_handle = opendir(current_dir.c_str());
            if (dir_handle == nullptr) {
                continue;
            }
        }

        // Read entries from current directory
        struct dirent* entry;
        while ((entry = readdir(dir_handle)) != nullptr) {
            // Skip hidden files and directories
            if (entry->d_name[0] == '.') {
                continue;
            }

            // Check if name matches prefix
            if (strncmp(entry->d_name, text, len) == 0) {
                // Check if file is executable
                string full_path = current_dir + "/" + entry->d_name;
                if (is_executable(full_path)) {
                    // Return this match (keep dir_handle open for next call)
                    return strdup(entry->d_name);
                }
            }
        }

        // Finished with this directory
        closedir(dir_handle);
        dir_handle = nullptr;
    }

    return nullptr;
}
#endif
