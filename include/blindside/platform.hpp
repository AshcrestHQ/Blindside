#ifndef BLINDSIDE_PLATFORM_HPP
#define BLINDSIDE_PLATFORM_HPP

#include "blindside/types.hpp"
#include <string>
#include <memory>

namespace blindside {

/**
 * @brief Platform capability diagnostics info.
 */
struct PlatformDiagnostics {
    std::string os_name;
    bool supports_native_redaction = false;
    bool supports_screen_lock = false;
    bool supports_desktop_notifications = false;
    int monitor_count = 1;
};

/**
 * @brief Abstract interface for OS-specific privacy actions and window management.
 */
class PlatformManager {
public:
    virtual ~PlatformManager() = default;

    virtual bool initialize() = 0;
    
    virtual PlatformDiagnostics get_diagnostics() const = 0;

    virtual WindowRect get_active_window_geometry() = 0;

    // Privacy Triggers
    virtual void trigger_targeted_blur(const WindowRect& rect) = 0;
    virtual void trigger_soft_alert(const std::string& message) = 0;
    virtual void trigger_hard_defense(const std::string& message) = 0;
    virtual void clear_alerts() = 0;

    // Factory method that returns the correct native implementation
    static std::unique_ptr<PlatformManager> create();
};

} // namespace blindside

#endif // BLINDSIDE_PLATFORM_HPP
