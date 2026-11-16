#!/bin/bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
mkdir -p "$HOME/Documents/Trem de Medir/Logs"
LOGFILE="$HOME/Documents/Trem de Medir/Logs/compilacao-$(date +%Y%m%d-%H%M%S).log"
exec > >(tee -a "$LOGFILE") 2>&1

on_error() {
  local status=$?
  echo
  echo "ERRO: compilação interrompida (código $status)."
  echo "Registro completo: $LOGFILE"
  read -r -p "Pressione ENTER para fechar esta janela..." _ || true
  exit "$status"
}
trap on_error ERR

echo "TREM DE MEDIR - compilação local e instalação para Apple Silicon"
echo "Projeto: $ROOT"
echo "Registro: $LOGFILE"

if [[ "$(uname -m)" != "arm64" ]]; then
  echo "É necessário executar em um Mac Apple Silicon, sem Terminal em Rosetta."
  exit 2
fi
if ! xcrun --find clang >/dev/null 2>&1; then
  echo "Instale as ferramentas Apple. O instalador será aberto."
  xcode-select --install || true
  exit 3
fi
export PATH="$HOME/Applications/CMake.app/Contents/bin:/opt/homebrew/bin:/usr/local/bin:$PATH"
if ! command -v cmake >/dev/null 2>&1; then
  if command -v brew >/dev/null 2>&1; then
    echo "Instalando CMake com Homebrew..."
    brew install cmake
  else
    echo "CMake ausente. Instale pelo site: https://cmake.org/download/"
    open "https://cmake.org/download/" || true
    exit 4
  fi
fi

echo "Testando o motor DSP..."
cmake -S . -B build-dsp-local -DTREM_DSP_TEST_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp-local --parallel 3
ctest --test-dir build-dsp-local --output-on-failure

echo "Compilando AU, VST3 e aplicativo nativos ARM64..."
cmake -S . -B build-local -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-local --parallel 3

ARTIFACTS="$ROOT/build-local/TremDeMedir_artefacts"
AU="$(find "$ARTIFACTS" -type d -name '*.component' -print -quit)"
VST="$(find "$ARTIFACTS" -type d -name '*.vst3' -print -quit)"
APP="$(find "$ARTIFACTS" -type d -name '*.app' -print -quit)"
[[ -n "$AU" && -d "$AU" ]] || { echo "AU não encontrado."; exit 5; }
[[ -n "$VST" && -d "$VST" ]] || { echo "VST3 não encontrado."; exit 6; }
[[ -n "$APP" && -d "$APP" ]] || { echo "Standalone não encontrado."; exit 9; }

OUT="$HOME/Documents/Trem de Medir/Instaladores"
STAGING="$OUT/Packages"
BACKUP="$OUT/Backup-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$STAGING" "$BACKUP"
rm -rf "$STAGING/TremDeMedir.component" "$STAGING/TremDeMedir.vst3" "$STAGING/TremDeMedir.app"
ditto "$AU" "$STAGING/TremDeMedir.component"
ditto "$VST" "$STAGING/TremDeMedir.vst3"
if [[ -n "$APP" && -d "$APP" ]]; then
  ditto "$APP" "$STAGING/TremDeMedir.app"
fi

for ext in component vst3 app; do
  bundle="$STAGING/TremDeMedir.$ext"
  [[ -d "$bundle" ]] || continue
  binary="$(find "$bundle/Contents/MacOS" -maxdepth 1 -type f -print -quit)"
  [[ -n "$binary" ]] || { echo "Executável não encontrado em $bundle"; exit 7; }
  lipo -archs "$binary" | grep -qw arm64 || { echo "Binário não é ARM64: $binary"; exit 8; }
  codesign --force --deep --sign - "$bundle"
  codesign --verify --deep "$bundle"
  ditto -c -k --sequesterRsrc --keepParent "$bundle" "$OUT/TremDeMedir-$ext-arm64.zip"
done

install_bundle() {
  local source_bundle="$1" destination="$2"
  mkdir -p "$(dirname "$destination")"
  if [[ -e "$destination" ]]; then
    mv "$destination" "$BACKUP/$(basename "$destination")"
  fi
  ditto "$source_bundle" "$destination"
}
install_bundle "$STAGING/TremDeMedir.component" "$HOME/Library/Audio/Plug-Ins/Components/TremDeMedir.component"
install_bundle "$STAGING/TremDeMedir.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/TremDeMedir.vst3"
if [[ -d "$STAGING/TremDeMedir.app" ]]; then
  install_bundle "$STAGING/TremDeMedir.app" "$HOME/Applications/TremDeMedir.app"
fi

echo
echo "SUCESSO: plugins instalados no usuário atual."
echo "AU: ~/Library/Audio/Plug-Ins/Components/TremDeMedir.component"
echo "VST3: ~/Library/Audio/Plug-Ins/VST3/TremDeMedir.vst3"
echo "Pacotes ZIP: $OUT"
echo "Backup de versões anteriores: $BACKUP"
echo "Reabra o Logic Pro e o Harrison Mixbus. A validação nas DAWs ainda é necessária."
open "$OUT" || true
read -r -p "Pressione ENTER para fechar esta janela..." _ || true
