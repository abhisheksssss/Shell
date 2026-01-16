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

using namespace std;

const char* builtin_command[] = {"exit", "echo", "type", "pwd", "cd", "history", nullptr};

bool isNumeric(const string& str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!isdigit(c)) return false;
    }
    return true;
}

vector<string> split_path(const string& env) {
    vector<string> paths;
    stringstream ss(env);
    string item;
#ifdef _WIN32
    char delimiter = ';';
#else
    char delimiter = ':';
#endif
    while (getline(ss, item, delimiter)) {
        if (!item.empty()) {
            paths.push_back(item);
        }
    }
    return paths;
}

bool is_executable(const string& path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES);
#else
    return access(path.c_str(), X_OK) == 0;
#endif
}

vector<string> split_args(const string& input) {
    vector<string> args;
    string current;
    bool in_quotes = false;
    char quote_char = 0;
    
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        if (c == '\\' && i + 1 < input.length() && !in_quotes) { // Handle escaped characters outside quotes
            current += input[++i];
        } 
        else if ((c == '\'' || c == '"')) {
            if (!in_quotes) {
                in_quotes = true;
                quote_char = c;
            } else if (c == quote_char) {
                if (i + 1 < input.length() && (input[i+1] == '\'' || input[i+1] == '"')) {
                    // quote concatenation like "foo""bar" -> foobar
                } else {
                    in_quotes = false;
                }
            } else {
                current += c;
            }
        } else if (c == ' ' && !in_quotes) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
             if (in_quotes && c == '\\' && (i+1 < input.length()) && (input[i+1] == quote_char || input[i+1] == '\\')) {
                current += input[++i];
             } else {
                current += c;
             }
        }
    }
    if (!current.empty()) args.push_back(current);
    return args;
}

vector<char*> to_char_array(const vector<string>& args) {
    vector<char*> result;
    for (const auto& s : args) {
        result.push_back(const_cast<char*>(s.c_str()));
    }
    result.push_back(nullptr);
    return result;
}

class StdoutRedirect {
    int original_stdout;
    int original_stderr;
    bool redirecting_out;
    bool redirecting_err;

public:
    StdoutRedirect(bool redirect_out, const string& outfile, bool append, 
                   bool redirect_err = false, const string& errfile = "", bool err_append = false) 
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

    ~StdoutRedirect() {
        if (redirecting_out && original_stdout != -1) {
            dup2(original_stdout, STDOUT_FILENO);
            close(original_stdout);
        }
        if (redirecting_err && original_stderr != -1) {
            dup2(original_stderr, STDERR_FILENO);
            close(original_stderr);
        }
    }
};

void run_external(vector<string>& args, bool redirect_out, const string& outfile, bool append, bool redirect_err, const string& errfile, bool err_append) {
#ifdef _WIN32
    string command_line;
    for(const auto& arg : args) command_line += arg + " ";
    
    // Simple execution for Windows
    intptr_t ret = _spawnvp(_P_WAIT, args[0].c_str(), to_char_array(args).data());
#else
    pid_t pid = fork();
    if (pid == 0) {
        // Child
        StdoutRedirect r(redirect_out, outfile, append, redirect_err, errfile, err_append);
        vector<char*> c_args = to_char_array(args); // Create a non-const copy for execvp
        execvp(c_args[0], c_args.data());
        cout << c_args[0] << ": command not found" << endl;
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
    }
#endif
}

#ifndef _WIN32
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
        if (path_env) {
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

char** command_completion(const char* text, int start, int end){
    rl_attempted_completion_over = 1; 
    
    if(start == 0){
        return rl_completion_matches(text, command_generator);
    }
    
    return nullptr;
}
#endif

// ... (omitted)

int main()
{
    cout << unitbuf;
    cerr << unitbuf;
    
#ifndef _WIN32
    rl_attempted_completion_function = command_completion;
#endif

     string input;
     vector<string>history;
    int savedtillTheIndex=0;
    char* histfile=getenv("HISTFILE");
        if(histfile && filesystem::exists(histfile)){
            ifstream file(histfile);
            string line;

            while(getline(file,line)){
                if(!line.empty()){
                    history.push_back(line);
        #ifdef _WIN32
                   add_history(line.c_str());
        #endif
                }
            }
            file.close();
        }
       
    
        
 
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

            if(args.size() > 1 && args[1]=="-r"){
              const string history_file= args[2];

              if(filesystem::exists(history_file)){
                ifstream file(history_file);
                string line;

                while(getline(file,line)){
                    if(!line.empty()){
                        history.push_back(line);

            #ifndef _WIN32
                      add_history(line.c_str());
            #endif
                    }
                }
                file.close();
              }else{
                cout<<"history: "<<history_file<<": No such file or directory\n";
              }
              continue;
            }

                if(args.size() > 1 && args[1]=="-w"){
                    const string history_file = args[2];
                        ofstream file(history_file); // create / overwrite
                            if (!file.is_open()) {
                            cout << "history: cannot write to file\n";
                            continue;
                            }
                            for (const string& cmd : history) {
                            file << cmd << '\n';
                            }
                            savedtillTheIndex=history.size();
                    file.close();
                    continue;
                }

                    if (args.size() > 1 && args[1] == "-a") {
                    const string history_file = args[2];

                    ofstream file(history_file, ios::app);
                    if (!file.is_open()) {
                        cout << "history: cannot write to file\n";
                        continue;
            }



    // Append only NEW commands
    for (size_t i = savedtillTheIndex; i < history.size(); i++) {
        file << history[i] << '\n';
    }

    // Update checkpoint
    savedtillTheIndex = history.size();

    file.close();
    continue;
}


  
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
        if (args[0] == "exit"){
          char* histfile=getenv("HISTFILE");
          if(histfile){
            ofstream file(histfile);
            if(file.is_open()){
              for(const string& cmd:history){
                file<<cmd<<'\n';
              }
              file<<'\n';
              file.close();
            }
          }
          return 0;
        }

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
