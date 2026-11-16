# TREM DE MEDIR — plugin de medição de áudio

**Versão:** 0.1.0 (protótipo técnico independente)

Projeto de plugin para **Logic Pro (Audio Unit / AU)**, **Harrison Mixbus (VST3)** e outros hosts compatíveis. Inclui opção de aplicativo autônomo (`Standalone`) fornecida pelo framework JUCE.

Não se trata de uma versão, cópia de código ou produto oficial da Solid State Logic. A aparência, marca e implementação são independentes. O PDF do SSL Meter Pro fornecido como referência foi usado para levantar os requisitos, não como fonte de código.

## O que está implementado neste código

- Áudio de entrada/saída preservado: a medição **não altera, normaliza nem limita o sinal**.
- Estrutura de plugin **AU, VST3 e Standalone** via JUCE; a compilação das versões para macOS depende de Xcode e CMake.
- Processamento de 1 a 12 canais (mono até o número de canais de 7.1.4). O mapeamento correto deve ser selecionado/configurado conforme o host.
- **Loudness K-weighted**: momentâneo (400 ms), curto (3 s), integrado com gate absoluto e gate relativo de -8/-10 LU, máximos e estimativa estatística de LRA.
- **True Peak por interpolação**: 4x padrão ou 8x HQ, com teto de alerta ajustável.
- Métricas derivadas **PLR, PSR**.
- Gráfico de histórico de 72 s, faixa-resumo tipo *Universe* e histograma da distribuição de valores de curto prazo.
- Medidores de **Peak, RMS, máximos e escala digital** linear, não linear e referências K-20, K-14, K-12.
- Estéreo: **correlação de fase, balanço e vectorscópio Lissajous**.
- Dois medidores analógicos inspirados no comportamento **VU (300 ms) e PPM** com referência dBFS ajustável.
- **RTA de 31 bandas** em terços de oitava, ponderação **A / C / sem ponderação**, seleção L, R ou L+R e retenção de pico.
- Presets editáveis de trabalho com níveis de referência para Spotify, EBU, Apple, YouTube, ATSC, Netflix e estúdio. **Não são garantia de atendimento às políticas ou normas atuais das plataformas.**
- **CSV** automático com data, LUFS, True Peak, dinâmica, fase e balanço. Gravação feita fora da thread de áudio.
- **Reset, A/B de configurações, copiar A/B, desfazer/refazer, interface ajustável** e parâmetros salvos com a sessão.

## O que AINDA NÃO equivale ao SSL Meter Pro

1. **Não está reproduzida a integração SSL 360° / UF1** (ecossistema proprietário).
2. **Não há Dolby Dialogue Intelligence** (tecnologia proprietária); sem dados oficiais/licença não seria correto afirmar que uma detecção substituta é idêntica.
3. **Não há modo Leq(m)**, presets normativos completos, detectores de diálogo nem conformidade certificada ITU-R BS.1770-5 / EBU R128. O algoritmo K-weighted e o true-peak são implementações próprias que **precisam de medição de referência** antes de um uso de conformidade.
4. Não foram implementados DAW Sync ligado à posição de timeline, registro por relogio sincronizado à timeline, histórico até 24 h, seleção livre dos nove campos de leitura, alertas independentes para todas as métricas, ajuste de todas as janelas de integração, rotas de todas as somas multicanal, nem hold de agulha ajustável.
5. **O código da interface e o empacotamento AU/VST3 não puderam ser compilados ou validados neste ambiente**, que usa Linux sem o SDK JUCE. O motor DSP foi compilado e testado. A build nativa e os testes dentro de Logic/Mixbus são etapas pendentes.

## Instalação no Mac mini M4

**Uma vez**, instale as ferramentas necessárias:

1. Instale as ferramentas de desenvolvedor da Apple: abra o Terminal e execute `xcode-select --install`.
2. Instale o **CMake** (pelo instalador oficial ou pelo Homebrew: `brew install cmake`).
3. Tenha conexão com a internet **na primeira compilação**, pois o CMake baixará o framework **JUCE 8.0.10** do projeto oficial.

**Para compilar:** descompacte o ZIP no seu Mac e execute com duplo clique `scripts/compilar_no_mac.command`. Se a proteção do macOS impedir a abertura, execute no Terminal:

```bash
cd /caminho/para/TremDeMedir
zsh scripts/compilar_no_mac.command
```

Se a compilação concluir com sucesso, o JUCE deverá copiar automaticamente os artefatos para as pastas comuns:

- AU: `~/Library/Audio/Plug-Ins/Components/`
- VST3: `~/Library/Audio/Plug-Ins/VST3/`

Depois, reinicie o Logic Pro ou o Mixbus e procure por **Trem de Medir** no gerenciador de plugins. Se ocorrer erro de build, registre a saída completa do Terminal para identificar e corrigir a incompatibilidade específica.

> **Importante:** este arquivo ZIP contém **o projeto e o código-fonte**, não um `.component`/`.vst3` pré-compilado para macOS. A possibilidade de compilação está prevista, mas não foi validada em Mac nesta sessão.

## Testar o motor de análise

Em um computador com CMake e um compilador C++17:

```bash
bash scripts/testar_dsp.sh
```

A suíte atual contém verificações de: sinal de 1 kHz, True Peak, LUFS integrado estéreo, correlação de fase positiva/negativa, silêncio com gate, comportamento da medição em 12 canais e estabilidade a 44,1 kHz.

## Arquivos

```text
TremDeMedir/
  CMakeLists.txt                 Projeto CMake/JUCE
  Source/
    MeterDSP.h/.cpp             Motor independente de medição
    PluginProcessor.h/.cpp      Bridge com a DAW, parâmetros e CSV
    PluginEditor.h/.cpp         Interface original em português
  tests/test_meter.cpp          Testes automatizados do DSP
  scripts/compilar_no_mac.command
  scripts/testar_dsp.sh
  LEIA-ME.md
  LICENCA.txt
```

## Licenças

O código deste projeto é distribuído sob MIT (consulte `LICENCA.txt`). O **JUCE** é um framework de terceiros com sua própria licença (AGPLv3 / comercial, conforme o tipo de utilização e a versão). Antes de distribuir um plugin comercial feito com JUCE, verifique e cumpra os termos da licença JUCE apropriada. Os nomes de plataformas de terceiros são usados apenas como referência técnica.
