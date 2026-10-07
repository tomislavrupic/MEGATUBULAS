#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p third_party
if [ ! -d third_party/JUCE ]; then
 git clone --depth 1 --branch 8.0.14 https://github.com/juce-framework/JUCE.git third_party/JUCE
fi
[ "$(git -C third_party/JUCE rev-parse HEAD)" = '2cdfca8feb300fb424002ba2c2751569e5bacb64' ] || { echo 'Unexpected JUCE revision'; exit 1; }
