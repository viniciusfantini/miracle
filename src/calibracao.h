// calibracao.h -- fluxo interativo (console) que calibra o Miracle:
// 1) o badge de posicao, so' pra saber distinguir FLAT de "tem posicao"
//    (nao precisa de 1..N como no roboclone -- ver config.h);
// 2) os glifos (0-9, "-", ",") do campo "Resultado em Aberto", pra poder
//    ler o valor monetario continuo digito a digito.
#pragma once

#include "config.h"

// devolve false se o operador cancelar (ESC) em algum passo.
bool rodarCalibracao(Calibracao& out);
