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

    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];

        // NEW: tokenize stdout redirection operators when not inside quotes. [web:60]
        if (!in_single_quote && !in_double_quote) {
            if (c == '1' && i + 1 < input.size() && input[i + 1] == '>') {
                if (!current.empty()) { args.push_back(current); current.clear(); }
                args.push_back("1>");
                i++; // consume '>'
                continue;
            }
            if(c=='2'&&i+1<input.size() && input[i+1]){
                if(!current.empty()){
                    args.push_back(current);
                    current.clear();
                }
                args.push_back("2>");
                i++;
                continue;
            }
            if (c == '>') {
                if (!current.empty()) { args.push_back(current); current.clear(); }
                args.push_back(">");
                continue;
            }
        }

        /* Backslash handling (kept) */
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
        }

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
    }

    if (!current.empty()) args.push_back(current);
    return args;
}




vector<char*> to_char_array(vector<string> &args) {
    vector<char*> result;
    for (auto &arg : args) result.push_back(const_cast<char*>(arg.c_str()));
    result.push_back(nullptr);
    return result;
}


// Needed so echo (builtin) can redirect stdout in the shell process. [web:21]
struct StdoutRedirect {
    int saved = -1;
    bool active = false;

    StdoutRedirect(bool enable, const string& outfile) {
        if (!enable) return;

        saved = dup(STDOUT_FILENO);
        if (saved < 0) { perror("dup"); return; }

        int fd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) { perror("open"); close(saved); saved = -1; return; }

        if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); close(fd); close(saved); saved = -1; return; }
        close(fd);
        active = true;
    }

    ~StdoutRedirect() {
        if (!active) return;
        dup2(saved, STDOUT_FILENO);
        close(saved);
    }
};

// External redirection: open + dup2 before execvp in child. [web:22]
void run_external(vector<string> &args, bool redirect_out, const string &outfile,bool redirect_err,const string &errfile) {
    
    
    pid_t pid = fork();
    if (pid == 0) {
        if (redirect_out) {
            int fd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); _exit(1); }
            if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); _exit(1); }
            close(fd);
        }
        if(redirect_err){
            int fd = open(outfile.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0644);
            if(fd<0){
                perror("open");
                _exit(1);
            }
            dup2(fd,STDERR_FILENO);
            close(fd);
        }

        vector<char*> c_args = to_char_array(args);
        execvp(c_args[0], c_args.data());
       cerr << args[0] << ": command not found\n";
        _exit(127);
    } else if (pid > 0) {
        waitpid(pid, nullptr, 0);
    } else {
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

        vector<string> args = split_args(input);

        // Parse and remove > / 1> so execvp doesn't receive them. [web:60]
        bool redirect_out= false;
       bool redirect_err=false;
        string outfile;
        string errfile;

        for (size_t i = 0; i < args.size(); ) {
            if (args[i] == ">" || args[i] == "1>") {
                if (i + 1 >= args.size()) break;
                redirect_out = true;
                outfile = args[i + 1];
                args.erase(args.begin() + i, args.begin() + i + 2);
                continue;
            }
            if(args[i]=="2>"){
                redirect_err=true;
                errfile=args[i+1];
                args.erase(args.begin()+i,args.begin()+i+2);
                continue;
            }
            i++;
        }

        if (args.empty()) continue;

        /* exit builtin */
        if (args[0] == "exit") return 0;

        /* echo builtin (ONLY this echo; remove input.rfind echo) */
        if (args[0] == "echo") {
            StdoutRedirect r(redirect_out, outfile);
            for (size_t i = 1; i < args.size(); i++) {
                cout << args[i];
                if (i + 1 < args.size()) cout << ' ';
            }
            cout << '\n';
            continue;
        }

        /* pwd builtin */
        if (args[0] == "pwd") {
            cout << filesystem::current_path().string() << '\n';
            continue;
        }

        /* cd builtin */
        if (args[0] == "cd") {
            string path = (args.size() >= 2 ? args[1] : "");
            if (path == "~") {
                const char* home = std::getenv("HOME");
                if (home) filesystem::current_path(home);
                continue;
            }

            try {
                filesystem::current_path(path);
            } catch (const filesystem::filesystem_error&) {
                cout << "cd: " << path << ": No such file or directory\n";
            }
            continue;
        }

        /* type builtin */
        if (args[0] == "type") {
            string cmd = (args.size() >= 2 ? args[1] : "");

            if (cmd == "exit" || cmd == "echo" || cmd == "type" || cmd == "pwd" || cmd == "cd") {
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

            if (!found) cout << cmd << ": not found\n";
            continue;
        }

        /* external command */
        run_external(args, redirect_out, outfile,redirect_err,errfile);
    }
}

