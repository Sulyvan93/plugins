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

## Baixar quando o workflow estiver verde
Acesse [GitHub Actions](https://github.com/Sulyvan93/plugins/actions), abra o workflow
**Trem de Medir - macOS Apple Silicon** e, ao final de uma execução concluída com sucesso,
baixe o artefato **TremDeMedir-macOS-arm64**. Ele contém ZIPs com
`TremDeMedir.component`, `TremDeMedir.vst3` e o aplicativo, caso este seja gerado.

## Instalar
- AU: descompacte e coloque `TremDeMedir.component` em `~/Library/Audio/Plug-Ins/Components/`
- VST3: descompacte e coloque `TremDeMedir.vst3` em `~/Library/Audio/Plug-Ins/VST3/`
- Feche e reabra Logic Pro e Mixbus.

Os binários gerados no CI não são assinados/notarizados pela Apple. Ajustes de
assinatura e validação nos aplicativos ainda poderão ser necessários.

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
