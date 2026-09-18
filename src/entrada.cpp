#include "entrada.h"
#include <cstdio>
#include <thread>
#include <chrono>
#include <iostream>
#include <string>

POINT aguardarClique(const std::string& instrucao) {
    std::printf("\n>> %s\n>>   (clique com o botao ESQUERDO; ESC cancela)\n", instrucao.c_str());
    std::fflush(stdout);

    while (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    while (true) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            std::printf(">> cancelado.\n");
            return POINT{-1, -1};
        }
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            POINT p;
            GetCursorPos(&p);
            std::printf(">> clique em (%ld, %ld)\n", p.x, p.y);
            while (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }
            return p;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}

void aguardarEnter(const std::string& instrucao) {
    std::printf("\n>> %s\n>>   (ENTER pra confirmar)\n", instrucao.c_str());
    std::fflush(stdout);
    std::string linha;
    std::getline(std::cin, linha);
}
