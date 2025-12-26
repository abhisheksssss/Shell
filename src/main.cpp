#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <filesystem>
#include <sstream>

using namespace std;


bool is_executable(const string &path) {
    return access(path.c_str(), X_OK) == 0;
}

vector<string> split_path(const string &path) {
    vector<string> dirs;
    stringstream ss(path);
    string item;
    while (getline(ss, item, ':')) {
        dirs.push_back(item);
    }
    return dirs;
}

vector<string> split_args(const string &input) {
    vector<string> args;
    string current;
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (char c : input) {

        if(c=='\\' && !in_single_quote || !in_double_quote ){
            if(i+1<input.size()){
                current +=input[i+1];
                i++;
            }
            continue;
        }
        
        // Toggle single quote (only if not inside double quote)
        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            continue;
        }

        // Toggle double quote (only if not inside single quote)
        if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            continue;
        }

        // Whitespace splits args only outside BOTH quotes
        if ((c == ' ' || c == '\t') && !in_single_quote && !in_double_quote) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        }
        else {
            current += c;
        }
    }

    if (!current.empty()) {
        args.push_back(current);
    }

    return args;
}


vector<char*> to_char_array(vector<string> &args) {
    vector<char*> result;
    for (auto &arg : args) {
        result.push_back(const_cast<char*>(arg.c_str()));
    }
    result.push_back(nullptr);
    return result;
}


void run_external(vector<string> &args) {
    pid_t pid = fork();

    if (pid == 0) {
        vector<char*> c_args = to_char_array(args);
        execvp(c_args[0], c_args.data());

        cerr << args[0] <<": command not found\n";
        exit(127);
    } 
    else if (pid > 0) {
        wait(nullptr);
    } 
    else {
        perror("fork");
    }
}


int main() {
    cout << unitbuf;
    cerr << unitbuf;

    while (true) {
        cout << "$ ";
        string input;
        getline(cin, input);

        if (input.empty()) continue;

        /* exit builtin */
        if (input == "exit") {
            return 0;
        }

        /* echo builtin */
      if (input.rfind("echo ", 0) == 0) {
    vector<string> args = split_args(input.substr(5));

    for (size_t i = 0; i < args.size(); i++) {
        cout << args[i];
        if (i < args.size() - 1) cout << ' ';
    }
    cout << '\n';
    continue;
}
        

if(input=="pwd"){
          cout << filesystem::current_path().string() << '\n';
             continue;
               }

        if(input.rfind("cd ",0)==0){
            string path=input.substr(3);
        
            if(path=="~"){
                 const char* home = std::getenv("HOME");
                filesystem::current_path(home);                 
                 continue;
            }

               

            try{
                filesystem::current_path(path);
            }catch(const filesystem::filesystem_error& e){
                cout<<"cd: "<<path<<": No such file or directory"<<"\n";
            }
            continue;
        }

        /* type builtin */
        if (input.rfind("type ", 0) == 0) {
            string cmd = input.substr(5);

            if (cmd == "exit" || cmd == "echo" || cmd == "type"||cmd=="pwd") {
                cout << cmd << " is a shell builtin\n";
                continue;
            }

            char *path_env = getenv("PATH");
            bool found = false;

            if (path_env) {
                vector<string> paths = split_path(path_env);
                for (const string &dir : paths) {
                    string full_path = dir + "/" + cmd;
                    if (is_executable(full_path)) {
                        cout << cmd << " is " << full_path << '\n';
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                cout << cmd << ": not found\n";
            }
            continue;
        }

        /* external command */
        vector<string> args = split_args(input);
        if (!args.empty()) {
            run_external(args);
        }
    }
}
