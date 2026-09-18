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

Não é OCR de propósito geral: o valor (ex. `-15,50`, `2,00`, `R$1.234,56`)
é segmentado em fatias verticais (colunas com "tinta" separadas por
fundo) e cada fatia é comparada, bitmap exato, contra os glifos
calibrados (`0`-`9`, `-`, `,`, `.`, `R`, `$`) do MESMO tamanho. Se uma
fatia não bater com nenhum glifo dentro da tolerância, **ou** se alguma
fatia encostar na borda da região capturada (risco de caractere cortado,
o mais perigoso sendo o sinal `-` sumir e inverter o sinal do valor), a
leitura inteira é descartada (nunca adivinha) — mesma filosofia do badge
de posição no `roboclone`.

O campo é alinhado à direita: a mesma região precisa ser larga o
bastante pro maior valor esperado, o que faz o label `"R$"` aparecer
sozinho quando o valor é pequeno (sobra espaço à esquerda) — o `"R"` e
o `"$"` são reconhecidos e simplesmente ignorados na conversão pra
centavos, em qualquer posição no texto reconhecido.

## Uso

```
build.bat            # compila Miracle.exe (precisa do MSVC Build Tools)
Miracle.exe calibrar # passo a passo: badge FLAT + glifos do Resultado em Aberto
Miracle.exe debug    # roda o ciclo lendo tudo, mas SO' AVISA o que mandaria -- nao envia nada
Miracle.exe rodar    # roda o ciclo de verdade (ver acima) -- manda ordem na janela simulador
```

Na calibração do Resultado em Aberto, compre bastante contrato ANTES de
clicar os cantos da região, pra desenhá-la já no tamanho máximo real
(ex. `-R$10.394,00`) em vez de justa demais em cima de um valor pequeno.
Cada rodada de calibração salva uma foto congelada
(`miracle_calibracao_valor.bmp`) pra você digitar o valor sem correr
atrás do preço mudando ao vivo na tela.

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
