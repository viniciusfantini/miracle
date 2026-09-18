#include "calibracao.h"
#include "entrada.h"
#include "captura_tela.h"
#include "leitura_valor.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

namespace {

constexpr long long TOLERANCIA_GLIFO_PADRAO = 60;
const char* CAMINHO_IMAGEM_CALIBRACAO = "miracle_calibracao_valor.bmp";

// salva a captura como .bmp -- uma "foto congelada" do que foi lido, pra
// o operador poder digitar o valor olhando uma imagem PARADA em vez de
// correr atras do preco mudando ao vivo na tela (o Resultado em Aberto
// pode atualizar varias vezes por segundo).
bool salvarBmp(const CapturaRegiao& cap, const std::string& caminho) {
    const RegiaoTela& r = cap.regiao();
    const std::vector<BYTE>& buf = cap.pixelsBrutos();
    if (r.largura <= 0 || r.altura <= 0 || buf.size() < (size_t)r.largura * r.altura * 4) return false;

    BITMAPFILEHEADER fh = {};
    BITMAPINFOHEADER ih = {};
    ih.biSize = sizeof(BITMAPINFOHEADER);
    ih.biWidth = r.largura;
    ih.biHeight = r.altura; // positivo = BMP padrao (bottom-up)
    ih.biPlanes = 1;
    ih.biBitCount = 32;
    ih.biCompression = BI_RGB;
    ih.biSizeImage = (DWORD)((size_t)r.largura * r.altura * 4);

    fh.bfType = 0x4D42; // "BM"
    fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fh.bfSize = fh.bfOffBits + ih.biSizeImage;

    std::ofstream f(caminho, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    f.write(reinterpret_cast<const char*>(&ih), sizeof(ih));

    // buf e' top-down (linha 0 = topo da regiao); BMP bottom-up precisa
    // escrever a ULTIMA linha primeiro.
    for (int linha = r.altura - 1; linha >= 0; --linha) {
        const char* p = reinterpret_cast<const char*>(buf.data() + (size_t)linha * r.largura * 4);
        f.write(p, (std::streamsize)r.largura * 4);
    }
    return (bool)f;
}

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
    std::printf("diferentes. O Resultado em Aberto pode mudar varias vezes por\n");
    std::printf("segundo, rapido demais pra digitar olhando a tela ao vivo -- por\n");
    std::printf("isso, a cada rodada o programa captura e salva uma FOTO CONGELADA\n");
    std::printf("do campo em '%s' (nesta pasta): abra esse arquivo\n", CAMINHO_IMAGEM_CALIBRACAO);
    std::printf("num visualizador de imagens e digite exatamente o que esta' nele --\n");
    std::printf("ele nao muda mais, mesmo que o preco continue mudando na tela real.\n");
    std::printf("A regiao clicada tem tamanho FIXO -- deixe BASTANTE folga nas\n");
    std::printf("laterais (principalmente a ESQUERDA, pra caber o sinal \"-\" quando\n");
    std::printf("o resultado ficar negativo) e embaixo/em cima, e nao so' o tamanho\n");
    std::printf("exato do valor que esta' na tela agora -- se o valor crescer (ex.:\n");
    std::printf("de \"2,00\" pra \"1234,56\" ou de \"2,00\" pra \"-2,00\"), uma regiao\n");
    std::printf("justa demais corta caracteres, e o programa avisa isso mais abaixo.\n");

    while (true) {
        if (!capResultado.capturar()) { std::printf(">> falha ao capturar a tela.\n"); return false; }
        std::vector<BYTE> primeiraCaptura = capResultado.pixelsBrutos();
        Sleep(80);
        if (!capResultado.capturar()) { std::printf(">> falha ao capturar a tela.\n"); return false; }
        if (capResultado.pixelsBrutos() != primeiraCaptura) {
            std::printf(">> o valor mudou bem na hora da captura (o preco deve ter mexido) --\n"
                        ">> tentando de novo...\n");
            continue;
        }

        if (!salvarBmp(capResultado, CAMINHO_IMAGEM_CALIBRACAO)) {
            std::printf(">> falha ao salvar a imagem de calibracao (%s).\n", CAMINHO_IMAGEM_CALIBRACAO);
            return false;
        }

        std::printf("\n>> Salvei '%s' -- abra esse arquivo agora e digite\n", CAMINHO_IMAGEM_CALIBRACAO);
        std::printf(">> EXATAMENTE o valor que esta' nele (ex.: 2,00 ou -15,50, sem\n");
        std::printf(">> \"R$\", usando VIRGULA, nao ponto): ");
        std::fflush(stdout);
        std::string digitado;
        std::getline(std::cin, digitado);

        bool caractereInvalido = false;
        for (char c : digitado) {
            if (glifosNecessarios().find(c) == std::string::npos) { caractereInvalido = true; break; }
        }
        if (caractereInvalido) {
            std::printf(">> caractere fora do esperado (so' 0-9, \"-\" e \",\" sao validos --\n"
                        ">> confira se nao digitou ponto no lugar de virgula). Tente de novo\n"
                        ">> com a MESMA foto ainda salva em '%s'.\n", CAMINHO_IMAGEM_CALIBRACAO);
            continue;
        }

        std::vector<Segmento> segmentos = segmentarCaracteres(capResultado);

        bool encostouBorda = !segmentos.empty() &&
                              (segmentos.front().offsetX == 0 ||
                               segmentos.back().offsetX + segmentos.back().largura >= capResultado.regiao().largura);
        if (encostouBorda) {
            std::printf(">> AVISO: um pedaco encostou na borda da regiao capturada em '%s' --\n"
                        ">> a regiao provavelmente esta' cortando um caractere (ex.: o sinal\n"
                        ">> \"-\" ou um digito). Cancele (ESC no proximo clique) e recalibre a\n"
                        ">> regiao do Resultado em Aberto com bem mais folga nas laterais.\n",
                        CAMINHO_IMAGEM_CALIBRACAO);
        }

        if (digitado.size() != segmentos.size()) {
            std::printf(">> nao bate: voce digitou %zu caractere(s) mas o programa achou %zu\n"
                        ">> pedaco(s) na imagem. Confira em '%s' se a regiao esta'\n"
                        ">> certa (so' o numero, sem \"R$\") e tente de novo.\n",
                        digitado.size(), segmentos.size(), CAMINHO_IMAGEM_CALIBRACAO);
            continue;
        }

        for (size_t i = 0; i < segmentos.size(); ++i) {
            Glifo g;
            g.bitmap = segmentos[i].bitmap;
            g.largura = segmentos[i].largura;
            out.glifos[digitado[i]] = std::move(g);
        }
        std::printf(">> calibrado: ");
        for (const auto& par : out.glifos) std::printf("'%c' ", par.first);
        std::printf("\n");

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
