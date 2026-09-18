// leitura_valor.h -- le' o campo "Resultado em Aberto" (valor monetario
// CONTINUO, ex. "2,00"/"-15,50") digito a digito, reaproveitando a mesma
// ideia de comparacao de bitmap exata do roboclone -- so' que em pedacos
// pequenos (um por caractere) em vez de um conjunto fixo de aparencias
// inteiras.
//
// Processo, a cada leitura:
//   1. Segmenta a regiao capturada em fatias verticais (colunas com
//      "tinta" separadas por colunas de fundo) -- cada fatia e' um
//      candidato a caractere.
//   2. Cada fatia e' comparada (bitmap exato) contra os glifos
//      calibrados QUE TEM A MESMA LARGURA (larguras diferentes nunca
//      batem, isso ja' distingue "1" de "0" sem nem comparar pixel).
//   3. Se ALGUMA fatia nao bater com nenhum glifo dentro da tolerancia,
//      a leitura inteira falha (nunca "adivinha" -- mesma filosofia do
//      badge de posicao no roboclone).
//   4. A sequencia de caracteres reconhecidos e' convertida pra um
//      inteiro de CENTAVOS (com sinal), evitando ponto flutuante.
#pragma once

#include "captura_tela.h"
#include "config.h"
#include <optional>
#include <string>
#include <vector>

// resultado de segmentar uma captura em possiveis caracteres.
struct Segmento {
    int offsetX = 0; // relativo ao inicio da regiao capturada
    int largura = 0;
    std::vector<BYTE> bitmap; // largura x altura da regiao, BGRA
};

// varre as colunas da captura procurando "tinta" (qualquer pixel que
// destoe do fundo, amostrado no canto superior esquerdo da regiao) e
// agrupa colunas vizinhas com tinta em segmentos -- cada um um candidato
// a caractere. Devolve vazio se a regiao parecer toda fundo (campo em
// branco/zero sem digitos, ou falha de captura).
std::vector<Segmento> segmentarCaracteres(const CapturaRegiao& cap);

// tenta ler o valor monetario atual (em CENTAVOS, com sinal) comparando
// os segmentos contra os glifos calibrados. Devolve nullopt se qualquer
// segmento nao bater com nenhum glifo dentro da tolerancia, ou se a
// sequencia reconhecida nao formar um numero valido (ex.: sem virgula,
// virgula com menos/mais de 2 casas depois).
std::optional<long long> lerResultadoEmCentavos(const CapturaRegiao& cap, const Calibracao& cal);

// formata centavos de volta pra texto tipo "R$ 12,34" / "R$ -5,00", so'
// pra exibir no console.
std::string formatarCentavos(long long centavos);
