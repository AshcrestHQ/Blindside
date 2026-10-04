# 🖥️ Platform Support & OS Integration Guide

> Detailed specifications, capabilities, and behavioral contracts for Blindside's native platform backends.

Blindside interfaces directly with operating system window managers, display servers, and session controllers to execute privacy responses. Because windowing models differ dramatically across Windows, Linux X11, and Linux Wayland, Blindside provides discrete capability reporting and tailored platform implementations.

---

## 1. Core Platform Capability Model

Blindside avoids binary "supported / unsupported" generalizations. Instead, the runtime queries discrete OS capabilities via `PlatformManager::get_capabilities()`:

| Capability Flag | Description | Windows | Linux X11 | Linux Wayland |
| :--- | :--- | :---: | :---: | :---: |
| `screen_redaction` | Can spawn an opaque or translucent privacy overlay over workspace windows | **Yes** | **Yes** | **No** |
| `targeted_blur` | Native compositor-backed shader blur of background window content | **No** | **No** | **No** |
| `session_lock` | OS workstation lock primitive (`LockWorkStation`, `loginctl`) | **Yes** | **Yes** | **Fallback** |
| `click_through_overlay` | Overlay window allows mouse clicks and scroll events to pass through | **Yes** | **No** | **No** |
| `multi_monitor_aware` | Correctly positions overlays across multi-monitor virtual screen bounds | **Yes** | **Limited** | **No** |

---

## 2. Redaction vs. Targeted Blur

> [!IMPORTANT]
> **Terminology Rule:**
> - **Redaction / Screen Redaction**: Rendering a dark, high-opacity translucent or opaque overlay over window bounds to obscure sensitive text and graphics.
> - **Targeted Blur**: Applying a real-time Gaussian or dual-kawase blur filter shader to pixels rendered behind the window.

### Why Blindside Does Not Claim "Targeted Blur"
Early prototypes referenced `DwmEnableBlurBehindWindow`. On modern Windows (Windows 10 1803+ and Windows 11), Microsoft deprecated `DwmEnableBlurBehindWindow` into an ineffective no-op. Real-time background blur now requires XAML composition brushes or DirectComposition pixel shaders, which introduce heavy dependencies and latency.

On modern systems:
- **Windows**: Blindside uses **Alpha Redaction** (`WS_EX_LAYERED` with alpha 225/255, ~88% opacity dark mask).
- **Linux X11**: Blindside uses **Opaque Redaction** (solid black override-redirect surface).
- **Linux Wayland**: Window overlays are prohibited by protocol design; Blindside falls back to system notifications and session locks.

Blindside's capability diagnostics honestly report `targeted_blur = false` across all platforms.

---

## 3. Windows Backend (`src/platform/windows.cpp`)

### Architecture & Window Model
- **Registered Class**: Registers a dedicated window class `BlindsidePrivacyOverlay` with `CS_HREDRAW | CS_VREDRAW` and `BLACK_BRUSH`.
- **Extended Window Styles**:
  ```c
  WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE
  ```
  - `WS_EX_LAYERED`: Enables alpha blending via `SetLayeredWindowAttributes`.
  - `WS_EX_TRANSPARENT`: Tells Windows this window does not accept mouse clicks until underlying windows have hit-tested.
  - `WS_EX_TOPMOST`: Ensures the redaction overlay floats above standard workspace windows.
  - `WS_EX_TOOLWINDOW`: Prevents the overlay from appearing in the taskbar or `Alt+Tab` switcher.
  - `WS_EX_NOACTIVATE`: Guarantees the overlay never steals active focus when spawned or repositioned.

### Guaranteed Click-Through via Hit-Testing
To guarantee that user productivity is never interrupted while redaction is active, `OverlayWndProc` intercepts the non-client hit-test message:
```cpp
case WM_NCHITTEST:
    return HTTRANSPARENT;
```
Returning `HTTRANSPARENT` instructs the Windows window manager to pass all mouse clicks, drags, hover events, and scroll wheel ticks straight through to the underlying application.

### High-DPI & Multi-Monitor Geometry
- **DPI Awareness**: At initialization, Blindside dynamically loads `user32.dll` to establish `DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2`. On older Windows releases, it falls back to `shcore.dll` (`PROCESS_PER_MONITOR_DPI_AWARE`).
- **Virtual Desktop Negative Coordinates**: Multi-monitor setups where secondary monitors are positioned to the left of or above the primary display have negative origin coordinates ($x < 0$ or $y < 0$).
  Blindside calculates fallback screen bounds using:
  - `x = GetSystemMetrics(SM_XVIRTUALSCREEN)`
  - `y = GetSystemMetrics(SM_YVIRTUALSCREEN)`
  - `width = GetSystemMetrics(SM_CXVIRTUALSCREEN)`
  - `height = GetSystemMetrics(SM_CYVIRTUALSCREEN)`
  Negative offsets are fully supported without integer underflow or invalid window placement.

### Non-Blocking Message Pumping
Layered Win32 windows require message queue servicing; otherwise, the Windows Desktop Window Manager (DWM) flags the application thread as unresponsive.
Blindside implements a non-blocking pump called once per frame:
```cpp
void pump_events() override {
    if (!overlay_hwnd_) return;
    MSG msg;
    while (PeekMessageA(&msg, overlay_hwnd_, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
```
This loop drains pending messages in $< 0.1\text{ms}$ without stalling the computer vision detection loop.

### Workstation Lock
Calls the native Win32 API:
```cpp
LockWorkStation();
```

---

## 4. Linux X11 Backend (`src/platform/linux_x11.cpp`)

### Architecture & Window Model
- **Override-Redirect Window**: Creates an X11 window with `attrs.override_redirect = True`. This bypasses the window manager (Mutter, KWin, i3, etc.) so the overlay displays instantly without window decorations or animations.
- **Background**: Solid black fill using `BlackPixel(display_, screen)`.
- **Idempotent Repositioning**: If the active window geometry changes, Blindside calls `XMoveResizeWindow()` and `XRaiseWindow()` on the existing XID rather than destroying and recreating windows.

### Non-Blocking Event Servicing
To prevent X11 socket buffer saturation:
```cpp
void pump_events() override {
    if (!display_) return;
    while (XPending(display_) > 0) {
        XEvent ev;
        XNextEvent(display_, &ev);
    }
}
```

### Known X11 Limitations
1. **Click-Through**: Standard X11 override-redirect windows consume pointer events. Transparent input pass-through requires the XShape extension (`XShapeCombineRectangles` with `ShapeInput`), which is not enabled by default in standard builds (`click_through_overlay = false`).
2. **ScreenCount vs Physical Monitors**: `ScreenCount(display_)` reports the number of X Screens. On modern multi-monitor Linux setups using XRandR or Xinerama, all physical monitors are unified into a single virtual X Screen (Screen 0). Diagnostics clarify that `monitor_count` reflects X screens rather than physical display outputs.

### Workstation Lock
Executes a cascade of standard desktop locking mechanisms:
```bash
loginctl lock-session 2>/dev/null || xset ss activate 2>/dev/null || dbus-send --type=method_call --dest=org.gnome.ScreenSaver /org/gnome/ScreenSaver org.gnome.ScreenSaver.Lock 2>/dev/null &
```

---

## 5. Linux Wayland Backend (`src/platform/linux_wayland.cpp`)

### Protocol Security Restrictions
Wayland was designed from the ground up with strict security isolation between applications:
- Unprivileged clients cannot query the global bounding box or focus status of other application windows.
- Unprivileged clients cannot create arbitrary surfaces with absolute screen coordinates floating over other windows without compositor-specific extensions (e.g. `wlr-layer-shell`).

### Honest Capability Declaration
Blindside does not pretend generic Wayland supports screen overlays:
```cpp
caps.screen_redaction = false;
caps.targeted_blur = false;
caps.click_through_overlay = false;
caps.multi_monitor_aware = false;
caps.session_lock = true; // Attempted via system session brokers
```

### Graceful Fallback Mechanics
When a privacy threat is detected under Wayland:
1. **Targeted Redaction Fallback**: Degrades safely to a high-urgency desktop notification via `notify-send`:
   ```bash
   notify-send -u critical -t 2000 '🛡️ Blindside Privacy Warning' 'Unrecognized face looking at screen!'
   ```
2. **Session Lock**: Dispatches standard compositor-agnostic session lock requests:
   ```bash
   loginctl lock-session 2>/dev/null || dbus-send --type=method_call --dest=org.gnome.ScreenSaver /org/gnome/ScreenSaver org.gnome.ScreenSaver.Lock 2>/dev/null &
   ```
   *Note: Session lock requires a running systemd login manager (`systemd-logind`) or a compatible D-Bus screensaver service.*

---

## 6. Diagnostic Inspection Example

To verify how Blindside detects your current platform capabilities, execute the daemon with the `--diagnostics` flag:

```bash
# Linux
./build/blindside_daemon --diagnostics

# Windows
.\build\Release\blindside_daemon.exe --diagnostics
```

Example output (Windows 11):
```text
[Platform] OS: Windows
[Platform] Native Redaction: Supported
[Platform] Targeted Blur: Unsupported (AlphaRedaction in use)
[Platform] Redaction Mode: AlphaRedaction (Universal High-Opacity Mask)
[Platform] Screen Lock: Supported
[Platform] Click-Through: Supported
[Platform] Monitor Count: 2
```
