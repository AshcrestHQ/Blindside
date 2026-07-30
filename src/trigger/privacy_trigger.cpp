#include "blindside/privacy_trigger.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <unistd.h>
#endif

namespace blindside {

struct PrivacyTriggerManager::PlatformImpl {
#if defined(__linux__)
    Display* display = nullptr;
    Window overlay_win = 0;
#elif defined(_WIN32)
    HWND overlay_hwnd = nullptr;
#endif
};

PrivacyTriggerManager::PrivacyTriggerManager(const Config& config)
    : config_(config), platform_impl_(std::make_unique<PlatformImpl>()) {
}

PrivacyTriggerManager::~PrivacyTriggerManager() {
    clear_alerts();
    if (log_stream_.is_open()) {
        log_stream_.close();
    }
}

bool PrivacyTriggerManager::initialize() {
    log_stream_.open(config_.log_file_path, std::ios::app);
    if (log_stream_.is_open()) {
        std::cout << "[PrivacyTrigger] Threat log opened at " << config_.log_file_path << std::endl;
    }

#if defined(__linux__)
    platform_impl_->display = XOpenDisplay(nullptr);
    if (platform_impl_->display) {
        std::cout << "[PrivacyTrigger] Native X11 display connection established." << std::endl;
    }
#endif

    return true;
}

void PrivacyTriggerManager::trigger_soft_alert(const std::string& message) {
    if (soft_alert_active_) return;
    soft_alert_active_ = true;

    std::cout << "\033[1;33m[SOFT ALERT] " << message << "\033[0m" << std::endl;

#if defined(__linux__)
    // Spawn desktop notification via notify-send if available
    int ret = std::system("notify-send -u critical -t 2000 '🛡️ Blindside Privacy Warning' 'Unrecognized face looking at screen!' 2>/dev/null &");
    (void)ret;
#elif defined(_WIN32)
    // Windows Tray notification sound
    MessageBeep(MB_ICONWARNING);
#endif
}

void PrivacyTriggerManager::trigger_hard_defense(const std::string& message) {
    if (hard_defense_active_) return;
    hard_defense_active_ = true;

    std::cout << "\033[1;31m[HARD DEFENSE TRIGGERED] " << message << "\033[0m" << std::endl;

    if (config_.enable_screen_lock) {
#if defined(_WIN32)
        std::cout << "[PrivacyTrigger] Calling Win32 LockWorkStation()..." << std::endl;
        LockWorkStation();
#elif defined(__linux__)
        std::cout << "[PrivacyTrigger] Invoking Linux native workstation lock..." << std::endl;
        int ret = std::system("loginctl lock-session 2>/dev/null || xset ss activate 2>/dev/null || dbus-send --type=method_call --dest=org.gnome.ScreenSaver /org/gnome/ScreenSaver org.gnome.ScreenSaver.Lock 2>/dev/null &");
        (void)ret;
#endif
    }
}

void PrivacyTriggerManager::clear_alerts() {
    if (soft_alert_active_ || hard_defense_active_) {
        std::cout << "[PrivacyTrigger] Clearing active privacy alerts and overlays." << std::endl;
        soft_alert_active_ = false;
        hard_defense_active_ = false;
    }
}

void PrivacyTriggerManager::log_threat_event(const std::string& threat_type, double gaze_duration_sec, size_t face_count) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    std::cout << "[AUDIT LOG] Threat: " << threat_type 
              << " | Gaze Duration: " << std::fixed << std::setprecision(2) << gaze_duration_sec << "s"
              << " | Faces Present: " << face_count << std::endl;

    if (log_stream_.is_open()) {
        log_stream_ << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S")
                    << " [AUDIT_ALERT] Threat=" << threat_type
                    << " GazeDurationSec=" << std::fixed << std::setprecision(2) << gaze_duration_sec
                    << " FacesDetected=" << face_count
                    << " Control=NIST_SP_800_53_PE_3" << std::endl;
        log_stream_.flush();
    }
}

void PrivacyTriggerManager::execute_triggers(const FrameResult& result) {
    if (result.trigger_hard_defense) {
        if (config_.trigger_mode == TriggerMode::HardDefenseOnly || config_.trigger_mode == TriggerMode::Both) {
            trigger_hard_defense();
        }
        log_threat_event("HARD_EAVESDROPPER_GAZE", result.secondary_gaze_duration_sec, result.faces.size());
    } else if (result.trigger_soft_alert) {
        if (config_.trigger_mode == TriggerMode::SoftAlertOnly || config_.trigger_mode == TriggerMode::Both) {
            trigger_soft_alert();
        }
        log_threat_event("SOFT_SECONDARY_FACE_DETECTED", result.secondary_gaze_duration_sec, result.faces.size());
    } else {
        clear_alerts();
    }
}

} // namespace blindside
