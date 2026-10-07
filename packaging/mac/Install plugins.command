#!/bin/sh
set -eu
cd "$(dirname "$0")"
backup_root="$HOME/Library/Application Support/Pixel Records/MEGATUBULAS/Backups/$(date +%Y%m%d-%H%M%S)-$$"
install_plugin() {
  bundle="$1"; destination="$2"
  [ -d "$bundle" ] || { echo "Missing $bundle. Unzip the complete download first."; exit 1; }
  mkdir -p "$(dirname "$destination")"
  if [ -e "$destination" ]; then
    mkdir -p "$backup_root"
    ditto "$destination" "$backup_root/$(basename "$destination")"
  fi
  ditto "$bundle" "$destination"
  touch "$destination/Contents/Info.plist" "$destination"
  codesign --verify --deep --strict "$destination"
}
install_plugin MEGATUBULAS.component "$HOME/Library/Audio/Plug-Ins/Components/MEGATUBULAS.component"
install_plugin MEGATUBULAS.vst3 "$HOME/Library/Audio/Plug-Ins/VST3/Pixel Records/MEGATUBULAS.vst3"
echo "Installed. Restart and rescan your DAW. Look for Pixel Records / MEGATUBULAS v0.2.1."
echo "Existing copies were backed up, if present. Press Return to close."
read -r answer
