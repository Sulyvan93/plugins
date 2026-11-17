# Trem de Medir 0.2.0 — interface Glass e ferramentas de análise

Atualização local de 16/11/2026. Referências observadas na sessão do Logic Pro 13: MultiMeter, Loudness Meter, Level Meter, Correlation Meter, BPM Counter e Tuner. Implementação independente; não usa código, imagens, componentes privados ou algoritmos proprietários da Apple.

## Funções implementadas

| Referência funcional | Trem de Medir 0.2.0 |
| --- | --- |
| Spectrum Analyzer | 31, 63 ou 127 bandas; Peak, RMS Fast e RMS Slow; Mono L+R, L, R ou LR Max; ponderação A/C; faixa 30/60/90/120 dB; hold 0/1/2/5 s ou infinito; retorno 6/12/24 dB/s; reset dos marcadores |
| Level Meter | Peak, RMS ou ambos; retenção e retorno dos picos; contagem de amostras com amplitude >= 0 dBFS, somada entre canais |
| Loudness Meter | M/S/I, LRA, true peak 4x/8x, máximos, PLR/PSR, histórico e histograma; LUFS ou LU relativo ao alvo; pausa do loudness sem interromper o áudio nem congelar Peak/RMS/VU |
| Correlation / Goniometer | Vectorscópio, barra de correlação de -1 a +1, leitura numérica de fase e balanço |
| Tuner | Detecção monofônica de frequência, nota/oitava, desvio em cents, indicador de afinação e referência A4 entre 400 e 480 Hz |
| BPM | Estimativa por fluxo de energia e autocorrelação, confiança do resultado, tempo informado pelo host em campo separado e Tap Tempo |

Preservados: presets, medição multicanal, VU/PPM, gate, histórico vinculado ao transporte, CSV, undo/redo, A/B, IDs e intervalos dos parâmetros anteriores. Os onze parâmetros novos são anexados; os índices dos três modos antigos de fonte RTA permanecem iguais.

A interface usa superfícies escuras, gradientes, bordas luminosas, sombras e controles arredondados inspirados na linguagem visual Liquid Glass. É desenhada pelo JUCE e não emprega o material nativo de blur do macOS. A identidade de Minas aparece no triângulo vermelho sem inscrições sobre um campo branco. Janela inicial 1280 x 1100, mínimo 1120 x 1040.

## Compilação e testes confirmados

- macOS Apple Silicon, AppleClang 21, CMake 3.31.8 e JUCE 8.0.10.
- AU, VST3 e Standalone Release compilados com código de saída 0; versão dos três bundles conferida como 0.2.0.
- Arquitetura arm64 e assinatura ad hoc verificadas, também nas cópias instaladas.
- Testes originais DSP aprovados.
- Afinador: 55/110/220/440/880/1000 Hz em 44,1/48/96 kHz, com harmônico adicional e erro inferior a 2 cents nos casos testados; silêncio e ruído rejeitados.
- BPM: pulsos sintéticos de 72/90/120/150/180 BPM, erro inferior a 2 BPM nos casos testados; silêncio e trecho insuficiente sem estimativa. Um erro inicial de sub-harmônico em 180 BPM foi identificado, corrigido e o teste repetido.
- Hold, decaimento e pausa de loudness testados; durante a pausa, tempo e integrado ficam constantes e RMS continua respondendo ao silêncio.
- Integração do processador: áudio estéreo preservado amostra por amostra em 44,1/48/96 kHz, 25 parâmetros, estado salvo/recuperado e defaults dos parâmetros novos ao carregar estado antigo.
- Standalone aberto; interface revisada; seleção de 127 bandas e histograma confirmada pela interface.
- Análise de pitch e BPM ocorre fora da thread de áudio; captura de energia usa armazenamento pré-alocado e atômico. Não há alocação nova no caminho de captura de BPM.

## Instalação e recuperação

- AU: `~/Library/Audio/Plug-Ins/Components/TremDeMedir.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/TremDeMedir.vst3`
- Standalone: `~/Applications/TremDeMedir.app`

A instalação anterior foi preservada no pacote `Backup-TremDeMedir-0.1.zip`, junto aos resultados locais. Os ZIPs da nova versão levam `0.2.0` no nome. A assinatura é local, sem notarização Apple.

A sessão aberta foi salva como cópia `Teste Trem 0.2.logicx` nos resultados locais antes de fechar o Logic. Não houve alteração nos faders nem nos arquivos de áudio originais.

## Limitações e validação pendente

A versão 0.1 teve carregamento e resposta a áudio confirmados no Logic em teste anterior. Isso não equivale à aprovação da versão 0.2: ao reabrir o Logic após a instalação, a automação da interface passou a retornar timeout. A tentativa alternativa de abrir a sessão pelo Finder não permitiu confirmar o novo AU. Não apareceu novo relatório de crash do Logic durante a verificação, mas isso também não comprova estabilidade. O teste de integração cobre o processador, não o wrapper AU dentro do host.

Ainda validar a versão 0.2 com reprodução no Logic/Mixbus, `auval`, comparação sincronizada com medidores de referência, automação e CSV. A precisão do RTA está limitada à resolução da FFT de 4096 amostras; aumentar o número de bandas não aumenta essa resolução. O afinador é destinado a fontes monofônicas; uma mixagem pronta pode não ter fundamental única. A estimativa BPM requer ao menos 6 segundos e pode sofrer ambiguidade de metade/dobro em material musical. Os testes sintéticos não comprovam equivalência aos algoritmos da Apple nem certificação ITU/EBU.

## Reproduzir os testes

```sh
cmake -S . -B build-dsp -DTREM_DSP_TEST_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp --parallel 3
ctest --test-dir build-dsp --output-on-failure
cmake -S . -B build-arm64 -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_BUILD_TYPE=Release -DTREM_PROCESSOR_TESTS=ON
cmake --build build-arm64 --parallel 3
ctest --test-dir build-arm64 --output-on-failure
```

Se o JUCE já estiver extraído, usar `-DFETCHCONTENT_SOURCE_DIR_JUCE=/caminho/JUCE-8.0.10`. Para evitar arquivos `dataless`, compilar em uma pasta local fora da sincronização do iCloud.
