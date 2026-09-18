#include "leitura_valor.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace {

// diferenca por canal (BGR) pra considerar "tem tinta" nesse pixel.
// Medido ao vivo (18/09/2026) em DUAS capturas reais diferentes do
// Resultado em Aberto: o anti-aliasing entre caracteres vizinhos chega a
// uns 39 de diferenca mesmo SEM ser o traco real do digito -- um limiar
// baixo deixa colado sem nenhuma coluna de fundo puro entre caracteres
// (em "R$127,00" isso colava especificamente o "2" e o "7", nao o "R$"
// como se pensou antes de olhar o pixel de verdade). Testado varios
// limiares nas duas capturas: 135 fica dentro do platô estavel das duas
// (80-150 na 1a, 120-200 na 2a), separando TODOS os caracteres
// corretamente sem erodir o traco real dos digitos.
constexpr int LIMIAR_TINTA = 135;

bool pixelTemTinta(const BYTE* p, BYTE fundoB, BYTE fundoG, BYTE fundoR) {
    return std::abs((int)p[0] - fundoB) > LIMIAR_TINTA ||
           std::abs((int)p[1] - fundoG) > LIMIAR_TINTA ||
           std::abs((int)p[2] - fundoR) > LIMIAR_TINTA;
}

std::vector<BYTE> extrairFatia(const std::vector<BYTE>& bufferGrande, int larguraGrande, int altura,
                                int offsetX, int largura) {
    std::vector<BYTE> sub((size_t)largura * altura * 4);
    for (int linha = 0; linha < altura; ++linha) {
        const BYTE* origem = bufferGrande.data() + ((size_t)linha * larguraGrande + offsetX) * 4;
        BYTE* destino = sub.data() + (size_t)linha * largura * 4;
        std::memcpy(destino, origem, (size_t)largura * 4);
    }
    return sub;
}

long long diferencaEntre(const std::vector<BYTE>& a, const std::vector<BYTE>& b) {
    if (a.size() != b.size()) return -1;
    long long soma = 0;
    for (size_t i = 0; i < a.size(); ++i) soma += std::abs((int)a[i] - (int)b[i]);
    return soma;
}

} // namespace

std::vector<Segmento> segmentarCaracteres(const CapturaRegiao& cap) {
    std::vector<Segmento> segmentos;
    const RegiaoTela& r = cap.regiao();
    const std::vector<BYTE>& buf = cap.pixelsBrutos();
    if (r.largura <= 0 || r.altura <= 0 || buf.size() < (size_t)r.largura * r.altura * 4) return segmentos;

    // amostra o fundo no canto superior esquerdo -- assume que a
    // calibracao da regiao deixou uma margem de fundo ali (sem cortar em
    // cima de um digito).
    BYTE fundoB = buf[0], fundoG = buf[1], fundoR = buf[2];

    std::vector<bool> temTinta(r.largura, false);
    for (int x = 0; x < r.largura; ++x) {
        for (int y = 0; y < r.altura; ++y) {
            const BYTE* p = buf.data() + ((size_t)y * r.largura + x) * 4;
            if (pixelTemTinta(p, fundoB, fundoG, fundoR)) { temTinta[x] = true; break; }
        }
    }

    int inicio = -1;
    for (int x = 0; x <= r.largura; ++x) {
        bool tinta = (x < r.largura) && temTinta[x];
        if (tinta && inicio < 0) {
            inicio = x;
        } else if (!tinta && inicio >= 0) {
            Segmento s;
            s.offsetX = inicio;
            s.largura = x - inicio;
            s.bitmap = extrairFatia(buf, r.largura, r.altura, inicio, s.largura);
            segmentos.push_back(std::move(s));
            inicio = -1;
        }
    }

    return segmentos;
}

bool valorTocaBorda(const CapturaRegiao& cap) {
    std::vector<Segmento> segmentos = segmentarCaracteres(cap);
    if (segmentos.empty()) return false;
    return segmentos.front().offsetX == 0 ||
           segmentos.back().offsetX + segmentos.back().largura >= cap.regiao().largura;
}

std::optional<long long> lerResultadoEmCentavos(const CapturaRegiao& cap, const Calibracao& cal) {
    std::vector<Segmento> segmentos = segmentarCaracteres(cap);
    if (segmentos.empty()) return std::nullopt;

    // se um pedaco encosta na borda da regiao capturada, a leitura pode
    // estar com um caractere cortado -- o caso mais perigoso e' o sinal
    // "-" sumir (ex.: "-10,00" virar so' "10,00"), o que INVERTE o sinal
    // do resultado (prejuizo lido como lucro). Descarta a leitura inteira
    // em vez de arriscar isso -- mesma filosofia de nunca adivinhar.
    bool encostouBorda = segmentos.front().offsetX == 0 ||
                          segmentos.back().offsetX + segmentos.back().largura >= cap.regiao().largura;
    if (encostouBorda) return std::nullopt;

    std::string reconhecido;
    reconhecido.reserve(segmentos.size());

    for (const auto& seg : segmentos) {
        char melhorChar = 0;
        long long melhorDiff = -1;
        for (const auto& par : cal.glifos) {
            if (par.second.largura != seg.largura) continue;
            long long d = diferencaEntre(seg.bitmap, par.second.bitmap);
            if (d < 0) continue;
            if (melhorDiff < 0 || d < melhorDiff) { melhorDiff = d; melhorChar = par.first; }
        }
        if (melhorDiff < 0 || melhorDiff > cal.toleranciaGlifo) return std::nullopt; // nao adivinha
        reconhecido.push_back(melhorChar);
    }

    // reconhecido agora e' algo tipo "-15,50", "2,00" ou "R$-15,50" (o
    // campo e' alinhado a' direita e tem tamanho fixo -- quando o valor e'
    // pequeno sobra espaco na regiao e o label "R$" aparece junto). "R" e
    // "$" sao so' decoracao, tira antes de interpretar o numero -- em
    // QUALQUER posicao, nao so' no inicio, pra nao depender de "R$" vir
    // sempre antes do sinal de menos.
    std::string semPrefixo;
    semPrefixo.reserve(reconhecido.size());
    for (char c : reconhecido) {
        if (c == 'R' || c == '$') continue;
        semPrefixo.push_back(c);
    }

    bool negativo = false;
    size_t idx = 0;
    if (!semPrefixo.empty() && semPrefixo[0] == '-') { negativo = true; idx = 1; }

    size_t posVirgula = semPrefixo.find(',', idx);
    if (posVirgula == std::string::npos) return std::nullopt;

    std::string parteInteira = semPrefixo.substr(idx, posVirgula - idx);
    std::string parteCentavos = semPrefixo.substr(posVirgula + 1);
    if (parteInteira.empty() || parteCentavos.size() != 2) return std::nullopt;
    for (char c : parteCentavos) if (c < '0' || c > '9') return std::nullopt;

    // "." e' o separador de milhar do formato BR (ex. "1.000") -- so'
    // marca agrupamento, nao entra no valor. Ignora na hora de converter,
    // mas exige que o resto seja digito (nunca adivinha se aparecer algo
    // fora do esperado).
    std::string parteInteiraSemPontos;
    for (char c : parteInteira) {
        if (c == '.') continue;
        if (c < '0' || c > '9') return std::nullopt;
        parteInteiraSemPontos.push_back(c);
    }
    if (parteInteiraSemPontos.empty()) return std::nullopt;

    long long valorInteiro = std::atoll(parteInteiraSemPontos.c_str());
    long long valorCentavos = std::atoll(parteCentavos.c_str());
    long long total = valorInteiro * 100 + valorCentavos;
    return negativo ? -total : total;
}

std::string formatarCentavos(long long centavos) {
    bool negativo = centavos < 0;
    long long abs = negativo ? -centavos : centavos;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "R$ %s%lld,%02lld", negativo ? "-" : "", abs / 100, abs % 100);
    return buf;
}
