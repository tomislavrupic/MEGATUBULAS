"""Offline executable contract; writes only disposable synthetic audio."""
import math, pathlib, struct, subprocess, sys, tempfile, wave
renderer = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    root = pathlib.Path(tmp)
    source = root / 'bass.wav'
    with wave.open(str(source), 'wb') as out:
        out.setparams((1, 2, 48000, 0, 'NONE', 'not compressed'))
        out.writeframes(b''.join(struct.pack('<h', int(10000 * math.sin(2 * math.pi * 55 * i / 48000))) for i in range(24000)))
    def run(extra, name):
        target = root / name
        result = subprocess.run([renderer, str(source), str(target), '35', '0', '0', '40', '30', *extra], capture_output=True, text=True)
        return result, target
    legacy, old = run([], 'old.wav')
    assert legacy.returncode == 0, legacy.stderr
    off, zero = run(['0', '25', '0'], 'off.wav')
    assert off.returncode == 0 and old.read_bytes() == zero.read_bytes(), 'old CLI must retain exact FM-off audio'
    on, active = run(['100', '70', '1'], 'on.wav')
    assert on.returncode == 0 and old.read_bytes() != active.read_bytes(), 'valid FM arguments must change audio'
    for arguments in (['nan'], ['inf'], ['-1'], ['101'], ['20oops'], ['20', 'nan'], ['20', '101'], ['20', '25', '-1'], ['20', '25', '3'], ['20', '25', '.5'], ['20', '25', '1junk']):
        result, target = run(arguments, 'invalid.wav')
        assert result.returncode != 0 and not target.exists(), f'invalid FM argument accepted: {arguments}'
    print('14 renderer CLI checks passed')
