# Compilação na nuvem — Trem de Medir

O repositório já está conectado: https://github.com/Sulyvan93/plugins

Não é necessário copiar código, carregar ZIP nem criar outro repositório.

1. Acesse [Actions](https://github.com/Sulyvan93/plugins/actions).
2. Selecione **Trem de Medir - macOS Apple Silicon**.
3. Abra a execução mais recente. Um indicador verde significa que a compilação e a verificação da arquitetura foram bem-sucedidas. Um indicador vermelho exige correção dos logs.
4. Na execução verde, baixe o artefato **TremDeMedir-macOS-arm64**, contendo os ZIPs AU, VST3 e, se produzido, aplicativo independente.
5. Descompacte os ZIPs e copie os pacotes para:
   - AU: `~/Library/Audio/Plug-Ins/Components/`
   - VST3: `~/Library/Audio/Plug-Ins/VST3/`
6. Reinicie o Logic Pro e o Harrison Mixbus e procure **Trem de Medir**.

O workflow `testes-dsp.yml` valida o motor de medição separadamente no Linux.
Os plugins ainda necessitam testes de carregamento, áudio e medição dentro das DAWs.
O workflow cria binários ARM64 sem assinatura/notarização Apple.
