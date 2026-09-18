// entrada.h -- espera o operador clicar em algum ponto da tela (usado
// tanto pra escolher janela quanto pra calibrar regioes/cores por clique).
#pragma once

#include <windows.h>
#include <string>

// devolve a posicao (coordenadas de TELA) do clique com botao esquerdo.
// {-1,-1} se o operador apertar ESC pra cancelar.
POINT aguardarClique(const std::string& instrucao);

// espera o operador apertar ENTER no console (usado quando o que importa
// e' o ESTADO da conta no momento, nao um ponto na tela -- ex.: "fique
// flat e aperte ENTER").
void aguardarEnter(const std::string& instrucao);
