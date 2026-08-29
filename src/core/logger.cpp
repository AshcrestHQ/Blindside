#include "blindside/logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace blindside {

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

bool Logger::initialize(const std::string& log_file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) return true;

    log_file_.open(log_file_path, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "[Logger] Failed to open log file: " << log_file_path << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}

std::string Logger::get_current_timestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

void Logger::log_threat(const std::string& event_type, const std::string& threat_classification, double gaze_duration_sec, int face_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_ || !log_file_.is_open()) return;

    log_file_ << "[" << get_current_timestamp() << "] "
              << "EVENT=" << event_type << " "
              << "CLASS=" << threat_classification << " "
              << "DURATION=" << std::fixed << std::setprecision(2) << gaze_duration_sec << "s "
              << "FACES=" << face_count << "\n";
    log_file_.flush();
}

void Logger::log_system(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_ || !log_file_.is_open()) return;

    log_file_ << "[" << get_current_timestamp() << "] "
              << "SYSTEM: " << message << "\n";
    log_file_.flush();
}

} // namespace blindside
