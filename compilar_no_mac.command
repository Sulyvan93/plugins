#!/bin/zsh
set -e
cd "$(dirname "$0")/.."
echo "=========================================="
echo " TREM DE MEDIR - COMPILACAO APPLE SILICON"
echo "=========================================="
if ! command -v cmake >/dev/null 2>&1; then
  echo "Falta CMake. Instale com: brew install cmake"
  exit 1
fi
if ! xcode-select -p >/dev/null 2>&1; then
  echo "Faltam as ferramentas Apple. Execute: xcode-select --install"
  exit 1
fi
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release --parallel 4
printf '\nCompilacao finalizada. As versoes AU e VST3 normalmente sao copiadas\npara ~/Library/Audio/Plug-Ins/ pelo sistema de build do JUCE.\n'
printf 'Reinicie o Logic Pro ou o Mixbus e procure por Trem de Medir.\n'
