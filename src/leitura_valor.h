// leitura_valor.h -- le' o campo "Resultado em Aberto" (valor monetario
// CONTINUO, ex. "2,00"/"-15,50"/"R$1.234,56") via OCR nativo do Windows
// (Windows.Media.Ocr, WinRT) em vez de comparacao de bitmap por
// caractere. Motivo da troca (18/09/2026): o campo muda de fonte/cor
// (lucro=verde, prejuizo=vermelho) e de tamanho (mais digitos, "R$"
// aparecendo/sumindo, separador de milhar) -- exatamente o cenario que
// OCR resolve de fabrica, ao contrario do badge de posicao do roboclone
// (poucas aparencias fixas, onde bitmap exato ainda faz sentido).
#pragma once

#include "captura_tela.h"
#include <optional>
#include <string>

// precisa ser chamada 1 vez, no inicio do programa (main), antes de
// qualquer chamada a lerResultadoEmCentavos -- inicializa a apartment
// WinRT e o motor de OCR. Devolve false se nao conseguir criar o motor
// de OCR (ex.: pacote de idioma de OCR nao instalado no Windows).
bool iniciarOcr();

// tenta ler o valor monetario atual (em CENTAVOS, com sinal) na regiao
// capturada, via OCR. Devolve nullopt se o OCR nao reconhecer nada
// interpretavel como um valor monetario valido (nunca adivinha).
// 'textoBrutoOut', se nao for nullptr, recebe o texto cru que o OCR
// reconheceu (antes de filtrar/interpretar) -- so' pra diagnostico.
std::optional<long long> lerResultadoEmCentavos(const CapturaRegiao& cap, std::string* textoBrutoOut = nullptr);

// formata centavos de volta pra texto tipo "R$ 12,34" / "R$ -5,00", so'
// pra exibir no console.
std::string formatarCentavos(long long centavos);
