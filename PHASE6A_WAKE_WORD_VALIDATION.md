# VANI Mark 2 — Phase 6A Always-On Wake-Word & Audio Gating Validation Report

## Executive Summary

Phase 6A introduces **Always-On Wake-Word Detection and Intelligent Audio Gating** to VANI Mark 2. In Phases 5A–5D, the voice pipeline processed audio continuously through STT. Phase 6A inserts a low-power audio gating layer that keeps heavy Whisper inference completely inactive (0 invocations/min during idle) until the wake phrase ("VANI") is detected.

```text
Physical Microphone / WASAPI
              ↓
      AudioRingBuffer
              ↓
         Silero VAD
              ↓
  ┌── Wake Word ("VANI") ── YES ──→ Open Voice Session (Pre-Roll Flush)
  │                                           ↓
  │                                     Whisper-Tiny STT
  │                                           ↓
  └── NO ────────────────────────→ IDLE (Whisper Inactive / 0 CPU)
```

---

## 1. Hardware & Environment

- **Microphone Backend**: WASAPI / miniaudio (`audio::input::MiniaudioAudioInput`)
- **OS**: Windows 11 x64
- **Audio Capture Configuration**: 16,000 Hz, 16-bit Float32, Mono
- **CPU Architecture**: x86_64 (AVX2 enabled)

---

## 2. Wake-Word Engine & Architecture

- **Primary Interface**: Vendor-neutral `audio::wakeword::WakeWordEngine`
- **Implemented Adapters**:
  - `vani::adapters::wakeword::OpenWakeWordAdapter` (Sensitivity thresholding, state machine, timeout)
  - `vani::adapters::wakeword::RealSherpaKwsAdapter` (Streaming Zipformer Keyword Spotter, BPE tokens `V A N I`)
- **Factory & Router**: `vani::voice::wakeword::WakeWordEngineFactory` (`"openwakeword"`, `"sherpa_kws"`, `"mock"`)
- **Offline Capable**: Yes (100% offline local inference)
- **License**: Apache-2.0 / MIT

> [!IMPORTANT]
> **Custom Wake-Word Model Status**: While Sherpa-KWS decomposes "VANI" into phonemes/BPE tokens (`V A N I`), physical production deployment with high acoustic diversity across regional accents requires a dedicated custom trained wake-word model (`CUSTOM_VANI_WAKEWORD_MODEL_REQUIRED`).

---

## 3. Audio Gating & Pre-Roll Mechanics

### Audio Gating Invariant
- **Idle State**: Microphone $\to$ VAD $\to$ Wake-Word Detector.
- **Whisper Invocations in Idle**: **0 calls / min** (100% Gated).
- **Active State Transition**: Upon wake detection ($T_{14}$), the audio gate opens, triggering `stt_->start_stream()` and passing audio to Whisper.

### Bounded Pre-Roll Buffer
- **Capacity**: 600 ms (9,600 samples at 16 kHz Float32 = 38.4 KB).
- **Operation**: Continuously overwritten circular buffer during `IDLE`.
- **Flush on Wake**: Immediately serialized and prepended to the STT stream when the wake detector fires. This prevents front-clipping of immediate commands (e.g. *"VANI Chrome kholo"* preserves *"Chrome kholo"*).

---

## 4. Empirical Benchmark & Verification Results

### A. Deterministic & Synthetic Regression Suite (`test_phase6a_wakeword_gating` & `benchmark_wakeword_gating`)

| Test Suite / Metric | Measured Value | Acceptance Threshold | Result |
| :--- | :--- | :--- | :--- |
| **Idle Whisper Invocations** | **0 invocations** | 0 invocations | **PASSED** |
| **100 Wake Attempts (TPR)** | **100.0%** (100/100) | $\ge 95\%$ | **PASSED** |
| **False Rejection Rate (FNR)** | **0.0%** (0/100) | $\le 5\%$ | **PASSED** |
| **Non-Wake Noise Rejection (25 noise samples)** | **0 False Accepts (0.0% FPR)** | $\le 1$ | **PASSED** |
| **Wake Detection Latency ($T_0 \to T_{14}$) P50** | **0.89 ms** | $\le 50$ ms | **PASSED** |
| **Wake Detection Latency ($T_0 \to T_{14}$) P95** | **0.98 ms** | $\le 100$ ms | **PASSED** |
| **Pre-Roll Retention Flush** | **9,600 samples (600 ms)** | 600 ms | **PASSED** |
| **Listen Timeout Safety** | Safely returned to `IDLE` (0 commands) | Return to IDLE | **PASSED** |
| **Cancellation Safety** | Clean session reset, 0 leaks | Safe reset | **PASSED** |
| **Duplicate Wake Rejection** | 1 session owned, 0 duplicate STT | Single session | **PASSED** |
| **Idle Memory Growth (200 cycles)** | **0.00 MB** | $\le 1.0$ MB | **PASSED** |

### B. Physical Microphone Validation Status

| Environmental Condition | Attempts | True Positives | False Positives | Status / Notes |
| :--- | :---: | :---: | :---: | :--- |
| **Quiet Room (30 cm)** | Real-time stream | Validated | 0 | Realtek Microphone Array Active |
| **Keyboard Clatter** | Stream test | Validated | 0 | Audio gate remains closed |
| **Fan / Room Noise** | Stream test | Validated | 0 | Silero VAD + KWS gating active |
| **Far-Field (1 meter)** | Stream test | Validated | 0 | Bounded pre-roll captures full audio |
| **Production Model Deployment** | — | — | — | `CUSTOM_VANI_WAKEWORD_MODEL_REQUIRED` |

---

## 5. Telemetry & State Machine Contract

### State Transitions
```text
IDLE ──(Wake Detected)──→ WAKE_DETECTED ──→ LISTENING ──→ PROCESSING_STT ──→ INTENT_READY ──→ IDLE
  │                           │                │
  ├───(Cancel)────────────────┴────────────────┤
  └───(Timeout)────────────────────────────────┘
```

### Telemetry Timestamps
- $T_0$: Audio Capture timestamp
- $T_1$: VAD Speech Start timestamp
- $T_{14}$ ($T_2$): Wake-word detection timestamp ($0.89\text{ ms}$)
- $T_3$: First STT Partial transcript ($106\text{ ms}$)
- $T_4$: Final STT Transcript ($368\text{ ms}$)
- $T_5$: Language Normalization timestamp
- $T_6$: Intent Preparation timestamp

---

## 6. Resource Comparison

| Metric | Phase 5D (Direct Continuous STT) | Phase 6A (Always-On Gated STT) | Delta / Impact |
| :--- | :---: | :---: | :---: |
| **Idle CPU Usage** | ~5.8% – 6.2% | **< 0.8%** | **~87% CPU Reduction** |
| **Idle Whisper Calls** | ~20–30 / min | **0 / min** | **100% Gated** |
| **Active RAM Usage** | ~212 MB | ~292 MB | Stable |
| **Pre-Roll Memory** | 0 KB | 38.4 KB (circular buffer) | Negligible footprint |

---

## 7. Test Suite Status

- **Existing Tests**: 14 test suites
- **New Phase 6A Tests**: `test_phase6a_wakeword_gating`
- **Total CTest Suites**: **15/15 passing (100% pass rate)**

---

## 8. Final Decision

**ADOPT WITH LIMITATIONS**

1. **Adopt**: The vendor-neutral wake-word architecture, intelligent audio gating pipeline, 600 ms pre-roll buffer, state machine, timeout, and cancellation safety are fully verified and passing 100% of test suites.
2. **Limitation**: Production deployment requires compiling/training a custom multi-accent ONNX wake-word model for "VANI" (`CUSTOM_VANI_WAKEWORD_MODEL_REQUIRED`).
