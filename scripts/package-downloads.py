#!/usr/bin/env python3
"""Create real local Mac/source downloads and a checksum-gated site manifest.

Run after build + validation. JUCE source is included at its pinned revision.
Does not publish anything or package private recordings/references/backups.
"""
from pathlib import Path
import hashlib
import json
import re
import shutil
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
site = root / "Marketing/site"
downloads = site / "downloads"
downloads.mkdir(parents=True, exist_ok=True)
version = re.search(r"project\(MEGATUBULAS VERSION ([0-9.]+)", (root / "CMakeLists.txt").read_text())[1]
cache = (root / "build/CMakeCache.txt").read_text()
juce = Path(re.search(r"^JUCE_PATH:PATH=(.+)$", cache, re.M)[1])
pin = "2cdfca8feb300fb424002ba2c2751569e5bacb64"
actual = subprocess.check_output(["git", "-C", str(juce), "rev-parse", "HEAD"], text=True).strip()
if actual != pin:
    raise SystemExit("JUCE revision does not match reviewed pin")
if subprocess.check_output(["git", "-C", str(juce), "status", "--porcelain"], text=True).strip():
    raise SystemExit("JUCE has local changes; do not mislabel corresponding source")

source_name = f"MEGATUBULAS-{version}-source.zip"
source_zip = downloads / source_name
prefix = f"MEGATUBULAS-{version}-source"
with zipfile.ZipFile(source_zip, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for name in ["CMakeLists.txt", "README.md", "LICENSE", ".gitignore"]:
        archive.write(root / name, f"{prefix}/{name}")
    for folder in ["Source", "Tests", "scripts", "docs", ".github"]:
        for path in sorted((root / folder).rglob("*")):
            if path.is_file() and "__pycache__" not in path.parts:
                archive.write(path, f"{prefix}/{path.relative_to(root)}")
    for path in sorted((root / "packaging").rglob("*")):
        if path.is_file() and "stage" not in path.relative_to(root / "packaging").parts and path.suffix in [".txt", ".command", ".md"]:
            archive.write(path, f"{prefix}/{path.relative_to(root)}")
    for path in sorted((root / "Artwork/Animation/v2").glob("*.png")):
        archive.write(path, f"{prefix}/{path.relative_to(root)}")
    for name in ["frame-background-v1.png", "lattice-strip-v2.png"]:
        path = root / "Artwork/AI" / name
        archive.write(path, f"{prefix}/{path.relative_to(root)}")
    for name in ["provenance.json"]:
        path = root / "Artwork/Animation/v2" / name
        data = json.loads(path.read_text())
        # Preserve hashes/lineage, omit the artist's local file-system paths.
        def redact(value):
            if isinstance(value, dict):
                return {k: redact(v) for k, v in value.items() if not (isinstance(v, str) and v.startswith(("/Users/", "/Volumes/")))}
            if isinstance(value, list):
                return [redact(v) for v in value]
            return value
        archive.writestr(f"{prefix}/{path.relative_to(root)}", json.dumps(redact(data), indent=2))
    for path in sorted(site.rglob("*")):
        if path.is_file() and "downloads" not in path.relative_to(site).parts and path.name != "downloads.json":
            archive.write(path, f"{prefix}/{path.relative_to(root)}")
    tracked = subprocess.check_output(["git", "-C", str(juce), "ls-files", "-z"]).decode().split("\0")
    for name in tracked:
        if name and (juce / name).is_file():
            archive.write(juce / name, f"{prefix}/third_party/JUCE/{name}")
    archive.writestr(f"{prefix}/JUCE-REVISION.txt", pin + "\n")

mac_name = f"MEGATUBULAS-{version}-macOS-arm64.zip"
stage = root / "packaging/stage" / f"MEGATUBULAS-{version}-macOS-arm64"
if stage.exists():
    shutil.rmtree(stage)  # Only this script's known derived staging folder.
stage.mkdir(parents=True)
build = root / "build/Megatubulas_artefacts/Release"
for format_, bundle in [("AU", "MEGATUBULAS.component"), ("VST3", "MEGATUBULAS.vst3"), ("Standalone", "MEGATUBULAS.app")]:
    source = build / format_ / bundle
    if not source.is_dir():
        raise SystemExit(f"Missing native build: {source}")
    binary = source / "Contents/MacOS/MEGATUBULAS"
    info = subprocess.check_output(["vtool", "-show-build", str(binary)], text=True)
    if "minos 12.0" not in info:
        raise SystemExit(f"Unexpected deployment target: {source}")
    subprocess.run(["ditto", str(source), str(stage / bundle)], check=True)
    subprocess.run(["codesign", "--force", "--deep", "--sign", "-", str(stage / bundle)], check=True)
    subprocess.run(["codesign", "--verify", "--deep", "--strict", str(stage / bundle)], check=True)
shutil.copy2(root / "LICENSE", stage / "LICENSE.txt")
shutil.copy2(root / "packaging/mac/INSTALL.txt", stage / "INSTALL.txt")
shutil.copy2(root / "packaging/mac/Install plugins.command", stage / "Install plugins.command")
shutil.copy2(root / "packaging/SOURCE.txt", stage / "SOURCE.txt")
(stage / "Install plugins.command").chmod(0o755)
mac_zip = downloads / mac_name
if mac_zip.exists():
    mac_zip.unlink()
subprocess.run(["ditto", "-c", "-k", "--sequesterRsrc", "--keepParent", str(stage), str(mac_zip)], check=True)

def entry(path):
    return {"status": "ready", "url": "downloads/" + path.name, "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
manifest = {"version": version, "downloads": {"mac": entry(mac_zip), "source": entry(source_zip),
             "windows": {"status": "pending", "reason": "Native Windows x64 build and checks have not run."}}}
# Preserve a separately verified Windows package if a later native build has been integrated.
windows_zip = downloads / f"MEGATUBULAS-{version}-Windows-x64.zip"
evidence = root / "packaging/windows-validation.json"
if windows_zip.is_file() and evidence.is_file():
    checked = json.loads(evidence.read_text())
    if (checked.get("version") == version and checked.get("platform") == "Windows-x64"
            and checked.get("sha256") == hashlib.sha256(windows_zip.read_bytes()).hexdigest()
            and checked.get("testsPassed") is True):
        manifest["downloads"]["windows"] = entry(windows_zip)
(site / "downloads.json").write_text(json.dumps(manifest, indent=2) + "\n")
(downloads / "SHA256SUMS.txt").write_text("".join(f"{item['sha256']}  {Path(item['url']).name}\n" for item in manifest["downloads"].values() if item["status"] == "ready"))
print(json.dumps(manifest, indent=2))
