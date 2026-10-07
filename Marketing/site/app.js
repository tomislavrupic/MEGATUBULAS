"use strict";
const audio = document.querySelector("#demo-audio");
const audioStatus = document.querySelector("#audio-status");
const deck = document.querySelector(".listening-deck");
const audioFiles = { dry: "bass-dry.wav", warm: "bass-warm65.wav", tense: "bass-tense85.wav", memory0: "bass-memory0.wav", memory100: "bass-memory100.wav", coupling0: "bass-coupling0.wav", coupling100: "bass-coupling100.wav" };
let selectedAudio = "dry";
let audioSwitch = 0;
document.querySelectorAll("[data-audio]").forEach(button => {
  button.addEventListener("click", () => {
    const key = button.dataset.audio;
    if (key === selectedAudio) return;
    selectedAudio = key;
    const position = Number.isFinite(audio.currentTime) ? audio.currentTime : 0;
    const wasPlaying = !audio.paused;
    const generation = ++audioSwitch;
    document.querySelectorAll("[data-audio]").forEach(item => item.setAttribute("aria-pressed", String(item === button)));
    audio.pause();
    audio.src = `assets/audio/${audioFiles[key]}`;
    audioStatus.textContent = "LOADING COMPARISON";
    audio.addEventListener("loadedmetadata", () => {
      if (generation !== audioSwitch) return;
      audio.currentTime = Math.min(position, Math.max(0, audio.duration - .01));
      audioStatus.textContent = "READY WHEN YOU ARE";
      if (wasPlaying) audio.play().catch(() => { audioStatus.textContent = "PRESS PLAY TO LISTEN"; });
    }, { once: true });
    audio.load();
  });
});
audio.addEventListener("play", () => { deck.classList.add("is-playing"); audioStatus.textContent = "LISTENING"; });
audio.addEventListener("pause", () => { deck.classList.remove("is-playing"); audioStatus.textContent = "PAUSED"; });
audio.addEventListener("ended", () => { deck.classList.remove("is-playing"); audioStatus.textContent = "AGAIN? YOUR CALL."; });
audio.addEventListener("error", () => { deck.classList.remove("is-playing"); audioStatus.textContent = "AUDIO UNAVAILABLE — PLEASE RELOAD"; });
audio.addEventListener("timeupdate", () => {
  const ratio = audio.duration > 0 ? audio.currentTime / audio.duration : 0;
  document.querySelector("#playhead").style.left = `${Math.min(100, ratio * 100)}%`;
});

const descriptions = {
  warm: ["Weight without the glare.", "Asymmetrical saturation, a bass-body lift and the strongest top-end softening of the three characters. Start here for a fuller foundation."],
  tense: ["A firmer grip on the signal.", "Stronger drive, asymmetry and stage interaction. Push it for a denser, more compressed character. The top-end shelf still keeps the brightness in check."],
  open: ["A little more breathing room.", "Lower drive, bias and coupling, with gentler top-end softening. A lighter starting point when you want colour with less compression."]
};
document.querySelectorAll("[data-character]").forEach(button => {
  button.addEventListener("click", () => {
    const copy = descriptions[button.dataset.character];
    document.querySelectorAll("[data-character]").forEach(item => item.setAttribute("aria-pressed", String(item === button)));
    document.querySelector("#character-description h3").textContent = copy[0];
    document.querySelector("#character-description p").textContent = copy[1];
  });
});

const dialog = document.querySelector("#interface-dialog");
document.querySelectorAll(".image-open").forEach(button => button.addEventListener("click", () => dialog.showModal()));
document.querySelector(".dialog-close").addEventListener("click", () => dialog.close());
dialog.addEventListener("click", event => { if (event.target === dialog) dialog.close(); });

// Only make a platform downloadable when the packaged-file manifest says it is ready.
// Packaging verifies the file first; publication rewrites these URLs to release assets.
async function loadDownloads() {
  try {
    const response = await fetch("downloads.json", { cache: "no-cache" });
    if (!response.ok) throw new Error("No release manifest");
    const manifest = await response.json();
    for (const platform of ["mac", "windows", "source"]) {
      const entry = manifest.downloads?.[platform];
      if (!entry || entry.status !== "ready" || !entry.bytes || !/^[a-f0-9]{64}$/.test(entry.sha256)) continue;
      const url = new URL(entry.url, location.href);
      if (url.origin !== location.origin && url.protocol !== "https:") continue;
      const link = document.querySelector(`[data-download="${platform}"]`);
      link.href = url.href;
      link.removeAttribute("aria-disabled");
      link.removeAttribute("tabindex");
      if (platform === "windows") {
        link.innerHTML = 'Download for Windows <span aria-hidden="true">↓</span>';
        const state = document.querySelector("#windows-state");
        state.textContent = "AVAILABLE";
        state.classList.remove("pending");
        document.querySelector("#windows-explanation").textContent = "Native Windows x64 build. Numerical, processor-state and VST3 pluginval checks passed before packaging. This build is unsigned. Additional DAW compatibility remains a listening and host check.";
      }
      const info = document.querySelector(`#${platform}-file-info`);
      if (info) info.textContent = `v${manifest.version} · ${(entry.bytes / 1048576).toFixed(1)} MB · ZIP`;
    }
  } catch { /* Verified static download links remain usable without the manifest. */ }
}
loadDownloads();
