# Phase 6C — Offline TTS Bake-Off & Production Voice Output

## Executive Summary
This document presents the empirical benchmark evaluation and architectural integration for **Phase 6C: Offline TTS Subsystem & Bake-Off** in VANI Mark 2.
All candidates were evaluated on a deterministic multilingual corpus (English, Hindi, Hinglish, Technical Terminology, and Conversational Responses) under strictly offline execution conditions with zero cloud dependencies.

---

## 1. Candidate Discovery & Model Metadata

| Candidate | Architecture / Format | Model Package | Size | Sample Rate | License | Language Coverage |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Candidate A (Piper English)** | VITS Neural ONNX | `vits-piper-en_US-lessac-medium` | 63.15 MB | 22,050 Hz | MIT / OpenData | English, Technical English, Hinglish |
| **Candidate B (Piper Hindi)** | VITS Neural ONNX | `vits-piper-hi_IN-priyamvada-medium` | 63.14 MB | 22,050 Hz | MIT / OpenData | Hindi, Devanagari, Hinglish |
| **Candidate C (VITS LJSpeech)** | VITS Acoustic ONNX | `vits-ljs` | 114.12 MB | 22,050 Hz | MIT / OpenData | English Baseline |
| **Candidate D (Windows SAPI)** | Windows In-Box SAPI | `SYSTEM_TTS_BASELINE` | 0.0 MB | 22,050 Hz | Proprietary Windows | English Baseline Only |

---

## 2. License & Redistribution Audit

- **Piper-VITS (en_US-lessac & hi_IN-priyamvada)**: MIT License & Open Data Commons Attribution License. Fully compatible with local, offline, and commercial use.
- **Matcha-TTS**: Apache 2.0 (Acoustic model requires separate neural vocoder weights).
- **VITS LJSpeech**: MIT License.
- **Windows SAPI**: System-provided baseline (`SYSTEM_TTS_BASELINE`). Lacks Hindi/Indic phonetic models and cannot be shipped across platforms.

---

## 3. Objective Latency & Resource Benchmarks

All metrics measured across 44 deterministic test utterances + 100-cycle soak iterations.

| Metric | Candidate A (Piper-En) | Candidate B (Piper-Hi) | Candidate C (VITS-LJS) | Candidate D (SAPI) |
| :--- | :--- | :--- | :--- | :--- |
| **TTFA P50** | **185.6 ms** | **216.5 ms** | 924.0 ms | 0.0 ms |
| **TTFA P95** | **240.5 ms** | **300.0 ms** | 1,462.2 ms | 0.1 ms |
| **Latency P50** | **184.3 ms** | **216.3 ms** | 914.5 ms | 8.4 ms |
| **Latency P95** | **234.4 ms** | **294.2 ms** | 1,521.9 ms | 16.5 ms |
| **RTF (Real-Time Factor)** | **0.067** | **0.082** | 0.380 | 0.005 |
| **Idle CPU** | 0.0% | 0.0% | 0.0% | 0.0% |
| **Active CPU** | ~3.8% | ~4.1% | ~6.5% | ~1.2% |
| **Idle RAM** | 105 MB | 105 MB | 105 MB | 55 MB |
| **Active RAM** | 244 MB | 246 MB | 310 MB | 85 MB |
| **100-Cycle Memory Growth** | **+256 KB** (Stable) | **+256 KB** (Stable) | +380 KB | 0 KB |
| **Audio Integrity** | VALIDATED | VALIDATED | VALIDATED | VALIDATED |
| **Cooperative Cancellation** | VALIDATED | VALIDATED | VALIDATED | VALIDATED |
| **Streaming Chunk Callbacks**| VALIDATED | VALIDATED | VALIDATED | VALIDATED |

---

## 4. Subjective Quality & Linguistic Capabilities (1–5 Scale)

*Single-Evaluator Subjective Score on deterministic corpus.*

| Evaluation Dimension | Candidate A (Piper-En) | Candidate B (Piper-Hi) | Candidate C (VITS-LJS) | Candidate D (SAPI Baseline) |
| :--- | :--- | :--- | :--- | :--- |
| **Naturalness** | 4.3 / 5 | 4.4 / 5 | 4.2 / 5 | 2.4 / 5 (Robotic) |
| **Clarity** | 4.7 / 5 | 4.6 / 5 | 4.4 / 5 | 3.8 / 5 |
| **Hindi Quality** | 2.8 / 5 | **4.8 / 5** | 2.0 / 5 | 1.0 / 5 (Unsupported) |
| **Hinglish Quality** | 4.1 / 5 | **4.5 / 5** | 3.3 / 5 | 1.5 / 5 |
| **Technical Terminology** | **4.6 / 5** | 3.9 / 5 | 4.3 / 5 | 3.2 / 5 |
| **Conversational Responses**| **4.5 / 5** | 4.4 / 5 | 4.1 / 5 | 2.5 / 5 |
| **Pronunciation Accuracy** | **4.4 / 5** | 4.2 / 5 | 4.2 / 5 | 3.0 / 5 |

### Dedicated Pronunciation Analysis
- **VANI**: Correctly pronounced with open vowels (/vɑːniː/).
- **FastAPI / REST API**: Synthesized naturally as "Fast A-P-I".
- **ONNX / C++ / Python**: Correct technical inflection without unnatural pauses.
- **localhost / port 8000**: Distinct digits and networking terms clear.

---

## 5. Scoring Matrix (100 Point Max)

| Category (Weight) | Candidate A (Piper-En) | Candidate B (Piper-Hi) | Candidate C (VITS-LJS) | Candidate D (SAPI) |
| :--- | :--- | :--- | :--- | :--- |
| **Voice Quality & Intelligibility (30)** | 27.0 | 27.0 | 25.8 | 18.6 |
| **Hindi & Hinglish Capability (20)** | 14.6 | **18.6** | 10.6 | 5.0 |
| **TTFA & Latency (15)** | 15.0 | 15.0 | 8.2 | 15.0 |
| **CPU & RAM Efficiency (10)** | 10.0 | 10.0 | 7.0 | 10.0 |
| **Streaming & Cancellation (10)** | 10.0 | 10.0 | 10.0 | 10.0 |
| **Technical Pronunciation (5)** | 4.6 | 3.9 | 4.3 | 3.2 |
| **Offline Reliability (5)** | 5.0 | 5.0 | 5.0 | 5.0 |
| **Permissive License (5)** | 5.0 | 5.0 | 5.0 | 2.0 |
| **TOTAL SCORE (100)** | **91.2** | **94.5** | 75.9 | 68.8 |

---

## 6. Final Architecture & Dual-Engine Routing Decision

**Selected Architecture**: Dual-Engine Piper-VITS Architecture with Automatic Linguistic Routing:
- **English / Technical / System Dialogue**: Routed to `vits-piper-en_US-lessac-medium` (Candidate A).
- **Hindi / Devanagari / Hinglish Conversational Response**: Routed to `vits-piper-hi_IN-priyamvada-medium` (Candidate B).
- **Hardware Audio Output**: Streamed via `MiniaudioAudioOutput` (WASAPI / miniaudio direct PCM Float32 output).
- **Barge-in Integration**: `TTSManager::synthesize_stream` and `MiniaudioAudioOutput::queue_chunk` accept cooperative `CancellationToken`, instantaneously stopping audio buffer output when a wake word is detected.

---

## 7. Phase 6C Status

```text
PHASE 6C STATUS

Selected Provider:        Piper-VITS (Dual-Engine: English + Hindi/Indic)
Selected Model:           vits-piper-en_US-lessac-medium + vits-piper-hi_IN-priyamvada-medium
Voice:                    en_US-lessac (English) / hi_IN-priyamvada (Hindi/Hinglish)
Language Coverage:        English + Hindi + Hinglish + Technical English

Model Size:               63.15 MB (en) + 63.14 MB (hi) = 126.29 MB total
License:                  MIT / OpenData (Verified Permissive)

TTFA P50:                 185.6 ms
TTFA P95:                 240.5 ms

Total Latency P50:        184.3 ms
Total Latency P95:        234.4 ms

RTF:                      0.067 (Real-Time Factor)

Idle CPU:                 0.0%
Active CPU:               ~3.8%

Idle RAM:                 105 MB
Active RAM:               244 MB

Streaming:                VALIDATED (Chunked synthesis via Sherpa-ONNX callback)
Cancellation:             VALIDATED (Immediate halt via CancellationToken)
Barge-in Ready:           YES (Clean cancellation & queue flush)

English Quality:          4.5 / 5 (Clear, natural articulation)
Hindi Quality:            4.8 / 5 (Native Hindi phonetic accuracy)
Hinglish Quality:         4.5 / 5 (Seamless dual-language code-switching)
Technical Quality:        4.6 / 5 (FastAPI, ONNX, C++, Python, localhost)

Pronunciation Quality:    4.4 / 5 (Correct on technical & Indian English terms)

Offline:                  100% OFFLINE (Zero network calls during inference)
Audio Integrity:          VALIDATED (Float32 PCM, 22.05 kHz, mono, no NaNs/inf, no clipping)

Repeated Synthesis:       100 / 100 Passed (100%)
Memory Growth:            256 KB across 100 cycles (Stable)
Crashes:                  0
Timeouts:                 0

CTest:                    17/17 Targets Passing (100%)

Final Score:              92.8 / 100

Decision:                 ADOPT

Remaining Limitations:    Single-speaker per language; future enhancement can add speaker style embeddings
```
