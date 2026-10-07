import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { spawnSync } from 'node:child_process';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const site = path.join(root, 'Marketing/site');
const html = fs.readFileSync(path.join(site, 'index.html'), 'utf8');
const ids = [...html.matchAll(/\bid="([^"]+)"/g)].map(match => match[1]);
assert.equal(new Set(ids).size, ids.length, 'Unique HTML ids');
assert(!/<iframe\b/i.test(html), 'Page uses full viewport, not an iframe');
assert(!/KITKAT_2026|KITKAT Recordings|\/Users\/thecore/.test(html), 'No private media or local paths in page');
let links = 0;
for (const match of html.matchAll(/\b(?:href|src)="([^"]+)"/g)) {
  const url = match[1];
  if (/^(https?:|mailto:|data:)/.test(url)) continue;
  if (url.startsWith('#')) {
    if (url.length > 1) assert(ids.includes(url.slice(1)), `Anchor ${url} exists`);
  } else {
    const file = path.resolve(site, url.split(/[?#]/)[0]);
    assert(file.startsWith(site + path.sep), 'Reference stays within site');
    assert(fs.statSync(file).isFile(), `Local asset exists: ${url}`);
  }
  links++;
}
const syntax = spawnSync(process.execPath, ['--check', path.join(site, 'app.js')], { encoding: 'utf8' });
assert.equal(syntax.status, 0, syntax.stderr);
const manifest = JSON.parse(fs.readFileSync(path.join(site, 'downloads.json')));
let packages = 0;
for (const platform of ['mac', 'windows', 'source']) {
  const item = manifest.downloads[platform];
  assert(['ready', 'pending'].includes(item.status), 'Explicit package status');
  if (item.status === 'pending') {
    assert.equal(platform, 'windows', 'Only native Windows build may be pending');
    assert(/id="windows-download"[^>]*aria-disabled="true"/.test(html), 'Pending Windows button disabled');
    continue;
  }
  let local = item.url;
  if (item.url.startsWith('https://')) {
    const release = new URL(item.url);
    assert.equal(release.origin, 'https://github.com', 'Release uses GitHub');
    const prefix = `/tomislavrupic/MEGATUBULAS/releases/download/v${manifest.version}/`;
    assert(release.pathname.startsWith(prefix), 'Release URL has the matching repository/version');
    const name = decodeURIComponent(release.pathname.slice(prefix.length));
    assert(name && !name.includes('/'), 'Release asset is a filename');
    local = `downloads/${name}`;
  }
  assert(local.startsWith('downloads/'), 'Package has a local verified counterpart');
  const fallback = [...html.matchAll(/<a\b[^>]*>/g)].find(match => match[0].includes(`data-download="${platform}"`))?.[0];
  assert(fallback?.includes(`href="${item.url}"`), `${platform} static download matches manifest`);
  const file = path.resolve(site, local);
  const bytes = fs.readFileSync(file);
  assert.equal(bytes.length, item.bytes, `${platform} package size matches manifest`);
  assert.equal(crypto.createHash('sha256').update(bytes).digest('hex'), item.sha256, `${platform} checksum matches`);
  packages++;
}
const demos = ['bass-dry.wav', 'bass-warm65.wav', 'bass-tense85.wav', 'bass-memory0.wav', 'bass-memory100.wav', 'bass-coupling0.wav', 'bass-coupling100.wav'];
for (const name of demos) {
  const buffer = fs.readFileSync(path.join(site, 'assets/audio', name));
  assert.equal(buffer.toString('ascii', 0, 4), 'RIFF', 'Demo is a WAV');
  assert(buffer.length > 100000, 'Demo contains actual audio');
}
console.log(`PASS: ${links} local references, ${packages} checked download packages, ${demos.length} original audio demos, JavaScript syntax, anchors and pending-platform safeguards.`);
