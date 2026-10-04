# 🏗️ Blindside Architecture

> Deep technical specification and pipeline reference for Blindside.

Blindside is an edge-native, local-first physical privacy daemon designed to protect computer screens from unauthorized visual eavesdropping ("shoulder surfing").

This document describes the system architecture, processing pipeline, layer boundaries, and failure modes.

---

## 1. System Overview & Principles

```text
Camera Sensor
     │
     ▼
┌─────────────────────────────────────────────────────────────┐
│ Core Computer Vision Pipeline                               │
│                                                             │
│   CameraCapture ──► FaceDetector (YuNet ONNX)               │
│                            │                                │
│                            ▼                                │
│                     PoseEstimator (cv::solvePnP)            │
│                            │                                │
│                            ▼                                │
│                     FaceTracker (Temporal Identity)         │
│                            │                                │
│                            ▼                                │
│                     LivenessEngine (Micro-motion / EAR)     │
│                            │                                │
│                            ▼                                │
│                     ThreatEngine (Spatial Gaze / Dwell)     │
└────────────────────────────┬────────────────────────────────┘
                             │
                             ▼ ThreatAssessment / FrameResult
┌─────────────────────────────────────────────────────────────┐
│ Response & Platform Abstraction Layer                       │
│                                                             │
│   PrivacyTriggerManager                                     │
│          │                                                  │
│          ▼                                                  │
│   PlatformManager                                           │
│          ├─► Windows (Alpha Redaction Overlay, Lock)        │
│          ├─► Linux X11 (Override-Redirect Redaction, Lock)  │
│          └─► Linux Wayland (Notification & Lock Fallback)   │
└─────────────────────────────────────────────────────────────┘
```

### Core Design Rules
1. **Unidirectional Dependency Flow**: Detection and threat evaluation never depend on platform response code. Platform implementations never perform computer vision or threat evaluation.
2. **Local-First & Ephemeral**: Video frames are processed in-memory and immediately discarded. No camera frames, cropped faces, or biometric vectors are ever saved to disk or transmitted across network interfaces.
3. **Zero-Allocation Hot Path**: Once initialized, frame cycling and rolling history buffers operate on fixed-capacity containers (`RingBuffer`, `std::array`).
4. **Honest Capability Reporting**: Platform layers expose genuine OS primitives. Alpha redaction overlays are never mislabeled as compositor blur, and protocol-restricted environments (such as Wayland) explicitly report missing capabilities.

---

## 2. Processing Pipeline

### Frame Ingestion (`src/core/camera.cpp`)
- `CameraCapture` manages the hardware video stream via OpenCV `cv::VideoCapture` (V4L2 on Linux, Media Foundation / DirectShow on Windows).
- Uses a fixed-capacity `RingBuffer<RawFrame, 4>` to prevent memory allocations per frame.
- **Resilience & Fallback**: If the hardware webcam is disconnected or unavailable, the capture pipeline safely falls back to a synthetic frame generator (`SyntheticCameraFallback`), preventing daemon crashes.

### Face Detection (`src/engine/face_detector.cpp`)
- Employs OpenCV's **YuNet** ONNX model (`face_detection_yunet_2023mar.onnx`), an ultra-lightweight, high-speed convolutional neural network tailored for edge face detection.
- Produces:
  - Bounding box (`x, y, width, height`)
  - Detection confidence score (thresholded at `min_confidence`, default 0.60)
  - 5 facial landmarks: right eye, left eye, nose tip, right mouth corner, left mouth corner.

### Pose & Gaze Estimation (`src/engine/pose_estimator.cpp`)
- Solves the Perspective-n-Point problem using `cv::solvePnP`.
- Maps 2D image coordinates from the 5 YuNet landmarks against a canonical 3D anthropometric face model.
- Derives 6-degree-of-freedom head pose:
  - **Pitch** (degrees, up/down rotation)
  - **Yaw** (degrees, left/right rotation)
  - **Roll** (degrees, head tilt)
  - **Gaze Vector**: Normalized 3D unit vector extending outward from the face center.
- **Screen Gaze Bounds**: Gaze is classified as looking at the screen when:
  $$\text{Pitch} \in [-25^\circ, +25^\circ] \quad \text{and} \quad \text{Yaw} \in [-30^\circ, +30^\circ]$$

### Temporal Tracking
- Assigns persistent identities across consecutive frames to distinguish:
  - **Primary User**: Calibrated at startup based on centered screen position and dominant bounding box area.
  - **Secondary Faces**: Candidates detected in the background or periphery.
- Tracks transition through lifecycle states: `New` $\to$ `Tentative` $\to$ `Confirmed` $\to$ `Lost` $\to$ `Expired`.
- Prevents spurious single-frame detections from triggering privacy responses.

### Liveness Verification
- Distinguishes physical human presence from static photographic spoofing (e.g. photos, printed posters, or digital portraits).
- Uses a rolling fixed circular buffer of recent pose observations to evaluate:
  1. **Rotational Micro-motion**: Evaluates pitch and yaw variance over time. A live subject displays natural involuntary micro-movements, whereas a static photo exhibits near-zero angular variance.
  2. **Eye Aspect Ratio (EAR)**: Computes eye openness ratios. Variations confirm natural blink activity.
- Tracks lacking sufficient motion variance or blink evidence are marked as `SpoofStatic`.

### Threat Assessment
- Evaluates persistent secondary gaze directed at the user's display.
- **Hysteresis Window**: Transient glances or passersby do not trigger disruption. A secondary face must maintain a sustained gaze toward the screen exceeding the hysteresis threshold (default: `1.0s` via `config.hysteresis_sec`).
- Elevation levels:
  - **Benign**: Primary user or secondary face looking away.
  - **Candidate**: Secondary gaze directed at screen, but within hysteresis threshold ($< 1.0\text{s}$). Triggers advisory soft alerts.
  - **Active Eavesdropper**: Confirmed secondary gaze holding $> 1.0\text{s}$. Triggers hard privacy responses (workspace redaction or session lock).

---

## 3. Response & Platform Layer

### Privacy Trigger Manager (`src/trigger/privacy_trigger.cpp`)
`PrivacyTriggerManager` serves as the mediator between the detection engine and the operating system:
- Receives immutable threat assessment results (`const FrameResult&`).
- Evaluates daemon configuration (`config.daemon_state`):
  - `DaemonState::GracefulTargetedBlur`: Requests active workspace window redaction.
  - `DaemonState::StrictFullLock`: Requests OS workstation lock.
- Coordinates overlay lifecycle: triggers targeted overlays when threats appear, repositions overlays if active windows move, and clears overlays when the space is clear.
- Drains the non-blocking platform message loop (`pump_events()`) every frame to maintain UI thread responsiveness.
- Writes structured compliance audit logs to `blindside_threats.log`.

### Platform Manager Abstraction (`include/blindside/platform.hpp`)
`PlatformManager` defines the pure virtual interface for native OS integrations:

```cpp
class PlatformManager {
public:
    virtual ~PlatformManager() = default;
    virtual bool initialize() = 0;
    virtual PlatformDiagnostics get_diagnostics() const = 0;
    virtual PlatformCapabilities get_capabilities() const = 0;
    virtual WindowRect get_active_window_geometry() = 0;
    virtual void trigger_targeted_blur(const WindowRect& rect) = 0;
    virtual void trigger_soft_alert(const std::string& message) = 0;
    virtual void trigger_hard_defense(const std::string& message) = 0;
    virtual void clear_alerts() = 0;
    virtual void pump_events() {}
    static std::unique_ptr<PlatformManager> create();
};
```

---

## 4. Platform Backends

### Windows (`src/platform/windows.cpp`)
- **Overlay Window**: Creates a top-level popup using a registered custom Win32 class `BlindsidePrivacyOverlay`.
- **Extended Styles**: `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE`.
- **Input Pass-Through**: Handles `WM_NCHITTEST` returning `HTTRANSPARENT`. Combined with `WS_EX_TRANSPARENT`, all mouse clicks, movements, and scroll events pass through to the underlying application.
- **Focus Safety**: `WS_EX_NOACTIVATE` and `SWP_NOACTIVATE` prevent the overlay from stealing keyboard focus.
- **Redaction Mode**: High-opacity alpha mask (`LWA_ALPHA` = 225/255, ~88% opacity). Honestly reports `targeted_blur = false`.
- **DPI & Multi-Monitor**: Initializes Per-Monitor DPI Awareness V2. Fallback geometry uses `SM_XVIRTUALSCREEN`, `SM_YVIRTUALSCREEN`, `SM_CXVIRTUALSCREEN`, and `SM_CYVIRTUALSCREEN` to support displays placed left of or above the primary monitor (negative virtual coordinates).
- **Message Loop**: Non-blocking `PeekMessageA` pump drains Win32 message queues per frame to avoid DWM thread freezing.
- **Session Lock**: Calls Win32 `LockWorkStation()`.

### Linux X11 (`src/platform/linux_x11.cpp`)
- **Overlay Window**: Creates an unmanaged X11 window with `override_redirect = True` and a solid `BlackPixel` background.
- **Repositioning**: Idempotently repositions existing windows via `XMoveResizeWindow` and `XRaiseWindow` without creating duplicate window handles.
- **Event Draining**: Non-blocking `XPending` / `XNextEvent` loop in `pump_events()` discards pending server events.
- **Session Lock**: Executes standard desktop session lock commands (`loginctl lock-session`, `xset ss activate`, or D-Bus screensaver lock).
- **Limitations**: Standard X11 override-redirect consumes mouse events without XShape masks (`click_through_overlay = false`). `ScreenCount()` reflects X11 screens, not individual XRandR physical heads.

### Linux Wayland (`src/platform/linux_wayland.cpp`)
- **Protocol Restrictions**: Pure Wayland security architecture explicitly restricts unprivileged clients from reading global active window coordinates or spawning absolute-positioned overlays over other applications.
- **Honest Capability Reporting**: Reports `screen_redaction = false`, `targeted_blur = false`, `click_through_overlay = false`, `multi_monitor_aware = false`.
- **Safe Fallback**: Overlay requests gracefully degrade to desktop notifications (`notify-send`) and compositor-agnostic workstation locking (`loginctl lock-session` / D-Bus screensaver). Never crashes or throws unhandled exceptions.

---

## 5. Diagnostic & Capability Specification

Applications and administrative scripts inspect native runtime capabilities via `get_capabilities()` and `get_diagnostics()`:

```cpp
struct PlatformCapabilities {
    bool screen_redaction = false;       // Can render an opaque or translucent privacy overlay
    bool targeted_blur = false;          // True DWM/compositor-backed blur shader
    bool session_lock = false;           // OS session lock primitive
    bool click_through_overlay = false; // Overlay passes mouse input to underlying application
    bool multi_monitor_aware = false;   // Can enumerate and position across virtual screen bounds
};
```

---

## 6. Audit Logging & Compliance

Blindside logs structured security alerts for compliance reporting (NIST SP 800-53 PE-3, ISO 27001 physical security domain):
- File: `blindside_threats.log`
- Format:
  ```text
  2026-10-04 09:30:15 [AUDIT_ALERT] Threat=TARGETED_WORKSPACE_BLUR GazeDurationSec=1.25 FacesDetected=2 LivenessVerified=1 Control=NIST_SP_800_53_PE_3
  ```
- **Privacy Assurance**: The log records *events and measurements* (gaze duration, face count, verification flags). It **never** writes image buffers, facial biometric vectors, or identity identifiers.

---

## 7. Testing & Verification Architecture

Blindside employs strict isolation between test tiers:
- **Platform Hardening Suite (`tests/test_platform.cpp`)**: Verifies 10 platform contracts (capability detection, overlay lifecycle, idempotent resizing, cleanup, unsupported fallbacks, invalid geometry, multi-monitor negative coordinates, resilience, and threat engine decoupling).
- **Pipeline & Math Tests**: Dedicated tests for ring buffers, gaze math, solver stability, and synthetic camera failure recovery.
- **Python Reference Suite (`tools/reference/test_blindside.py`)**: Independent algorithmic reference test suite validating synthetic camera fallback, gaze hysteresis, calibration, gaze vectors, and liveness.
