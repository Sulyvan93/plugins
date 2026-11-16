# Validação local — 16/11/2026

## Ambiente

- Mac Apple Silicon, execução nativa arm64, macOS 27.2.1.
- Command Line Tools 27.2, AppleClang 21.0.0, CMake 3.31.8, JUCE 8.0.10.
- Base do código: `26bd754507e7cb0baad91440a99034a2ca944ab1`.
- CMake instalado em `~/Applications/CMake.app`.

## Alterações

Interface branca com detalhes vermelhos e triângulo sem inscrições, conforme a descrição de Minas Gerais. A imagem de referência não foi recebida; sua correspondência visual ainda precisa ser conferida.

Cabeçalho e controles de referência em linhas separadas, cartões de loudness com regiões distintas para nome/valor/unidade e ponteiros VU recortados aos respectivos mostradores. Altura mínima de 880 px para preservar a leitura. Textos portugueses do editor e nomes de parâmetros usam conversão UTF-8 explícita; IDs, intervalos e processamento não foram alterados. `MeterDSP.cpp` e `MeterDSP.h` permanecem idênticos à base.

O script local encontra o CMake instalado na pasta Applications do usuário e exige também o Standalone antes de declarar sucesso.

## Resultado confirmado

- Configuração e compilação Release de AU, VST3 e Standalone concluídas, código de saída 0.
- `file` e `lipo -archs`: todos os três executáveis são arm64.
- Assinatura ad hoc aplicada; `codesign --verify --deep --strict` passou nos três bundles, inclusive após a instalação.
- Testes DSP em Release aprovados: loudness integrado, RMS, true peak, gate/silêncio, reset, fase invertida, 7.1.4 a 96 kHz, mono a 44,1 kHz e pesos de surround em quad.
- Aplicativo Standalone instalado, aberto e revisado visualmente; acentos, triângulo, cartões e recorte dos ponteiros confirmados.
- Sintaxe do script de instalação verificada com `bash -n`.

## Instalação

- AU: `~/Library/Audio/Plug-Ins/Components/TremDeMedir.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/TremDeMedir.vst3`
- Standalone: `~/Applications/TremDeMedir.app`

Os ZIPs contêm os bundles assinados localmente. Essa assinatura não equivale à notarização Apple.

## Limitações e pendências

- `auval -v aufx Tmdr Tram` não encontrou o componente; a listagem global de componentes também não retornou itens nesta sessão de execução. Um teste adicional carregou a biblioteca e localizou a função AU, mas `AudioComponentRegister` retornou nulo. Isso não comprova aprovação nem reprovação funcional do plugin no host; a validação AU permanece pendente.
- Logic Pro abriu e iniciou a atualização dos Audio Units, mas a automação passou a responder com timeout antes de confirmar a inserção do plugin.
- Mixbus 11 abriu em Recent Sessions; não foi possível avançar pela interface para inserir o plugin. Carregamento AU/VST3 e reprodução na DAW permanecem pendentes.
- Ainda validar sinal real nas DAWs, automação, reabertura de sessão, CSV e roteamento multicanal com o equipamento do usuário. Os testes DSP não certificam conformidade ITU/EBU.
- O SDK 27.2 emitiu avisos de APIs obsoletas dentro do JUCE/VST3 SDK. Não impediram a compilação. O pós-build do JUCE corrigiu a assinatura VST3 e a verificação final passou.
- A primeira compilação em Documentos encontrou arquivos JUCE `dataless` e timeouts de leitura. A compilação confirmada ocorreu em `/private/tmp/trem-m4-20261116`, fora da sincronização. Para recompilar, usar uma cópia local fora do iCloud ou garantir que todas as fontes estejam baixadas.

## Reproduzir a compilação

Com as fontes e JUCE baixados localmente:

```sh
CMAKE="$HOME/Applications/CMake.app/Contents/bin/cmake"
CTEST="$HOME/Applications/CMake.app/Contents/bin/ctest"
"$CMAKE" -S . -B build-dsp -DTREM_DSP_TEST_ONLY=ON -DCMAKE_BUILD_TYPE=Release
"$CMAKE" --build build-dsp --parallel 3
"$CTEST" --test-dir build-dsp --output-on-failure
"$CMAKE" -S . -B build-arm64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DFETCHCONTENT_SOURCE_DIR_JUCE=/caminho/local/JUCE-8.0.10
"$CMAKE" --build build-arm64 --parallel 3
```
