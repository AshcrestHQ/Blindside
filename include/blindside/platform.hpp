#ifndef BLINDSIDE_PLATFORM_HPP
#define BLINDSIDE_PLATFORM_HPP

#include "blindside/types.hpp"
#include <string>
#include <memory>

namespace blindside {

/**
 * @brief Discrete platform capabilities representing OS-level privacy primitives.
 */
struct PlatformCapabilities {
    bool screen_redaction = false;       // Can render an opaque or translucent privacy overlay
    bool targeted_blur = false;          // True DWM/compositor-backed blur
    bool session_lock = false;           // OS session lock (LockWorkStation / loginctl)
    bool click_through_overlay = false; // Overlay allows input to pass through to underlying app
    bool multi_monitor_aware = false;   // Can enumerate and position across multi-monitor virtual screen
};

/**
 * @brief Platform capability diagnostics info.
 */
struct PlatformDiagnostics {
    std::string os_name;
    bool supports_native_redaction = false;
    bool supports_targeted_blur = false;
    bool supports_screen_lock = false;
    bool supports_desktop_notifications = false;
    bool supports_click_through = false;
    int monitor_count = 1;
    std::string redaction_mode; // e.g. "AlphaRedaction", "OpaqueRedaction", "DwmBlur", "None"
    PlatformCapabilities capabilities{};
};

/**
 * @brief Abstract interface for OS-specific privacy actions and window management.
 */
class PlatformManager {
public:
    virtual ~PlatformManager() = default;

    virtual bool initialize() = 0;
    
    virtual PlatformDiagnostics get_diagnostics() const = 0;
    virtual PlatformCapabilities get_capabilities() const = 0;

    virtual WindowRect get_active_window_geometry() = 0;

    // Privacy Triggers
    virtual void trigger_targeted_blur(const WindowRect& rect) = 0;
    virtual void trigger_soft_alert(const std::string& message) = 0;
    virtual void trigger_hard_defense(const std::string& message) = 0;
    virtual void clear_alerts() = 0;

    // Non-blocking platform event pump to prevent UI thread freezing
    virtual void pump_events() {}

    // Factory method that returns the correct native implementation
    static std::unique_ptr<PlatformManager> create();
};

} // namespace blindside

#endif // BLINDSIDE_PLATFORM_HPP
