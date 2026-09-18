#include "config.h"
#include <fstream>
#include <map>
#include <cstdlib>
#include <cstdint>

namespace {
std::string caminhoRefs(const std::string& caminhoBase) { return caminhoBase + ".refs"; }
}

bool salvarCalibracao(const Calibracao& c, const std::string& caminhoBase) {
    {
        std::ofstream f(caminhoBase, std::ios::trunc);
        if (!f) return false;
        f << "flat.x=" << c.regiaoFlat.x << "\n";
        f << "flat.y=" << c.regiaoFlat.y << "\n";
        f << "flat.largura=" << c.regiaoFlat.largura << "\n";
        f << "flat.altura=" << c.regiaoFlat.altura << "\n";
        f << "toleranciaFlat=" << c.toleranciaFlat << "\n";
        f << "resultado.x=" << c.regiaoResultado.x << "\n";
        f << "resultado.y=" << c.regiaoResultado.y << "\n";
        f << "resultado.largura=" << c.regiaoResultado.largura << "\n";
        f << "resultado.altura=" << c.regiaoResultado.altura << "\n";
        f << "toleranciaGlifo=" << c.toleranciaGlifo << "\n";
        f << "quantidadeGlifos=" << c.glifos.size() << "\n";
    }

    std::ofstream fr(caminhoRefs(caminhoBase), std::ios::trunc | std::ios::binary);
    if (!fr) return false;

    fr.write(reinterpret_cast<const char*>(c.referenciaFlat.data()), (std::streamsize)c.referenciaFlat.size());

    for (const auto& par : c.glifos) {
        char ch = par.first;
        int32_t largura = par.second.largura;
        fr.write(&ch, 1);
        fr.write(reinterpret_cast<const char*>(&largura), sizeof(largura));
        fr.write(reinterpret_cast<const char*>(par.second.bitmap.data()), (std::streamsize)par.second.bitmap.size());
    }
    return (bool)fr;
}

bool carregarCalibracao(Calibracao& c, const std::string& caminhoBase) {
    std::ifstream f(caminhoBase);
    if (!f) return false;

    std::map<std::string, long long> valores;
    std::string linha;
    while (std::getline(f, linha)) {
        auto pos = linha.find('=');
        if (pos == std::string::npos) continue;
        std::string chave = linha.substr(0, pos);
        long long valor = std::atoll(linha.substr(pos + 1).c_str());
        valores[chave] = valor;
    }
    if (valores.empty()) return false;

    c.regiaoFlat.x = (int)valores["flat.x"];
    c.regiaoFlat.y = (int)valores["flat.y"];
    c.regiaoFlat.largura = (int)valores["flat.largura"];
    c.regiaoFlat.altura = (int)valores["flat.altura"];
    c.toleranciaFlat = valores["toleranciaFlat"];

    c.regiaoResultado.x = (int)valores["resultado.x"];
    c.regiaoResultado.y = (int)valores["resultado.y"];
    c.regiaoResultado.largura = (int)valores["resultado.largura"];
    c.regiaoResultado.altura = (int)valores["resultado.altura"];
    c.toleranciaGlifo = valores["toleranciaGlifo"];
    size_t quantidadeGlifos = (size_t)valores["quantidadeGlifos"];

    size_t bytesFlat = (size_t)c.regiaoFlat.largura * c.regiaoFlat.altura * 4;
    if (bytesFlat == 0) return false;

    std::ifstream fr(caminhoRefs(caminhoBase), std::ios::binary);
    if (!fr) return false;

    c.referenciaFlat.resize(bytesFlat);
    fr.read(reinterpret_cast<char*>(c.referenciaFlat.data()), (std::streamsize)bytesFlat);
    if (!fr) return false;

    c.glifos.clear();
    for (size_t i = 0; i < quantidadeGlifos; ++i) {
        char ch = 0;
        int32_t largura = 0;
        fr.read(&ch, 1);
        fr.read(reinterpret_cast<char*>(&largura), sizeof(largura));
        if (!fr) return false;

        Glifo g;
        g.largura = largura;
        g.bitmap.resize((size_t)largura * c.regiaoResultado.altura * 4);
        fr.read(reinterpret_cast<char*>(g.bitmap.data()), (std::streamsize)g.bitmap.size());
        if (!fr) return false;

        c.glifos[ch] = std::move(g);
    }
    return true;
}
