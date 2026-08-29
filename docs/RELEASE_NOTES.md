# Blindside V2 — Release Candidate

## What's new
Blindside V2 is a complete rewrite of the initial prototype, transforming it from an experimental concept into a stable, demonstrable, local-first physical privacy daemon. 

## The big changes
- We stripped out the hardcoded boundary boxes and unreliable heuristics.
- Integrated a production-ready C++20 `cv::solvePnP` threat engine that accurately maps facial orientation.
- Implemented a rolling 3-second liveness hysteresis window to eliminate erratic jitter.

## Cross-platform support
- **Windows**: Native support for redaction overlays and `LockWorkStation`, with deployment-ready packaging for all required dependencies (OpenCV).
- **Linux X11**: Native support for redaction overlays covering active windows.
- **Linux Wayland**: Added as a graceful fallback mode (triggering session locks) since global compositing restrictions block third-party redaction overlays.

## CV pipeline
- Built around the OpenCV YuNet face detection model, offering high-speed performance at standard resolutions (640x480).
- Hand-tuned 6D Pose Estimation logic that safely catches math degeneracies instead of crashing.
- Zero-allocation runtime execution loop using a custom lock-free RingBuffer to handle image data.

## Privacy
- Removed all network telemetry.
- Replaced ambiguous logging with a structured local security event ledger (`blindside_threats.log`).
- Ensures absolute privacy by refusing to persist any screenshots, frames, or biometric templates to disk.

## Testing
- Added regression tests specifically targeting transient occlusion drops and edge-case pose geometry.
- Verified pipeline safety with model-backed integration runs in CI.

## Known limitations
- Wayland environments cannot utilize targeted redaction overlays and will fall back to system locking.
- Heavy occlusions (large masks, dark sunglasses) or extreme low-light environments will impair YuNet's ability to detect landmarks, blinding the daemon.
- Local physical hardware tests are actively pending final validation for both X11 runtime and Windows runtime environments.
