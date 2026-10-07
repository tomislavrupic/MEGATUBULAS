# Local implementation plan
1. Build a framework-independent three-stage engine, fixed candidate selector and bounded entropy/replay configuration; exercise it numerically.
2. Wrap it in JUCE with FIR oversampling, dry latency alignment, native parameters, held RMS matching, presets and validated persistence.
3. Build an original resizable metal interface with cached helical geometry and real telemetry.
4. Build AUv2, VST3 and standalone on this Mac, run numerical/host validation and reproducible measurements. Supply Windows commands without claiming a Windows build.
5. Record mechanisms, provenance, actual evidence and remaining limitations. No publication or deployment.
