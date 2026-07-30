#include "blindside/privacy_trigger.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#include <dwmapi.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
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
        std::cout << "[PrivacyTrigger] Native X11 display connection established for targeted window hooks." << std::endl;
    }
#endif

    return true;
}

WindowRect PrivacyTriggerManager::get_active_window_geometry() {
    WindowRect rect;
#if defined(_WIN32)
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
#elif defined(__linux__)
    if (platform_impl_->display) {
        Window focus_win = 0;
        int revert_to = 0;
        XGetInputFocus(platform_impl_->display, &focus_win, &revert_to);
        if (focus_win != 0 && focus_win != PointerRoot) {
            Window root = 0;
            int x = 0, y = 0;
            unsigned int width = 0, height = 0, border = 0, depth = 0;
            if (XGetGeometry(platform_impl_->display, focus_win, &root, &x, &y, &width, &height, &border, &depth)) {
                int child_x = 0, child_y = 0;
                Window child = 0;
                XTranslateCoordinates(platform_impl_->display, focus_win, root, 0, 0, &child_x, &child_y, &child);
                rect.x = child_x;
                rect.y = child_y;
                rect.width = static_cast<int>(width);
                rect.height = static_cast<int>(height);
                rect.valid = true;
            }
        }
    }
#endif

    if (!rect.valid) {
        // Fallback default screen bounds if active window lookup fails
        rect.x = 100;
        rect.y = 100;
        rect.width = 1280;
        rect.height = 800;
        rect.valid = true;
    }

    return rect;
}

void PrivacyTriggerManager::trigger_targeted_blur(const WindowRect& rect) {
    if (targeted_blur_active_) return;
    targeted_blur_active_ = true;

    std::cout << "\033[1;36m[TARGETED PRIVACY OVERLAY] Redacting Active Workspace Window [" 
              << rect.x << ", " << rect.y << ", " << rect.width << "x" << rect.height << "]\033[0m" << std::endl;

#if defined(_WIN32)
    // Windows Win32 layered transparent window overlay matching active window bounds
    if (!platform_impl_->overlay_hwnd) {
        platform_impl_->overlay_hwnd = CreateWindowExA(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
            "STATIC", "BlindsidePrivacyBlur",
            WS_POPUP | WS_VISIBLE,
            rect.x, rect.y, rect.width, rect.height,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );
        if (platform_impl_->overlay_hwnd) {
            SetLayeredWindowAttributes(platform_impl_->overlay_hwnd, RGB(0, 0, 0), 200, LWA_ALPHA);
            DWM_BLURBEHIND bb = {0};
            bb.dwFlags = DWM_BB_ENABLE;
            bb.fEnable = TRUE;
            DwmEnableBlurBehindWindow(platform_impl_->overlay_hwnd, &bb);
        }
    } else {
        SetWindowPos(platform_impl_->overlay_hwnd, HWND_TOPMOST, rect.x, rect.y, rect.width, rect.height, SWP_SHOWWINDOW);
    }
#elif defined(__linux__)
    if (platform_impl_->display && platform_impl_->overlay_win == 0) {
        int screen = DefaultScreen(platform_impl_->display);
        Window root = RootWindow(platform_impl_->display, screen);

        XSetWindowAttributes attrs;
        attrs.override_redirect = True;
        attrs.background_pixel = BlackPixel(platform_impl_->display, screen);

        platform_impl_->overlay_win = XCreateWindow(
            platform_impl_->display, root,
            rect.x, rect.y, static_cast<unsigned int>(rect.width), static_cast<unsigned int>(rect.height),
            0, CopyFromParent, InputOutput, CopyFromParent,
            CWOverrideRedirect | CWBackPixel, &attrs
        );

        if (platform_impl_->overlay_win) {
            XMapRaised(platform_impl_->display, platform_impl_->overlay_win);
            XFlush(platform_impl_->display);
        }
    }
#endif
}

void PrivacyTriggerManager::trigger_soft_alert(const std::string& message) {
    if (soft_alert_active_) return;
    soft_alert_active_ = true;

    std::cout << "\033[1;33m[SOFT ALERT] " << message << "\033[0m" << std::endl;

#if defined(__linux__)
    int ret = std::system("notify-send -u critical -t 2000 '🛡️ Blindside Privacy Warning' 'Unrecognized face looking at screen!' 2>/dev/null &");
    (void)ret;
#elif defined(_WIN32)
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
    if (soft_alert_active_ || hard_defense_active_ || targeted_blur_active_) {
        std::cout << "[PrivacyTrigger] Clearing active privacy alerts and overlays." << std::endl;
        soft_alert_active_ = false;
        hard_defense_active_ = false;
        targeted_blur_active_ = false;

#if defined(_WIN32)
        if (platform_impl_->overlay_hwnd) {
            DestroyWindow(platform_impl_->overlay_hwnd);
            platform_impl_->overlay_hwnd = nullptr;
        }
#elif defined(__linux__)
        if (platform_impl_->display && platform_impl_->overlay_win != 0) {
            XDestroyWindow(platform_impl_->display, platform_impl_->overlay_win);
            platform_impl_->overlay_win = 0;
            XFlush(platform_impl_->display);
        }
#endif
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
        WindowRect active_rect = const_cast<PrivacyTriggerManager*>(this)->get_active_window_geometry();
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

} // namespace blindside
