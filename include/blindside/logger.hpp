#ifndef BLINDSIDE_LOGGER_HPP
#define BLINDSIDE_LOGGER_HPP

#include <string>
#include <mutex>
#include <fstream>
#include <chrono>

namespace blindside {

class Logger {
public:
    static Logger& get_instance();

    bool initialize(const std::string& log_file_path);
    void log_threat(const std::string& event_type, const std::string& threat_classification, double gaze_duration_sec, int face_count);
    void log_system(const std::string& message);

private:
    Logger() = default;
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string get_current_timestamp() const;

    std::mutex mutex_;
    std::ofstream log_file_;
    bool initialized_ = false;
};

} // namespace blindside

#endif // BLINDSIDE_LOGGER_HPP
