# Shell Implementation in C++

This project is a custom shell implementation written in C++. It parses and interprets commands, handles built-in functionality, and executes external programs.

## Features

### Built-in Commands
- `cd [path]`: Change the current working directory. Supports `~` for the home directory.
- `pwd`: Print the current working directory.
- `echo [text]`: Print text to standard output.
- `exit`: Terminate the shell.
- `type [command]`: Identify whether a command is a builtin or an external executable.
- `history`: Display the command history.

### Core Capabilities
- **Command Parsing**: Handles arguments with single (`'`) and double (`"`) quotes, including escape characters.
- **External Execution**: Runs executable programs found in the system `PATH`.
- **I/O Redirection**:
    - `>` or `1>`: Redirect standard output (overwrite).
    - `>>` or `1>>`: Redirect standard output (append).
    - `2>`: Redirect standard error (overwrite).
    - `2>>`: Redirect standard error (append).
- **Cross-Platform**: Support for Windows (using `_spawnvp`) and POSIX (using `fork`/`execvp`).

## Build Instructions

### Prerequisites
- CMake (version 3.13 or higher)
- C++ Compiler (supporting C++23 standard) in windows `g++` 

### Steps

1.  **Clone the repository**:
    ```sh
    git clone https://github.com/abhisheksssss/Shell.git
    cd Shell
    ```

2.  **Configure and Build**:
    ```sh
    cmake -S . -B build
    cmake --build build
    ```

3.  **Run**:
    - **Windows**: `.\build\shell.exe`
    - **Linux/Mac**: `./build/shell`

## Project Structure

The project code is organized for modularity:

- `src/main.cpp`: The entry point and main REPL loop.
- `src/utils/`: Contains helper functions and classes.
    - `builtins.hpp/cpp`: Definitions of built-in command names.
    - `isNumeric.hpp/cpp`: Utility to check for numeric strings.
    - `split_path.hpp/cpp`: Splits the `PATH` environment variable.
    - `is_executable.hpp/cpp`: Checks if a file is executable.
    - `split_args.hpp/cpp`: Parses input strings into arguments, handling quotes.
    - `to_char_array.hpp/cpp`: Converts `std::vector<string>` to `char**` for C APIs.
    - `StdoutRedirect.hpp/cpp`: RAII class for handling file descriptor redirection.
    - `run_external.hpp/cpp`: Logic for executing external processes.
    - `command_generator.hpp/cpp`: (POSIX) Auto-completion logic.
    - `command_completion.hpp/cpp`: (POSIX) Readline completion hook.
