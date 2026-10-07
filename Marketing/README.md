# MEGATUBULAS download landing page

Dependency-free static page, prepared for GitHub Pages. Approved banner v2 and actual native interface screenshot; no iframe or fixed-height embedded website. Copper, charcoal and engraved serif direction follows the plugin.

The interface section uses `assets/product-shot.png`, a new cathedral-scale imagegen product composition based on the native UI. Clicking it opens the unaltered `assets/interface.png`. Artwork provenance and prompt are retained in `Artwork/Banners/README.md` and `Artwork/Prompts/`.

## Local preparation

1. Build Apple Silicon with `-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DCMAKE_OSX_ARCHITECTURES=arm64` and run native validation.
2. `python3 scripts/make-site-demo.py` generates an original bass phrase at `Marketing/demo-source/original-bass.wav`.
3. Render it with `MegaRender` to `Marketing/site/assets/audio/{bass-dry,bass-warm65,bass-tense85}.wav`, using `dry 0 1`, `65 0 1`, and `85 1 1`. These are uniformly matched to −20 dBFS RMS. The supplied private recording is excluded.
4. `python3 scripts/package-downloads.py` creates Mac and corresponding-source ZIPs, plus SHA256 manifest. Includes pinned JUCE source and required build artwork; excludes private References, recordings, install backups and build output.
5. `node scripts/check-site.mjs` checks local links/anchors, package hashes and the corresponding-source download.
6. `python3 -m http.server 3037 --bind 127.0.0.1 --directory Marketing/site`.

Version 0.2.1 adds four comparison renders to the same player: `bass-memory{0,100}.wav` at Warm / Drive 75 / Coupling 0; `bass-coupling{0,100}.wav` at Warm / Drive 75 / Memory 40. Render with the optional Memory/Coupling arguments to `MegaRender`, matching to −20 dBFS RMS. The Warm/Tense demos are also refreshed with 0.2.1; dry remains the same latency-aligned bypass. All seven use the original public demo source, never the private supplied recording. The unaltered native screenshot is refreshed to v0.2.1.

## Windows release boundary

`.github/workflows/windows-build.yml` is a manual, read-only-permission CI workflow using Windows Server 2022 / Visual Studio 2022 x64. It builds VST3/standalone, runs numerical and processor checks and pluginval 1.0.4 strictness 5, then produces a ZIP and checksum/provenance evidence. It does not create a public release or deploy the site.

After the workflow succeeds, download `MEGATUBULAS-Windows-x64`, then run `python3 scripts/integrate-windows.py /artifact/folder`. This enables the Windows button only for matching version/hash/test evidence. Native build checks do not prove compatibility with every Windows DAW.

At preparation time, the Windows button explicitly says BUILD PENDING. No fabricated Windows download URL or Mac binary labelled as Windows.

## Publishing

The original brief says **“Do not publish, deploy”**. Local page and packages are prepared; repository creation/publication still requires the user's go-ahead. Recommended repository: `tomislavrupic/MEGATUBULAS`, public source under AGPLv3, with GitHub Pages serving `Marketing/site` and release assets holding the binaries/source ZIP.

Before publishing: run native Windows CI, attach all three checked ZIPs plus SHA256SUMS to a release, rewrite `downloads.json` entries to those actual release asset URLs, update the two static Mac/source fallback links and social image/canonical URL, and verify all downloads. Configure Pages only after approval. Check the deployed route independently of the push.

Official references: [GitHub Windows runners](https://docs.github.com/en/actions/how-tos/write-workflows/choose-where-workflows-run/choose-the-runner-for-a-job), [workflow artifacts](https://docs.github.com/en/actions/concepts/workflows-and-actions/workflow-artifacts), [JUCE source/build guidance](https://github.com/juce-framework/JUCE), [pluginval 1.0.4](https://github.com/Tracktion/pluginval/releases/tag/v1.0.4).
