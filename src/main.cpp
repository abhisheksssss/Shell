#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <fcntl.h>



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
    bool redirect=false;
    string outfile;



    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
      

        /* Backslash handling */
        if (c == '\\') {

            if (!in_single_quote && !in_double_quote) {
                if (i + 1 < input.size()) {
                    current += input[i + 1];
                    i++;
                }
                continue;
            }

            if (in_double_quote) {
                if (i + 1 < input.size()) {
                    char next = input[i + 1];
                    if (next == '"' || next == '\\') {
                        current += next;
                        i++;
                    } else {
                        current += '\\';
                    }
                }
                continue;
            }

            current += '\\';
            continue;
        }  // ← closes: if (c == '\\')

        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            continue;
        }

        if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            continue;
        }

        if ((c == ' ' || c == '\t') && !in_single_quote && !in_double_quote) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }

    } // ← closes: for loop

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


void run_external(vector<string> &args,bool redirect , const string &outfile){
    pid_t pid=fork();

      if (pid == 0) {
        if (redirect) {
            int fd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); _exit(1); }

            if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); _exit(1); }
            close(fd);
        }

        vector<char*>c_args=to_char_array(args);
        execvp(c_args[0],c_args.data());
        perror("execvp");
        _exit(127);
 }else if(pid>0){
    waitpid(pid,nullptr,0);
 }else{
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

bool redirect = false;
string outfile;

for (size_t i = 0; i < args.size(); ) {
    if (args[i] == ">" || args[i] == "1>") {
        if (i + 1 >= args.size()) {
            // optional: print syntax error; for now just stop
            break;
        }
        redirect = true;
        outfile = args[i + 1];
        args.erase(args.begin() + i, args.begin() + i + 2); // remove op + filename
        continue;
    }
    i++;
}

if (!args.empty()) {
    run_external(args, redirect, outfile);
}

    }
}
