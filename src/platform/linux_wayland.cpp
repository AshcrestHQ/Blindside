#include "blindside/platform.hpp"
#include <iostream>
#include <cstdlib>

namespace blindside {

class LinuxWaylandPlatformManager : public PlatformManager {
public:
    LinuxWaylandPlatformManager() = default;
    ~LinuxWaylandPlatformManager() = default;

    bool initialize() override {
        // Since Wayland is restrictive, we don't open a generic display connection.
        // We rely on standard dbus / command line utilities for notifications and locks.
        std::cout << "[Platform] Initialized Wayland backend (Native overlays unsupported, using fallbacks)." << std::endl;
        return true;
    }

    PlatformCapabilities get_capabilities() const override {
        PlatformCapabilities caps;
        caps.screen_redaction = false;       // Wayland protocol forbids arbitrary surface positioning
        caps.targeted_blur = false;          // No compositor blur protocol
        caps.session_lock = true;           // loginctl / dbus org.gnome.ScreenSaver
        caps.click_through_overlay = false; // Overlays unsupported
        caps.multi_monitor_aware = false;   // Protocol restricted
        return caps;
    }

    PlatformDiagnostics get_diagnostics() const override {
        PlatformDiagnostics diag;
        diag.os_name = "Linux (Wayland)";
        diag.supports_native_redaction = false; // Wayland restricts absolute window positioning overlays
        diag.supports_targeted_blur = false;
        diag.supports_screen_lock = true;       // Attempted via loginctl / dbus (availability subject to host session manager)
        diag.supports_desktop_notifications = true; // Attempted via notify-send
        diag.supports_click_through = false;
        diag.monitor_count = 1; // Difficult to poll without specific compositor protocols
        diag.redaction_mode = "None (Protocol Restricted)";
        diag.capabilities = get_capabilities();
        return diag;
    }

    WindowRect get_active_window_geometry() override {
        // Cannot reliably get active window geometry on pure Wayland without compositor-specific protocols
        WindowRect rect;
        rect.valid = false;
        return rect;
    }

    void trigger_targeted_blur(const WindowRect& rect) override {
        // Not supported on Wayland due to protocol restrictions.
        // Fallback to soft alert.
        trigger_soft_alert("Privacy overlay requested, but unsupported on Wayland.");
    }

    void trigger_soft_alert(const std::string& message) override {
        int ret = std::system("notify-send -u critical -t 2000 '🛡️ Blindside Privacy Warning' 'Unrecognized face looking at screen!' 2>/dev/null &");
        (void)ret;
    }

    void trigger_hard_defense(const std::string& message) override {
        // Use loginctl or dbus which are compositor-agnostic standard session lock mechanisms
        int ret = std::system("loginctl lock-session 2>/dev/null || dbus-send --type=method_call --dest=org.gnome.ScreenSaver /org/gnome/ScreenSaver org.gnome.ScreenSaver.Lock 2>/dev/null &");
        (void)ret;
    }

    void clear_alerts() override {
        // Nothing to clear since overlays aren't supported.
    }
};

std::unique_ptr<PlatformManager> PlatformManager::create() {
    return std::make_unique<LinuxWaylandPlatformManager>();
}

} // namespace blindside
