// janela_alvo.h -- escolhe uma janela (origem ou destino) pedindo pro
// operador clicar nela uma vez, em vez de confiar em titulo (duas janelas
// do Profit podem ter titulo IDENTICO -- mesmo ativo/timeframe -- achado
// documentado em 4q-winfut/b3_money_copy/README.md). HWND capturado so'
// vale pra sessao atual: refazer o clique toda vez que o Profit reiniciar.
#pragma once

#include <windows.h>
#include <string>

// bloqueia o console ate' o operador clicar com o botao esquerdo em algum
// lugar da janela desejada (ou apertar ESC pra cancelar). Devolve o HWND
// da janela MDI (ou de topo, se o app nao for MDI) -- nao o controle
// filho especifico clicado. 'rotulo' aparece na instrucao impressa no
// console.
HWND escolherJanelaPorClique(const std::string& rotulo);

// resolve qual janela MDI (ou de topo) esta' AGORA num ponto de tela --
// mesma logica do clique, mas sem esperar clique nenhum. Usado pra
// confirmar, durante o "rodar"/"debug", que a regiao calibrada da origem
// ainda pertence a' janela que foi apontada como origem (evita ler a
// janela errada se ela mudar de lugar/for sobreposta).
HWND janelaNoPonto(POINT p);
