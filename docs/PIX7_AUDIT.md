# PIX-7 audit: MEGATUBULAS 0.2.0

Date: 2026-10-07. Scope: the local original DSP implementation and native interface. This follows the invoked PIX-7 skill's audit structure; it is an engineering review, not independent certification.

| Step | Finding |
| --- | --- |
| 1. Signal intake | Source, numerical fixtures, processor-state tests, native screenshots and local host-validator logs. |
| 2. Claim type | Software mechanism and bounded reproducibility claims. Artistic visual identity is a separate claim. |
| 3. Core claim | Three nonlinear stages retain causal histories; linked descriptors select bounded voices on a sample clock. |
| 4. Hidden assumptions | Identical input, sample rate, automation and initialization are required for replay. State loading does not restore every transient filter history. |
| 5. Dependency graph | Input trim → descriptors/dry split → wet shaping → FIR upsampling → stateful stages/selector → FIR downsampling → EQ/wet gain → aligned blend → output trim. |
| 6. Mechanism status | MC within the documented digital architecture: explicit equations, ranges and causal updates. Sound-quality preference remains unverified. |
| 7. Interaction model | Shared stereo selection, separate channel histories, previous-sample upstream coupling. No implicit nonlinear feedback solver. |
| 8. Perturbation test | Sample-rate/block partition matrix, non-finite/extreme inputs, rapid core automation, state loads during processing, malformed imports and finite sequence exhaustion. |
| 9. Recoverability | Stable in exercised numerical configurations. Deterministic seeded/reset/replay output survives tested block partitions. |
| 10. Failure point | Imported provenance is unverified. Arbitrary seek changes signal history. A host that does not reprepare cannot apply pending quality. No final limiter means output can exceed 0 dBFS. |
| 11. Surviving signal | The memory-history probe and ablation produce measurable differences; identical selector draws produce the same audio regardless of provenance label. |
| 12. Compression loss | “Stateful saturation” abbreviates several distinct dynamics. RMS match is not LUFS matching. Reference residual is not isolated alias power. |
| 13. Ontology leap | None supported: no biological simulation, quantum processing, consciousness or entropy-based sound-quality advantage is claimed. |
| 14. Confidence topology | High local confidence for exercised deterministic behavior; narrower confidence for untested hosts, long sessions and extreme full-pipeline automation. Windows, Intel and matched musical listening remain open. |
| 15. Classification | ENGINEERING FEASIBLE / local numerical and host validation evidence. OPEN: musical acceptance and additional platform testing. |
| 16. Comment | The lattice is artwork. The filter histories are the mechanism. Keep them distinguishable. |

The rename itself does not change the mechanism. Version 0.2.0 separately revises Drive gain compensation and bass/top voicing, supported by regression and spectral measurements in VALIDATION.md.
