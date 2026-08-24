# 🛡️ Technical Threat Model: Blindside Physical Privacy Daemon

## 1. Executive Overview & Scope

**Blindside** is an edge-native, zero-latency physical security daemon for desktop and mobile workstations. While modern cybersecurity posture heavily focuses on network perimeter defenses, endpoint detection and response (EDR), and identity management, **Visual Social Engineering**—specifically shoulder surfing and visual eavesdropping—remains one of the highest-yield physical attack vectors against sensitive environments (e.g., open offices, coffee shops, financial trading floors, executive desktop spaces).

This document establishes the threat model for visual eavesdropping attacks, maps physical security controls to **NIST SP 800-53 (Rev. 5)** and **ISO/IEC 27001:2022**, and details how Blindside's computer vision and C++20 background runtime mitigate these risks without compromising user privacy.

---

## 2. Threat Actor & Attack Vectors

```
  [ Attacker / Shoulder Surfer ]
               │
               ▼ (Line-of-Sight / Optical Capture)
    ┌─────────────────────┐
    │  Workstation Screen │ ◄── [ Primary User ] (Calibrated Face / Position)
    └─────────────────────┘
               ▲
               │ (Continuous 30 FPS / 5 FPS Spatial Pose Scanning)
    ┌─────────────────────┐
    │   Webcam Sensor     │
    └──────────┬──────────┘
               │
               ▼
   [ Blindside Edge Vision Engine ] ──► [ Soft Alert / OS Hard Workstation Lock ]
```

### Threat Taxonomy

| Threat ID | Threat Category | Attack Vector | Impact Level | Mitigated by Blindside |
| :--- | :--- | :--- | :--- | :--- |
| **THREAT-01** | Visual Shoulder Surfing | Unrecognized bystander looks directly over user's shoulder at displayed confidential data. | **CRITICAL** (PII / Secrets Exfiltration) | **YES** (Gaze vector calculation & hard defense lock > 1.0s) |
| **THREAT-02** | Optical Capture / Unauthorized Photography | Passerby aims smartphone/camera lens at screen while standing in background. | **HIGH** (Persistent Data Theft) | **YES** (Secondary face gaze detection & screen blur overlay) |
| **THREAT-03** | Physical Absence / Abandoned Workstation | User steps away from workstation without manually invoking screen lock (`Win+L`). | **HIGH** (Unauthorized Physical Access) | **YES** (Primary user presence spatial drift detection) |
| **THREAT-04** | Biometric / Camera Telemetry Leakage | Malicious third-party background process or cloud service intercepts webcam feed. | **CRITICAL** (Privacy / Surveillance Risk) | **YES** (100% Local C++ ONNX inference; zero network sockets) |

---

## 3. Compliance & Physical Security Standards Mapping

Blindside directly satisfies requirements across major cybersecurity and physical compliance frameworks:

### NIST SP 800-53 Rev. 5 Controls
- **PE-3 (Physical Access Control)**: Enforces real-time access restriction to physical display surfaces when unauthorized personnel enter line-of-sight bounds.
- **PE-6 (Monitoring Physical Access)**: Continuously monitors physical proximity and visual orientation of individuals in the workstation perimeter.
- **AC-11 (Device Lock)**: Automated initiating of OS workstation lock (`LockWorkStation()` / `loginctl lock-session`) upon confirmation of secondary gaze > 1.0s.

### ISO/IEC 27001:2022 Controls
- **Control A.7.7 (Clear Desk and Clear Screen)**: Automates clear-screen enforcement when secondary visual observation is detected.
- **Control A.7.1 (Physical Security Perimeters)**: Extends logical access control to the immediate physical radius of the endpoint.

---

## 4. Architecture & Technical Mitigation Strategy

### A. Edge-Native C++ Processing (Zero Data Exfiltration)
- **Zero-Cloud Dependency**: All spatial face detection and 3D head pose estimations execute locally via standard C++20 and ONNX Runtime / OpenCV.
- **Zero Disk Buffer for Frames**: Frames pass through a fixed, pre-allocated `RingBuffer<RawFrame, 4>`. No video frames are ever written to disk or transmitted over network interfaces.

### B. Spatial & 3D Gaze Estimation
1. **Primary User Calibration**: Registers primary user center position $(X_{center}, Y_{center})$ directly in front of display.
2. **Head Pose Vector Computation**:
   \[
   \vec{g} = \begin{pmatrix} \sin(\text{yaw}) \cdot \cos(\text{pitch}) \\ -\sin(\text{pitch}) \\ \cos(\text{yaw}) \cdot \cos(\text{pitch}) \end{pmatrix}
   \]
3. **Eavesdropper Gaze Verification**: Calculates if secondary face gaze vector $\vec{g}$ falls within display normal boundaries ($\pm 30^\circ$ yaw, $\pm 25^\circ$ pitch).

### C. Hysteresis & False Positive Reduction
- **Temporal Filter**: Secondary gaze must persist continuously for $> 1.0$ second (`config.hysteresis_sec`) before elevating from a Soft Alert (notification/edge glow) to a Hard Defense (blurred overlay / OS workstation lock).

### D. Low-Power Hardware Footprint
- **Adaptive Frame Rate Sampling**:
  - **Active State (30 FPS)**: Engaged when secondary movement or multiple faces are detected.
  - **Idle State (5 FPS)**: Automatically drops frame rate when only the primary user is steadily present, maintaining CPU footprint $< 2\%$.

---

## 5. Security Audit Logging Schema

Blindside maintains an immutable audit log (`blindside_threats.log`) adhering to SIEM ingestion standards:

```log
2026-07-30 20:24:45 [AUDIT_ALERT] Threat=HARD_EAVESDROPPER_GAZE GazeDurationSec=1.20 FacesDetected=2 Control=NIST_SP_800_53_PE_3
```

---

## 6. Summary & References

By shifting physical security monitoring directly onto edge C++ runtime hardware, Blindside eliminates the attack vector of shoulder surfing without introducing cloud privacy risks or hardware battery drain.

- **Engineering Benchmarks**: Refer to [docs/BENCHMARKS.md](docs/BENCHMARKS.md) for full test environment telemetry and latency measurements.
- **Vulnerability Disclosure Policy**: Refer to [SECURITY.md](SECURITY.md) for private reporting procedures.
- **Contribution Guidelines**: Refer to [CONTRIBUTING.md](CONTRIBUTING.md) for developer standards.

