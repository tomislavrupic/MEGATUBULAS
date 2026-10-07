# MEGATUBULAS · Pixel Records

*Size is a matter of perspective. Drive isn’t.*

Experimental original stateful saturation effect. Three causal saturating stages retain envelope/bias histories; a signal-conditioned selector explores eight bounded voices per character. It does not model biology or quantum consciousness. Imported entropy provenance is not an audio-quality claim.

The project directory retains its original MICROTUBULAS name for asset lineage. The plugin, bundles and UI are now MEGATUBULAS. Parameter IDs, the schema-1 `MICROTUBULAS` state root, and the AU/VST3 manufacturer/plugin codes remain unchanged. Version 0.2.1 intentionally strengthens Memory/Coupling: saved sessions load the same parameter values but their sound changes with the revised engine. The installer backs up the previous binary.

## Harmonic FM experiment (v0.3.0 candidate)

Version 0.3 adds an optional pitch-following FM voice before saturation. Memory sits in the left lattice, Drive in the centre, and FM Amount in the right. The small 1x/2x/3x harmonic ratio picker sits beside FM; Depth is in Advanced and defaults to 66%; the main interface shows tracking status and detected frequency. It targets single bass notes from 25–400 Hz, with continuous bends and a measured acquisition delay. Amount or Depth zero silences the entire layer. Existing presets and older sessions start with FM off. Version 0.3.0 is the experimental FM release; v0.2.1 is retained as the previous release archive. See [FM validation](docs/HARMONIC_FM_VALIDATION.md).

## Local build

C++20, CMake ≥3.24, JUCE **8.0.14**, pinned commit `2cdfca8feb300fb424002ba2c2751569e5bacb64`. This Mac build used Xcode 27 / Apple Clang 21, arm64. New project is isolated from Titty Tweeter; no source was copied from that processor. JUCE was consumed as a package dependency from its existing local cache during development.

```sh
./scripts/bootstrap.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release -j 8
ctest --test-dir build --output-on-failure
./build/MicroTests_artefacts/Release/MicroTests "$PWD/analysis"
python3 -m venv .venv
.venv/bin/pip install numpy==2.2.6
.venv/bin/python scripts/analysis.py
```

An existing JUCE checkout can be supplied with `-DJUCE_PATH=/absolute/path/to/JUCE`; verify its pinned revision before use. `scripts/bootstrap.sh` verifies the exact revision and refuses a mismatch. Ninja/Make artefacts use the configured build type; multi-configuration generators require `--config Release` / `ctest -C Release`.

Outputs: `build/Megatubulas_artefacts/Release/{AU,VST3,Standalone}`. On Windows, install Visual Studio C++ tools, CMake and Git:

```powershell
git clone --depth 1 --branch 8.0.14 https://github.com/juce-framework/JUCE.git third_party/JUCE
git -C third_party/JUCE rev-parse HEAD # must match the pinned revision above
cmake -S . -B build-win -A x64
cmake --build build-win --config Release --target Megatubulas_VST3 Megatubulas_Standalone MicroTests MicroIntegration MicroFMTests MegaRender
ctest --test-dir build-win -C Release --output-on-failure
```

Download v0.3 Mac AU/VST3/standalone, Windows x64 VST3/standalone and full corresponding source from [the landing page](https://tomislavrupic.github.io/MEGATUBULAS/) or [release v0.3.0](https://github.com/tomislavrupic/MEGATUBULAS/releases/tag/v0.3.0). [Native Windows CI](https://github.com/tomislavrupic/MEGATUBULAS/actions/runs/37619284751) passed 345 numerical checks, 118 processor integration checks and pluginval 1.0.4 strictness 5. Windows binaries are unsigned. Intel Mac / universal binaries are unverified.

## Install on macOS

```sh
./scripts/install-mac.sh
auval -v aufx Mctb PxRc
```

The AU belongs directly in `~/Library/Audio/Plug-Ins/Components/MEGATUBULAS.component`; Apple did not discover the initial nested folder installation. VST3 is in `~/Library/Audio/Plug-Ins/VST3/Pixel Records/MEGATUBULAS.vst3`. The host groups both under Pixel Records. Restart/rescan your DAW after installation. These are local ad-hoc-signed builds, **not Developer ID signed or notarized distribution packages**. The installer preserves any existing bundle in a timestamped backup before replacing it, and archives the earlier MICROTUBULAS-named development bundles to avoid registering the same plugin codes twice.

Standalone is an audition application and may request microphone/input permission. Numerical/preview/validator tools render offline and do not connect to speakers. No listening acceptance in a named DAW is claimed.

## Bass-first character

Three asymmetrical saturating stages generate harmonics from the signal. Candidate emphasis now softens upper frequencies instead of pre-boosting them. Before the user EQ, Drive introduces a gentle 100 Hz body shelf and a 5 kHz top-end shelf: Warm reaches +2 / −10 dB, Tense +1.5 / −8 dB, Open +0.75 / −6 dB at maximum Drive. Both shelves are neutral at Drive zero and smoothly re-derived under automation. This is explicit tonal voicing, not a limiter. The four EQ knobs and clean Blend remain available for brighter or more transparent settings.

## Controls

- **Drive:** amount of saturation. Zero is linear apart from deliberate EQ/conditioning and the resampling filters. Version 0.2.0 uses exponential stage drive with partial gain compensation, so quiet input also develops harmonics at higher settings. Drive adds input-dependent gain: use Match or Output for level comparisons.
- **Memory:** version 0.2.1 adds a fast 3 ms envelope attack, 12 ms–2.4 s nominal release (logarithmic), and increasing sag depth. Low values recover quickly; high values let a loud note reshape quieter notes that follow, then bloom back as the histories discharge. Higher values also increase bias memory.
- **Coupling:** version 0.2.1 gives a stronger upstream envelope influence on later drive/bias plus a bounded, 700 Hz filtered feed-forward path from the earlier stages. High values add weight and a denser harmonic texture. All reads use previous-sample state; no feedback loop.
- **Blend:** linear dry/wet crossfade, with dry delayed by measured conversion latency. **Output** trims the final mix. Advanced **Wet level** changes only the processed path.
- Four post-distortion EQ bands: 100 Hz low shelf, 500 Hz bell, 1.5 kHz bell, 5 kHz high shelf; ±12 dB. Advanced bass/treble conditioning changes what reaches saturation (−4/0/+4 dB shelves).
- **Warm / Tense / Open:** original curvature/asymmetry/coupling mappings, documented in `docs/DSP_DESIGN.md`.
- **Variation:** 0 is deterministic base voicing. Above zero, constrained choices use the selected entropy source. **Freeze** locks the selected voice vector, while audio-dependent memory keeps evolving. **Explore** requests a fresh decision at the next control tick; it respects Freeze.
- **Match:** two seconds of linked dry/wet RMS, then holds a correction limited to ±12 dB, applied smoothly. Silent measurements leave the previous correction unchanged. It is RMS matching, not perceptual LUFS matching. The held offset is saved in session state.
- **4× / 8×:** pending until the host calls prepare again. Stop playback and reopen/reprepare the plugin after changing quality. The callback never reallocates resources or notifies host latency changes.
- **Reset:** clears histories/control clock/entropy position to the saved starting position, preserving configuration and held match compensation. Bypass fades to the latency-aligned input-trimmed dry path over 20 ms.
- Advanced panel: input trim, wet level, pre-saturation voicing, seed, entropy import, reduced motion, memory/coupling ablation.

Sliders support mouse drag, editable numeric values, double-click default reset, keyboard arrows and host automation gestures. Controls have keyboard focus and tooltips. Parameter IDs are permanent at schema 1; future releases must migrate rather than repurpose them.

## Interface and animation assets

`Artwork/Banners` contains the generated banner and its provenance. `Artwork/Layers` contains registered transparent PNG layers and the actual native interface preview. `Artwork/AI` preserves generated background/source plates and versioned redesigns. Generation/edit prompts are in `Artwork/Prompts`; the built-in imagegen tool was used. The approved organic/gothic redesign is `lattice-organic-gothic-redesign-v3.png`.

The supplied second animation is embedded as 31 sampled PNG frames, with shared decoding outside the callback. The 121 supplied frames have no timing metadata; the mapping assumes nominal 30 fps. A one-source-second window plays forward and back; Drive 0–100 chooses its start from nominal 0–3 seconds. After the request for slower playback, wall-clock speed is now `0.50 − 0.35×Memory + 0.25×Variation`, with normalized knobs. Default Memory 40 / Variation 0 gives about 2.78 seconds each direction; maximum Memory / zero Variation gives 6.67 seconds each direction. Speed changes smooth over 400 ms; Drive position smooths over 50 ms. Reduced motion holds the Drive-selected frame and freezes the noise drift.

The native interface is 1320×880, with an undistorted 3:2 background plate drawn in one pass. Signal- and Drive-dependent amber lights glow inside the side grilles with a 65 ms attack / 650 ms release; their coherent highlights respect reduced motion. Black fluted synth-style knobs have aluminium caps and ivory pointers. A true premultiplied RGBA interpolation replaces two source-over draws, avoiding the 25% midpoint opacity dip. A bounded 49×19 coherent value-noise field modulates highlights through the lattice; audio peak telemetry controls its brightness with an 80 ms attack and a 350–1000 ms release set by Memory. This is a CPU-rendered shader-like effect on the UI thread, not a GPU shader or biological measurement. Memory also controls afterglow, Coupling draws connecting strands, and selection events are deliberately subdued.

Input animations include low-alpha codec residue; a bounded alpha threshold is applied in the UI texture importer, without changing the originals. Source paths and hashes are in `Artwork/Animation/v2/provenance.json`. The previous animation is preserved separately. Geometry is cached at fixed depth/count; no spectrum analysis is performed by the UI.

Export layers again without opening audio hardware:

```sh
./build/MicroPreview_artefacts/Release/MicroPreview.app/Contents/MacOS/MicroPreview "$PWD/Artwork/Layers"
```

## Evidence and limits

See `docs/CONTROL_REVISION_021.md`, `docs/VALIDATION.md`, `analysis/*.log`, CSV measurements, SVG plots and `docs/PIX7_AUDIT.md`. Mac AU validation, numerical tests, VST3 validator and state tests are distinct checks. They do not prove sound quality or compatibility with every DAW. The user reported positive listening feedback on their test (“sounds perfect”). Broader matched listening across bass, synthesizers, drums and full mixes remains unverified.

No hidden limiter. Input sanitization rejects NaN/infinity and bounds unusually large incoming samples to ±32 before processing; output may exceed 0 dBFS and is flagged by the meter. Internal stage safety bounds are documented. Final output is not brickwall limited.

No QRNG service adapter or credentials are included. Seeded mode is a PRNG, never quantum. Imported “quantum-derived” sequences carry file-supplied provenance that is not independently verified. Exhaustion holds the last voice and reports it. Offline rendering never accesses the network. See `docs/ENTROPY_AND_REPLAY.md` for reproducibility/reset rules and import schema.

## Licensing

Project source: AGPL-3.0-only, following the open-source route chosen for the Pixel Records plugin work. JUCE modules are used under their AGPLv3 option; see `LICENSE` and JUCE's `LICENSE.md` for bundled third-party notices. Binary redistribution includes the full corresponding-source ZIP and required notices alongside the platform downloads. Public release and GitHub Pages publication were authorized on 2026-10-07.
