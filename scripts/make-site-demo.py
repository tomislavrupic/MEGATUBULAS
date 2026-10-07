#!/usr/bin/env python3
"""Original deterministic bass phrase; no user recordings or external samples."""
from pathlib import Path
import math
import struct
import wave

root = Path(__file__).resolve().parents[1]
target = root / "Marketing" / "demo-source" / "original-bass.wav"
target.parent.mkdir(parents=True, exist_ok=True)
rate, bpm = 48000, 104
beat = 60 / bpm
notes = [33, 33, 45, 40, 33, 43, 40, 38, 33, 33, 45, 40, 36, 43, 38, 31]
frames = bytearray()
for n in range(round(rate * beat * len(notes))):
    t = n / rate
    index = min(len(notes) - 1, int(t / beat))
    local = t - index * beat
    f = 440 * 2 ** ((notes[index] - 69) / 12)
    env = min(1, local / .008) * math.exp(-local / .33)
    env *= min(1, max(0, beat - local) / .03)
    # Smooth three-partial oscillator, with a small pitch-settling transient.
    phase = 2 * math.pi * f * (local + .0008 * (1 - math.exp(-local / .025)))
    bass = env * (.22 * math.sin(phase) + .045 * math.sin(2 * phase) + .012 * math.sin(3 * phase))
    kick = .12 * math.exp(-local / .09) * math.sin(2 * math.pi * (47 * local + 2 * (1 - math.exp(-local / .02))))
    value = round(max(-.95, min(.95, bass + kick)) * 32767)
    frames.extend(struct.pack("<hh", value, value))
with wave.open(str(target), "wb") as out:
    out.setnchannels(2)
    out.setsampwidth(2)
    out.setframerate(rate)
    out.writeframes(frames)
print(target)
