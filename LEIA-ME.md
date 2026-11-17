> **Versão 0.2.0:** interface Glass, afinador, BPM e novos controles de análise. [Novidades, testes e limitações](NOVIDADES_0_2.md).

# Trem de Medir
Plugin independente de medição de áudio para **Logic Pro (AU)**, **Mixbus (VST3)** e macOS Apple Silicon.

## Versão em desenvolvimento
O projeto usa C++17, JUCE 8.0.10 e um motor DSP que pode ser testado sem JUCE.
A compilação macOS acontece em `.github/workflows/compilar-mac.yml`.
Os testes DSP em Linux acontecem em `.github/workflows/testes-dsp.yml`.

### Implementado no código, sujeito a testes de integração
- Leitura de LUFS momentâneo (400 ms), curto (3 s), integrado com gate e estimativa de LRA.
- Peak, RMS, True Peak por interpolação 4×/8×, PLR e PSR.
- Correlação e balanço estéreo, vectorscópio, VU/PPM e RTA de 31 bandas.
- Histórico e histograma, parâmetros persistentes, referências configuráveis, registro CSV.
- Bypass transparente: o plugin mede e não altera as amostras de áudio.
- Formatos AU/VST3/Standalone e layouts de entrada até 12 canais, que ainda requerem validação com hosts reais.

**Não certificado:** ITU-R BS.1770, EBU R128, true peak e multicanal exigem testes de conformidade com sinais oficiais e validação dos canais no host. Não há algoritmo proprietário Dolby nem controle SSL 360/UF1.

## Compilar no Mac M4 ou baixar no GitHub
Os workflows **Trem de Medir - macOS ARM64 (15)** e **(26)** geram os pacotes
`TremDeMedir.component`, `TremDeMedir.vst3` e, quando possível, aplicativo.
O pacote **TremDeMedir-macOS-arm64** só estará disponível em execuções verdes
com a etapa de publicação de artefatos aprovada.

Se o GitHub não disponibilizar runners macOS, use a alternativa local:
**Code > Download ZIP**, descompacte e execute com duplo clique
`scripts/compilar_no_mac.command` no seu Mac mini M4.
O script exige ferramentas de desenvolvimento Apple e CMake, executa testes,
compila, assina localmente (ad hoc), cria ZIPs, faz backup de versões anteriores
e instala os plugins na conta atual. Detalhes em [COMO_COMPILAR_NO_GITHUB.md](COMO_COMPILAR_NO_GITHUB.md).

**Locais de instalação:**
- AU: `~/Library/Audio/Plug-Ins/Components/TremDeMedir.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/TremDeMedir.vst3`
- App: `~/Applications/TremDeMedir.app`

Os binários ainda exigem validação no Logic Pro e no Mixbus; a assinatura local
ad hoc não é notarização Apple.

## Testes locais (opcional)
```sh
cmake -S . -B build-dsp -DTREM_DSP_TEST_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

## Licenças
Código do projeto: MIT (`LICENCA.txt`). JUCE possui licença própria:
a utilização comercial exige revisão de seus termos atuais.
SSL Meter Pro é apenas referência documental de funcionalidades; o código e
a interface deste projeto são próprios.
