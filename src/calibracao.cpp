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

    std::printf("\nParte 2: campo \"Resultado em Aberto\" (ver imagem/resultado em\n");
    std::printf("aberto.png) -- o valor monetario que o Miracle vai ler pra decidir\n");
    std::printf("reforco/saida. Como o campo e' alinhado a' DIREITA e a regiao tem\n");
    std::printf("tamanho FIXO, dessa vez o \"R$\" PODE entrar na regiao -- ele vai\n");
    std::printf("aparecer sozinho quando o valor for pequeno (sobra espaco a'\n");
    std::printf("esquerda) e o programa ja' sabe ignorar isso.\n");
    std::printf(">> Antes de clicar os cantos, compre BASTANTE contrato (o suficiente\n");
    std::printf(">> pra deixar o Resultado em Aberto BEM grande, positivo ou negativo\n");
    std::printf(">> -- ex.: -R$10.394,00) -- assim a regiao ja' e' desenhada no\n");
    std::printf(">> tamanho maximo real que ela vai precisar exibir, em vez de ficar\n");
    std::printf(">> justa demais so' com a posicao pequena de 1 contrato. Deixe uma\n");
    std::printf(">> pequena margem a' esquerda de onde o \"-\" ou o primeiro digito\n");
    std::printf(">> aparecem agora, pra nenhum caractere ficar EXATAMENTE colado na\n");
    std::printf(">> borda (o programa rejeita leitura que encosta na borda, pra nunca\n");
    std::printf(">> arriscar cortar um sinal \"-\").\n");
    aguardarEnter("compre bastante contrato agora e confirme quando o valor estiver bem grande");

    POINT r1 = aguardarClique("canto SUPERIOR ESQUERDO do valor \"Resultado em Aberto\" (pode incluir o \"R$\")");
    if (r1.x < 0 && r1.y < 0) return false;
    POINT r2 = aguardarClique("canto INFERIOR DIREITO desse valor");
    if (r2.x < 0 && r2.y < 0) return false;
    out.regiaoResultado = regiaoDeDoisPontos(r1, r2);

    CapturaRegiao capResultado(out.regiaoResultado);
    out.glifos.clear();

    std::printf("\nAgora vamos calibrar os caracteres (0-9, \"-\", \",\", \".\" e o\n");
    std::printf("simbolo \"R$\" junto) um valor de cada vez. Como a regiao e'\n");
    std::printf("alinhada a' DIREITA e tem tamanho FIXO, quando o valor for PEQUENO\n");
    std::printf("vai sobrar espaco a' esquerda mostrando o label \"R$\" -- e' esperado,\n");
    std::printf("digite \"R$\" junto (ex.: \"R$2,00\", \"R$-8,00\") quando ele aparecer na\n");
    std::printf("foto (o programa trata \"R$\" como 1 caractere so' -- nessa fonte ele\n");
    std::printf("sempre aparece colado, sem espaco entre o R e o $). Quando o valor\n");
    std::printf("for GRANDE o \"R$\" pode nao aparecer -- digite so' o numero nesse\n");
    std::printf("caso. Precisa de pelo menos uma rodada com o valor PEQUENO (pra\n");
    std::printf("calibrar o \"R$\") e uma com o valor GRANDE (mais digitos), alem de\n");
    std::printf("variar o preco/posicao entre rodadas pra cobrir todos os digitos.\n");
    std::printf("O Resultado em Aberto pode mudar varias vezes por segundo, rapido\n");
    std::printf("demais pra digitar olhando a tela ao vivo -- por isso, a cada\n");
    std::printf("rodada o programa captura e salva uma FOTO CONGELADA do campo em\n");
    std::printf("'%s' (nesta pasta): abra esse arquivo num visualizador\n", CAMINHO_IMAGEM_CALIBRACAO);
    std::printf("de imagens e digite exatamente o que esta' nele -- ele nao muda\n");
    std::printf("mais, mesmo que o preco continue mudando na tela real.\n");

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
        std::printf(">> EXATAMENTE o valor que esta' nele (ex.: 2,00 ou -15,50, ou\n");
        std::printf(">> 1.234,56 se passar de mil; digite \"R$\" junto se aparecer na\n");
        std::printf(">> foto -- a VIRGULA e' sempre a dos centavos, o PONTO (quando\n");
        std::printf(">> tiver) e' so' separador de milhar): ");
        std::fflush(stdout);
        std::string digitado;
        std::getline(std::cin, digitado);

        // "R" e "$" ficam colados sem nenhum espaco nessa fonte -- a
        // segmentacao SEMPRE junta os dois num pedaco so' (nunca 2
        // separados). Colapsa "R$" digitado num "R" so' antes de
        // comparar/gravar, senao a contagem nunca bate (o operador ve'
        // "R$" na imagem e digita 2 caracteres pra 1 pedaco real).
        for (size_t pos = digitado.find("R$"); pos != std::string::npos; pos = digitado.find("R$")) {
            digitado.replace(pos, 2, "R");
        }

        bool caractereInvalido = false;
        for (char c : digitado) {
            if (glifosNecessarios().find(c) == std::string::npos) { caractereInvalido = true; break; }
        }
        if (caractereInvalido) {
            std::printf(">> caractere fora do esperado (so' 0-9, \"-\", \",\", \".\" e \"R\"\n"
                        ">> [pro \"R$\" junto] sao validos). Tente de novo com a MESMA\n"
                        ">> foto ainda salva em '%s'.\n", CAMINHO_IMAGEM_CALIBRACAO);
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
