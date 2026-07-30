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
     * @brief Evaluates frame detection results and executes soft alert or hard defense if needed.
     */
    void execute_triggers(const FrameResult& result);

    /**
     * @brief Triggers a soft alert (e.g. screen edge glow, tray notification).
     */
    void trigger_soft_alert(const std::string& message = "Visual eavesdropper detected nearby!");

    /**
     * @brief Triggers a hard defense (e.g. blurred window overlay, OS workstation lock).
     */
    void trigger_hard_defense(const std::string& message = "Eavesdropper gaze verified > 1.0s. Locking workstation!");

    /**
     * @brief Removes/deactivates active alerts or overlays when space is clear.
     */
    void clear_alerts();

    /**
     * @brief Writes threat event to audit log for NIST SP 800-53 / ISO 27001 compliance.
     */
    void log_threat_event(const std::string& threat_type, double gaze_duration_sec, size_t face_count);

    bool is_hard_defense_active() const { return hard_defense_active_; }
    bool is_soft_alert_active() const { return soft_alert_active_; }

private:
    Config config_;
    bool soft_alert_active_ = false;
    bool hard_defense_active_ = false;
    std::ofstream log_stream_;

    struct PlatformImpl;
    std::unique_ptr<PlatformImpl> platform_impl_;
};

} // namespace blindside

#endif // BLINDSIDE_PRIVACY_TRIGGER_HPP
