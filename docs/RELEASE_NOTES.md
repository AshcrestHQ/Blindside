# Blindside V4.0.0 — Release Notes

> For complete release history, see [../RELEASE_NOTES.md](../RELEASE_NOTES.md).

## Overview
Blindside V4.0.0 marks the formal generational release featuring **Platform Hardening**, transforming OS privacy responses into robust, input-safe, multi-monitor aware, and honestly reported platform primitives across Windows, Linux X11, and Linux Wayland.

## What's New in V4.0.0

### Discrete Platform Capability Model
- Introduced `PlatformCapabilities` and `PlatformDiagnostics` structs exposing explicit capability flags (`screen_redaction`, `targeted_blur`, `session_lock`, `click_through_overlay`, `multi_monitor_aware`).
- Replaced ambiguous "blur" claims with honest reporting: Windows and Linux X11 provide high-opacity **Screen Redaction**, not compositor-backed blur shaders (`targeted_blur = false`).

### Windows Platform Backend
- **Input Pass-Through**: Implemented `WM_NCHITTEST -> HTTRANSPARENT` alongside `WS_EX_TRANSPARENT`, guaranteeing that all mouse clicks and scroll wheel events pass through the overlay to underlying workspace applications.
- **Focus Safety**: Extended window styles with `WS_EX_NOACTIVATE` and `SWP_NOACTIVATE` to prevent the overlay from stealing keyboard focus.
- **Per-Monitor High-DPI Awareness V2**: Dynamic runtime initialization of `SetProcessDpiAwarenessContext` (`PER_MONITOR_AWARE_V2`) for crisp multi-monitor coordinate alignment.
- **Virtual Desktop Negative Coordinates**: Support for displays positioned to the left of (`x < 0`) or above (`y < 0`) the primary display via `SM_XVIRTUALSCREEN` and `SM_YVIRTUALSCREEN`.
- **Non-Blocking Message Pump**: Implemented `pump_events()` with `PeekMessageA` draining to eliminate DWM "Not Responding" thread freezing without stalling the CV detection loop.

### Linux X11 Backend
- **Override-Redirect Solid Redaction**: Instant solid black redaction surface bypassing window manager compositor lag.
- **Idempotent Repositioning**: Window moves and resizes use `XMoveResizeWindow` and `XRaiseWindow` on existing surfaces, eliminating XID handle leaks.
- **Non-Blocking Event Pump**: Drains server events with non-blocking `XPending` / `XNextEvent`.

### Linux Wayland Backend
- **Honest Protocol Restrictions**: Formally declares `screen_redaction = false` due to Wayland security boundaries restricting arbitrary global surface positioning.
- **Graceful Fallbacks**: Overlay requests safely degrade to high-priority desktop notifications (`notify-send`) and compositor-agnostic workstation locking (`loginctl lock-session` / D-Bus screensaver).

### Platform Hardening Test Suite
- Registered 10 dedicated test scenarios under CTest (`tests/test_platform.cpp`) validating capability detection, overlay lifecycles, idempotent repositioning, clean deactivation, unsupported fallbacks, invalid geometries, negative multi-monitor offsets, headless resilience, threat/platform separation, and state immutability.

---

## Blindside V3.1.0 — Historical Release Baseline (2026-09-20)

### Diagnostic Observability
- **Explicit Raw vs Validated Detections**: Standard output diagnostics clearly distinguish raw YuNet candidate detections from validated primary and secondary faces.
- **Detailed Face Count Summaries**: Diagnostics output comprehensive breakdowns per frame, detailing total validated faces, primary user presence, secondary candidate count, and raw detection totals.
- **Transparent Liveness Reporting**: Diagnostic logs explicitly report the active liveness verification status and note architectural bounds.

### Pipeline & Core Stability
- **Camera Recovery Integration**: Preserves thread-safe non-busy-loop recovery for GStreamer and OpenCV camera capture streams.
- **Zero-Allocation Runtime**: Retains high-performance C++20 RingBuffer frame management and 6D `cv::solvePnP` pose estimation.
- **Hysteresis Privacy Triggers**: Preserves strict 1.0-second gaze hysteresis and multi-platform defense responses (targeted window blur & desktop session lock).
