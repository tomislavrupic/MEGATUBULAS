# MEGATUBULAS DSP design · schema 1 / version 0.2.1

Experimental original design; no circuit emulation or equivalence claim. Darkglass's B7K control descriptions informed the familiar distinction between saturation amount, processed-path level, clean blend, and pre-distortion frequency emphasis. The engine and graphics are original. Its published ±12 dB nominal EQ frequencies match the supplied brief. The main Output control here trims the final blend; separate advanced Wet level provides the processed-path level function.

## Signal path and units

Input trim → linked analysis and dry split → optional −4/0/+4 dB bass/treble shelves on wet → JUCE FIR oversampling → three stages → FIR downsample → Drive/mode body and top shelves → four user RBJ EQ bands → wet level / held Match → linear latency-aligned Blend → Output trim → meters.

Base/oversampled rates are explicit; core coefficients use `fs_base * factor`. Samples are dimensionless full-scale amplitudes. Envelope is an absolute-amplitude proxy, not joules or physical energy. Bias is a dimensionless transfer offset. Coefficients and stage histories use double precision; JUCE conversion buffers are float.

For stage k, conditioned input begins as `u = y + amount*emphasis*(y-lowpass700(y))`. Let `q = Coupling/100 * voiceCoupling` (zero in ablation), and `m = Memory/100` (zero in ablation). Previous-sample upstream envelope contributes `c = q*tanh(4*e_previousStage)`; first-stage c=0. For stages 2 and 3, a filtered feed-forward contribution is added: `u += amount*.35*q*tanh(3*low_previousStage)`. Both upstream histories come from the previous oversampled sample. The low history is a 700 Hz one-pole of that stage's input. This gives Coupling a bass-focused interaction path in addition to drive/bias influence, without a feedback loop.

Envelope is `e = a*e + (1-a)*abs(u)`, bounded [0,8]. Rising input uses `a=exp(-1/(.003*fs_os))`; falling input uses `a=exp(-1/(tau*fs_os))`.

Nominal release `tau = .012 * 200^(Memory/100)` seconds, multiplied by candidate recovery [0.7,1.3]. This is a **1/e** convention, not complete settling. Fast attack captures the note; longer discharge retains its influence. Memory also raises the depth of that retained influence, since release-time changes alone were nearly inaudible on the user's already-compressed musical signal.

Bias target is `voiceBias*tanh(e+2*c)*(1+2*m*m)`, recovered with `0.6*tau`, bounded ±0.3. Set `charge=tanh(4*e)` and `sag=1/(1+1.8*m*m*charge)`.

Effective stage drive:
`d = 1 + (10^(1.6*amount)-1) * voiceDrive * (.8 + .18*k) * (1 + 1.4*c + .2*tanh(e)) * sag`.

Transfer:
`f(u) = [tanh(d*u+b) - tanh(b)] / [d^(1-.5*amount)*(1-tanh(b)^2)]`, `b=voiceAsymmetry+bias`.

Stage output is `[u + amount*(f(u)-u)] / [1 + .5*amount*m*m*charge]`, followed by a 12 Hz DC blocker while Drive > 0. Memory changes both saturation headroom and signal-dependent attenuation, so its audible result depends on input level/history; it is not a fixed-ratio compressor or a physical power-supply model. A loud note can suppress a subsequent quiet note until the envelopes discharge. Coupling changes harmonic texture as well as level. Use Match or Output to compare at similar levels.

Subtracting the zero-input transfer avoids a constant offset. Drive zero has unity small-signal slope; emphasis, nonlinear mix, feed-forward and sag attenuation all vanish and the DC blockers are bypassed. This retains a neutral calibration path through conversion/EQ. Controls smooth over 50 ms. Partial compensation at higher Drive retains the quiet-bass harmonics introduced in 0.2.0.

All coupling reads a copy of stage histories from the preceding oversampled sample. Stages themselves form a causal cascade. There is no implicit algebraic feedback or iterative solver. Ablation zeroes memory/bias/coupling/sag/feed-forward while retaining the basic nonlinear transfer and conditioning; it is not a bypass. The previous 0.2.0 core is preserved locally in the revision evidence, not used by the new processor.

Incoming nonfinite samples become zero; exceptionally large samples are bounded to ±32 before nonlinear processing. Internal envelope/bias/nonfinite guards are explicit safety bounds. Output is not limited to 0 dBFS; the meter flags overs. This is not a brickwall mastering limiter.

## Voicing and selection

Eight candidates per mode; `u=(index-3.5)/3.5`:

| Field | Warm | Tense | Open | Candidate deviation |
|---|---:|---:|---:|---|
| drive factor | 1 | 1.28 | 0.82 | ×(1+0.15u) |
| static asymmetry | 0.065 | 0.13 | 0.025 | +0.025u |
| bias sensitivity | 0.09 | 0.16 | 0.04 | ×(1+0.2u) |
| coupling factor | 1 | 1.35 | 0.65 | ×(1+0.2u) |
| recovery factor | 1 | 1 | 1 | +0.3u |
| high emphasis | −0.085 | −0.055 | −0.015 | +0.025u |

Open uses lower drive/bias/coupling to restrain compression; these mappings express a voicing target, not evidence of listener preference.

Linked stereo absolute level uses a 60 ms envelope; transient activity compares a 3 ms envelope against it. Per-channel 900 Hz low-pass histories supply low/high absolute-band proxies, combined across channels. Silence is defined by denominator floors (1e−9 for spectral ratio, .01 for transient normalization). Decisions share descriptors/draws, while each channel keeps its own nonlinear/filter histories.

Every 20 ms on the **oversampled sample clock**, candidates are scored for proximity to `0.2+0.4*brightness+0.2*transient`, with transition penalty `0.18*abs(index-previousIndex)`. Minimum dwell is 160 ms except explicit Explore/mode changes. Stable softmax subtracts maximum score before exponentiation; temperature is `.06+.65*Variation/100`. Draws choose from normalized probabilities. Variation also interpolates candidate deviation away from a base voice; at zero, a deterministic base voice is used and no random draws are consumed. Targets and primary nonlinear controls use 50 ms one-pole smoothing. No random samples are added to audio.

This is constrained state selection over an engineered bank, not a physical model of “collapse.” A future novelty review would compare closely with adaptive waveshaping, hysteretic/state-space nonlinear systems and stochastic parameter selection; no priority claim is made here.

## EQ, dynamics, phase and latency

Bass-first tone revision: after conversion, before user EQ, two slope-1 RBJ shelves provide deliberate voicing. At Drive 100, the 100 Hz body / 5 kHz top gains are Warm +2/−10 dB, Tense +1.5/−8 dB, Open +0.75/−6 dB. Gains scale linearly with Drive and are zero at Drive 0. Gains smooth over 20 ms; coefficients are re-derived every 32 base-rate samples. Mode changes also smooth through those gains. This preserves an explicit neutral calibration path and avoids a hidden limiter. Low-order harmonic generation remains the three-stage asymmetrical transfer; shelves do not themselves generate harmonics. Their phase affects the dry/wet mix. The user EQ remains downstream so the tonal target can be adjusted.

EQ: W3C/RBJ low/high shelves with slope 1, and bells Q=1/√2. Gains are smoothed over 20 ms; stable coefficients are re-derived from the smoothed gains on a fixed 32-sample base-rate clock. Arbitrary coefficients are not linearly interpolated. Frequencies are fixed, clamped to 0.4fs where necessary. Rapid-automation tests bound numerical behavior; this is not a proof of arbitrary time-varying filter stability.

JUCE `dsp::Oversampling<float>` uses maximum-quality FIR equiripple stages, exponent 2/3 for 4×/8×, integer latency enabled. `initProcessing` and filter construction run in prepare. Current resources stay active after quality automation; the new quality applies on the next prepare, which reports new latency outside the callback. Chunking supports blocks larger than the prepare hint with fixed 4096-or-smaller work buffers.

At 48 kHz the measured impulse maxima are **61 samples (4×)** and **65 (8×)**, matching `getLatencyInSamples` rounded to integer. Dry delay uses that latency. FIR conversion is linear phase; delay alignment does not remove the deliberately introduced phase of conditioning, EQ and DC blockers, nor nonlinear dry/wet cancellation. Linear calibration tests compare settled low-frequency signals with the delayed source to within RMS 0.002; partition comparisons tolerate 2e−6 due to float SIMD conversion paths.

Match measures linked dry/wet sum-of-squares over 2 s, before wet-level trim, and holds `10log10(E_dry/E_wet)` clamped to ±12 dB. A 20 ms gain smoother applies the result. Mean dry energy ≤1e−8 (−80 dBFS RMS) or negligible wet energy cancels the measurement and retains prior compensation. It is not a continuous level rider or LUFS match.

## Real-time boundaries

Fixed stage/candidate/entropy arrays, preallocated wet/dry/ring buffers, no file/network/GUI calls from processing. Four-slot bounded configuration queue: writers serialize under a non-audio mutex; audio consumes at most four fixed payloads, using release/acquire indexes. A full queue rejects a new configuration and reports failure; it never blocks audio. Configuration consumption resets transient history. State is serialized/parsed off audio. Atomic float/unsigned telemetry is compile-time checked lock-free; UI reads it at 30 Hz. Guarded integration tests intercept C++ new/new[] and find zero allocations in exercised callbacks, including configuration transfer. They do not instrument all platform malloc calls or prove every framework path.

## Consequential primary sources

- JUCE pinned source `juce_dsp/processors/juce_Oversampling.h`, [Oversampling API](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html): proven conversion filters, explicit init/latency semantics, FIR phase choice.
- [JUCE AudioProcessor](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html): prepare/reset/non-realtime/latency/state boundaries; installed pinned headers were checked as well as current docs.
- [W3C Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook/): RBJ shelf/peaking coefficient derivation.
- [Apple Audio Unit V2 API](https://developer.apple.com/documentation/audiotoolbox/audio-unit-v2-c-api), [VST3 processor lifecycle](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Workflow+Diagrams/Audio+Processor+Call+Sequence.html): native format/lifecycle checks, without asserting DAW acceptance.
- [B7K V2 control descriptions](https://www.darkglass.com/en-eu/products/b7k2): functional control inspiration only, no emulation claim.
- [JUCE licensing](https://juce.com/get-juce/), pinned JUCE `LICENSE.md`: AGPLv3 route chosen, no commercial license assumed.

Measurements and remaining uncertainties are in `VALIDATION.md` and `PIX7_AUDIT.md`.
