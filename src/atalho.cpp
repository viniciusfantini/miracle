#include "atalho.h"
#include <chrono>
#include <thread>
#include <cstdio>

namespace {

LPARAM montarLParam(BYTE scanCode, bool contextoAlt, bool subindo) {
    LPARAM l = 1; // repeat count = 1
    l |= (LPARAM)scanCode << 16;
    if (contextoAlt) l |= (LPARAM)1 << 29;
    if (subindo) {
        l |= (LPARAM)1 << 30; // previous key state = down
        l |= (LPARAM)1 << 31; // transition state = up
    }
    return l;
}

int g_espacamentoMinimoMs = DELAY_MIN_ENTRE_COPIAS_MS_PADRAO;

} // namespace

void definirEspacamentoMinimoMs(int ms) {
    g_espacamentoMinimoMs = ms > 0 ? ms : 0;
}

int espacamentoMinimoAtualMs() {
    return g_espacamentoMinimoMs;
}

void enviarAltTecla(HWND alvo, char tecla) {
    if (!alvo || !IsWindow(alvo)) {
        std::fprintf(stderr, "[atalho] janela alvo invalida, ALT+%c NAO enviado\n", tecla);
        return;
    }

    WORD vk = (WORD)(unsigned char)tecla;
    BYTE scanAlt   = (BYTE)MapVirtualKeyA(VK_MENU, MAPVK_VK_TO_VSC);
    BYTE scanTecla = (BYTE)MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);

    // 1) ALT desce
    PostMessageA(alvo, WM_SYSKEYDOWN, VK_MENU, montarLParam(scanAlt, false, false));
    // 2) tecla desce, com o contexto ALT marcado (bit 29)
    PostMessageA(alvo, WM_SYSKEYDOWN, vk, montarLParam(scanTecla, true, false));
    // 3) tecla sobe
    PostMessageA(alvo, WM_SYSKEYUP, vk, montarLParam(scanTecla, true, true));
    // 4) ALT sobe
    PostMessageA(alvo, WM_SYSKEYUP, VK_MENU, montarLParam(scanAlt, false, true));

    SYSTEMTIME st;
    GetLocalTime(&st);
    std::fprintf(stdout, "[atalho] ALT+%c enviado as %02d:%02d:%02d.%03d\n", tecla,
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void enviarAltTeclaComFoco(HWND alvo, char tecla) {
    if (!alvo || !IsWindow(alvo)) {
        std::fprintf(stderr, "[atalho-foco] janela alvo invalida, ALT+%c NAO enviado\n", tecla);
        return;
    }

    HWND anterior = GetForegroundWindow();
    DWORD threadAtual = GetCurrentThreadId();
    DWORD threadAlvo = GetWindowThreadProcessId(alvo, nullptr);
    bool anexado = false;
    if (threadAlvo != 0 && threadAlvo != threadAtual) {
        anexado = AttachThreadInput(threadAtual, threadAlvo, TRUE) != 0;
    }

    if (IsIconic(alvo)) ShowWindow(alvo, SW_RESTORE);
    SetForegroundWindow(alvo);

    INPUT in[4] = {};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_MENU;
    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = (WORD)(unsigned char)tecla;
    in[2].type = INPUT_KEYBOARD;
    in[2].ki.wVk = in[1].ki.wVk;
    in[2].ki.dwFlags = KEYEVENTF_KEYUP;
    in[3].type = INPUT_KEYBOARD;
    in[3].ki.wVk = VK_MENU;
    in[3].ki.dwFlags = KEYEVENTF_KEYUP;
    UINT enviados = SendInput(4, in, sizeof(INPUT));

    if (anterior && anterior != alvo) {
        SetForegroundWindow(anterior);
    }
    if (anexado) {
        AttachThreadInput(threadAtual, threadAlvo, FALSE);
    }

    SYSTEMTIME st;
    GetLocalTime(&st);
    std::fprintf(stdout, "[atalho-foco] ALT+%c enviado as %02d:%02d:%02d.%03d (%s)\n", tecla,
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                 enviados == 4 ? "ok" : "FALHOU (SendInput bloqueado)");
}

void enviarAltTeclaComEspacamento(HWND alvo, char tecla) {
    // espera o espacamento INTEIRO sempre, nao so' quando o envio anterior
    // foi recente -- vale tanto entre ordens em sequencia quanto do
    // instante em que a leitura detectou a mudanca ate' o envio de
    // verdade (pedido do dono, 11/09/2026: testando com origem e destino
    // na MESMA janela do Profit, o espacamento precisa cobrir esse
    // caminho todo pra nao se confundir com o proprio efeito do envio).
    if (g_espacamentoMinimoMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(g_espacamentoMinimoMs));
    }
    enviarAltTecla(alvo, tecla);
}
