#include "blindside/platform_paths.hpp"
#include <unistd.h>
#include <limits.h>

namespace blindside {

std::filesystem::path get_executable_directory() {
    char buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    
    if (length != -1) {
        buffer[length] = '\0';
        std::filesystem::path exe_path(buffer);
        return exe_path.parent_path();
    }
    
    // Fallback if readlink fails
    return std::filesystem::current_path();
}

} // namespace blindside
