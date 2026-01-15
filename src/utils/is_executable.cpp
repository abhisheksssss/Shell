#include "is_executable.hpp"
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

bool is_executable(const std::string& path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES);
#else
    return access(path.c_str(), X_OK) == 0;
#endif
}
