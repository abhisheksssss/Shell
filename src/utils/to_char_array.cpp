#include "to_char_array.hpp"

std::vector<char*> to_char_array(const std::vector<std::string>& args) {
    std::vector<char*> result;
    for (const auto& s : args) {
        result.push_back(const_cast<char*>(s.c_str()));
    }
    result.push_back(nullptr);
    return result;
}
