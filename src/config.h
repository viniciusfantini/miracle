// config.h -- calibracao do Miracle.
//
// Duas coisas bem diferentes calibradas aqui:
//
// 1) Badge de posicao (mesmo "Qtd" do roboclone) -- mas o Miracle so'
//    precisa saber se esta' FLAT ou nao, nao o valor exato (1C/2C/...).
//    Motivo: quem manda as ordens aqui e' o proprio Miracle, entao ele
//    confia na PROPRIA contagem de contratos (sabe que comprou, sabe que
//    reforcou) -- o badge so' serve pra confirmar "esta' zerado" antes de
//    comecar um ciclo novo, nao pra recontar a posicao toda vez.
//
// 2) Campo "Resultado em Aberto" (ver imagem/resultado em aberto.png) --
//    um valor monetario CONTINUO (ex. "2,00", "-15,50"), bem diferente do
//    badge (que tem um conjunto pequeno e fixo de aparencias possiveis).
//    Lido DIGITO A DIGITO: a regiao inteira e' segmentada em caracteres
//    (por coluna de pixel com/sem "tinta"), cada segmento e' comparado
//    contra os glifos calibrados (0-9, "-", ",", ".") por bitmap exato,
//    igual a tecnica ja' usada pro badge -- so' que agora com varios
//    "moldes" pequenos em vez de um conjunto fixo de aparencias inteiras.
//    O "." e' o separador de milhar do formato BR (ex. "1.000,00" quando
//    o resultado passa de R$1.000) -- ignorado na hora de converter pra
//    centavos, so' precisa ser reconhecido pra nao quebrar a segmentacao.
#pragma once

#include "captura_tela.h"
#include <string>
#include <vector>
#include <map>

struct Glifo {
    std::vector<BYTE> bitmap; // largura x altura (a altura de regiaoResultado), BGRA
    int largura = 0;
};

struct Calibracao {
    RegiaoTela regiaoFlat;
    std::vector<BYTE> referenciaFlat;
    long long toleranciaFlat = 0;

    RegiaoTela regiaoResultado; // cobre so' o valor numerico, sem o "R$ " na frente
    std::map<char, Glifo> glifos; // chaves esperadas: '0'..'9', '-', ',', '.'
    long long toleranciaGlifo = 0;
};

// caracteres que uma leitura valida do Resultado em Aberto precisa ter
// calibrados (ver calibracao.cpp). "." e' o separador de milhar BR (ex.
// "1.000,00").
inline std::string glifosNecessarios() { return "0123456789-,."; }

bool salvarCalibracao(const Calibracao& c, const std::string& caminhoBase);
bool carregarCalibracao(Calibracao& c, const std::string& caminhoBase);
