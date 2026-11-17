# Verificação e correções 0.2.1 — 16/11/2026

A reprodução da versão 0.2 foi confirmada no Logic Pro 13. A referência BPM Counter nativa exibiu 80 BPM na faixa em teste, mas a estimativa do Trem oscilou ou ficou sem leitura. Isso confirmou uma limitação real da análise musical, não uma aprovação de precisão.

## Corrigido

- O detector deixou de favorecer o primeiro intervalo da busca por causa de um limiar incremental. Agora busca o maior pico de correlação e trata múltiplos exatos apenas em máximos locais.
- A apresentação de BPM exige consenso entre janelas sucessivas e pontuação mínima de correlação de 0,65. Estimativas fracas não são mostradas como valores confiáveis.
- Auto usa a oitava de 60–120 BPM; controles explícitos `/2` e `x2` permitem resolver ambiguidades de pulso/subdivisão. O andamento do host permanece separado e nunca é usado para fabricar a detecção por áudio.
- A interface identifica BPM como **experimental** e orienta usar TAP quando há áudio sem estimativa confiável.
- Confiança e frequência selecionada no espectro passam a ter arredondamento apropriado. O contador de clipping identifica amostras em clip, em vez de sugerir eventos separados.
- Versão atualizada para 0.2.1; novo parâmetro de oitava BPM anexado, totalizando 26, com compatibilidade de estados anteriores testada.

## Validação

Compilação e instalação dos formatos AU, VST3 e Standalone arm64; assinatura ad hoc e versão dos bundles verificadas. Testes DSP, ferramentas de análise e integração do processador repetidos, incluindo áudio estéreo preservado amostra por amostra em 44,1/48/96 kHz, estados antigos, consenso BPM e rejeição de estimativas fracas.

## Limitações que permanecem

O estimador ainda não reproduz de forma confiável os 80 BPM da referência na mixagem pronta. O novo comportamento é recusar resultados incertos; isso não equivale a ter corrigido a precisão para essa música. Tuner e BPM são implementações independentes e não são equivalentes aos algoritmos proprietários da Apple.

Não foi concluída a comparação sincronizada de loudness/true peak com os medidores nativos. As tentativas de operar pausa e menus do AU pela automação não tiveram alteração confirmada; a causa entre automação e interação do editor não foi isolada. Esses controles, CSV, Mixbus/VST3 e auval continuam sem aprovação completa no host. Os testes sintéticos e do processador não substituem essa aprovação.

A sessão de teste foi preservada antes de fechar o Logic para substituir os bundles. É necessário reabrir o Logic para carregar o AU 0.2.1. Não foi declarada validação do novo binário dentro do host após a instalação.
