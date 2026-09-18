#include "calibracao.h"
#include "entrada.h"
#include "captura_tela.h"
#include "leitura_valor.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <string>

namespace {

constexpr long long TOLERANCIA_GLIFO_PADRAO = 60;

RegiaoTela regiaoDeDoisPontos(POINT a, POINT b) {
    RegiaoTela r;
    r.x = std::min(a.x, b.x);
    r.y = std::min(a.y, b.y);
    r.largura = std::max(1L, std::abs(b.x - a.x));
    r.altura = std::max(1L, std::abs(b.y - a.y));
    return r;
}

long long diferencaEntre(const std::vector<BYTE>& a, const std::vector<BYTE>& b) {
    if (a.size() != b.size()) return -1;
    long long soma = 0;
    for (size_t i = 0; i < a.size(); ++i) soma += std::abs((int)a[i] - (int)b[i]);
    return soma;
}

bool perguntarSimNao(const std::string& pergunta) {
    std::printf("\n%s (s/N): ", pergunta.c_str());
    std::fflush(stdout);
    std::string linha;
    std::getline(std::cin, linha);
    return !linha.empty() && (linha[0] == 's' || linha[0] == 'S');
}

std::string glifosFaltando(const Calibracao& cal) {
    std::string faltando;
    for (char c : glifosNecessarios()) {
        if (cal.glifos.find(c) == cal.glifos.end()) faltando.push_back(c);
    }
    return faltando;
}

} // namespace

bool rodarCalibracao(Calibracao& out) {
    std::printf("== Calibracao do Miracle ==\n\n");
    std::printf("Parte 1: badge de posicao (so' precisa distinguir FLAT de\n");
    std::printf("\"tem posicao\" -- ver imagem/boleta.png).\n");

    POINT b1 = aguardarClique("canto SUPERIOR ESQUERDO do badge de posicao (\"Qtd\")");
    if (b1.x < 0 && b1.y < 0) return false;
    POINT b2 = aguardarClique("canto INFERIOR DIREITO do badge");
    if (b2.x < 0 && b2.y < 0) return false;
    out.regiaoFlat = regiaoDeDoisPontos(b1, b2);

    CapturaRegiao capFlat(out.regiaoFlat);

    std::printf("\nUse a conta SIMULADORA -- os proximos passos fazem operacao de\n");
    std::printf("verdade, so' pra calibrar.\n");

    aguardarEnter("deixe a posicao ZERADA/FLAT agora");
    if (!capFlat.capturar()) { std::printf(">> falha ao capturar a tela.\n"); return false; }
    out.referenciaFlat = capFlat.pixelsBrutos();

    aguardarEnter("compre 1 contrato a mercado (fique comprado) e confirme");
    if (!capFlat.capturar()) { std::printf(">> falha ao capturar a tela.\n"); return false; }
    std::vector<BYTE> referenciaComprado = capFlat.pixelsBrutos();

    long long diffFlatComprado = diferencaEntre(out.referenciaFlat, referenciaComprado);
    out.toleranciaFlat = diffFlatComprado > 0 ? diffFlatComprado / 3 : 30;
    std::printf(">> diferenca flat<->comprado=%lld -> toleranciaFlat=%lld\n", diffFlatComprado, out.toleranciaFlat);
    std::printf(">> pode zerar a posicao de teste agora.\n");

    std::printf("\nParte 2: campo \"Resultado em Aberto\" (ver imagem/resultado em\n");
    std::printf("aberto.png) -- o valor monetario que o Miracle vai ler pra decidir\n");
    std::printf("reforco/saida. Aponte SO' pro numero, sem o \"R$\" na frente (o\n");
    std::printf("sinal de menos e a virgula, quando aparecerem, fazem parte).\n");

    POINT r1 = aguardarClique("canto SUPERIOR ESQUERDO do valor \"Resultado em Aberto\" (sem o \"R$\")");
    if (r1.x < 0 && r1.y < 0) return false;
    POINT r2 = aguardarClique("canto INFERIOR DIREITO desse valor");
    if (r2.x < 0 && r2.y < 0) return false;
    out.regiaoResultado = regiaoDeDoisPontos(r1, r2);

    CapturaRegiao capResultado(out.regiaoResultado);
    out.glifos.clear();

    std::printf("\nAgora vamos calibrar os caracteres (0-9, \"-\", \",\") um valor de\n");
    std::printf("cada vez. Pode ser com a conta parada (o valor so' muda com o\n");
    std::printf("preco ou com uma operacao nova) -- va' variando a posicao/deixando\n");
    std::printf("o preco andar um pouco entre cada rodada, pra pegar digitos\n");
    std::printf("diferentes.\n");

    while (true) {
        if (!capResultado.capturar()) { std::printf(">> falha ao capturar a tela.\n"); return false; }
        std::vector<Segmento> segmentos = segmentarCaracteres(capResultado);

        std::printf("\n>> %zu caractere(s) detectado(s) no campo agora.\n", segmentos.size());
        std::printf(">> Digite EXATAMENTE o que esta' aparecendo nesse campo agora\n");
        std::printf(">> (ex.: 2,00 ou -15,50, sem \"R$\"): ");
        std::fflush(stdout);
        std::string digitado;
        std::getline(std::cin, digitado);

        if (digitado.size() != segmentos.size()) {
            std::printf(">> nao bate: voce digitou %zu caractere(s) mas o programa achou %zu\n"
                        ">> pedaco(s) na imagem. Confira se a regiao esta' certa (so' o\n"
                        ">> numero, sem \"R$\") e tente de novo.\n", digitado.size(), segmentos.size());
        } else {
            for (size_t i = 0; i < segmentos.size(); ++i) {
                Glifo g;
                g.bitmap = segmentos[i].bitmap;
                g.largura = segmentos[i].largura;
                out.glifos[digitado[i]] = std::move(g);
            }
            std::printf(">> calibrado: ");
            for (const auto& par : out.glifos) std::printf("'%c' ", par.first);
            std::printf("\n");
        }

        std::string faltando = glifosFaltando(out);
        if (!faltando.empty()) {
            std::printf(">> ainda faltam: %s -- continue variando o valor.\n", faltando.c_str());
        } else {
            std::printf(">> todos os caracteres necessarios (%s) ja' foram calibrados.\n",
                        glifosNecessarios().c_str());
            if (!perguntarSimNao("Quer calibrar mais alguma rodada mesmo assim (mais amostras)")) break;
        }
    }

    // toleranciaGlifo = menor diferenca entre QUAISQUER dois glifos de
    // MESMA LARGURA (larguras diferentes nunca sao comparadas de
    // verdade, entao nao entram nessa conta) -- se nao houver nenhum par
    // assim (ex.: cada caractere tem uma largura unica), usa um padrao
    // fixo generoso.
    long long menor = -1;
    char pa = 0, pb = 0;
    for (auto ia = out.glifos.begin(); ia != out.glifos.end(); ++ia) {
        auto ib = ia; ++ib;
        for (; ib != out.glifos.end(); ++ib) {
            if (ia->second.largura != ib->second.largura) continue;
            long long d = diferencaEntre(ia->second.bitmap, ib->second.bitmap);
            if (d < 0) continue;
            if (menor < 0 || d < menor) { menor = d; pa = ia->first; pb = ib->first; }
        }
    }
    out.toleranciaGlifo = menor >= 0 ? menor / 3 : TOLERANCIA_GLIFO_PADRAO;

    std::printf("\n== Calibracao concluida ==\n");
    std::printf("regiao flat: x=%d y=%d %dx%d, toleranciaFlat=%lld\n",
                out.regiaoFlat.x, out.regiaoFlat.y, out.regiaoFlat.largura, out.regiaoFlat.altura, out.toleranciaFlat);
    std::printf("regiao resultado: x=%d y=%d %dx%d\n",
                out.regiaoResultado.x, out.regiaoResultado.y, out.regiaoResultado.largura, out.regiaoResultado.altura);
    std::printf("glifos calibrados: %zu\n", out.glifos.size());
    if (menor >= 0) {
        std::printf("par mais parecido (mesma largura): '%c' vs '%c', diferenca=%lld -> toleranciaGlifo=%lld\n",
                    pa, pb, menor, out.toleranciaGlifo);
        if (menor < 30) {
            std::printf(">> AVISO: dois glifos de mesma largura ficaram muito parecidos --\n"
                        ">> confira se a regiao do resultado esta' bem enquadrada.\n");
        }
    } else {
        std::printf("nenhum par de glifos com a mesma largura -- usando toleranciaGlifo padrao=%lld\n",
                    out.toleranciaGlifo);
    }

    return true;
}
