# 🛡️ Security Policy

We build privacy tools. If Blindside has a vuln, that defeats the whole point. We take this seriously.

---

## 🔒 Supported Versions

Only the `main` branch (v2.x) gets patches. If you're on v1.x, upgrade. We ripped out the old fake AI engine anyway, you don't want to be running that.

| Version | Supported | Notes |
| :--- | :--- | :--- |
| `2.0.x` (`main`) | ✅ **Yes** | Active development. |
| `1.x` | ❌ No | Dead. RIP. |

---

## 🛡️ The Guarantees

If you find a bug that breaks any of these four rules, we want to know immediately:
1. **Zero Network Traffic**: Blindside shouldn't even look at a network socket.
2. **Zero Disk I/O for Video**: Frames go into the RAM ring buffer and die there.
3. **100% Local Inference**: YuNet and SolvePnP run on your local silicon.
4. **No Biometrics Saved**: We log threat events, not faces.

---

## 🚨 Reporting a Vuln

Found a memory leak? A buffer overflow? A way to bypass the screen lock?

**DO NOT POST IT IN GITHUB ISSUES.**

Email us directly:
- **Email**: `security@ashcrest.org` or `medhansh@ashcrest.org`

Include:
- OS/Kernel version.
- CMake flags you used.
- A Proof-of-Concept (PoC) if you have one.
- How to reproduce it.

---

## ⏱️ Response Timeline

We're engineers, we get it. We won't leave you on read.
- **Acknowledgment**: 24 hours
- **Triage**: 72 hours
- **Patch**: 7–14 days

We will give you full credit in the CVE and release notes (unless you want to stay anonymous).
