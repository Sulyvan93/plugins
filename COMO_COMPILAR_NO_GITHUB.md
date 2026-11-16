# Trem de Medir — como compilar no Mac mini M4

## Diagnóstico do GitHub Actions (16/11/2026)
Os jobs macOS 15 e 26 da revisão 968c5fa6 foram concluídos sem obter runner hospedado (`runner_id=0`, `steps=[]`). Portanto, **nenhum compilador Apple executou o projeto** nessas tentativas. Alterar o DSP ou ignorar uma etapa não resolve falta de capacidade de runner. O workflow JUCE no Linux e os testes DSP passaram.

Há duas alternativas. A primeira usa o GitHub Actions quando um runner macOS estiver disponível; a segunda usa o próprio Mac mini M4, sem precisar editar código.

## Opção A — GitHub Actions (nuvem)
1. Abra [Actions](https://github.com/Sulyvan93/plugins/actions).
2. Selecione `Trem de Medir - macOS ARM64 (15)` ou `Trem de Medir - macOS ARM64 (26)`.
3. Uma execução verde **com o artefato publicado** terá o pacote `TremDeMedir-macOS-arm64`.
4. Baixe os ZIPs AU, VST3 e aplicativo. **Sem artefato, não há plugin para instalar.**

Os workflows cancelam execuções antigas do próprio ambiente ao receberem uma revisão nova. A versão macOS 14 foi retirada do CI por não agregar um caminho útil diante dos repetidos cancelamentos.

## Opção B — compilar e instalar direto no Mac M4 (sem editar código)
1. Acesse o [repositório](https://github.com/Sulyvan93/plugins), clique em **Code > Download ZIP**.
2. Descompacte o projeto e abra `scripts/compilar_no_mac.command` com duplo clique. Se macOS bloquear o script por ser baixado, confira a origem e use **Abrir** pelo menu de contexto, respeitando os avisos de segurança.
3. O script testa o DSP, compila AU/VST3/Standalone, valida a arquitetura ARM64, assina localmente (ad hoc), gera os ZIPs e instala os pacotes apenas para o seu usuário.
4. São necessárias as **Apple Command Line Tools** e o **CMake**. Se faltar alguma delas, o script indica a instalação (Homebrew pode instalar CMake se já estiver disponível). Execute-o novamente depois de instalar as ferramentas.
5. Os ZIPs ficam em `~/Documents/Trem de Medir/Instaladores/`, os logs em `~/Documents/Trem de Medir/Logs/` e backups das versões antigas na pasta de instaladores.
6. Reabra Logic Pro e Harrison Mixbus e valide o carregamento e as medições.

**Atenção:** os binários locais terão assinatura ad hoc, não notarização Apple. Não foram validados nas DAWs e não devem ser usados para controle normativo até haver testes de conformidade.

## Pastas de instalação
- AU: `~/Library/Audio/Plug-Ins/Components/TremDeMedir.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/TremDeMedir.vst3`
- App independente: `~/Applications/TremDeMedir.app`

**Nenhum ZIP previamente criado ou execução antiga substitui a validação da compilação macOS.**
