// main.cpp -- Miracle.exe: primeiro teste da CONDUCAO do trade (entrada
// fixa + gestao por "Resultado em Aberto"), sem leitura de gatilho ainda
// (isso fica pra depois -- ver CLAUDE.md/README.md).
//
// Roteiro deste primeiro teste (definido pelo dono, 18/09/2026):
//   1. escolhe a janela "leitora" (le' o badge de posicao + o Resultado
//      em Aberto) e confirma que esta' FLAT;
//   2. manda COMPRA (entrada, 1 contrato) na janela "simulador";
//   3. fica lendo o Resultado em Aberto:
//        >= +R$10,00 -> manda VENDA (saida) e encerra;
//        <= -R$20,00 -> manda COMPRA (reforco, vira 2 contratos);
//      depois do reforco, na posicao de 2 contratos:
//        >= +R$20,00 -> manda ZERAR (ALT+A) e encerra;
//   4. roda EXATAMENTE 1 ciclo e para -- sem reentrada automatica (decisao
//      de seguranca, pra nao repetir os loops descontrolados ja' vividos
//      no roboclone).
#include "config.h"
#include "calibracao.h"
#include "captura_tela.h"
#include "leitura_valor.h"
#include "janela_alvo.h"
#include "atalho.h"
#include <windows.h>
#include <cstdio>
#include <iostream>
#include <string>

namespace {

const std::string CAMINHO_CALIBRACAO = "calibracao_miracle.txt";

constexpr long long ALVO_CENTAVOS_1 = 1000;    // +R$10,00 -> sai (venda)
constexpr long long STOP_CENTAVOS_1 = -2000;   // -R$20,00 -> reforco (compra)
constexpr long long ALVO_CENTAVOS_2 = 2000;    // +R$20,00, ja' com reforco -> zera

int pedirEspacamentoMinimoMs() {
    std::printf("\nEspacamento minimo entre comandos em ms (ENTER = %d): ", DELAY_MIN_ENTRE_COPIAS_MS_PADRAO);
    std::fflush(stdout);
    std::string linha;
    std::getline(std::cin, linha);
    if (linha.empty()) return DELAY_MIN_ENTRE_COPIAS_MS_PADRAO;
    int ms = std::atoi(linha.c_str());
    return ms > 0 ? ms : DELAY_MIN_ENTRE_COPIAS_MS_PADRAO;
}

bool carregarCalibracaoOuAvisar(Calibracao& cal) {
    if (carregarCalibracao(cal, CAMINHO_CALIBRACAO)) return true;
    std::printf(">> nenhuma calibracao encontrada (%s). Rode 'Miracle.exe calibrar' primeiro.\n",
                CAMINHO_CALIBRACAO.c_str());
    return false;
}

bool estaFlat(CapturaRegiao& capFlat, const Calibracao& cal) {
    if (!capFlat.capturar()) return false;
    return capFlat.diferencaPara(cal.referenciaFlat) <= cal.toleranciaFlat;
}

// espera ficar flat, checando a cada 200ms; imprime uma vez que esta'
// esperando (nao a cada rodada, pra nao poluir o console).
void aguardarFlat(CapturaRegiao& capFlat, const Calibracao& cal) {
    if (estaFlat(capFlat, cal)) return;
    std::printf(">> posicao atual nao esta' flat -- aguardando ficar flat...\n");
    while (!estaFlat(capFlat, cal)) {
        Sleep(200);
    }
}

int modoCalibrar() {
    Calibracao cal;
    if (carregarCalibracao(cal, CAMINHO_CALIBRACAO)) {
        std::printf(">> ja' existe uma calibracao salva em %s.\n", CAMINHO_CALIBRACAO.c_str());
        std::printf(">> Recalibrar do zero? (s/N): ");
        std::fflush(stdout);
        std::string linha;
        std::getline(std::cin, linha);
        if (linha.empty() || (linha[0] != 's' && linha[0] != 'S')) {
            std::printf(">> mantendo a calibracao existente, nada foi alterado.\n");
            return 0;
        }
    }

    Calibracao nova;
    if (!rodarCalibracao(nova)) {
        std::printf(">> calibracao cancelada.\n");
        return 1;
    }
    if (!salvarCalibracao(nova, CAMINHO_CALIBRACAO)) {
        std::printf(">> falha ao salvar a calibracao em %s.\n", CAMINHO_CALIBRACAO.c_str());
        return 1;
    }
    std::printf(">> calibracao salva em %s.\n", CAMINHO_CALIBRACAO.c_str());
    return 0;
}

int modoRodar() {
    Calibracao cal;
    if (!carregarCalibracaoOuAvisar(cal)) return 1;

    HWND leitora = escolherJanelaPorClique("LEITORA (onde le' o badge de posicao e o Resultado em Aberto)");
    if (!leitora) { std::printf(">> cancelado.\n"); return 1; }

    HWND simulador = escolherJanelaPorClique("SIMULADOR (pra onde vai mandar as ordens)");
    if (!simulador) { std::printf(">> cancelado.\n"); return 1; }

    if (leitora == simulador) {
        std::printf(">> AVISO: leitora e simulador sao a MESMA janela -- os comandos enviados\n"
                    ">> vao mudar a leitura, o que pode nao fazer sentido pra esse teste.\n");
    }

    definirEspacamentoMinimoMs(pedirEspacamentoMinimoMs());

    CapturaRegiao capFlat(cal.regiaoFlat);
    CapturaRegiao capResultado(cal.regiaoResultado);

    std::printf("\n== Passo 1: confirmar FLAT ==\n");
    aguardarFlat(capFlat, cal);
    std::printf(">> flat confirmado.\n");

    std::printf("\n== Passo 2: entrada (COMPRA, 1 contrato) ==\n");
    enviarAltTeclaComEspacamento(simulador, 'C');
    std::printf(">> COMPRA enviada.\n");

    std::printf("\n== Passo 3: gestao pelo Resultado em Aberto ==\n");
    std::printf(">> alvo 1: +R$10,00 (venda) | stop 1: -R$20,00 (reforco -> 2 contratos)\n");
    std::printf(">> alvo 2 (apos reforco): +R$20,00 (zera)\n");

    enum class Estado { COMPRADO_1, COMPRADO_2, FINALIZADO };
    Estado estado = Estado::COMPRADO_1;

    std::optional<long long> ultimoImpresso;

    while (estado != Estado::FINALIZADO) {
        Sleep(50);
        if (!capResultado.capturar()) continue;

        auto valor = lerResultadoEmCentavos(capResultado, cal);
        if (!valor) continue; // nao bateu com nenhum glifo -- ignora, nao adivinha

        if (!ultimoImpresso || *ultimoImpresso != *valor) {
            std::printf(">> resultado em aberto: %s\n", formatarCentavos(*valor).c_str());
            ultimoImpresso = valor;
        }

        if (estado == Estado::COMPRADO_1) {
            if (*valor >= ALVO_CENTAVOS_1) {
                std::printf(">> alvo de +R$10,00 atingido (%s) -- enviando VENDA (saida).\n",
                            formatarCentavos(*valor).c_str());
                enviarAltTeclaComEspacamento(simulador, 'V');
                estado = Estado::FINALIZADO;
            } else if (*valor <= STOP_CENTAVOS_1) {
                std::printf(">> stop de -R$20,00 atingido (%s) -- enviando COMPRA (reforco).\n",
                            formatarCentavos(*valor).c_str());
                enviarAltTeclaComEspacamento(simulador, 'C');
                estado = Estado::COMPRADO_2;
                ultimoImpresso.reset();
            }
        } else if (estado == Estado::COMPRADO_2) {
            if (*valor >= ALVO_CENTAVOS_2) {
                std::printf(">> alvo de +R$20,00 (com reforco) atingido (%s) -- enviando ZERAR.\n",
                            formatarCentavos(*valor).c_str());
                enviarAltTeclaComEspacamento(simulador, 'A');
                estado = Estado::FINALIZADO;
            }
            // sem stop adicional depois do reforco neste primeiro teste --
            // limitacao conhecida, nao pedida pelo dono ainda.
        }
    }

    std::printf("\n== Ciclo encerrado -- Miracle nao reentra sozinho. ==\n");
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    std::string modo = argc > 1 ? argv[1] : "";

    if (modo == "calibrar") return modoCalibrar();
    if (modo == "rodar") return modoRodar();

    std::printf("uso: Miracle.exe <calibrar|rodar>\n");
    return 1;
}
