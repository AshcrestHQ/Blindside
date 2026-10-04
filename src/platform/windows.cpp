#include "blindside/platform.hpp"
#include <iostream>
#include <windows.h>
#include <dwmapi.h>

namespace blindside {

namespace {

const char* OVERLAY_CLASS_NAME = "BlindsidePrivacyOverlay";

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1; // Handled by DefWindowProc/black brush
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HBRUSH brush = (HBRUSH)GetStockObject(BLACK_BRUSH);
            FillRect(hdc, &ps.rcPaint, brush);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_NCHITTEST:
            return HTTRANSPARENT; // Explicitly pass all mouse hit-tests through overlay to underlying window
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

} // namespace

class WindowsPlatformManager : public PlatformManager {
public:
    WindowsPlatformManager() = default;

    ~WindowsPlatformManager() {
        clear_alerts();
        if (class_registered_) {
            UnregisterClassA(OVERLAY_CLASS_NAME, GetModuleHandleA(NULL));
            class_registered_ = false;
        }
    }

    bool initialize() override {
        // Establish Per-Monitor DPI Awareness V2 for mixed-DPI multi-monitor precision
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32) {
            typedef BOOL(WINAPI *SetDpiContextProc)(DPI_AWARENESS_CONTEXT);
            auto setDpiContext = (SetDpiContextProc)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
            if (setDpiContext) {
                setDpiContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            } else {
                HMODULE shcore = LoadLibraryA("shcore.dll");
                if (shcore) {
                    typedef HRESULT(WINAPI *SetDpiAwareProc)(int);
                    auto setDpiAware = (SetDpiAwareProc)GetProcAddress(shcore, "SetProcessDpiAwareness");
                    if (setDpiAware) {
                        setDpiAware(2); // PROCESS_PER_MONITOR_DPI_AWARE
                    }
                    FreeLibrary(shcore);
                } else {
                    typedef BOOL(WINAPI *SetProcessDPIAwareProc)(VOID);
                    auto setOldDpi = (SetProcessDPIAwareProc)GetProcAddress(user32, "SetProcessDPIAware");
                    if (setOldDpi) {
                        setOldDpi();
                    }
                }
            }
        }

        // Register custom Win32 overlay class
        WNDCLASSEXA wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = OverlayWndProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = OVERLAY_CLASS_NAME;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

        if (RegisterClassExA(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
            class_registered_ = true;
        }

        return true;
    }

    PlatformCapabilities get_capabilities() const override {
        PlatformCapabilities caps;
        caps.screen_redaction = true;
        caps.targeted_blur = false; // DwmEnableBlurBehindWindow is legacy/no-op on Win10/Win11 DWM; honestly report alpha redaction
        caps.session_lock = true;
        caps.click_through_overlay = true;
        caps.multi_monitor_aware = true;
        return caps;
    }

    PlatformDiagnostics get_diagnostics() const override {
        PlatformDiagnostics diag;
        diag.os_name = "Windows";
        diag.supports_native_redaction = true;
        diag.supports_targeted_blur = false; // Honestly distinguishes alpha redaction from true compositor blur
        diag.supports_screen_lock = true;
        diag.supports_desktop_notifications = true;
        diag.supports_click_through = true;
        diag.monitor_count = GetSystemMetrics(SM_CMONITORS);
        diag.redaction_mode = "AlphaRedaction (Universal High-Opacity Mask)";
        diag.capabilities = get_capabilities();
        return diag;
    }

    WindowRect get_active_window_geometry() override {
        WindowRect rect;
        HWND hwnd = GetForegroundWindow();
        if (hwnd && hwnd != overlay_hwnd_) {
            RECT r;
            if (GetWindowRect(hwnd, &r)) {
                rect.x = r.left;
                rect.y = r.top;
                rect.width = r.right - r.left;
                rect.height = r.bottom - r.top;
                rect.valid = (rect.width > 0 && rect.height > 0);
            }
        }

        // Multi-monitor virtual desktop fallback handling negative coordinates
        if (!rect.valid) {
            rect.x = GetSystemMetrics(SM_XVIRTUALSCREEN);
            rect.y = GetSystemMetrics(SM_YVIRTUALSCREEN);
            rect.width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            rect.height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

            if (rect.width <= 0) rect.width = GetSystemMetrics(SM_CXSCREEN);
            if (rect.height <= 0) rect.height = GetSystemMetrics(SM_CYSCREEN);
            if (rect.width <= 0) rect.width = 1280;
            if (rect.height <= 0) rect.height = 800;
            rect.valid = true;
        }

        return rect;
    }

    void trigger_targeted_blur(const WindowRect& rect) override {
        // Validate geometry
        if (rect.width <= 0 || rect.height <= 0) {
            return;
        }

        // Overlay window creation
        if (!overlay_hwnd_) {
            const char* class_to_use = class_registered_ ? OVERLAY_CLASS_NAME : "STATIC";
            overlay_hwnd_ = CreateWindowExA(
                WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                class_to_use, "BlindsidePrivacyOverlay",
                WS_POPUP | WS_VISIBLE,
                rect.x, rect.y, rect.width, rect.height,
                NULL, NULL, GetModuleHandleA(NULL), NULL
            );

            if (overlay_hwnd_) {
                // High-opacity dark redaction mask (225/255 ~ 88% opacity, click-through)
                SetLayeredWindowAttributes(overlay_hwnd_, RGB(0, 0, 0), 225, LWA_ALPHA);
                SetWindowPos(overlay_hwnd_, HWND_TOPMOST, rect.x, rect.y, rect.width, rect.height,
                             SWP_SHOWWINDOW | SWP_NOACTIVATE);
                last_rect_ = rect;
            }
        } else {
            // Idempotent repositioning if geometry changed
            if (rect.x != last_rect_.x || rect.y != last_rect_.y ||
                rect.width != last_rect_.width || rect.height != last_rect_.height) {
                SetWindowPos(overlay_hwnd_, HWND_TOPMOST, rect.x, rect.y, rect.width, rect.height,
                             SWP_SHOWWINDOW | SWP_NOACTIVATE);
                last_rect_ = rect;
            }
        }

        pump_events();
    }

    void trigger_soft_alert(const std::string& /*message*/) override {
        MessageBeep(MB_ICONWARNING);
    }

    void trigger_hard_defense(const std::string& /*message*/) override {
        if (!LockWorkStation()) {
            DWORD err = GetLastError();
            std::cerr << "[Platform] LockWorkStation failed with error code: " << err << std::endl;
        }
    }

    void clear_alerts() override {
        if (overlay_hwnd_) {
            ShowWindow(overlay_hwnd_, SW_HIDE);
            DestroyWindow(overlay_hwnd_);
            overlay_hwnd_ = nullptr;
            last_rect_ = WindowRect{};
            pump_events();
        }
    }

    void pump_events() override {
        if (!overlay_hwnd_) return;
        MSG msg;
        while (PeekMessageA(&msg, overlay_hwnd_, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

private:
    HWND overlay_hwnd_ = nullptr;
    bool class_registered_ = false;
    WindowRect last_rect_{};
};

std::unique_ptr<PlatformManager> PlatformManager::create() {
    return std::make_unique<WindowsPlatformManager>();
}

} // namespace blindside
