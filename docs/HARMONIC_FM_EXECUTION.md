# Harmonic FM execution decisions

The user approved the 2026-10-07 plan and native execution, then authorized the public v0.3 release. Implementation was isolated on `codex/harmonic-fm`; the original checkout and prior public release were preserved.

Recorded rulings:

- The app worktree API resolved the chat's unrelated working directory and failed. A normal Git worktree was created in the MEGATUBULAS repository instead. No Titty Tweeter source was changed.
- The silence fixture waits one second for the specified 80 ms envelope release after noise. A half-second was too short for its absolute threshold; the required threshold was retained.
- Task 3's full processor integration gate ran with Task 4 because new parameter tests were already red. Detector/voice/numerical and FM-off reference tests passed before the pipeline commit; subsequent full integration passed.
- Profiling found 70–78 microsecond detector spikes in one-sample callbacks. The 8 ms analysis snapshot clock was retained, with fixed YIN lag work spread over native samples. This adds about 5 ms analysis-completion time at 48 kHz; measured acquisition remains under the 150 ms requirement. No worker, future sample controls or host-block clock was introduced. OS scheduling outliers remain a documented limitation.
- User revisions placed Memory / Drive / FM inside the left / centre / right lattice, added signal/Drive-responsive amber grilles, replaced stretched bitmap strips with one whole plate, and required circular native mounting bolts. After visual acceptance, the ratio picker moved onto the main interface beside FM and Depth default/reset/presets/missing legacy fields changed to 66%. Explicit saved Depth values remain unchanged; Amount defaults to zero.
- The requested old “0.2.2” archive did not exist. The actual prior v0.2.1 release remains intact and linked as the archive.
- Fresh whole-change review identified two important findings. Rapid zero-crossing re-enable now preserves the outgoing oscillator/filter tail and detector envelope while reacquiring pitch. Full anti-phase stronger-channel handoff checks now cover the previously omitted switch/recovery interval. The transient pitch bound uses the specified 50-cent continuity window (observed maximum 41.6 cents on the amplitude-crossfade fixture), with settled pitch below 20 cents. Six Amount/Depth toggle regressions failed before the fix and pass afterward. No review issues are deferred.

- Final native Windows compilation succeeded but the numerical harness exceeded MSVC’s default 1 MiB thread stack (direct exception 0xc00000fd). Large Pipeline fixtures now use heap ownership established before processing; assertions and product DSP are unchanged. The production Windows workflow was restored after a focused diagnostic run. The complete native Windows release run at `7c1f6b0` passed all four suites and VST3 pluginval strictness 5.

The reviewer declined to infer Windows compatibility, final release packaging, musical listening approval or chord/full-mix operation from the implementation review. Windows, packaging and deployment have separate recorded gates; chord/full-mix tracking remains outside this monophonic experiment. See [Harmonic FM validation](HARMONIC_FM_VALIDATION.md) for measured results and release evidence.
