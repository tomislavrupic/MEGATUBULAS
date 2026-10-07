# MEGATUBULAS — Harmonic FM Experimental

Date: 2026-10-07. Status: written design for user review, not implemented.

## Agreed intent

Add experimental frequency-aware FM that follows a played bass note and contributes harmonically related texture. The user selected single bass notes first and approved the proposed pitch-following FM layer. Preserve the established bass-first saturation and softened highs. Playing dynamics should animate the added texture. This is an original effect, not a circuit emulation or a claim of perfect pitch detection.

Success means an audible, useful added harmonic character that follows notes and bends, becomes quiet when tracking is uncertain, and contributes no audio at FM Amount zero. The existing v0.2.1 release remains the reference for disabled-FM comparisons.

## Approach selected

Use an audio-driven FM synthesis layer, then feed it into the existing oversampled saturation. Detect continuous fundamental frequency from the input; do not quantize to equal-tempered notes. The note name is display-only. A sine carrier follows the estimated fundamental and a sine modulator follows an integer multiple of it.

Direct modulation of the recorded audio is a different possible effect, but introduces a separate time-varying delay/resampling problem. An additive exciter would offer precise harmonic control but would not be the requested FM mechanism. Neither is part of this experiment.

## Signal path and ownership

1. Sanitize and input-trim audio using the existing pipeline.
2. Analyze the trimmed input before bass/treble conditioning and saturation.
3. Preserve a causal per-input-sample timeline of pitch, confidence gate and amplitude envelope in buffers allocated during prepare.
4. At the oversampled processing rate, consume the corresponding timeline and render the FM layer. Add it after existing preconditioning, immediately before the three saturation stages.
5. Retain existing character shelves, EQ, Match, wet level, Blend and Output. Dry remains the existing latency-aligned input.

The pitch timeline is necessary: analyzing an entire host block and applying its final estimate to the beginning of that block would leak future information and make results depend on host block size. Control history and interpolation must remain causal across arbitrary internal slices.

New focused components: `Source/PitchTracker.h` for analysis and lock state; `Source/HarmonicFM.h` for oscillators and FM smoothing. `Source/Pipeline.h` owns both and their preallocated timeline buffers. The saturation `Engine` retains its present responsibilities. The processor owns permanent parameter IDs and atomic display telemetry; the editor reads telemetry on its existing UI timer.

## Pitch tracking

- Supported initial target range: 25–400 Hz, including low B around 30.9 Hz and ordinary bass playing. Chords, mixed instruments and unpitched input are outside the tracking promise.
- Start with a bounded YIN-style normalized difference detector, with parabolic lag refinement and continuity checks. This is a candidate implementation to validate, not an assertion that it eliminates octave errors.
- Low-pass the analysis input with a sixth-order Butterworth response at 800 Hz. Decimate by an integer factor chosen during prepare so the analysis rate is approximately 5.5–6 kHz at supported host rates. Analysis filtering does not alter the audio path.
- Fixed 512-sample analysis ring; fixed 256-sample difference span; lag range derived from 25–400 Hz. Analyze every 8 ms of analysis time with a sample-count clock. All storage and loop bounds are fixed during prepare.
- Initial lock threshold: normalized difference below 0.15, input RMS above −60 dBFS, and three consecutive estimates within 50 cents. Use a separate release threshold of 0.25 to avoid gate chatter. These values must be measured against the fixtures before being accepted.
- During a large note change, fade down the FM contribution while acquiring the next stable pitch. Small continuous bends follow smoothly without restarting oscillator phase. Repeated octave-like jumps require stable new evidence rather than immediate switching.
- Stereo analysis follows the stronger low-frequency channel; do not sum L/R, which can cancel a valid anti-phase bass. Compare 50 ms smoothed channel energies. Select the stronger channel at initial acquisition (left on a tie); switch only after the other channel remains 6 dB stronger for 100 ms. Channel switching invalidates the current lock until new evidence is acquired. Use a shared pitch/phase for the generated stereo layer, with per-channel amplitude envelopes.
- Expected onset acquisition is tens to roughly 150 ms depending on pitch and waveform. Measure median and worst-case delay; do not advertise instantaneous tracking. No lookahead or additional dry-path delay is introduced.

## FM voice

Let the tracked frequency be `f0`, carrier phase `phi`, integer ratio `r`, and modulation index `I`:

`voice = sin(phi + I * sin(r * phi))`

Advance `phi` continuously from the smoothed tracked frequency. Carrier/modulator phases share the same phase accumulator so their frequency relationship does not drift. Integer ratios place steady-state FM sidebands on multiples of `f0`; note changes and amplitude envelopes still produce transient spectral spread.

- Ratio choices: 1×, 2×, 3× modulator frequency relative to the carrier.
- Depth maps to modulation index 0–3, multiplied by `0.25 + 0.75 * tanh(4 * linkedEnvelope)`, where the linked envelope is the stronger channel envelope. This bounded multiplier lets a harder note increase modulation depth. Use a 3 ms envelope attack and 80 ms release, independent of the existing Memory control. A full-scale input envelope is 1; sanitize and clamp detector/envelope inputs consistently with the existing pipeline.
- Amount scales the added layer. Depth zero also gates the layer to zero; it must not introduce an unmodulated sine tone. Silence and invalid lock likewise produce no sustained oscillator output.
- Smooth Amount/Depth over 20 ms, pitch over approximately 10 ms, and lock gain with 10 ms attack / 40 ms release. Crossfade between ratio voices over 20 ms using shared phase. Do not reset phase on every detector hop.
- Before filtering, the oscillator contribution per channel is `0.5 * channelEnvelope * amount * lockGain * voice`, with Amount normalized to 0–1. Thus its instantaneous contribution is bounded relative to the input envelope. DC-block it at 12 Hz and apply a second-order Butterworth 3 kHz low-pass before adding it to the wet path. The existing character shelves further soften the result. Filters may ring briefly; include their tails in silence/release tests.
- Render at the existing 4×/8× rate. Oversampling is not a proof of inaudible aliasing: measure residuals against a higher-rate reference at maximum pitch, ratio and depth.
- Adding a coherent voice can reinforce or cancel existing components. Preserve the source routing, but do not claim an unchanged fundamental amplitude or constant loudness when FM is enabled. Evaluate using level-matched comparisons.

## Controls and interface

User steering (2026-10-07): place Memory inside the left animation structure, Drive in the central structure, and FM Amount inside the right structure. Retain the large lattice around the controls. Move the remaining top-row controls into an evenly spaced Coupling / Blend / Output row. Add a clearly labelled **HARMONIC FM · EXPERIMENTAL** section for Depth in the expanded panel, with Ratio beside FM and tracking status below it on the main interface:

| Permanent ID | Display | Range / choices | Default |
| --- | --- | --- | --- |
| `fmAmount` | FM Amount | 0–100% | 0% (off) |
| `fmDepth` | FM Depth | 0–100% | 66% |
| `fmRatio` | Harmonic Ratio | 1× / 2× / 3× | 1× |

Show detected note and Hz with `OFF`, `LISTENING`, `LOCKED` or `RELEASING`. Uncertain estimates must not be displayed as confidently locked. Telemetry is bounded atomic data, never a UI callback from the audio thread. Sliders support the current numeric entry, reset, keyboard and automation behavior. Label the section for monophonic bass; no extra tuning, MIDI input or chord mode in this version.

## State, compatibility and lifecycle

- Append new parameters; retain all existing IDs, ranges, defaults, plugin codes and the schema-1 state root.
- When loading a v0.2.1 state without FM fields, explicitly insert the three FM defaults before replacing state. Do not retain FM settings from the previously loaded session. New state saves all three parameters and validates values as the existing loader does.
- Existing presets explicitly set FM Amount to zero and restore the FM defaults. Users can add FM after choosing a starting point.
- Reset, prepare, session restore and the existing offline-entry reset clear tracker history, FM phase, envelopes, locks and timeline clocks. Freeze continues to freeze selector choice; it does not freeze the played pitch.
- Bypass and Blend zero retain their existing dry behavior. FM Amount zero takes an exact bypass around the new audio contribution and skips detector work after the short disable fade. Re-enabling starts fresh acquisition.
- Current published binaries are not overwritten by design work. Build the candidate as version 0.3.0, visibly labelled Experimental. Preserve the prior installed build when later installing a tested candidate. Publication is a separate release step after validation and listening review.

## Tests and acceptance

Write failing behavioral tests before product code. Keep test fixtures synthetic/original; the supplied private recording remains local.

1. Pitch fixtures at 25, 30.8677, 41.2034, 55, 82.4069, 110, 220 and 400 Hz: sine, bass-like harmonics, weak fundamental and varying levels. Target settled error within 20 cents for clearly periodic fixtures; report failures and octave errors by waveform rather than claiming success from sine tests alone.
2. Slides, semitone changes and octave changes: measure acquisition time, lock loss, recovery and click/transient behavior. Strongly ambiguous or missing-fundamental cases may withhold lock; they must not generate loud unstable tones.
3. Silence, decays, noise, percussive attacks, NaN/infinity/extreme input, and anti-phase/asymmetric stereo: bounded finite output, no autonomous sound after release, no false lock on silence, and no stereo-sum cancellation failure.
4. FM output on stable notes: energy added at expected harmonic multiples for each ratio and depth; no contribution at Depth zero. Check DC, level, fundamental interaction and upper-frequency energy. Compare high-rate reference residuals rather than calling total residual pure alias power.
5. Disabled-FM output matches the v0.2.1 reference exactly for the same build/environment across modes, quality, Blend and block partitions. Enable/disable fades are explicitly excluded from the settled-off identity check.
6. Deterministic reset/replay and matching render across block sizes 1, 17, 127, 512 and 4096 at 44.1/48/96 kHz, mono/stereo, 4×/8×. Detector decisions must not depend on host block boundaries.
7. Parameter automation, ratio transitions, malformed state, old-state load with FM previously enabled, new-state round trip, presets, bypass, Reset and Freeze interaction. No guarded callback heap allocation or locks.
8. Profile total CPU and worst callback cost with minimum/maximum host blocks and 4×/8×. If detector hops cause excessive spikes, revise bounded scheduling before release; do not hide work on an unbounded worker that changes offline repeatability.
9. Run the complete numerical/integration suites and Mac AU/AU-VST3 pluginval checks. Windows needs a fresh native build and pluginval run for this revision; v0.2.1 CI evidence does not cover FM.
10. Render dry/current saturation/FM comparisons on an original bass phrase, matched in RMS, with ratio/depth examples and pitch/lock timing logs. Local listening review decides whether the experiment is musically worthwhile; waveform differences alone do not establish that.

## References and limits

- [de Cheveigné and Kawahara, YIN (2002)](https://www.ee.columbia.edu/~dpwe/papers/deChevK02-yin.pdf): basis for candidate fundamental estimation.
- [Carnegie Mellon, FM synthesis](https://www.cs.cmu.edu/~music/icm-online/readings/fm-synthesis/): sideband frequencies, harmonic ratios and modulation index.

The experimental layer synthesizes sound controlled by the input. It does not directly frequency-modulate the recorded waveform. Tracking confidence, octave ambiguity, acquisition time, spectral behavior and audible usefulness must be assessed separately. No implementation, build, install or deployment is established by this design document.

User revision (2026-10-07): FM ratio picker remains visible beside FM on the main interface. FM Depth defaults/reset/presets/legacy missing-field migration now use 66%; Amount still defaults to zero.
