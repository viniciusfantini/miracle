// atalho.h -- manda ALT+C / ALT+V / ALT+A pra uma janela especifica do
// Profit (conta simuladora), via PostMessage/WM_SYSKEYDOWN, SEM depender
// de foco.
//
// Baseado no achado validado ao vivo em 15/08/2026 no b3_money_copy
// (4q-winfut/b3_money_copy/README.md, secao "Limitacao atual"): PostMessage
// com WM_SYSKEYDOWN/WM_SYSKEYUP (precisa ser SYS* por causa do ALT) direto
// no HWND alvo entrega a tecla mesmo com a janela em segundo plano -- e' o
// que o ControlSend do AutoHotkey faz por baixo. A ferramenta de teste que
// validou isso nao foi commitada (so' ficou no scratch da sessao), entao
// a sequencia de eventos aqui e' reconstruida a partir da descricao do
// README ("EnumWindows + 4x PostMessage: ALT down, tecla down com bit 29
// do lParam = contexto ALT, tecla up, ALT up") -- PRECISA ser validada ao
// vivo de novo contra uma conta SIMULADORA antes de confiar em producao.
//
// SEGURANCA: isso manda ordem a mercado de verdade na janela alvo. Nunca
// apontar pra uma conta real -- so' simulador.
#pragma once

#include <windows.h>
#include <string>

// convencao (igual b3_money_copy): 'C' compra, 'V' vende, 'A' zera e
// cancela ordens.
void enviarAltTecla(HWND alvo, char tecla);

// garante o espacamento minimo entre comandos copiados (o Profit recusa
// com "Alerta de envio duplicado" se dois atalhos saem rapido demais --
// achado em 15/08/2026, 200ms resolveu la'). Bloqueia a thread chamadora
// pelo tempo que faltar, se precisar. Usa o valor configurado por
// definirEspacamentoMinimoMs() (default DELAY_MIN_ENTRE_COPIAS_MS_PADRAO).
void enviarAltTeclaComEspacamento(HWND alvo, char tecla);

// configura o espacamento minimo usado por enviarAltTeclaComEspacamento()
// daqui pra frente (ms). Pedido ao operador antes de iniciar o "rodar"/
// "testaratalho" -- ver main.cpp.
void definirEspacamentoMinimoMs(int ms);
int espacamentoMinimoAtualMs();

// alternativa COM foco: rouba o foco pra 'alvo' (SetForegroundWindow),
// manda a tecla via SendInput (tecla de verdade, injetada no stream de
// input do sistema -- ve' ate' hook global de teclado, ao contrario do
// PostMessage) e devolve o foco pra janela que estava em primeiro plano
// antes. Efeito colateral: a janela alvo pisca por cima na hora do envio
// (por isso o b3_money_copy original descartou essa opcao a favor do
// PostMessage) -- usar so' se enviarAltTecla (PostMessage) nao funcionar
// de verdade contra o Profit.
void enviarAltTeclaComFoco(HWND alvo, char tecla);

constexpr int DELAY_MIN_ENTRE_COPIAS_MS_PADRAO = 200;
