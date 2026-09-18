# Miracle.exe

Segundo robô, independente do `roboclone` (pasta irmã `C:\B3_AUT\roboclone`).
Reaproveita as mesmas técnicas de captura de tela e envio de atalho
(`ALT+C`/`ALT+V`/`ALT+A` via `PostMessage`, sem depender de foco), mas o
foco aqui é a **condução do trade** (gestão por Resultado em Aberto), não
a leitura do gatilho de compra/venda — isso fica para uma fase seguinte.

## Primeiro teste (escopo atual)

Fluxo fixo, sem leitura de gatilho:

1. Escolhe a janela **leitora** (lê o badge de posição — só pra confirmar
   FLAT — e o campo "Resultado em Aberto").
2. Escolhe a janela **simulador** (pra onde manda os atalhos).
3. Confirma que a posição está FLAT.
4. Manda **COMPRA** (entrada, 1 contrato).
5. Fica lendo o Resultado em Aberto:
   - **≥ +R$10,00** → manda **VENDA** (saída) e encerra o ciclo.
   - **≤ -R$20,00** → manda **COMPRA** (reforço, vira 2 contratos).
     Depois do reforço, na posição de 2 contratos:
     - **≥ +R$20,00** → manda **ZERAR** (`ALT+A`) e encerra o ciclo.
     - **≤ -R$60,00** → manda **ZERAR** (`ALT+A`), stop, e encerra o ciclo.
6. Roda **exatamente 1 ciclo e para** — sem reentrada automática. Decisão
   de segurança para evitar repetir os loops descontrolados já vividos no
   `roboclone` (ver `CLAUDE.md` de lá).

## Como o Resultado em Aberto é lido

Via **OCR nativo do Windows** (`Windows.Media.Ocr`, C++/WinRT). O badge
de posição (`Qtd`) continua por comparação de bitmap exato (poucas
aparências fixas, técnica que já funciona bem ali — ver `roboclone`),
mas o Resultado em Aberto é um valor CONTÍNUO com fonte/cor (lucro
verde, prejuízo vermelho)/tamanho variando — depois de várias rodadas
tentando fazer isso com segmentação por coluna + bitmap por caractere
(problemas recorrentes: limiar de tinta sensível a kerning de cada par
de dígitos, "R$" aparecendo/sumindo, provável quebra em valores
negativos), trocado (18/09/2026) pro OCR do sistema — resolve isso tudo
de fábrica, sem precisar treinar caractere nenhum.

A captura da região é ampliada 6x antes de mandar pro OCR (texto de UI
pequeno fica bem mais confiável ampliado), e o texto reconhecido é
filtrado pra só os caracteres que importam (dígitos, `-`, `,`, `.`)
antes de interpretar sinal/vírgula dos centavos/separador de milhar.
Se o OCR não reconhecer nada interpretável (ou reconhecer algo
ambíguo, tipo 0 ou 2+ vírgulas), a leitura inteira é descartada — nunca
adivinha, mesma filosofia do badge de posição no `roboclone`.

## Uso

```
build.bat            # compila Miracle.exe (precisa do MSVC Build Tools)
Miracle.exe calibrar # passo a passo: badge FLAT + regiao do Resultado em Aberto
Miracle.exe debug    # roda o ciclo lendo tudo, mas SO' AVISA o que mandaria -- nao envia nada
Miracle.exe rodar    # roda o ciclo de verdade (ver acima) -- manda ordem na janela simulador
```

Na calibração do Resultado em Aberto, agora é só clicar os 2 cantos —
sem treinar caractere nenhum. O programa salva uma foto
(`miracle_calibracao_valor.bmp`) e já mostra o que o OCR leu ali, pra
você confirmar visualmente que a região está bem enquadrada antes de
seguir.

Requer o pacote de OCR do Windows instalado (normalmente já vem, mas se
`Miracle.exe` reclamar na inicialização: Configurações > Hora e idioma
> Idioma e região > opções do idioma > adicionar "Reconhecimento óptico
de caracteres").

Valide sempre em `debug` primeiro: confirme que "flat" é reconhecido
corretamente e que o Resultado em Aberto lido no console bate com o que
está na tela, antes de confiar no `rodar` (que manda ordem de verdade na
conta simuladora).

Use sempre a conta **SIMULADORA** como destino — nunca aponte para uma
conta real.

## Pendências

- Leitura do gatilho de compra/venda (posição da janela de origem real) —
  fora de escopo deste primeiro teste.
- Sem git configurado ainda nesta pasta.
- Não validado ao vivo contra o Profit ainda (só compilado).
