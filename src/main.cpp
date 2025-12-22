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
    stringstream ss(input);
    string token;
    while (ss >> token) {
        args.push_back(token);
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

        cerr << args[0] << ": command not found\n";
        exit(127);
    } 
    else if (pid > 0) {
        wait(nullptr);
    } 
    else {
        perror("fork");
    }
}


string removeQuotes(const string&s){
  if (s.size() >= 2) {
        if ((s.front() == '"'  && s.back() == '"') ||
            (s.front() == '\'' && s.back() == '\'')) {
            return s.substr(1, s.size() - 2);
        }
    }
    return s;
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
             
       string msg = input.substr(5);

if (msg.front()=='\'' && msg.back()=="\'") {
    cout << removeQuotes(msg) << '\n';
    continue;
}

            cout << input.substr(5) << '\n';
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
