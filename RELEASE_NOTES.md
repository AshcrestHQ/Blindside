# 🛡️ Blindside Release Notes

---

## Blindside V4 — Architecture & Platform Hardening (In Development / Unreleased)

> **Release Status**: In active development. The current formal release baseline remains **V3.1.0**.

The V4 generational engineering cycle establishes formal architectural boundaries separating Face Detection, Multi-Face Tracking, Liveness Verification, Stateless Threat Assessment, and Platform Response Management.

Phase 6 focused specifically on **Platform Hardening**, transforming OS privacy responses into robust, input-safe, multi-monitor aware, and honestly reported platform primitives across Windows, Linux X11, and Linux Wayland.

### Key Highlights of Phase 6 Platform Hardening

#### 1. Discrete Platform Capability Model
- Introduced `PlatformCapabilities` and `PlatformDiagnostics` structs exposing explicit capability flags:
  - `screen_redaction`
  - `targeted_blur`
  - `session_lock`
  - `click_through_overlay`
  - `multi_monitor_aware`
- Replaced ambiguous "blur" claims with honest reporting: Windows and Linux X11 provide high-opacity **Screen Redaction**, not compositor-backed blur shaders (`targeted_blur = false`).

#### 2. Windows Platform Backend (`src/platform/windows.cpp`)
- **Input Pass-Through**: Implemented `WM_NCHITTEST -> HTTRANSPARENT` alongside `WS_EX_TRANSPARENT`, guaranteeing that all mouse clicks and scroll wheel events pass through the overlay to underlying workspace applications.
- **Focus Safety**: Extended window styles with `WS_EX_NOACTIVATE` and `SWP_NOACTIVATE` to prevent the overlay from stealing keyboard focus.
- **Per-Monitor High-DPI Awareness V2**: Dynamic runtime initialization of `SetProcessDpiAwarenessContext` (`PER_MONITOR_AWARE_V2`) for crisp multi-monitor coordinate alignment.
- **Virtual Desktop Negative Coordinates**: Support for displays positioned to the left of (`x < 0`) or above (`y < 0`) the primary display via `SM_XVIRTUALSCREEN` and `SM_YVIRTUALSCREEN`.
- **Non-Blocking Message Pump**: Implemented `pump_events()` with `PeekMessageA` draining to eliminate DWM "Not Responding" thread freezing without stalling the CV detection loop.

#### 3. Linux X11 Backend (`src/platform/linux_x11.cpp`)
- **Override-Redirect Solid Redaction**: Instant solid black redaction surface bypassing window manager compositor lag.
- **Idempotent Repositioning**: Window moves and resizes use `XMoveResizeWindow` and `XRaiseWindow` on existing surfaces, eliminating XID handle leaks.
- **Non-Blocking Event Pump**: Drains server events with non-blocking `XPending` / `XNextEvent`.
- **Diagnostic Clarity**: Documented that `ScreenCount()` reflects X11 screens rather than physical XRandR heads.

#### 4. Linux Wayland Backend (`src/platform/linux_wayland.cpp`)
- **Honest Protocol Restrictions**: Formally declares `screen_redaction = false` due to Wayland security boundaries restricting arbitrary global surface positioning.
- **Graceful Fallbacks**: Overlay requests safely degrade to high-priority desktop notifications (`notify-send`) and compositor-agnostic workstation locking (`loginctl lock-session` / D-Bus screensaver).

#### 5. Platform Hardening Test Suite (`tests/test_platform.cpp`)
- Registered 10 dedicated test scenarios under CTest:
  1. Platform capability detection
  2. Repeated overlay lifecycle (10 cycles, zero leaks)
  3. Idempotent overlay activation & repositioning
  4. Safe cleanup after deactivation
  5. Unsupported capability fallback
  6. Invalid display geometry resilience
  7. Multi-monitor negative coordinate handling
  8. Platform resilience in headless environments
  9. Threat assessment independence from platform state
  10. Decoupled platform execution with zero threat engine mutation

---

## Blindside V3.1.0 — Formal Release Baseline (2026-09-20)

### Overview
Blindside V3.1.0 is the first field-tested, post-release iteration following the V3.0 milestone. This release focused on diagnostic transparency, engine validation visibility, and camera pipeline stability across Linux and Windows desktop environments.

### What Was New in V3.1.0

#### Diagnostic Observability
- **Explicit Raw vs Validated Detections**: Standard output diagnostics clearly distinguish raw YuNet candidate detections from validated primary and secondary faces.
- **Detailed Face Count Summaries**: Diagnostics output comprehensive breakdowns per frame, detailing total validated faces, primary user presence, secondary candidate count, and raw detection totals.
- **Transparent Liveness Reporting**: Diagnostic logs explicitly report the active liveness verification status and note architectural bounds.

#### Pipeline & Core Stability
- **Camera Recovery Integration**: Preserves thread-safe non-busy-loop recovery for GStreamer and OpenCV camera capture streams.
- **Zero-Allocation Runtime**: Retains high-performance C++20 RingBuffer frame management and 6D `cv::solvePnP` pose estimation.
- **Hysteresis Privacy Triggers**: Preserves strict 1.0-second gaze hysteresis and multi-platform defense responses (targeted window blur & desktop session lock).

#### Historical Limitations Documented for V3.1.0
1. **Rendered / Displayed Face False Positives**: Faces rendered within digital images, posters, or web content can be misclassified as physical secondary faces.
2. **Global Liveness Evaluation**: Liveness verification in V3.1.0 is evaluated over global secondary gaze history rather than independently established per individual secondary face track (addressed in V4 architectural development).
3. **Wayland Display Server Protocol Limits**: Pure Wayland compositors restrict global pixel access and custom window redaction overlays; falls back to system session locks (`loginctl`).
