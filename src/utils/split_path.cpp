#include "split_path.hpp"
#include <sstream>

std::vector<std::string> split_path(const std::string& env) {
    std::vector<std::string> paths;
    std::stringstream ss(env);
    std::string item;
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
