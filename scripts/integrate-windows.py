#!/usr/bin/env python3
"""Integrate a native CI package only when its evidence/checksum matches."""
from pathlib import Path
import hashlib
import json
import shutil
import sys

if len(sys.argv) != 2:
    raise SystemExit("Usage: integrate-windows.py /folder/with/downloaded/workflow/artifacts")
root = Path(__file__).resolve().parents[1]
folder = Path(sys.argv[1]).resolve()
evidence = json.loads((folder / "windows-validation.json").read_text())
manifest_path = root / "Marketing/site/downloads.json"
manifest = json.loads(manifest_path.read_text())
version = manifest["version"]
name = f"MEGATUBULAS-{version}-Windows-x64.zip"
package = folder / name
sha = hashlib.sha256(package.read_bytes()).hexdigest()
if evidence.get("version") != version or evidence.get("testsPassed") is not True or evidence.get("sha256") != sha or evidence.get("platform") != "Windows-x64":
    raise SystemExit("Windows version, test evidence or checksum mismatch")
if not evidence.get("commit") or not evidence.get("run"):
    raise SystemExit("Missing native CI provenance")
target = root / "Marketing/site/downloads" / name
shutil.copy2(package, target)
shutil.copy2(folder / "windows-validation.json", root / "packaging/windows-validation.json")
manifest["downloads"]["windows"] = {"status": "ready", "url": "downloads/" + name, "bytes": target.stat().st_size, "sha256": sha}
manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
print(f"Integrated checked Windows package: {target}")
