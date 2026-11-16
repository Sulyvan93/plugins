#!/bin/zsh
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release --parallel 4
echo "Build finalizada. Encontre os arquivos em build/TremDeMedir_artefacts/Release/"
echo "Instale o AU em ~/Library/Audio/Plug-Ins/Components e o VST3 em ~/Library/Audio/Plug-Ins/VST3."
