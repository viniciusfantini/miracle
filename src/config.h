// config.h -- calibracao do Miracle.
//
// Duas coisas bem diferentes calibradas aqui:
//
// 1) Badge de posicao (mesmo "Qtd" do roboclone) -- mas o Miracle so'
//    precisa saber se esta' FLAT ou nao, nao o valor exato (1C/2C/...).
//    Motivo: quem manda as ordens aqui e' o proprio Miracle, entao ele
//    confia na PROPRIA contagem de contratos (sabe que comprou, sabe que
//    reforcou) -- o badge so' serve pra confirmar "esta' zerado" antes de
//    comecar um ciclo novo, nao pra recontar a posicao toda vez. Isso
//    continua bitmap exato (poucas aparencias fixas, exatamente o caso
//    que essa tecnica resolve bem).
//
// 2) Campo "Resultado em Aberto" (ver imagem/resultado em aberto.png) --
//    um valor monetario CONTINUO (ex. "2,00", "-15,50", "R$1.234,56"),
//    com cor variando (lucro/prejuizo) e tamanho variando com o numero de
//    digitos. Depois de repetidas voltas tentando ler isso por
//    comparacao de bitmap por caractere (limiar de tinta sensivel a
//    kerning, "R$" as vezes colado, cor mudando com o sinal...), trocado
//    (18/09/2026) pro OCR nativo do Windows (Windows.Media.Ocr via
//    C++/WinRT, ver leitura_valor.cpp) -- resolve tudo isso de fabrica,
//    sem precisar calibrar caractere nenhum. So' precisa apontar a
//    regiao (2 cliques), nao treinar glifo.
#pragma once

#include "captura_tela.h"
#include <string>

struct Calibracao {
    RegiaoTela regiaoFlat;
    std::vector<BYTE> referenciaFlat;
    long long toleranciaFlat = 0;

    RegiaoTela regiaoResultado; // lido via OCR (Windows.Media.Ocr) -- ver leitura_valor.cpp
};

bool salvarCalibracao(const Calibracao& c, const std::string& caminhoBase);
bool carregarCalibracao(Calibracao& c, const std::string& caminhoBase);
