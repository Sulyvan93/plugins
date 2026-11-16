# Trem de Medir — Compilar na nuvem (sem instalar ferramentas de programação)

Este projeto já inclui o arquivo `.github/workflows/compilar-mac.yml` para o GitHub Actions tentar compilar as versões AU (Logic Pro), VST3 (Mixbus) e um aplicativo autônomo para Mac mini M4.

## Passo a passo

1. Crie uma conta em https://github.com se ainda não tiver.
2. Entre em https://github.com/new e crie um repositório chamado `TremDeMedir` (escolha **Private**, de preferência). Clique em **Create repository**.
3. Baixe o ZIP `Trem_de_Medir_GitHub_Actions.zip` e descompacte. Abra a pasta **TremDeMedir**.
4. Na página do repositório novo, escolha **uploading an existing file** ou **Add file > Upload files**. Envie **os arquivos e pastas que estão dentro de TremDeMedir**, não o ZIP original. É essencial que as pastas `Source`, `tests` e `.github` sejam enviadas preservando seus caminhos. Em macOS, `Command + Shift + .` exibe a pasta oculta `.github` no Finder.
5. Clique em **Commit changes**. Se a interface deixar algum arquivo para trás, confira que você tem `CMakeLists.txt`, `Source/PluginProcessor.cpp` e `.github/workflows/compilar-mac.yml` na página principal do repositório.
6. Clique na guia **Actions**. A compilação deverá iniciar automaticamente após o upload (na branch main). Se não iniciar, abra o fluxo **Compilar Trem de Medir para Mac M4**, clique em **Run workflow** e confirme.
7. Abra a execução mais recente. **Verde** = etapas concluídas; **vermelho** = houve um erro de compilação a corrigir. Se der vermelho, envie o texto do erro para correção.
8. Se deu verde, no fim da página da execução procure **Artifacts** > **Trem-de-Medir-macOS-M4**. Baixe e descompacte. Você verá ZIPs separados com os plugins AU, VST3 e possivelmente o aplicativo.

## Instalar no Mac mini M4

Descompacte `Trem-de-Medir-AU-macOS-M4.zip` e coloque o arquivo `.component` em:
`~/Library/Audio/Plug-Ins/Components/`

Descompacte `Trem-de-Medir-VST3-macOS-M4.zip` e coloque o arquivo `.vst3` em:
`~/Library/Audio/Plug-Ins/VST3/`

No Finder, pressione `Command + Shift + G` para ir até uma das pastas acima. Feche e abra novamente Logic Pro e Mixbus para que detectem o plugin.

**Atenção:** Trata-se de compilação experimental não assinada e ainda não validada num Mac real. O macOS pode mostrar avisos de segurança. Também podem surgir erros de compilação na primeira execução, pois o código da interface ainda não foi testado com JUCE/macOS. Não contorne avisos de segurança se não confiar na origem dos arquivos.
