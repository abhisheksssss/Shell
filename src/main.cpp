#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #define pid_t intptr_t
#else
    #include <sys/wait.h>
    // POSIX code
#endif
#include <filesystem>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#ifndef _WIN32
    #include <readline/readline.h>
    #include <readline/history.h>
#endif
#include<cstring>
#include<dirent.h>
#include <iomanip> 

#include "utils/isNumeric.hpp"
#include "utils/split_path.hpp"
#include "utils/is_executable.hpp"
#include "utils/split_args.hpp"
#include "utils/to_char_array.hpp"
#include "utils/StdoutRedirect.hpp"
#include "utils/run_external.hpp"
#include "utils/builtins.hpp"
#ifndef _WIN32
    #include "utils/command_generator.hpp"
    #include "utils/command_completion.hpp"
#endif

using namespace std;

int main()
{
    cout << unitbuf;
    cerr << unitbuf;
    
#ifndef _WIN32
    rl_attempted_completion_function = command_completion;
#endif

     string input;
     vector<string>history;

    while (true)
    {
#ifdef _WIN32
        cout << "$ ";
        if (!getline(cin, input)) {
            break;
        }
        if (!input.empty()) {
            history.push_back(input);
        }
#else
        char* input_buffer = readline("$ ");
        if (!input_buffer) {
            break;
        }
        input = string(input_buffer);
        free(input_buffer);
        if (!input.empty()) {
            add_history(input.c_str());
            history.push_back(input);
        }
#endif

        if (input.empty())
            continue;
// ...
              

        if (input.empty())
            continue;

        vector<string> args = split_args(input);

        // Parse and remove > / 1> so execvp doesn't receive them. [web:60]
        bool redirect_out = false;
        bool redirect_err = false;
        bool append = false;
        bool err_append=false;
        bool has_pipe=false;
        size_t pipe_index=0; // Keeping variable even if unused for now

          
        if (!args.empty() && args[0] == "history") {
            // Print history with proper formatting
  
            if(args.size() > 1 && isNumeric(args[1])){
                int n = stoi(args[1]);
                int start_idx = (int)history.size() - n;
                if (start_idx < 0) start_idx = 0;
                for(size_t i=(size_t)start_idx; i<history.size(); i++){
                    cout << "    " << setw(4) << right << (i + 1) << "  " << history[i] << endl;
                }
                cout << flush;
                continue;
            }
      
            for(size_t i = 0; i < history.size(); i++) {
                // Format: 4 spaces, 4-digit right-aligned index, 2 spaces, command
                cout << "    " << setw(4) << right << (i + 1) << "  " << history[i] << endl;
            }
            // CRITICAL: Flush output and continue without printing prompt
            cout << flush;
            continue;
        }


        string outfile;
        string errfile;

        for (size_t i = 0; i < args.size();)
        {
            if(args[i]=="|"){
                    has_pipe=true;
                    pipe_index=i;
                    break;
            }  

            if (args[i] == ">" || args[i] == "1>")
            {
                if (i + 1 >= args.size())
                    break;
                redirect_out = true;
                append = false;
                outfile = args[i + 1];
                args.erase(args.begin() + i, args.begin() + i + 2);
                continue;
            }
            if (args[i] == "2>")
            {
                redirect_err = true;
                errfile = args[i + 1];
                args.erase(args.begin() + i, args.begin() + i + 2);
                continue;
            }
            if (args[i] == ">>" || args[i] == "1>>")
            {
                redirect_out = true;
                append = true;
                outfile = args[i + 1];
                args.erase(args.begin() + i, args.begin() + i + 2);
                continue;
            }
            if (args[i] == "2>>")
            {
                 redirect_err = true;
        err_append = true;
        errfile = args[i + 1];
        args.erase(args.begin() + i, args.begin() + i + 2);
        continue;
            }
            i++;
        }






#ifndef _WIN32
   if(has_pipe) {
    // Split command into stages
    vector<vector<string>> commands;
    vector<string> current_cmd;
    
    for(size_t i = 0; i < args.size(); i++) {
        if(args[i] == "|") {
            if(!current_cmd.empty()) {
                commands.push_back(current_cmd);
                current_cmd.clear();
            }
        } else {
            current_cmd.push_back(args[i]);
        }
    }

    if(!current_cmd.empty()) {
        commands.push_back(current_cmd);
    }
    
    int num_commands = commands.size();
    int num_pipes = num_commands - 1;
    
    // Create all pipes
    int pipes[num_pipes][2];
    for(int i = 0; i < num_pipes; i++) {
        if(pipe(pipes[i]) < 0) {
            perror("pipe");
            continue;
        }
    }
    
    // Fork and execute each command
    vector<pid_t> pids;
    for(int i = 0; i < num_commands; i++) {
        pid_t pid = fork();
        
        if(pid == 0) {  // Child process
            // Redirect stdin from previous pipe (except first command)
            if(i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Redirect stdout to next pipe (except last command)
            if(i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe file descriptors
            for(int j = 0; j < num_pipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            if(commands[i][0] == "echo") {
                for(size_t k = 1; k < commands[i].size(); k++) {
                    cout << commands[i][k];
                    if(k + 1 < commands[i].size()) cout << " ";
                }
                cout << "\n";
                _exit(0);
            }
            else if(commands[i][0] == "pwd") {
                cout << filesystem::current_path().string() << "\n";
                _exit(0);
            }
            else if(commands[i][0] == "type") {
                string cmd = (commands[i].size() >= 2 ? commands[i][1] : "");
                
                if(cmd == "exit" || cmd == "echo" || cmd == "type" || 
                   cmd == "pwd" || cmd == "cd"|| cmd=="history") {
                    cout << cmd << " is a shell builtin\n";
                } else {
                    char* path_env = getenv("PATH");
                    bool found = false;
                    if(path_env) {
                        vector<string> paths = split_path(path_env);
                        for(const string& dir : paths) {
                            string full_path = dir + "/" + cmd;
                            if(is_executable(full_path)) {
                                cout << cmd << " is " << full_path << "\n";
                                found = true;
                                break;
                            }
                        }
                    }
                    if(!found) {
                        cout << cmd << ": not found\n";
                    }
                }
                _exit(0);
            }

            // Execute command (handle builtins if needed)
            vector<char*> cargs = to_char_array(commands[i]);
            execvp(cargs[0], cargs.data());
            cerr << commands[i][0] << ": command not found\n";
            _exit(127);
        } else if(pid > 0) {
            pids.push_back(pid);
        }
    }
    
    // Parent: close all pipes
    for(int i = 0; i < num_pipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }


    
    
    // Wait for all children
    for(pid_t pid : pids) {
        waitpid(pid, nullptr, 0);
    }
    
    continue;

   }
#else
    if(has_pipe) {
        cout << "Pipes not supported on Windows yet\n";
        continue;
    }
#endif

        if (args.empty())
            continue;

        /* exit builtin */
        if (args[0] == "exit")
            return 0;

        /* echo builtin (ONLY this echo; remove input.rfind echo) */
        if (args[0] == "echo")
        {

           
                // NEW: ensure 2> file exists for builtin

                if (redirect_err)
                {
                    int flags = O_WRONLY | O_CREAT | (err_append ? O_APPEND : O_TRUNC);
    int fd = open(errfile.c_str(), flags, 0644);
    if (fd >= 0) close(fd);
                }

                StdoutRedirect r(redirect_out, outfile, append);
                for (size_t i = 1; i < args.size(); i++)
                {
                    cout << args[i];
                    if (i + 1 < args.size())
                        cout << ' ';
                }
                cout << '\n';
                continue;
            
        }

        /* pwd builtin */
        if (args[0] == "pwd")
        {
            cout << filesystem::current_path().string() << '\n';
            continue;
        }

        /* cd builtin */
        if (args[0] == "cd")
        {
            string path = (args.size() >= 2 ? args[1] : "");
            if (path == "~")
            {
                const char *home = std::getenv("HOME");
                if (home)
                    filesystem::current_path(home);
                continue;
            }

            try
            {
                filesystem::current_path(path);
            }
            catch (const filesystem::filesystem_error &)
            {
                cout << "cd: " << path << ": No such file or directory\n";
            }
            continue;
        }

        /* type builtin */
        if (args[0] == "type")
        {
            string cmd = (args.size() >= 2 ? args[1] : "");

            if (cmd == "exit" || cmd == "echo" || cmd == "type" || cmd == "pwd" || cmd == "cd"||cmd=="history")
            {
                cout << cmd << " is a shell builtin\n";
                continue;
            }

            char *path_env = getenv("PATH");
            bool found = false;

            if (path_env)
            {
                vector<string> paths = split_path(path_env);
                for (const string &dir : paths)
                {
                    string full_path = dir + "/" + cmd;
                    if (is_executable(full_path))
                    {
                        cout << cmd << " is " << full_path << '\n';
                        found = true;
                        break;
                    }
                }
            }

        
        if(found){
            history.push_back(input);
        }else{
            string his="invalid command";
            history.push_back(his);
        }

            if (!found)
                cout << cmd << ": not found\n";
            continue;
        }

        /* external command */
        run_external(args, redirect_out, outfile, append, redirect_err, errfile,err_append);
    }
}
