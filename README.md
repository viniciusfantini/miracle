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

Não é OCR de propósito geral: o valor (ex. `-15,50`, `2,00`) é segmentado
em fatias verticais (colunas com "tinta" separadas por fundo) e cada
fatia é comparada, bitmap exato, contra os glifos calibrados (`0`-`9`,
`-`, `,`) do MESMO tamanho. Se uma fatia não bater com nenhum glifo
dentro da tolerância, a leitura inteira é descartada (nunca adivinha) —
mesma filosofia do badge de posição no `roboclone`.

## Uso

```
build.bat            # compila Miracle.exe (precisa do MSVC Build Tools)
Miracle.exe calibrar # passo a passo: badge FLAT + glifos do Resultado em Aberto
Miracle.exe rodar    # roda o ciclo do primeiro teste (ver acima)
```

Use sempre a conta **SIMULADORA** como destino — nunca aponte para uma
conta real.

## Pendências

- Leitura do gatilho de compra/venda (posição da janela de origem real) —
  fora de escopo deste primeiro teste.
- Sem git configurado ainda nesta pasta.
- Não validado ao vivo contra o Profit ainda (só compilado).
