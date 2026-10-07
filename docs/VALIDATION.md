# MEGATUBULAS 0.2.0 — local validation

Historical 0.2.0 evidence below. The subsequent Memory/Coupling revision and fresh 0.2.1 validation are documented in `CONTROL_REVISION_021.md`.

Date: 2026-10-07. Apple Silicon / macOS, Release build. Results describe this local build, not all DAWs or listening acceptance.

## Build, installation and native formats

- `cmake --build build --config Release -j 6`: exit 0 (`build/build-020-final.log`). AU, VST3, standalone, numerical/integration tools, preview and offline renderer built.
- `scripts/install-mac.sh`: AU and VST3 installed locally with backup and ad-hoc code signing, deep/strict signature verification. Installed bundle version 0.2.0. No Developer ID or notarization.
- AU: `~/Library/Audio/Plug-Ins/Components/MEGATUBULAS.component`.
- VST3: `~/Library/Audio/Plug-Ins/VST3/Pixel Records/MEGATUBULAS.vst3`.
- `auval -v aufx Mctb PxRc`: exit 0, AU VALIDATION SUCCEEDED, **Component Version 0.2.0 (0x200)** (`analysis/auval-megatubulas-v020.log`). Apple initially returned cached 0.1.0 metadata; bundle timestamps were refreshed and the user AudioComponentRegistrar restarted. The final direct registry probe and validator confirm 512 / 0x200. No global cache deletion or unrelated plugin scanning.
- pluginval strictness 5, installed VST3 and AU, in-process: SUCCESS, both explicitly identify v0.2.0 (`analysis/pluginval-vst3-v020.log`, `analysis/pluginval-au-v020.log`). Validators operate offline.
- Existing DAW processes may retain an already loaded binary. Restart the DAW/rescan and confirm **v0.2.0** in the editor footer.

## Numerical and processor behavior

- `ctest --test-dir build --output-on-failure -V`: 2/2 suites passed (`analysis/ctest-megatubulas.log`).
- Extended numerical/measurement run: **294 checks, 0 failures** (`analysis/tests-megatubulas.log`).
- Processor integration: **118 checks, 0 failures; guarded callback new/new[] allocations 0**. This guard is not a complete platform malloc audit.
- Rates 44.1/48/88.2/96 kHz; partitions 1/32/64/127/256/1024; 4×/8× FIR. Exercises deterministic replay, history dependence, memory ablation, finite extremes/automation, silence, DC recovery, import validation, state concurrency and latency.
- Measured 48 kHz latency: 61 samples (4×), 65 (8×).
- Quiet 100 Hz bass, −30 dBFS peak input, Warm / Drive 65: H3 is **−23.29 dBc**. The previous unity-slope engine failed this regression at −58.23 dBc (`analysis/drive-before.log`). The revised exponential Drive plus partial compensation produces substantially more low-order saturation.
- 100 Hz, 0.5 peak input, Warm / Drive 80: H2+H3 sum **−23.58 dBFS**; harmonic sum from 6–15 kHz **−50.47 dBFS**, about 26.89 dB lower. This is one probe, not a general musical harshness score.
- Warm / Drive 80 small-signal gains relative to Drive 0: 70 Hz +27.26 dB, 12 kHz +16.60 dB. These are deliberately signal-dependent gains, not output gain promises for a full mix. Use Match/Output to compare levels. Output has no brickwall limiter.
- Aligned two-tone residual against a 32× render: −35.28 dBFS (4×), −36.77 dBFS (8×). Includes conversion/filter/state differences; not isolated alias power (`analysis/metrics.json`).
- Short stereo 48 kHz benchmark: 0.067 / 0.131 seconds wall time per audio second at 4× / 8×. This occurred alongside other offline work; not a real-time deadline or host CPU guarantee (`analysis/cpu.csv`).

## Supplied musical input

Source preserved privately: the user-supplied test recording (not redistributed). 48 kHz, stereo, PCM24, 50.159 s. Input peak 0 dBFS; RMS −15.63 dBFS. This is a healthy-level input, not an under-level explanation for the earlier subtle effect.

Offline `MegaRender` processes the actual Pipeline at 8×. Files under `analysis/audition/` use independent uniform trims to **−20 dBFS RMS**, with a −1 dBFS peak ceiling policy (none requires extra peak attenuation); no limiter, no audio device, no replacement of the original:

| File | Setting | RMS before audition trim | Peak after trim |
| --- | --- | ---: | ---: |
| v020-dry.wav | delayed dry | −15.63 dBFS | −4.37 dBFS |
| v020-warm65.wav | Warm, Drive 65, Blend 100 | −15.76 dBFS | −14.67 dBFS |
| v020-tense85.wav | Tense, Drive 85, Blend 100 | −17.28 dBFS | −15.20 dBFS |

Warm 65 level-matched difference/source RMS is −5.63 dB; Tense 85 is −3.94 dB. This includes phase/voicing as well as distortion and is not a pure distortion percentage. The files enable user listening comparison. The user subsequently reported “sounds perfect”; exact loaded DAW version/settings were not independently captured.

## Native visual checks

- Native screenshot/layers exported with current editor, version footer and defaults (Drive 30 / Memory 40). `Artwork/Layers/00-interface-preview.png` is actual UI; `Artwork/Banners/megatubulas-banner-v2.png` is the selected generated marketing composition.
- Six-second native renderer audit: constant-sound maximum adjacent mean-brightness step **0.174%**; loud/quiet mean-light ratio **1.514**; PASS (`analysis/motion-audit-v020.log`, `analysis/motion-audit/motion-photometry.csv`). Test captures the actual interpolated lattice, not merely source images. Does not prove every display, frame rate or DAW UI path.
- Premultiplied RGBA blend regression prevents midpoint opacity dips; coherent-noise continuity/bounds tested. Memory controls slower playback/release; Variation controls speed; Drive controls the forward/back window start. Reduced motion freezes motion/noise drift.

## Remaining boundaries

User listening feedback is positive (“sounds perfect”). Exact loaded DAW version/reload verification remains uncaptured. Windows, Intel/universal Mac, long-session performance, additional hosts, accessibility screen-reader review and notarization are unverified. Local download packaging is described below. No source publication, binary release, deployment or Git initialization was performed.

## Download build and landing page

- Rebuilt Release arm64 with deployment target macOS 12.0 (`build/configure-download.log`, `build/build-download.log`). `vtool` confirms minimum OS 12.0, SDK 27.0; older macOS runtime compatibility was not separately exercised.
- Reinstalled AU/VST3 with ad-hoc signing (`analysis/install-download.log`). `ctest`: 2/2 passed, 274 numerical checks and 118 integration checks, zero failures / zero guarded callback new allocations (`analysis/ctest-download.log`). `auval` and VST3 pluginval strictness 5 passed (`analysis/auval-download.log`, `analysis/pluginval-download-vst3.log`).
- Mac download contains AU, VST3, standalone and optional user-folder installer with backups. Corresponding-source ZIP includes the complete pinned JUCE source plus build artwork, source and licence notices. SHA256 and byte counts are generated from the actual ZIPs. Private supplied audio is excluded.
- Landing page: real native screenshot lightbox, generated product artwork, three genuine RMS-matched native renders of an original bass phrase, installation disclosures, Mac/source downloads. Windows remains explicitly pending until native CI succeeds. Local asset/anchor/hash checks pass via `node scripts/check-site.mjs`.
- Browser checked at 1440 and 390 pixels: document width matches viewport, all page images load, character selection and audio-source selection work, screenshot lightbox opens. No audio autoplay. Browser evidence is under `output/playwright/`.

## Memory and Coupling on the supplied recording

Independent level-matched renders at Warm / Drive 75 / Blend 100 / 8×, excluding the first second, confirm small changes on this particular signal. Memory 0 versus 100 at Coupling 0: difference RMS is 0.493% of reference RMS (−46.15 dB relative). Coupling 0 versus 100 at Memory 40: 0.737% (−42.65 dB relative). This is waveform difference, not distortion percentage or a general audibility threshold. Evidence: `analysis/memory-coupling-comparison.json`. Their subtlety is consistent with the user's listening report. No DSP change was made after the user approved the sound.
