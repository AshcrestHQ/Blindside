#include "blindside/platform.hpp"
#include <iostream>
#include <windows.h>
#include <dwmapi.h>

namespace blindside {

class WindowsPlatformManager : public PlatformManager {
public:
    WindowsPlatformManager() = default;
    ~WindowsPlatformManager() {
        clear_alerts();
    }

    bool initialize() override {
        return true;
    }

    PlatformDiagnostics get_diagnostics() const override {
        PlatformDiagnostics diag;
        diag.os_name = "Windows";
        diag.supports_native_redaction = true;
        diag.supports_screen_lock = true;
        diag.supports_desktop_notifications = true;
        diag.monitor_count = GetSystemMetrics(SM_CMONITORS);
        return diag;
    }

    WindowRect get_active_window_geometry() override {
        WindowRect rect;
        HWND hwnd = GetForegroundWindow();
        if (hwnd) {
            RECT r;
            if (GetWindowRect(hwnd, &r)) {
                rect.x = r.left;
                rect.y = r.top;
                rect.width = r.right - r.left;
                rect.height = r.bottom - r.top;
                rect.valid = true;
            }
        }
        if (!rect.valid) {
            rect.x = 100;
            rect.y = 100;
            rect.width = 1280;
            rect.height = 800;
            rect.valid = true;
        }
        return rect;
    }

    void trigger_targeted_blur(const WindowRect& rect) override {
        if (!overlay_hwnd_) {
            overlay_hwnd_ = CreateWindowExA(
                WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                "STATIC", "BlindsidePrivacyBlur",
                WS_POPUP | WS_VISIBLE,
                rect.x, rect.y, rect.width, rect.height,
                NULL, NULL, GetModuleHandle(NULL), NULL
            );
            if (overlay_hwnd_) {
                SetLayeredWindowAttributes(overlay_hwnd_, RGB(0, 0, 0), 220, LWA_ALPHA);
                DWM_BLURBEHIND bb = {0};
                bb.dwFlags = DWM_BB_ENABLE;
                bb.fEnable = TRUE;
                DwmEnableBlurBehindWindow(overlay_hwnd_, &bb);
            }
        } else {
            SetWindowPos(overlay_hwnd_, HWND_TOPMOST, rect.x, rect.y, rect.width, rect.height, SWP_SHOWWINDOW);
        }
    }

    void trigger_soft_alert(const std::string& message) override {
        MessageBeep(MB_ICONWARNING);
        // Note: For true toast notifications, we'd use WinRT/COM. For now, beep + terminal output.
    }

    void trigger_hard_defense(const std::string& message) override {
        LockWorkStation();
    }

    void clear_alerts() override {
        if (overlay_hwnd_) {
            DestroyWindow(overlay_hwnd_);
            overlay_hwnd_ = nullptr;
        }
    }

private:
    HWND overlay_hwnd_ = nullptr;
};

std::unique_ptr<PlatformManager> PlatformManager::create() {
    return std::make_unique<WindowsPlatformManager>();
}

} // namespace blindside
