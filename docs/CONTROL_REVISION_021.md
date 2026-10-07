# MEGATUBULAS 0.2.1 — audible Memory and Coupling

2026-10-07. User requested a more audible range for both controls after approving the bass-first Drive sound. This revision changes the engine intentionally; parameter IDs, defaults, state schema, latency, plugin codes and interface layout are retained. Existing sessions load but sound different. The prior installed binary is preserved by the installer.

## Cause and selected change

In 0.2.0 the raw stage envelope was usually small, was compressed again by tanh, and affected drive only through a weak multiplier. Memory changed a time constant without a meaningful depth range. Heavy existing saturation masked these changes. On the supplied recording, independently RMS-matched extremes differed by only 0.493% (Memory) and 0.737% (Coupling) of reference RMS.

- Memory now captures rising amplitude with a 3 ms attack and releases over nominal 12 ms–2.4 s, with the existing bounded voice multiplier. Increasing Memory deepens envelope-controlled sag, attenuation and bias history. A loud note can reshape a quieter note until the histories discharge. It also retains its existing animation/afterglow role.
- Coupling now has stronger upstream envelope-to-drive/bias influence and a bounded, 700 Hz filtered feed-forward contribution from earlier stages. This changes the low/mid harmonic texture rather than relying on an overall gain change. Reads use previous-sample upstream histories; there is no feedback loop or autonomous oscillator.
- The bass body and soft top shelves, four user EQ bands, oversampling, clean Blend, matching and selector are retained. Drive zero still makes these new audio contributions vanish.

Exact equations are in `DSP_DESIGN.md`. Signal level and history affect the result; neither knob has a universal audibility guarantee or a physical circuit equivalence claim.

## Regression and measurement evidence

The dynamic bass regression removes the best-fitting constant gain before checking waveform differences. The old core fails all twelve mode/quality comparisons and the new long-recovery test (`analysis/controls-before.log`, `analysis/controls-recovery-before.log`). The selected new core passes. A constant level change alone cannot satisfy this regression.

At Drive 75, the three-character / 4× and 8× matrix gives:

| Control extremes | Difference after constant-gain removal |
| --- | ---: |
| Memory, Coupling 0 | 36.99–37.58% |
| Coupling, Memory 40 | 18.93–21.96% |

On the user's supplied private recording, Warm / Drive 75 / Blend 100 / 8×, omitting the first second:

| Control extremes | RMS-matched waveform difference | Difference after constant-gain removal |
| --- | ---: | ---: |
| Memory 0 vs 100, Coupling 0 | 27.88% | 27.60% |
| Coupling 0 vs 100, Memory 40 | 20.57% | 20.46% |

These percentages are waveform residuals, **not THD, perceived difference scores or listener acceptance**. Measurements are `analysis/controls-v021/comparison.json`; `scripts/compare-controls.py` reproduces the calculation from the four render files. The private source/outputs remain local and are excluded from downloads.

A 100 Hz loud-to-quiet probe demonstrates recovery: Memory 0's early/late quiet-note RMS differs by less than 0.001 dB; Memory 100's early quiet note is 3.24 dB below its later level. At the default Drive 30 on the original public demo phrase, shape differences are smaller: Memory 3.25%, Coupling 12.05%. For a clear first comparison, use Drive 70–75, Blend 100 and sweep one control at a time. Match again or adjust Output after a setting change.

Bass regression: 100 Hz / 0.5 peak / Warm / Drive 80 / Memory 60 / Coupling 30 gives H2+H3 −23.59 dBFS and 6–15 kHz harmonic sum −52.91 dBFS. Quiet −30 dBFS bass at Drive 65 has H3 −21.74 dBc. This supports the requested bass-first direction on the exercised probes; it is not a general harshness score.

## Build, installation and host checks

- Release arm64 build, macOS minimum deployment target 12.0, completed (`analysis/build-v021.log`). Older macOS runtime not separately tested.
- CTest: 2/2 suites passed; 345 numerical and 118 integration checks, zero failures, zero guarded callback new/new[] allocations (`analysis/ctest-v021.log`). Includes deterministic replay, partition/rate/quality matrix, silence, non-finite/extreme input, DC recovery, rapid automation and state concurrency.
- Installed AU/VST3 locally with timestamped backups and ad-hoc signatures (`analysis/install-v021.log`). Restart/rescan the DAW and confirm **v0.2.1** in the footer.
- AU validation succeeds and identifies **0.2.1 / 0x201** (`analysis/auval-v021-current.log`). The first post-install scan returned cached 0.2.0; the user AudioComponentRegistrar was refreshed and validation rerun. No global cache deletion or DAW termination.
- Installed AU and VST3 pluginval strictness 5 both identify v0.2.1 and finish SUCCESS (`analysis/pluginval-au-v021.log`, `analysis/pluginval-vst3-v021.log`). These tools process offline and do not play test audio through speakers.
- Native screenshot exported to `Artwork/Layers/v021/00-interface-preview.png`; landing page uses it in the lightbox. Generated cathedral artwork is retained as promotional artwork.

The new engine has not yet received the user's listening acceptance. Windows, Intel Mac, notarization and public publication remain unverified/pending. This document is a traceable local engineering review, not independent certification.
