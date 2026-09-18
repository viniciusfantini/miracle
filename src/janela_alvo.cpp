#include "janela_alvo.h"
#include "entrada.h"
#include <cstdio>
#include <cstring>

namespace {

// o Profit e' um app MDI (Multiple Document Interface): a janela que
// realmente "e' a boleta/DOM" pro proposito do atalho de teclado e' a
// JANELA-FILHA MDI (o painel dockado), nao o frame externo do aplicativo
// inteiro. GetAncestor(GA_ROOT) sobe passando pelo MDIClient direto ate' o
// frame de fora (ex.: "ProfitPro - 5.0.4.23 - Registrado") -- isso e'
// provavelmente a causa do atalho nao fazer efeito: mandamos pro frame
// errado, nao pro painel que o Profit considera "ativo" internamente.
//
// Em vez disso, sobe pela cadeia de pais so' ate' achar o filho direto de
// uma janela de classe "MDIClient" -- esse filho e' a janela MDI real.
HWND acharJanelaMdiOuTopo(HWND clicado) {
    HWND atual = clicado;
    while (true) {
        HWND pai = GetParent(atual);
        if (!pai) break;
        char classePai[64] = {0};
        GetClassNameA(pai, classePai, sizeof(classePai));
        if (std::strcmp(classePai, "MDIClient") == 0) {
            return atual; // atual e' a janela-filha MDI -- o alvo certo
        }
        atual = pai;
    }
    // nao achou MDIClient no caminho -- app normal (nao-MDI), usa o topo
    // absoluto de sempre.
    HWND topo = GetAncestor(clicado, GA_ROOT);
    return topo ? topo : atual;
}

} // namespace

HWND janelaNoPonto(POINT p) {
    HWND w = WindowFromPoint(p);
    if (!w) return nullptr;
    return acharJanelaMdiOuTopo(w);
}

HWND escolherJanelaPorClique(const std::string& rotulo) {
    POINT p = aguardarClique("Clique na janela: " + rotulo);
    if (p.x < 0 && p.y < 0) return nullptr;

    HWND alvo = janelaNoPonto(p);
    if (!alvo) return nullptr;

    char titulo[256] = {0};
    char classe[64] = {0};
    GetWindowTextA(alvo, titulo, sizeof(titulo));
    GetClassNameA(alvo, classe, sizeof(classe));
    std::printf(">> janela escolhida: HWND=%p classe=\"%s\" titulo=\"%s\"\n",
                (void*)alvo, classe, titulo);
    return alvo;
}
