#include "split_args.hpp"

std::vector<std::string> split_args(const std::string& input) {
    std::vector<std::string> args;
    std::string current;
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
