#!/usr/bin/env python3
"""Measure rendered knob extremes without counting a constant gain as a fix.

Usage: compare-controls.py /folder/with/memory-0.wav ... coupling-100.wav
The renderer settings must be identical apart from the named control. Uses
numpy like the project's existing analysis script. Does not upload audio.
"""
from pathlib import Path
import json
import sys
import wave
import numpy as np

folder = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("analysis/controls-v021")

def read(path):
    with wave.open(str(path), "rb") as wav:
        rate, channels, width = wav.getframerate(), wav.getnchannels(), wav.getsampwidth()
        raw = wav.readframes(wav.getnframes())
    if width == 3:
        b = np.frombuffer(raw, np.uint8).reshape(-1, 3).astype(np.int32)
        x = b[:, 0] | b[:, 1] << 8 | b[:, 2] << 16
        x = np.where(x & 0x800000, x - 0x1000000, x) / 8388608.
    elif width == 2:
        x = np.frombuffer(raw, "<i2").astype(np.float64) / 32768.
    else:
        raise ValueError("Expected PCM16 or PCM24")
    x = x.reshape(-1, channels)[rate:]
    if not len(x):
        raise ValueError("Need more than one second of audio")
    return x, rate

report = {}
for knob in ("memory", "coupling"):
    a, ra = read(folder / f"{knob}-0.wav")
    b, rb = read(folder / f"{knob}-100.wav")
    if a.shape != b.shape or ra != rb:
        raise ValueError("Comparison files must have identical format and length")
    aa, bb, ab = np.sum(a*a), np.sum(b*b), np.sum(a*b)
    if min(aa, bb) <= 1.e-20:
        raise ValueError("Cannot compare silence")
    # RMS-match independently here even if the render files were not trimmed.
    matched = b * np.sqrt(aa/bb)
    fit = ab/bb
    report[knob] = {
        "rms_matched_difference_percent": float(np.sqrt(np.sum((a-matched)**2)/aa)*100),
        "constant_gain_removed_difference_percent": float(np.sqrt(np.sum((a-fit*b)**2)/aa)*100),
        "correlation": float(np.corrcoef(a.flatten(), b.flatten())[0, 1])
    }
report["conditions"] = "Omit first second; RMS matching and best-fitting constant-gain removal. Same source/Drive/mode/quality required. Waveform residual is not THD or a general audibility score."
(folder / "comparison.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, indent=2))
