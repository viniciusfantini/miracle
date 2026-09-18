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

const char* CAMINHO_IMAGEM_CALIBRACAO = "miracle_calibracao_valor.bmp";

// salva a captura como .bmp -- so' pra o operador conferir visualmente
// que a regiao clicada enquadra bem o valor (sem cortar nada), antes de
// confiar no OCR pra ler ela de verdade.
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
    std::printf("reforco/saida. Lido via OCR (reconhecimento de texto nativo do\n");
    std::printf("Windows) -- so' precisa apontar a regiao, sem treinar caractere\n");
    std::printf("nenhum. Pode incluir o \"R$\" na regiao sem problema, o OCR ignora.\n");

    while (true) {
        POINT r1 = aguardarClique("canto SUPERIOR ESQUERDO do valor \"Resultado em Aberto\"");
        if (r1.x < 0 && r1.y < 0) return false;
        POINT r2 = aguardarClique("canto INFERIOR DIREITO desse valor");
        if (r2.x < 0 && r2.y < 0) return false;
        out.regiaoResultado = regiaoDeDoisPontos(r1, r2);

        CapturaRegiao capResultado(out.regiaoResultado);
        if (!capResultado.capturar()) { std::printf(">> falha ao capturar a tela.\n"); return false; }

        salvarBmp(capResultado, CAMINHO_IMAGEM_CALIBRACAO);
        auto valor = lerResultadoEmCentavos(capResultado);

        std::printf("\n>> Salvei '%s' -- confira se a regiao enquadra bem o valor\n", CAMINHO_IMAGEM_CALIBRACAO);
        std::printf(">> (sem cortar nada, com uma pequena folga nas bordas).\n");
        if (valor) {
            std::printf(">> OCR leu: %s\n", formatarCentavos(*valor).c_str());
        } else {
            std::printf(">> OCR nao conseguiu reconhecer nada nessa captura.\n");
        }

        if (perguntarSimNao("A leitura acima bate com o que esta' na tela e a regiao ficou bem enquadrada")) break;
        std::printf(">> vamos clicar os cantos de novo.\n");
    }

    std::printf("\n== Calibracao concluida ==\n");
    std::printf("regiao flat: x=%d y=%d %dx%d, toleranciaFlat=%lld\n",
                out.regiaoFlat.x, out.regiaoFlat.y, out.regiaoFlat.largura, out.regiaoFlat.altura, out.toleranciaFlat);
    std::printf("regiao resultado: x=%d y=%d %dx%d\n",
                out.regiaoResultado.x, out.regiaoResultado.y, out.regiaoResultado.largura, out.regiaoResultado.altura);

    return true;
}
