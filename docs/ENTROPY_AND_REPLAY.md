# Entropy, Freeze and replay

The plugin works offline. `EntropySource` has seeded xorshift32 and bounded imported/replay sequences. There is no network provider adapter, credential field, OS-random fallback or claim of verified quantum access. Xorshift32 is a deterministic non-cryptographic PRNG. Imported mode is labelled **provenance unverified**; the JSON supplies attribution, it does not prove physical quantum generation or statistical quality.

## Import format

```json
{
  "kind": "saved-sequence",
  "source": "Your generator / device and provenance reference",
  "sequenceId": "take-001",
  "importedAt": "2026-10-07T12:00:00Z",
  "start": 0,
  "values": [0, 4294967295, 18273645]
}
```

`kind` is `saved-sequence` or `quantum-derived`. At most 64 KiB UTF-8 JSON, 1–1024 integer uint32 values (0..4294967295), metadata fields nonempty and at most 127 UTF-8 bytes each, optional integer starting position within the array. No implicit repetition. Exhaustion **holds the last selected target** and displays `EXHAUSTED · HOLD`. No fallback randomness is substituted. Seeded mode remains available explicitly through the source menu. An imported sequence can be selected as saved sequence, preserving the values and metadata. Imports occur through the editor file chooser on the message thread, never the callback.

Draw mapping: `(uint32 + 0.5)/2^32`, strictly inside (0,1). Seed zero maps to PRNG state one; this avoids the xorshift zero absorbing state. Fixed candidate bank/score/mapping is identical across sources. Numerical tests confirm that identical draw values produce identical audio; provenance alone provides no sonic advantage.

## Repeatability contract

Save the plugin session state to store all parameter values, schema version, character, seed or full bounded sequence, saved starting position, source/ID/import time, Freeze target vector/index and held RMS-match offset. Credentials and transient signal histories are not stored. Save data is size-bounded and version/type checked before replacement. Invalid imports/states are rejected; a saturated transfer queue rejects updates without blocking audio.

**Freeze** stops voice selection, while stage envelopes/bias/DC/filter histories still respond to the input. It stores the selected target vector, not an entire prior performance. Initial interpolation settles toward that vector; a restored session starts with cleared histories.

**Saved sequence replay** evolves deterministically for identical input, automation, sample rate, quality, initial configuration and reset conditions. Reset/prepare/session restore clear histories, selector clock, prior decisions and draw cursor to the saved starting position. Variation zero consumes no draws. Explicit Explore and automation are part of the performance and must be repeated identically.

On transition into `isNonRealtime()`, histories/clock/cursor reset. Hosts that render multiple times while remaining continuously in non-realtime mode must reprepare or invoke Reset before each pass. The processor cannot infer arbitrary host bounce boundaries from an unchanged flag. Every offline draw comes from the saved PRNG/sequence; no live network exists in this build.

Arbitrary seek equivalence is **not promised**: to reproduce an intermediate passage, render its preceding history/preroll from the same reset point, or use an externally established checkpoint. Transport position alone does not reconstruct nonlinear histories. Floating-point identity across different CPUs/compiler versions is also not promised; same-build partition/replay tolerances are measured.

## Limits

A 1024-value sequence supplies about 164 seconds of selections at the 160 ms dwell in typical unchanged-mode operation; Explore and mode changes can consume it faster. This is a capacity estimate, not a timing guarantee. No auto-download, entropy certification, QRNG credentials or subjective “quantum sound” claim is included.
