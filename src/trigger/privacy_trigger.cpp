#include "blindside/privacy_trigger.hpp"
#include "blindside/platform.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

namespace blindside {

PrivacyTriggerManager::PrivacyTriggerManager(const Config& config)
    : config_(config) {
    platform_manager_ = PlatformManager::create();
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

    if (!platform_manager_->initialize()) {
        std::cerr << "[PrivacyTrigger] Failed to initialize platform manager." << std::endl;
        return false;
    }

    return true;
}

WindowRect PrivacyTriggerManager::get_active_window_geometry() {
    return platform_manager_->get_active_window_geometry();
}

void PrivacyTriggerManager::trigger_targeted_blur(const WindowRect& rect) {
    if (targeted_blur_active_) return;
    targeted_blur_active_ = true;

    auto diag = platform_manager_->get_diagnostics();
    if (diag.supports_native_redaction) {
        std::cout << "\033[1;36m[TARGETED PRIVACY OVERLAY] Redacting Active Workspace Window [" 
                  << rect.x << ", " << rect.y << ", " << rect.width << "x" << rect.height << "]\033[0m" << std::endl;
    } else {
        std::cout << "\033[1;33m[TARGETED PRIVACY OVERLAY] Requested, but unsupported on this platform. Falling back.\033[0m" << std::endl;
    }

    platform_manager_->trigger_targeted_blur(rect);
}

void PrivacyTriggerManager::trigger_soft_alert(const std::string& message) {
    if (soft_alert_active_) return;
    soft_alert_active_ = true;

    std::cout << "\033[1;33m[SOFT ALERT] " << message << "\033[0m" << std::endl;
    platform_manager_->trigger_soft_alert(message);
}

void PrivacyTriggerManager::trigger_hard_defense(const std::string& message) {
    if (hard_defense_active_) return;
    hard_defense_active_ = true;

    std::cout << "\033[1;31m[HARD DEFENSE TRIGGERED] " << message << "\033[0m" << std::endl;

    if (config_.enable_screen_lock) {
        platform_manager_->trigger_hard_defense(message);
    }
}

void PrivacyTriggerManager::clear_alerts() {
    if (soft_alert_active_ || hard_defense_active_ || targeted_blur_active_) {
        std::cout << "[PrivacyTrigger] Clearing active privacy alerts and overlays." << std::endl;
        soft_alert_active_ = false;
        hard_defense_active_ = false;
        targeted_blur_active_ = false;
        platform_manager_->clear_alerts();
    }
}

void PrivacyTriggerManager::log_threat_event(const std::string& threat_type, double gaze_duration_sec, size_t face_count, bool live_verified) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    std::cout << "[AUDIT LOG] Threat: " << threat_type 
              << " | Gaze Duration: " << std::fixed << std::setprecision(2) << gaze_duration_sec << "s"
              << " | Faces Present: " << face_count
              << " | Liveness Verified: " << (live_verified ? "YES" : "NO") << std::endl;

    if (log_stream_.is_open()) {
        log_stream_ << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S")
                    << " [AUDIT_ALERT] Threat=" << threat_type
                    << " GazeDurationSec=" << std::fixed << std::setprecision(2) << gaze_duration_sec
                    << " FacesDetected=" << face_count
                    << " LivenessVerified=" << (live_verified ? "1" : "0")
                    << " Control=NIST_SP_800_53_PE_3" << std::endl;
        log_stream_.flush();
    }
}

void PrivacyTriggerManager::execute_triggers(const FrameResult& result) {
    if (result.trigger_targeted_blur) {
        WindowRect active_rect = get_active_window_geometry();
        trigger_targeted_blur(active_rect);
        log_threat_event("TARGETED_WORKSPACE_BLUR", result.secondary_gaze_duration_sec, result.faces.size(), result.secondary_liveness_verified);
    } else if (result.trigger_hard_defense) {
        trigger_hard_defense();
        log_threat_event("HARD_WORKSTATION_LOCK", result.secondary_gaze_duration_sec, result.faces.size(), result.secondary_liveness_verified);
    } else if (result.trigger_soft_alert) {
        trigger_soft_alert();
        log_threat_event("SOFT_SECONDARY_FACE_DETECTED", result.secondary_gaze_duration_sec, result.faces.size(), result.secondary_liveness_verified);
    } else {
        clear_alerts();
    }
}

PlatformDiagnostics PrivacyTriggerManager::get_diagnostics() const {
    return platform_manager_->get_diagnostics();
}

} // namespace blindside
