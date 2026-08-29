# 🛡️ Threat Model: Blindside

Listen, everyone talks about zero-day exploits and network perimeters, but the most common data leak is some guy on a train staring at your laptop screen. Visual eavesdropping (or "shoulder surfing") is a massive vulnerability that most security tooling completely ignores.

Blindside fixes that at the edge. This document outlines exactly what we're protecting you from, how we do it, and what happens to your data (spoiler: nothing, because we don't save it).

---

## 2. Who Are We Protecting You From?

```
  [ Attacker / Rando on the train ]
               │
               ▼ (Line-of-Sight)
    ┌─────────────────────┐
    │  Your Laptop Screen │ ◄── [ You ] 
    └─────────────────────┘
               ▲
               │ (Continuous 30 FPS YuNet + SolvePnP Scanning)
    ┌─────────────────────┐
    │   Webcam Sensor     │
    └──────────┬──────────┘
               │
               ▼
    [ Blindside Daemon ] ──► [ Screen Redaction / Hard Lock ]
```

### Threat Taxonomy

| Threat ID | Threat Category | Attack Vector | Impact Level | Mitigated by Blindside? |
| :--- | :--- | :--- | :--- | :--- |
| **THREAT-01** | Visual Shoulder Surfing | A stranger looks directly over your shoulder at confidential code or data. | **CRITICAL** (Data Exfiltration) | **YES** (SolvePnP gaze vector math triggers hard lock > 1.0s) |
| **THREAT-02** | Optical Capture / Photos | Someone points a smartphone camera at your screen while standing behind you. | **HIGH** (Persistent Data Theft) | **YES** (Secondary face detection triggers active window redaction) |
| **THREAT-03** | Abandoned Workstation | You step away from your desk and forget to hit `Win+L`. | **HIGH** (Physical Access) | **YES** (We detect primary user absence) |
| **THREAT-04** | Cloud Telemetry Leakage | A "security tool" uploads your face to an AWS bucket for "processing". | **CRITICAL** (Privacy Nightmare) | **YES** (100% Local OpenCV inference. Zero network sockets open.) |

---

## 3. How We Actually Stop It

### A. Edge-Native Processing (No Cloud BS)
Everything runs locally on your CPU/GPU using C++20 and OpenCV. 
We allocate a fixed-size `RingBuffer<RawFrame, 4>` at startup. No frames are written to disk. No network sockets are opened. Your face stays on your machine.

### B. True Spatial Gaze Math
1. **Calibration**: When you start the daemon with `--calibrate`, we memorize where *your* face sits in the frame.
2. **Pose Vector Math**:
   We map YuNet's 5 facial landmarks to a 3D model using `cv::solvePnP`. This gives us accurate pitch and yaw.
3. **Trigger**: If a secondary face has a gaze vector pointing at your screen (within $\pm 30^\circ$ yaw, $\pm 25^\circ$ pitch), we flag it.

### C. Hysteresis (No Jitter)
We don't lock your screen because someone walked past you. The secondary gaze must hold for $> 1.0$ second (`config.hysteresis_sec`) before elevating from a soft alert to a hard lock or redaction overlay.

---

## 4. Audit Logging

For the compliance folks (NIST SP 800-53, ISO 27001), Blindside writes structured local security event logging to `blindside_threats.log`. It logs *what* happened, not *who* it was (no image data is saved).

```log
2026-07-30 20:24:45 [AUDIT_ALERT] Threat=HARD_WORKSTATION_LOCK GazeDurationSec=1.20 FacesDetected=2 Control=NIST_SP_800_53_PE_3
```

---

## 5. Summary

Blindside takes physical security and pushes it to the edge hardware. It kills the shoulder surfing attack vector without draining your laptop battery and without violating your privacy.

- **Check the benchmarks**: [docs/BENCHMARKS.md](docs/BENCHMARKS.md)
- **Found a vuln?**: [SECURITY.md](SECURITY.md)
- **Want to write some code?**: [CONTRIBUTING.md](CONTRIBUTING.md)
