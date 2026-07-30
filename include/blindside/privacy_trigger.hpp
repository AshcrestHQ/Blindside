#ifndef BLINDSIDE_PRIVACY_TRIGGER_HPP
#define BLINDSIDE_PRIVACY_TRIGGER_HPP

#include "blindside/config.hpp"
#include "blindside/types.hpp"
#include <string>
#include <memory>
#include <fstream>

namespace blindside {

class PrivacyTriggerManager {
public:
    explicit PrivacyTriggerManager(const Config& config);
    ~PrivacyTriggerManager();

    bool initialize();

    /**
     * @brief Evaluates frame detection results and executes soft alert, targeted blur, or hard defense if needed.
     */
    void execute_triggers(const FrameResult& result);

    /**
     * @brief Retrieves exact active focused window rectangle via native OS APIs.
     */
    WindowRect get_active_window_geometry();

    /**
     * @brief Spawns a targeted transparent/blurred privacy overlay strictly covering active window bounds.
     */
    void trigger_targeted_blur(const WindowRect& rect);

    /**
     * @brief Triggers a soft alert (e.g. screen edge glow, tray notification).
     */
    void trigger_soft_alert(const std::string& message = "Visual eavesdropper detected nearby!");

    /**
     * @brief Triggers a hard defense (e.g. OS workstation lock).
     */
    void trigger_hard_defense(const std::string& message = "Eavesdropper gaze verified > 1.0s. Locking workstation!");

    /**
     * @brief Removes/deactivates active alerts or overlays when space is clear.
     */
    void clear_alerts();

    /**
     * @brief Writes threat event to audit log for NIST SP 800-53 / ISO 27001 compliance.
     */
    void log_threat_event(const std::string& threat_type, double gaze_duration_sec, size_t face_count, bool live_verified);

    bool is_hard_defense_active() const { return hard_defense_active_; }
    bool is_soft_alert_active() const { return soft_alert_active_; }
    bool is_targeted_blur_active() const { return targeted_blur_active_; }

private:
    Config config_;
    bool soft_alert_active_ = false;
    bool hard_defense_active_ = false;
    bool targeted_blur_active_ = false;
    std::ofstream log_stream_;

    struct PlatformImpl;
    std::unique_ptr<PlatformImpl> platform_impl_;
};

} // namespace blindside

#endif // BLINDSIDE_PRIVACY_TRIGGER_HPP
