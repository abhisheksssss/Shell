#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
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

vector<string> split_args(const string &input){
vector<string> args:
stringstream ss(input);
string token;
while(ss>token){
    args.push_back(token);
}
return args;
}


vector<char*> to_char_array(vector<string>&args){
    vector<char*> result;

    for(auto &arg:args){
        result.push_back(&args[0]);
    }
    result.push_back(nullptr);
    return result;
}
void run_external(vector<string> &args){
    pid_t=fork();
    if(pid==0){
        vector<char*> c_args=to_char_array(args);
          execvp(c_args[0], c_args.data());
        perror("execvp");
        exit(1);
    }else if(pid>0){
        wait(nullptr)
    }else{
        perror("fork")
    }
}
int main() {
    cout << unitbuf;
    cerr << unitbuf;

    while (true) {
        cout << "$ ";
        string input;
        getline(cin, input);

        if (input == "exit") {
            return 0;
        }

        // echo builtin
        if (input.rfind("echo ", 0) == 0) {
            cout << input.substr(5) << '\n';
            continue;
        }

        // type builtin
        if (input.rfind("type ", 0) == 0) {
            string cmd = input.substr(5);

            // Builtins
            if (cmd == "exit" || cmd == "echo" || cmd == "type") {
                cout << cmd << " is a shell builtin\n";
                continue;
            }

            // Search in PATH
            char *path_env = getenv("PATH");
            if (path_env) {
                vector<string> paths = split_path(path_env);
                bool found = false;

                for (const string &dir : paths) {
                    string full_path = dir + "/" + cmd;
                    if (is_executable(full_path)) {
                        cout << cmd << " is " << full_path << '\n';
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    cout << cmd << ": not found\n";
                }
            }else {

                cout << cmd << ": not found\n";
            }
            continue;
        }

        


        // Default case
       vector<string> args = split_args(input);
if (!args.empty()) {
    run_external(args);
}
    }
}

