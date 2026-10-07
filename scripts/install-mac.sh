#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
product_build="$PWD/build/Megatubulas_artefacts/Release"
install_bundle() {
 source_bundle="$1"; target_bundle="$2"
 [ -d "$source_bundle" ] || { echo "Missing build: $source_bundle"; exit 1; }
 mkdir -p "$(dirname "$target_bundle")"
 if [ -e "$target_bundle" ]; then
  backup_folder="$PWD/install-backups/$(date +%Y%m%d-%H%M%S)"
  mkdir -p "$backup_folder"
  ditto "$target_bundle" "$backup_folder/$(basename "$target_bundle")"
 fi
 ditto "$source_bundle" "$target_bundle"
 # Do not preserve the old registration timestamp when replacing an AU bundle.
 touch "$target_bundle/Contents/Info.plist" "$target_bundle"
 codesign --force --deep --sign - "$target_bundle"
 codesign --verify --deep --strict "$target_bundle"
}
install_bundle "$product_build/AU/MEGATUBULAS.component" "$HOME/Library/Audio/Plug-Ins/Components/MEGATUBULAS.component"
install_bundle "$product_build/VST3/MEGATUBULAS.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/Pixel Records/MEGATUBULAS.vst3"
# Retire our earlier development name after the renamed build is installed.
# The AU/VST identity codes are intentionally retained: do not register both names.
legacy_backup="$PWD/install-backups/legacy-name-$(date +%Y%m%d-%H%M%S)"
for legacy_bundle in "$HOME/Library/Audio/Plug-Ins/Components/MICROTUBULAS.component" "$HOME/Library/Audio/Plug-Ins/VST3/Pixel Records/MICROTUBULAS.vst3"; do
 if [ -d "$legacy_bundle" ]; then
  mkdir -p "$legacy_backup"
  mv "$legacy_bundle" "$legacy_backup/"
 fi
done
echo 'Installed locally. Restart/rescan the DAW; manufacturer is Pixel Records.'
