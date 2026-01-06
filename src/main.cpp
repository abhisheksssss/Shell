#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/wait.h>
#endif
#include <filesystem>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <cstring>
#include <dirent.h>

using namespace std;

bool is_executable(const string &path);
vector<string> split_path(const string &path);

bool is_executable(const string &path)
{
    return access(path.c_str(), X_OK) == 0;
}

vector<string> split_path(const string &path)
{
    vector<string> dirs;
    stringstream ss(path);
    string item;
    while (getline(ss, item, ':'))
    {
        dirs.push_back(item);
    }
    return dirs;
}

vector<string> split_args(const string &input)
{
    vector<string> args;
    string current;
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (size_t i = 0; i < input.size(); i++)
    {
        char c = input[i];
        if (!in_single_quote && !in_double_quote)
        {
            //1>>
            if (c == '1' && i + 2 < input.size() && input[i + 1] == '>' && input[i + 2] == '>')
            {
                if (!current.empty())
                {
                    args.push_back(current);
                    current.clear();
                }
                args.push_back("1>>");
                i += 2;
                continue;
            }

            // 2>>
            if (c == '2' && i + 2 < input.size() &&
                input[i + 1] == '>' && input[i + 2] == '>')
            {
                if (!current.empty())
                {
                    args.push_back(current);
                    current.clear();
                }
                args.push_back("2>>");
                i += 2;
                continue;
            }

            if (c == '>' && i + 1 < input.size() && input[i + 1] == '>')
            {
                if (!current.empty())
                {
                    args.push_back(current);
                    current.clear();
                }
                args.push_back(">>");
                i++;
                continue;
            }

            if (c == '1' && i + 1 < input.size() && input[i + 1] == '>')
            {
                if (!current.empty())
                {
                    args.push_back(current);
                    current.clear();
                }
                args.push_back("1>");
                i++;
                continue;
            }
            if (c == '2' && i + 1 < input.size() && input[i + 1] == '>')
            {
                if (!current.empty())
                {
                    args.push_back(current);
                    current.clear();
                }
                args.push_back("2>");
                i++;
                continue;
            }
            if (c == '>')
            {
                if (!current.empty())
                {
                    args.push_back(current);
                    current.clear();
                }
                args.push_back(">");
                continue;
            }
        }

        /* Backslash handling */
        if (c == '\\')
        {
            if (!in_single_quote && !in_double_quote)
            {
                if (i + 1 < input.size())
                {
                    current += input[i + 1];
                    i++;
                }
                continue;
            }

            if (in_double_quote)
            {
                if (i + 1 < input.size())
                {
                    char next = input[i + 1];
                    if (next == '"' || next == '\\')
                    {
                        current += next;
                        i++;
                    }
                    else
                    {
                        current += '\\';
                    }
                }
                continue;
            }

            current += '\\';
            continue;
        }

        if (c == '\'' && !in_double_quote)
        {
            in_single_quote = !in_single_quote;
            continue;
        }

        if (c == '"' && !in_single_quote)
        {
            in_double_quote = !in_double_quote;
            continue;
        }

        if ((c == ' ' || c == '\t') && !in_single_quote && !in_double_quote)
        {
            if (!current.empty())
            {
                args.push_back(current);
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
        args.push_back(current);
    return args;
}

vector<char *> to_char_array(vector<string> &args)
{
    vector<char *> result;
    for (auto &arg : args)
        result.push_back(const_cast<char *>(arg.c_str()));
    result.push_back(nullptr);
    return result;
}

struct StdoutRedirect
{
    int saved = -1;
    bool active = false;

    StdoutRedirect(bool enable, const string &outfile, bool append)
    {
        if (!enable)
            return;

        saved = dup(STDOUT_FILENO);
        if (saved < 0)
        {
            perror("dup");
            return;
        }

        int fd = open(outfile.c_str(), O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
        if (fd < 0)
        {
            perror("open");
            close(saved);
            saved = -1;
            return;
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2");
            close(fd);
            close(saved);
            saved = -1;
            return;
        }
        close(fd);
        active = true;
    }

    ~StdoutRedirect()
    {
        if (!active)
            return;
        dup2(saved, STDOUT_FILENO);
        close(saved);
    }
};

void run_external(vector<string> &args, bool redirect_out, const string &outfile, bool append, bool redirect_err, const string &errfile, bool err_append)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        if (redirect_out)
        {
            int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
            int fd = open(outfile.c_str(), flags, 0644);
            if (fd < 0)
                _exit(1);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (redirect_err)
        {
            int flags = O_WRONLY | O_CREAT | (err_append ? O_APPEND : O_TRUNC);
            int fd = open(errfile.c_str(), flags, 0644);
            if (fd < 0)
                _exit(1);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        vector<char *> c_args = to_char_array(args);
        execvp(c_args[0], c_args.data());
        cerr << args[0] << ": command not found\n";
        _exit(127);
    }
    else if (pid > 0)
    {
        waitpid(pid, nullptr, 0);
    }
    else
    {
        perror("fork");
    }
}

int main()
{
    cout << unitbuf;
    cerr << unitbuf;

    string input;
    vector<string>history;

    while (true)
    {
        cout << "$ ";
        cout.flush();

        if (!getline(cin, input))
            break;

        if (input.empty())
            continue;

        vector<string> args = split_args(input);
        
        
        if(args[0]=="history"){
            for(int i=0;i<history.size();i++){
                cout<<history[i]<<endl;
            }
        }
        history.push_back(input);


        bool redirect_out = false;
        bool redirect_err = false;
        bool append = false;
        bool err_append = false;
        bool has_pipe = false;
        size_t pipe_index = 0;

        string outfile;
        string errfile;


        for (size_t i = 0; i < args.size();)
        {
            if (args[i] == "|")
            {
                has_pipe = true;
                pipe_index = i;
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

        if (has_pipe)
        {
            vector<vector<string>> commands;
            vector<string> current_cmd;

            for (size_t i = 0; i < args.size(); i++)
            {
                if (args[i] == "|")
                {
                    if (!current_cmd.empty())
                    {
                        commands.push_back(current_cmd);
                        current_cmd.clear();
                    }
                }
                else
                {
                    current_cmd.push_back(args[i]);
                }
            }

            if (!current_cmd.empty())
            {
                commands.push_back(current_cmd);
            }

            int num_commands = commands.size();
            int num_pipes = num_commands - 1;

            int pipes[num_pipes][2];
            for (int i = 0; i < num_pipes; i++)
            {
                if (pipe(pipes[i]) < 0)
                {
                    perror("pipe");
                    continue;
                }
            }

            vector<pid_t> pids;
            for (int i = 0; i < num_commands; i++)
            {
                pid_t pid = fork();

                if (pid == 0)
                {
                    if (i > 0)
                    {
                        dup2(pipes[i - 1][0], STDIN_FILENO);
                    }

                    if (i < num_commands - 1)
                    {
                        dup2(pipes[i][1], STDOUT_FILENO);
                    }

                    for (int j = 0; j < num_pipes; j++)
                    {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    
                    if (commands[i][0] == "echo")
                    {
                        for (size_t k = 1; k < commands[i].size(); k++)
                        {
                            cout << commands[i][k];
                            if (k + 1 < commands[i].size())
                                cout << " ";
                        }
                        cout << "\n";
                        _exit(0);
                    }
                    else if (commands[i][0] == "pwd")
                    {
                        cout << filesystem::current_path().string() << "\n";
                        _exit(0);
                    }
                    else if (commands[i][0] == "type")
                    {
                        string cmd = (commands[i].size() >= 2 ? commands[i][1] : "");

                        if (cmd == "exit" || cmd == "echo" || cmd == "type" ||
                            cmd == "pwd" || cmd == "cd")
                        {
                            cout << cmd << " is a shell builtin\n";
                        }
                        else
                        {
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
                                        cout << cmd << " is " << full_path << "\n";
                                        found = true;
                                        break;
                                    }
                                }
                            }
                            if (!found)
                            {
                                cout << cmd << ": not found\n";
                            }
                        }
                        _exit(0);
                    }

                    vector<char *> cargs = to_char_array(commands[i]);
                    execvp(cargs[0], cargs.data());
                    cerr << commands[i][0] << ": command not found\n";
                    _exit(127);
                }
                else if (pid > 0)
                {
                    pids.push_back(pid);
                }
            }

            for (int i = 0; i < num_pipes; i++)
            {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            for (pid_t pid : pids)
            {
                waitpid(pid, nullptr, 0);
            }

            continue;
        }




        if (args.empty())
            continue;

        if (args[0] == "exit")
            return 0;

        if (args[0] == "echo")
        {
            if (redirect_err)
            {
                int flags = O_WRONLY | O_CREAT | (err_append ? O_APPEND : O_TRUNC);
                int fd = open(errfile.c_str(), flags, 0644);
                if (fd >= 0)
                    close(fd);
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

        if (args[0] == "pwd")
        {
            cout << filesystem::current_path().string() << '\n';
            continue;
        }

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

        if (args[0] == "type")
        {
            string cmd = (args.size() >= 2 ? args[1] : "");

            if (cmd == "exit" || cmd == "echo" || cmd == "type" || cmd == "pwd" || cmd == "cd",cmd=="history")
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

            if (!found)
                cout << cmd << ": not found\n";
            continue;
        }

        run_external(args, redirect_out, outfile, append, redirect_err, errfile, err_append);
    }
}
