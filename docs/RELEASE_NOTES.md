# Blindside V3.1.0 — Release Notes

## Overview
Blindside V3.1.0 is the first field-tested, post-release iteration following the V3.0 milestone. This release focuses on diagnostic transparency, engine validation visibility, and camera pipeline stability across Linux and Windows desktop environments.

## What's New in V3.1.0

### Diagnostic Observability
- **Explicit Raw vs Validated Detections**: Standard output diagnostics now clearly distinguish raw YuNet candidate detections from validated primary and secondary faces.
- **Detailed Face Count Summaries**: Diagnostics output comprehensive breakdowns per frame, detailing total validated faces, primary user presence, secondary candidate count, and raw detection totals.
- **Transparent Liveness Reporting**: Diagnostic logs explicitly report the active liveness verification status and note architectural bounds.

### Pipeline & Core Stability
- **Camera Recovery Integration**: Preserves thread-safe non-busy-loop recovery for GStreamer and OpenCV camera capture streams.
- **Zero-Allocation Runtime**: Retains high-performance C++20 RingBuffer frame management and 6D `cv::solvePnP` pose estimation.
- **Hysteresis Privacy Triggers**: Preserves strict 1.0-second gaze hysteresis and multi-platform defense responses (targeted window blur & desktop session lock).

---

## Known Limitations

As part of our commitment to transparent open-source engineering, the following limitations are actively documented for V3.1.0:

1. **Rendered / Displayed Face False Positives**:
   - Faces rendered within digital images, posters, or web content (e.g., Pinterest boards, advertisements, video thumbnails) can currently be misclassified by the detection layer as physical secondary faces. Geometry bounds alone do not distinguish physical human faces from rendered image content.

2. **Global Liveness Evaluation**:
   - Liveness verification in V3.1.0 is evaluated over global secondary gaze history rather than independently established per individual secondary face track. Dedicated per-track liveness verification architecture is planned for V3.2.

3. **Wayland Display Server Protocol Limits**:
   - Pure Wayland compositors restrict global pixel access and custom window redaction overlays. Blindside gracefully falls back to system session locks (`loginctl`) on Wayland.
