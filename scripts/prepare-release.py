#!/usr/bin/env python3
"""Point the site at the actual, checksum-verified GitHub release asset names.

Run after local packaging and Windows integration, before uploading/publishing.
Does not itself publish or upload anything.
"""
from pathlib import Path
import hashlib
import json
import re

root = Path(__file__).resolve().parents[1]
site = root / "Marketing/site"
manifest_path = site / "downloads.json"
manifest = json.loads(manifest_path.read_text())
version = manifest["version"]
base = f"https://github.com/tomislavrupic/MEGATUBULAS/releases/download/v{version}/"
page = "https://tomislavrupic.github.io/MEGATUBULAS/"
html = (site / "index.html").read_text()
for platform, item in manifest["downloads"].items():
    if item["status"] != "ready":
        continue
    name = item["url"].split("/")[-1]
    package = site / "downloads" / name
    if package.stat().st_size != item["bytes"] or hashlib.sha256(package.read_bytes()).hexdigest() != item["sha256"]:
        raise SystemExit(f"Package verification failed: {platform}")
    item["url"] = base + name
    pattern = rf'<a\b[^>]*data-download="{platform}"[^>]*>.*?</a>'
    def update(match):
        tag = re.sub(r'\s(?:aria-disabled|tabindex)="[^"]*"', '', match[0])
        if 'href="' in tag:
            tag = re.sub(r'href="[^"]*"', f'href="{item["url"]}"', tag, count=1)
        else:
            tag = tag.replace('>', f' href="{item["url"]}" download>', 1)
        return tag.replace('Windows build pending', 'Download for Windows')
    html, count = re.subn(pattern, update, html, flags=re.S)
    if count != 1:
        raise SystemExit(f"Expected one {platform} download link")
    info = f"v{version} · {item['bytes']/1048576:.1f} MB · ZIP"
    html = re.sub(rf'(<p[^>]*id="{platform}-file-info"[^>]*>).*?(</p>)', lambda m: m[1]+info+m[2], html, flags=re.S)
if manifest["downloads"]["windows"]["status"] == "ready":
    html = html.replace('class="release-state pending" id="windows-state">BUILD PENDING', 'class="release-state" id="windows-state">AVAILABLE')
    html = html.replace('The Windows source target and build workflow are prepared. A downloadable Windows binary has not yet been built and checked.', 'Native Windows x64 build. Numerical, processor-state and VST3 pluginval checks passed before packaging. This build is unsigned. Additional DAW compatibility remains a listening and host check.')
    html = html.replace('Once available, unzip', 'Unzip').replace('AU and VST3 validated on this Apple Silicon Mac.', 'Mac AU/VST3 and Windows x64 VST3 passed native validation.')
html = html.replace('content="assets/megatubulas-banner.png"', f'content="{page}assets/megatubulas-banner.png"')
if 'rel="canonical"' not in html:
    html = html.replace('  <title>', f'  <link rel="canonical" href="{page}">\n  <meta property="og:url" content="{page}">\n  <title>', 1)
(site / "index.html").write_text(html)
manifest_path.write_text(json.dumps(manifest, indent=2)+"\n")
(site / "downloads/SHA256SUMS.txt").write_text(''.join(f"{item['sha256']}  {item['url'].split('/')[-1]}\n" for item in manifest["downloads"].values() if item["status"] == "ready"))
print(f"Prepared release links: v{version}")
