// calibracao.h -- fluxo interativo (console) que calibra o Miracle:
// 1) o badge de posicao, so' pra saber distinguir FLAT de "tem posicao"
//    (nao precisa de 1..N como no roboclone -- ver config.h);
// 2) a regiao do campo "Resultado em Aberto" -- so' aponta onde ele
//    esta' na tela, a leitura em si e' via OCR (ver leitura_valor.h),
//    sem precisar treinar caractere nenhum.
#pragma once

#include "config.h"

// devolve false se o operador cancelar (ESC) em algum passo.
bool rodarCalibracao(Calibracao& out);
