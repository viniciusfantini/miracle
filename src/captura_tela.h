// captura_tela.h -- captura de uma regiao PEQUENA da tela (BitBlt puro,
// sem OCR) pra vigiar o indicador de posicao ("Qtd", ex.: "1C"/"2V") no
// painel do Profit, com baixa latencia.
//
// Por que nao UI Automation: testado ao vivo em 11/09/2026 contra o
// ProfitPro 5.0.4.23 rodando na maquina do dono -- a grade/paineis de
// negociacao sao controles Delphi customizados, sem filhos/patterns
// acessiveis. So' da pra ler por pixel.
//
// Por que o indicador de posicao (badge "Qtd") em vez da grade Executadas:
// o dono apontou (11/09/2026) que contar so' o LADO de cada execucao pode
// dessincronizar se uma saida sair como VARIAS execucoes parciais em vez
// de uma so' do tamanho da posicao inteira. O badge de posicao mostra o
// estado REAL e atual (comprado/vendido/zerado), entao so' fica "zerado"
// quando a posicao de fato zerou, nao importa quantas execucoes levou.
//
// Por que comparacao de bitmap (nao so' cor media): o badge de comprado e
// o de vendido podem ter o MESMO fundo, so' mudando o numero/letra -- cor
// media do retangulo inteiro nao distingue. Comparar o bitmap inteiro
// (pixel a pixel, soma de diferencas absolutas) contra uma referencia POR
// QUANTIDADE EXATA (vazio, 1..N comprado, 1..N vendido -- ver config.h)
// pega a forma certa de cada digito/letra, e de quebra permite detectar
// REDUCAO parcial (ex.: 7C -> 6C), nao so' reforco -- mudanca de design
// de 15/09/2026, ver sinal.h.
#pragma once

#include <windows.h>
#include <vector>
#include <cstdint>

struct RegiaoTela {
    int x = 0, y = 0, largura = 0, altura = 0;
};

// snapshot de pixels (BGRA, 4 bytes/pixel) de uma regiao da tela em
// coordenadas de TELA (absolutas). Quem calibra e' responsavel por
// recalcular x/y se a janela de origem se mover -- ver calibracao.h.
class CapturaRegiao {
public:
    explicit CapturaRegiao(RegiaoTela regiao);

    // recaptura a regiao; devolve false se falhar (ex.: monitor
    // desconectado). Custo esperado: regiao pequena (dezenas de pixels)
    // fica bem abaixo de 1ms num BitBlt de memoria de video local.
    bool capturar();

    // cor media (BGR) dentro da regiao inteira do ultimo capturar() bem
    // sucedido.
    void corMedia(BYTE& b, BYTE& g, BYTE& r) const;

    // true se a regiao mudou (qualquer pixel) desde a captura anterior --
    // usado pra detectar "o badge mudou" sem precisar entender o layout.
    bool mudouDesdeUltimaCaptura() const { return mudou_; }

    // soma das diferencas absolutas, byte a byte, contra uma referencia do
    // MESMO tamanho (ver largura/altura de regiao()). Quanto menor, mais
    // parecido. Se os tamanhos nao baterem, devolve um valor bem alto
    // (LONG_MAX) pra nunca "bater" por engano.
    long long diferencaPara(const std::vector<BYTE>& referencia) const;

    const std::vector<BYTE>& pixelsBrutos() const { return atual_; }
    const RegiaoTela& regiao() const { return regiao_; }

private:
    RegiaoTela regiao_;
    std::vector<BYTE> atual_;
    std::vector<BYTE> anterior_;
    bool mudou_ = false;
};
