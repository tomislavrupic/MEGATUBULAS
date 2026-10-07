# Harmonic FM Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an experimental, monophonic, pitch-following FM layer to MEGATUBULAS with an exact settled-off reference path.

**Architecture:** A bounded native-rate pitch detector produces a causal control timeline. An oversampled FM voice consumes that timeline before the existing saturation stages; processor parameters and the expanded editor expose its amount, depth, ratio and tracking status.

**Tech Stack:** C++20, CMake, JUCE 8.0.14, existing CTest/numerical/integration harnesses, offline WAV renderer, macOS native validators and Windows GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-10-07-harmonic-fm-design.md`

## Global Constraints

- Initial pitch range 25–400 Hz; single bass notes, continuous bends, no semitone quantization.
- Preserve existing parameter IDs, plugin codes, schema-1 `MICROTUBULAS` state root and existing latency.
- FM IDs append: `fmAmount` 0–100 default 0; `fmDepth` 0–100 default 25; `fmRatio` choices 1×/2×/3× default 1×.
- Candidate version 0.3.0, visibly Experimental. Current public v0.2.1 is the comparison reference.
- No callback allocation, locks, network, unbounded loops or host-block-dependent analysis decisions.
- Keep supplied media private. Use original/synthetic fixtures for distributable demos.
- Preserve current 4×/8× processing, original dry path, Match, Memory/Coupling and reduced-motion behavior.
- Follow the spec's exact detector, smoothing, envelope, stereo hysteresis and FM/filter values; deviations require recorded rationale and user review when they change behavior.

## Review Focus

- Reloading an old session after FM was enabled must turn FM off, rather than retaining the previous session's values (Task 4).
- A stronger channel appearing during a sustained anti-phase stereo bass must not cause an FM pitch burst (Tasks 1/3).
- Large host blocks must not apply an estimate from the end of the block to audio from its beginning (Task 3).
- Depth zero and silence must not leave an audible sine carrier (Tasks 2/3).
- Reset/offline transitions during enabled FM must reproduce the same subsequent render from the same input history (Tasks 3/4).

## File Structure

Create `Source/PitchTracker.h`, `Source/HarmonicFM.h` and `Tests/FMTests.cpp`. The new headers own detection and synthesis respectively; the independent test executable owns synthetic detector/voice tests. Modify `Source/Core.h` only to append parameter fields. `Source/Pipeline.h` owns the tracker, voice and prepared control buffers. `Source/Processor.{h,cpp}` owns host parameters, migration and telemetry. `Source/Editor.{h,cpp}` owns the experimental panel. Extend existing `Tests/Tests.cpp`, `Tests/Integration.cpp` and `Tests/Render.cpp` at their established boundaries. Update CMake and user-facing docs; do not refactor unrelated engine code.

### Task 1: Bounded pitch detector

**Files:** Create `Source/PitchTracker.h`, `Tests/FMTests.cpp`; modify `CMakeLists.txt`.

**Interfaces:** `micro::PitchEstimate { double hz, confidence; std::array<double,2> envelope; bool locked; }`; `PitchTracker::prepare(double sampleRate, int channels)`, `reset() noexcept`, `process(double left, double right) noexcept -> PitchEstimate`. Internally fixed rings/filter states, no runtime heap. Mono passes the same sample twice.

- [ ] Write detector tests using literal frequencies 25, 30.8677, 41.2034, 55, 82.4069, 110, 220, 400 Hz; settled locked estimates within 20 cents on sine and clearly periodic bass fixtures. Add weak-fundamental, slide/octave-change, silence/noise, invalid-input, mono/stereo/anti-phase and stronger-channel-switch fixtures. Measure acquisition delay; ambiguous fixtures may withhold lock but must not assert a wrong stable lock.
- [ ] Register the FM test executable with CTest. Run it against the missing detector, then a minimal empty implementation, recording compilation absence and actual behavioral failures separately. No product behavior is accepted from a missing-symbol error alone.
- [ ] Implement the spec's 800 Hz sixth-order analysis low-pass, integer decimation, 512-sample ring, 256-sample YIN difference span, derived lag range, parabolic refinement, 8 ms hop, threshold/hysteresis/continuity rules and per-channel envelope/energy tracking. Keep every update causal on a sample clock.
- [ ] Run FM tests at 44.1/48/96 kHz. Record error, lock availability, acquisition/reacquisition timing and noise false locks. Fix failures without weakening the accepted periodic-fixture assertions.
- [ ] Commit the independently tested detector and fixtures.

### Task 2: Harmonic FM voice

**Files:** Create `Source/HarmonicFM.h`; extend `Tests/FMTests.cpp`.

**Interfaces:** `micro::FMControl { double hz, gate; std::array<double,2> envelope; }`; `HarmonicFM::prepare(double oversampledRate)`, `reset() noexcept`, `set(double amount, double depth, int ratio) noexcept`, `sample(const FMControl&) noexcept -> std::array<double,2>`, `gate() const noexcept -> double`. Ratio argument is the integer 1/2/3. The voice owns shared phase and per-channel output filters.

- [ ] Write failing tests for no output at Amount zero, Depth zero and zero envelopes; finite bounded output; deterministic reset; harmonic-multiple energy at a literal 55 Hz carrier for all three ratios; continuous parameter/ratio transitions; and decay to silence after gate loss. Calculate spectral expectations with independent test DFTs, not production helpers.
- [ ] Run tests and capture behavioral failures with an empty voice before adding synthesis.
- [ ] Implement continuous-phase sine FM, index 0–3 with the specified envelope multiplier, Amount/envelope/gate scaling, 20 ms parameter smoothing, 10 ms pitch smoothing, 10/40 ms lock fades and 20 ms ratio crossfade. Apply 12 Hz DC rejection and second-order 3 kHz low-pass at the oversampled rate. A zero Depth target fades the entire layer rather than exposing a sine carrier.
- [ ] Run FM tests including a high-rate-reference spectral comparison at 400 Hz / ratio 3 / maximum Depth and Amount. Report residual and upper-band energy without labelling all residual alias power.
- [ ] Commit the tested voice.

### Task 3: Causal wet-path integration and reference identity

**Files:** Modify `Source/Core.h`, `Source/Pipeline.h`, `Tests/Tests.cpp`, `Tests/FMTests.cpp`.

**Interfaces:** Append `double fmAmount=0, fmDepth=25; int fmRatio=0;` to `micro::Parameters` (host ratio index 0/1/2). Pipeline translates ratio index to 1/2/3. Publish `pitchEstimate() const noexcept -> PitchEstimate`, `fmGate() const noexcept -> double`, and `fmEnabled() const noexcept -> bool`. Timeline storage is allocated in `prepare`, sized by prepared capacity, and populated at native input sample cadence before wet rendering.

- [ ] Preserve a local v0.2.1 reference renderer and synthetic reference outputs outside distributable source. Write failing tests that enabled FM adds measured harmonic content; Amount/Depth zero settle to the reference; Blend zero/bypass remain aligned dry; silence produces no tail after the documented filters/gates settle.
- [ ] Add a note change deliberately inside host blocks. Compare output and detector events across partitions 1, 17, 127, 512, 4096 at 44.1/48/96 kHz and 4×/8×. Add anti-phase/asymmetric stereo and channel-switch integration fixtures. These must fail if one final block estimate is applied retrospectively.
- [ ] Run the regression suite against the existing pipeline and record the missing-feature failures.
- [ ] Integrate tracker/control timeline and FM contribution immediately before `engine.sample`. Consume timeline controls without future samples at each oversampled index; retain interpolation state across slices. Reset/re-enable acquisition consistently, skip settled-off analysis work, and never alter the legacy dry/match/filter path while FM is off.
- [ ] Run full CTest and reference comparisons. Require same-build settled-off identity, unchanged latency, bounded finite enabled output, causal partition agreement and deterministic reset/offline simulation.
- [ ] Commit pipeline integration and regressions.

### Task 4: Host parameters, state migration and lifecycle

**Files:** Modify `Source/Processor.h`, `Source/Processor.cpp`, `Tests/Integration.cpp`.

**Interfaces:** Append raw parameter pointers/IDs after the original 20. Expose float atomics `fmHz`, `fmConfidence`, `fmGate` and integer atomic `fmTrackingState` (0 Off, 1 Listening, 2 Locked, 3 Releasing); the UI reads them only. Existing processor reset/offline behavior remains the lifecycle entry point.

- [ ] Write integration tests for default-off audio, all FM parameter round trips, old schema-1 state missing FM nodes after a previously enabled FM session, invalid/non-finite FM state, and existing presets explicitly resetting all three FM fields.
- [ ] Add guarded callback tests with FM active and repeated automation, including ratio changes and state queue consumption. Add deterministic reprepare/reset/offline-entry checks and verify Freeze retains pitch following while freezing the existing selector.
- [ ] Run and record failures before appending parameters or adding migration.
- [ ] Implement the three spec parameters, append-only indexing, default-node insertion before `replaceState`, preset defaults and atomic telemetry publication. Retain existing validation policy and state size limits.
- [ ] Run all CTest suites, requiring zero guarded callback new/new[] allocations and successful legacy/new session behavior.
- [ ] Commit host/state integration.

### Task 5: Experimental panel and offline demos

**Files:** Modify `Source/Editor.h`, `Source/Editor.cpp`, `Tests/Render.cpp`, `CMakeLists.txt`, `README.md`; add `docs/HARMONIC_FM_VALIDATION.md`.

**Interfaces:** Editor APVTS attachments for FM Amount/Depth/Ratio in the expanded panel; timer-driven `OFF / LISTENING / LOCKED / RELEASING` and continuous Hz/display-only note name. Extend MegaRender's optional CLI arguments after existing arguments to accept FM Amount, Depth and ratio index without changing old invocations.

- [ ] Write renderer behavior tests for valid FM arguments and rejection of malformed/non-finite/out-of-range values; verify old invocations retain disabled-FM output.
- [ ] Run failures, then implement renderer argument validation and parameter forwarding.
- [ ] Add the labelled experimental panel using the current synth-style knobs, focus/tooltips/default reset, keyboard/numeric entry and responsive expanded layout. The main lattice remains large. Update bundle version to 0.3.0 and visible footer to Experimental.
- [ ] Export the native screenshot with MicroPreview and inspect normal/expanded editors at minimum/default/maximum sizes. Exercise lock display, numeric controls, ratio automation and reduced motion. No generated mockup substitutes for native screenshot verification.
- [ ] Render an original bass phrase through dry, current saturation and FM variants at ratios 1/2/3, with level-matched outputs and pitch/lock timing CSV. Private supplied audio may be tested locally but is excluded from artifacts/source/downloads.
- [ ] Document actual detector coverage, acquisition timing, CPU/worst callback cost, spectral results and remaining listening limits. Commit UI/renderer/docs only after checks pass.

### Task 6: Complete native validation and listening handoff

**Files:** Evidence under ignored `analysis/fm-v030/`; update `docs/HARMONIC_FM_VALIDATION.md` and plan status. Reuse current native CI workflow; do not reuse its v0.2.1 result as FM evidence.

- [ ] Run `cmake --build build --config Release -j 8` and `ctest --test-dir build --output-on-failure`; run the extended FM spectral/timing/CPU fixture set. Investigate any deadline spikes before validation is called complete.
- [ ] Validate the newly built Mac AU with targeted auval and AU/VST3 with pluginval strictness 5. Process offline; do not play unattended test tones through hardware. Check bundle version/minimum OS/ad-hoc signature separately.
- [ ] Obtain an independent whole-change review after the implementation suites pass, resolve substantive findings and rerun affected checks. Execution method determines whether review is per task or at the end.
- [ ] Present the native interface and matched original-bass audio examples for listening review, distinguishing builds/tests from subjective approval. Keep the public v0.2.1 download intact while the experiment is reviewed.
- [ ] A Windows candidate requires pushing an explicitly scoped experimental branch and running the native Windows workflow on that branch; record the new commit/run and checks. Public FM release/deployment is a subsequent release step, not implied by the old v0.2.1 publication.

## Execution decision

Recommended: **Native execution in this session**, followed by a fresh whole-change reviewer. Detector, voice and pipeline interfaces are tightly connected; one implementer can keep causal timing and state migration consistent. Subagent-driven execution with fresh task reviewers is available if the user prefers it. Await the user's plan review and execution choice before product code.

## Self-review

Each spec section maps to Tasks 1–6. Pitch ambiguity/low-note delay, Depth-zero carrier leakage, block-causal controls, stereo cancellation, legacy-state migration, replay, output filtering, host automation, UI, listening and cross-platform evidence have named checks. Interfaces consistently use host ratio index 0–2 and voice integer ratio 1–3. No new dependency, MIDI/chord mode or unrelated engine redesign is introduced.

User revision (2026-10-07): FM ratio picker remains visible beside FM on the main interface. FM Depth defaults/reset/presets/legacy missing-field migration now use 66%; Amount still defaults to zero.
