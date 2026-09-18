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
//        <= -R$60,00 -> manda ZERAR (ALT+A), stop, e encerra;
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
constexpr long long STOP_CENTAVOS_2 = -6000;   // -R$60,00, ja' com reforco -> zera (stop)

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

// manda o atalho de verdade (enviar=true, modo 'rodar') ou so' avisa o
// que MANDARIA sem enviar nada (enviar=false, modo 'debug') -- mesma
// separacao debug/rodar do roboclone, pra poder validar a leitura antes
// de confiar em mandar ordem de verdade.
void mandarComando(HWND alvo, char tecla, bool enviar, const char* rotulo) {
    if (enviar) {
        enviarAltTeclaComEspacamento(alvo, tecla);
        std::printf(">> %s enviado.\n", rotulo);
    } else {
        std::printf(">> [DEBUG] mandaria %s (ALT+%c) -- nada foi enviado de verdade.\n", rotulo, tecla);
    }
}

int executarCiclo(Calibracao& cal, HWND leitora, HWND simulador, bool enviar) {
    if (leitora == simulador) {
        std::printf(">> AVISO: leitora e simulador sao a MESMA janela -- se enviar=true isso muda\n"
                    ">> a propria leitura, o que pode nao fazer sentido pra esse teste.\n");
    }

    if (enviar) definirEspacamentoMinimoMs(pedirEspacamentoMinimoMs());

    CapturaRegiao capFlat(cal.regiaoFlat);
    CapturaRegiao capResultado(cal.regiaoResultado);

    std::printf("\n== Passo 1: confirmar FLAT ==\n");
    aguardarFlat(capFlat, cal);
    std::printf(">> flat confirmado.\n");

    std::printf("\n== Passo 2: entrada (COMPRA, 1 contrato) ==\n");
    mandarComando(simulador, 'C', enviar, "COMPRA (entrada)");

    std::printf("\n== Passo 3: gestao pelo Resultado em Aberto ==\n");
    std::printf(">> alvo 1: +R$10,00 (venda) | stop 1: -R$20,00 (reforco -> 2 contratos)\n");
    std::printf(">> alvo 2 (apos reforco): +R$20,00 (zera) | stop 2 (apos reforco): -R$60,00 (zera)\n");

    enum class Estado { COMPRADO_1, COMPRADO_2, FINALIZADO };
    Estado estado = Estado::COMPRADO_1;

    std::optional<long long> ultimoImpresso;
    std::string ultimoTextoBrutoFalho;

    while (estado != Estado::FINALIZADO) {
        Sleep(50);
        if (!capResultado.capturar()) continue;

        std::string textoBruto;
        auto valor = lerResultadoEmCentavos(capResultado, enviar ? nullptr : &textoBruto);
        if (!valor) {
            // no modo debug, mostra o texto cru que o OCR leu (so' quando
            // MUDA, pra nao poluir) -- ajuda a ver POR QUE nao reconheceu
            // em vez de so' ficar em silencio.
            if (!enviar && !textoBruto.empty() && textoBruto != ultimoTextoBrutoFalho) {
                std::printf(">> [DEBUG] OCR leu (bruto, nao interpretavel): \"%s\"\n", textoBruto.c_str());
                ultimoTextoBrutoFalho = textoBruto;
            }
            continue; // OCR nao reconheceu nada interpretavel -- ignora, nao adivinha
        }

        if (!ultimoImpresso || *ultimoImpresso != *valor) {
            std::printf(">> resultado em aberto: %s\n", formatarCentavos(*valor).c_str());
            ultimoImpresso = valor;
        }

        if (estado == Estado::COMPRADO_1) {
            if (*valor >= ALVO_CENTAVOS_1) {
                std::printf(">> alvo de +R$10,00 atingido (%s).\n", formatarCentavos(*valor).c_str());
                mandarComando(simulador, 'V', enviar, "VENDA (saida)");
                estado = Estado::FINALIZADO;
            } else if (*valor <= STOP_CENTAVOS_1) {
                std::printf(">> stop de -R$20,00 atingido (%s).\n", formatarCentavos(*valor).c_str());
                mandarComando(simulador, 'C', enviar, "COMPRA (reforco)");
                estado = Estado::COMPRADO_2;
                ultimoImpresso.reset();
            }
        } else if (estado == Estado::COMPRADO_2) {
            if (*valor >= ALVO_CENTAVOS_2) {
                std::printf(">> alvo de +R$20,00 (com reforco) atingido (%s).\n", formatarCentavos(*valor).c_str());
                mandarComando(simulador, 'A', enviar, "ZERAR (alvo)");
                estado = Estado::FINALIZADO;
            } else if (*valor <= STOP_CENTAVOS_2) {
                std::printf(">> stop de -R$60,00 (com reforco) atingido (%s).\n", formatarCentavos(*valor).c_str());
                mandarComando(simulador, 'A', enviar, "ZERAR (stop)");
                estado = Estado::FINALIZADO;
            }
        }
    }

    std::printf("\n== Ciclo encerrado -- Miracle nao reentra sozinho. ==\n");
    return 0;
}

// modo so' de leitura -- nenhum comando, nenhuma exigencia de FLAT,
// nenhuma janela "simulador". So' fica mostrando o Resultado em Aberto
// reconhecido ao vivo, pra validar a leitura isolada antes de confiar
// nela dentro do ciclo de trade (debug/rodar).
int modoLer() {
    Calibracao cal;
    if (!carregarCalibracaoOuAvisar(cal)) return 1;

    HWND leitora = escolherJanelaPorClique("LEITORA (onde le' o Resultado em Aberto)");
    if (!leitora) { std::printf(">> cancelado.\n"); return 1; }

    CapturaRegiao capResultado(cal.regiaoResultado);
    std::optional<long long> ultimoImpresso;
    std::string ultimoTextoBruto;

    std::printf("\n>> Lendo o Resultado em Aberto (Ctrl+C pra parar) -- mostrando o\n");
    std::printf(">> texto bruto do OCR junto, pra depurar (temporario).\n");

    while (true) {
        Sleep(50);
        if (!capResultado.capturar()) continue;

        std::string textoBruto;
        auto valor = lerResultadoEmCentavos(capResultado, &textoBruto);

        if (!textoBruto.empty() && textoBruto != ultimoTextoBruto) {
            std::printf(">> OCR bruto: \"%s\"\n", textoBruto.c_str());
            ultimoTextoBruto = textoBruto;
        }

        if (!valor) continue;

        if (!ultimoImpresso || *ultimoImpresso != *valor) {
            std::printf(">> resultado em aberto: %s\n", formatarCentavos(*valor).c_str());
            ultimoImpresso = valor;
        }
    }
}

int modoDebugOuRodar(bool enviar) {
    Calibracao cal;
    if (!carregarCalibracaoOuAvisar(cal)) return 1;

    HWND leitora = escolherJanelaPorClique("LEITORA (onde le' o badge de posicao e o Resultado em Aberto)");
    if (!leitora) { std::printf(">> cancelado.\n"); return 1; }

    HWND simulador = escolherJanelaPorClique(
        enviar ? "SIMULADOR (pra onde vai mandar as ordens)"
               : "SIMULADOR (so' pra referencia -- modo debug nao manda nada)");
    if (!simulador) { std::printf(">> cancelado.\n"); return 1; }

    return executarCiclo(cal, leitora, simulador, enviar);
}

} // namespace

int main(int argc, char** argv) {
    std::string modo = argc > 1 ? argv[1] : "";

    if (modo != "calibrar" && modo != "ler" && modo != "debug" && modo != "rodar") {
        std::printf("uso: Miracle.exe <calibrar|ler|debug|rodar>\n");
        return 1;
    }

    if (!iniciarOcr()) return 1;

    if (modo == "calibrar") return modoCalibrar();
    if (modo == "ler") return modoLer();
    if (modo == "debug") return modoDebugOuRodar(false);
    return modoDebugOuRodar(true);
}
