#ifndef BLINDSIDE_PLATFORM_PATHS_HPP
#define BLINDSIDE_PLATFORM_PATHS_HPP

#include <filesystem>
#include <string>

namespace blindside {

// Retrieves the directory containing the current executable.
// Uses GetModuleFileNameW on Windows and /proc/self/exe on Linux.
std::filesystem::path get_executable_directory();

} // namespace blindside

#endif // BLINDSIDE_PLATFORM_PATHS_HPP
