#include "blindside/platform_paths.hpp"
#include <windows.h>
#include <vector>

namespace blindside {

std::filesystem::path get_executable_directory() {
    std::vector<wchar_t> buffer(MAX_PATH);
    DWORD length = GetModuleFileNameW(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
    
    // If buffer is too small, double it until it fits (up to a reasonable limit)
    while (length == buffer.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        buffer.resize(buffer.size() * 2);
        length = GetModuleFileNameW(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
    }

    if (length == 0) {
        // Fallback if API fails
        return std::filesystem::current_path();
    }

    std::filesystem::path exe_path(buffer.data());
    return exe_path.parent_path();
}

} // namespace blindside
