# 🛡️ Security Policy & Vulnerability Disclosure

The **Blindside** engineering team treats physical and visual privacy as critical security domains. We take security vulnerabilities seriously and strive to maintain zero-trust edge isolation standards across all supported operating systems.

---

## 🔒 Supported Versions

Only the latest release and the `main` branch receive active security patches and threat updates.

| Version | Supported | Security Maintenance Level |
| :--- | :--- | :--- |
| `2.0.x` (`main`) | ✅ **Yes** | Active Security & Feature Maintenance |
| `1.x` | ❌ No | End of Life (Upgrade to 2.0+) |

---

## 🛡️ Security Architecture & Privacy Guarantees

Blindside enforces strict privacy and anti-exfiltration constraints by design:

1. **Zero Network Socket Calls**: Blindside does not open TCP/UDP sockets, perform HTTP/DNS requests, or communicate with external telemetry endpoints.
2. **Zero Disk Video Retention**: Raw camera frames are stored exclusively in an in-memory C++ circular ring buffer (`RingBuffer<T, 4>`). No video streams or frame captures are written to disk.
3. **Local ONNX Inference**: All computer vision model inferences (YuNet face detection, 3D pose estimation, liveness checks) run 100% on local CPU/GPU hardware.
4. **Anonymized Security Audit Logs**: Audit log records (`blindside_threats.log`) record only timestamps, threat classifications, gaze durations, and face counts—never facial features or personal biometrics.

For a full formal analysis, view [THREAT_MODEL.md](THREAT_MODEL.md).

---

## 🚨 Reporting a Vulnerability

If you discover a potential security vulnerability (e.g., memory corruption, privilege escalation, bypass of privacy redaction controls, or unexpected disk writing), **do not report it publicly via open GitHub issues.**

Please send a encrypted report to the security maintainers:

- **Email**: `security@ashcrest.org` or `medhansh@ashcrest.org`
- **GPG Key Fingerprint** (if available): Refer to maintainer profile

### Report Checklist
Please include the following details in your report:
- **Description**: Summary of the vulnerability and potential security impact.
- **Affected System**: Operating System (Linux kernel version / Windows build), CMake flags used, ONNX Runtime version.
- **Reproduction Steps**: Step-by-step instructions or Proof-of-Concept (PoC) code.
- **Mitigation Suggestion**: Any suggested patches or workarounds.

---

## ⏱️ Response Timeline

| Milestone | Target Response Time |
| :--- | :--- |
| **Initial Acknowledgment** | Within 24 hours |
| **Triage & Risk Assessment** | Within 72 hours |
| **Fix Development & Patch** | Within 7–14 days |
| **Public CVE Release** | Coordinated upon patch release |

---

## 🎖️ Security Recognition

We publicly credit security researchers who report valid vulnerabilities in our release notes (unless anonymity is requested).
