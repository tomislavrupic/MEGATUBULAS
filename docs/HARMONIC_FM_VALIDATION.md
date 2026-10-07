# Harmonic FM · v0.3.0 Experimental

Pixel Records / 2026-10-07. This layer synthesizes a harmonic voice from detected monophonic bass pitch and adds it before saturation. It does not directly frequency-modulate the recording. FM is off by default; Depth zero also silences the whole layer. Chords/full mixes are outside this first experiment. No claim of constant fundamental amplitude or subjective listening approval.

## Interface

Memory sits in the left animation structure, Drive in the centre, FM Amount in the right. Coupling / Blend / Output remain above. Advanced exposes Depth and integer ratio 1x / 2x / 3x. A timer-driven atomic readout reports OFF / LISTENING / LOCKED / RELEASING plus Hz and a display-only note name. No tuning quantization occurs. The blank 3:2 frame is drawn as a whole plate, with native circular mounting bolts and a fixed editor aspect ratio (uniform-scale fallback for hosts that ignore it). Signal/Drive-dependent amber grille heat uses 65 ms attack / 650 ms release and a slowly evolving coherent highlight; reduced motion freezes highlight drift. The real editor was captured at collapsed widths 960, 1320, 1800 and expanded widths 960, 1320, 1556; numeric entry, keyboard arrows, double-click defaults and attachments use the established JUCE controls. Reduced motion retains the existing Drive-selected still frame.

## Offline evidence

- 123 independent detector/voice checks pass: 25, 30.8677, 41.2034, 55, 82.4069, 110, 220, 400 Hz at 44.1/48/96 kHz, sine and weak-fundamental bass, including anti-phase stereo. Settled periodic estimates within 20 cents; onset acquisition 106.49–116.74 ms (median 108.98 ms) across these 48 fixtures; maximum settled error 1.51 cents. Noise fixture reports no false stable locks. Continuous bends, octave changes and stronger-channel changes are exercised.
- FM voice tests cover harmonic multiples at all ratios, zero Amount/Depth/envelope, reset replay, ratio crossfade, DC rejection and loss-of-lock release. At 400 Hz / maximum Depth / ratio 3, 192 kHz voice versus 1.536 MHz reference residual is -65.68 dB relative. This includes all implementation/filter differences; it is not a pure alias measurement.
- 533 pipeline checks pass, including partitions 1 / 17 / 127 / 512 / 4096 at 44.1/48/96 kHz, both 4x and 8x; a note change occurs inside host blocks. Finite output, silent tail, anti-phase tracking, Blend zero and deterministic replay are tested. Latency remains 61 / 65 samples at 4x / 8x.
- 155 processor integration checks pass, including old-session migration after active FM, parameter round trips, preset defaults, malformed state, ratio automation, Freeze pitch following, reset/reprepare/offline replay and state queue consumption. Guarded callbacks report zero C++ new/new[] allocations.
- 14 renderer CLI checks pass, including strict finite/range/integer rejection and exact old-invocation identity against explicit FM-off settings.
- The v0.2.1 renderer and v0.3 FM-off renderer produced byte-identical WAVs on the same original public bass phrase with Drive 65 / Memory 40 / Coupling 30. This is same-environment evidence, not a cross-compiler promise.

## Scheduling and CPU

Apple M3 Ultra / Release build, 48 kHz stereo, measured offline with no hardware connection. An initial all-at-once detector job caused 70–78 microsecond FM callbacks at a one-sample block (20.83 microsecond deadline). The final implementation snapshots at the same 8 ms sample clock and spreads bounded difference lags across native samples. This adds roughly 5 ms analysis completion latency at 48 kHz and preserves sample-causal/block-independent decisions.

After scheduling: one-sample mean / p99 is 2.15 / 2.96 us at 4x and 4.04 / 5.50 us at 8x with FM enabled. At 127 samples, mean / maximum is 253 / 352 us at 4x and 493 / 615 us at 8x, against a 2646 us deadline. At 4096 samples, mean / maximum is 8191 / 8469 us at 4x and 15740 / 16064 us at 8x, against 85333 us. One-sample worst wall-clock outliers (33–43 us) remain, also seen with FM off (33–51 us); these measurements include OS scheduling and do not establish hard real-time guarantees. No detector-correlated 70–78 us spikes remain in the measured run.

`MicroFMPerformance` reproduces the block/quality matrix; `MicroFMTests` prints pitch timing/error rows. Ignored `analysis/fm-v030/` retains logs, CSVs, actual screenshots and RMS-matched original bass comparisons (dry, saturation, FM ratios 1/2/3). The user's private test recording is excluded from source and packages.

## Native release gates

Mac AU, VST3, bundle metadata/signatures, independent review and the new Windows native workflow are separate release gates. Their final results are recorded below only after execution. Current local tools have not connected audio to speakers; a passing validator is not musical listening acceptance.
