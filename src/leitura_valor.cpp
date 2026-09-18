#include "leitura_valor.h"
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.Streams.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Media::Ocr;
using namespace winrt::Windows::Security::Cryptography;
using namespace winrt::Windows::Storage::Streams;

namespace {

OcrEngine g_engine{ nullptr };

// OCR de fonte pequena de UI fica bem mais confiavel com o texto
// ampliado antes de mandar pro motor. Testado ao vivo (18/09/2026):
// com ampliacao por "vizinho mais proximo" (blocuda, sem suavizacao) o
// OCR chegou a PERDER um digito inteiro (leu "-16 0" em vez de
// "-16,00"). Trocado pra interpolacao BILINEAR (bordas suaves, mais
// parecido com o texto anti-aliased que o OCR foi treinado pra ler).
// Tentativa de 16x + margem de 24px resolveu a duvida de exatidao mas
// deixou o RecognizeAsync bem mais lento (imagem ~3x maior em pixels) --
// inaceitavel pra esse projeto (delay = dinheiro, ver CLAUDE.md).
// Recuado pra um meio-termo mais leve: 8x + margem pequena.
constexpr int FATOR_AMPLIACAO = 8;

// margem de fundo (na imagem JA ampliada) ao redor do texto antes de
// mandar pro OCR -- texto colado na borda do recorte tende a confundir
// o motor (efeito de bordas conhecido em pre-processamento de OCR).
// Pequena de proposito pra nao inflar o tamanho da imagem (custa
// latencia no RecognizeAsync).
constexpr int PADDING_PX = 8;

BYTE amostra(const std::vector<BYTE>& origem, int largura, int altura, int x, int y, int canal) {
    x = std::clamp(x, 0, largura - 1);
    y = std::clamp(y, 0, altura - 1);
    return origem[((size_t)y * largura + x) * 4 + canal];
}

std::vector<BYTE> ampliar(const std::vector<BYTE>& origem, int largura, int altura, int fator,
                           int& larguraOut, int& alturaOut) {
    larguraOut = largura * fator;
    alturaOut = altura * fator;
    std::vector<BYTE> dst((size_t)larguraOut * alturaOut * 4);

    for (int y = 0; y < alturaOut; ++y) {
        double srcYf = (y + 0.5) / fator - 0.5;
        int y0 = (int)std::floor(srcYf);
        double fy = srcYf - y0;

        for (int x = 0; x < larguraOut; ++x) {
            double srcXf = (x + 0.5) / fator - 0.5;
            int x0 = (int)std::floor(srcXf);
            double fx = srcXf - x0;

            BYTE* q = dst.data() + ((size_t)y * larguraOut + x) * 4;
            for (int canal = 0; canal < 3; ++canal) {
                double v00 = amostra(origem, largura, altura, x0, y0, canal);
                double v01 = amostra(origem, largura, altura, x0 + 1, y0, canal);
                double v10 = amostra(origem, largura, altura, x0, y0 + 1, canal);
                double v11 = amostra(origem, largura, altura, x0 + 1, y0 + 1, canal);
                double topo = v00 + (v01 - v00) * fx;
                double base = v10 + (v11 - v10) * fx;
                double valor = topo + (base - topo) * fy;
                q[canal] = (BYTE)std::clamp(valor, 0.0, 255.0);
            }
            q[3] = 255;
        }
    }
    return dst;
}

// adiciona 'padding' pixels de fundo (cor amostrada do canto da propria
// imagem) em volta da imagem inteira.
std::vector<BYTE> comPadding(const std::vector<BYTE>& origem, int largura, int altura, int padding,
                              int& larguraOut, int& alturaOut) {
    larguraOut = largura + padding * 2;
    alturaOut = altura + padding * 2;
    BYTE fundoB = origem[0], fundoG = origem[1], fundoR = origem[2];

    std::vector<BYTE> dst((size_t)larguraOut * alturaOut * 4);
    for (size_t i = 0; i < (size_t)larguraOut * alturaOut; ++i) {
        dst[i * 4 + 0] = fundoB;
        dst[i * 4 + 1] = fundoG;
        dst[i * 4 + 2] = fundoR;
        dst[i * 4 + 3] = 255;
    }
    for (int y = 0; y < altura; ++y) {
        const BYTE* linhaSrc = origem.data() + (size_t)y * largura * 4;
        BYTE* linhaDst = dst.data() + ((size_t)(y + padding) * larguraOut + padding) * 4;
        std::memcpy(linhaDst, linhaSrc, (size_t)largura * 4);
    }
    return dst;
}

} // namespace

bool iniciarOcr() {
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
    } catch (...) {
        // ja' inicializada em modo compativel por outra parte do processo -- segue.
    }

    try {
        g_engine = OcrEngine::TryCreateFromUserProfileLanguages();
        if (!g_engine) {
            g_engine = OcrEngine::TryCreateFromLanguage(winrt::Windows::Globalization::Language(L"en"));
        }
    } catch (...) {
        g_engine = nullptr;
    }

    if (!g_engine) {
        std::printf(">> Falha ao iniciar o OCR do Windows -- verifique se o pacote de\n"
                    ">> reconhecimento de texto esta' instalado (Configuracoes > Hora e\n"
                    ">> idioma > Idioma e regiao > clique no seu idioma > opcoes >\n"
                    ">> adicionar 'Reconhecimento otico de caracteres').\n");
        return false;
    }
    return true;
}

std::optional<long long> lerResultadoEmCentavos(const CapturaRegiao& cap, std::string* textoBrutoOut) {
    if (!g_engine) return std::nullopt;

    const RegiaoTela& r = cap.regiao();
    const std::vector<BYTE>& buf = cap.pixelsBrutos();
    if (r.largura <= 0 || r.altura <= 0 || buf.size() < (size_t)r.largura * r.altura * 4) return std::nullopt;

    int largAmpliada = 0, altAmpliada = 0;
    std::vector<BYTE> ampliada = ampliar(buf, r.largura, r.altura, FATOR_AMPLIACAO, largAmpliada, altAmpliada);

    int largFinal = 0, altFinal = 0;
    std::vector<BYTE> comBorda = comPadding(ampliada, largAmpliada, altAmpliada, PADDING_PX, largFinal, altFinal);

    std::string reconhecido;
    try {
        IBuffer buffer = CryptographicBuffer::CreateFromByteArray(
            winrt::array_view<uint8_t const>(comBorda.data(), comBorda.data() + comBorda.size()));

        SoftwareBitmap bitmap = SoftwareBitmap::CreateCopyFromBuffer(
            buffer, BitmapPixelFormat::Bgra8, largFinal, altFinal, BitmapAlphaMode::Ignore);

        OcrResult resultado = g_engine.RecognizeAsync(bitmap).get();
        std::wstring texto{ resultado.Text().c_str() };

        std::string bruto;
        bruto.reserve(texto.size());
        for (wchar_t wc : texto) bruto.push_back(wc < 128 ? (char)wc : '?'); // ascii-lossy, so' diagnostico/parse

        if (textoBrutoOut) *textoBrutoOut = bruto;
        reconhecido = bruto;
    } catch (...) {
        if (textoBrutoOut) textoBrutoOut->assign("<excecao no OCR>");
        return std::nullopt;
    }

    // Testado ao vivo (18/09/2026): o OCR confunde caracteres parecidos
    // nessa fonte -- "$" vira "S" (ex. "R$" -> "RS"), "0" vira "O", "2"
    // vira "Z", e a virgula as vezes vira "r" ou simplesmente some (ex.
    // "R$-2,00" lido como "RS -ZOO", "-4,00" como "-4r00"). Em NENHUM
    // caso observado um digito de verdade sumiu -- so' trocou de forma.
    // Em vez de depender da virgula (que e' o caractere menos confiavel
    // de todos aqui), tira o prefixo "R$"/"RS" (so' pode aparecer no
    // INICIO, campo alinhado a' direita), corrige as confusoes de
    // digito conhecidas, descarta tudo que nao for digito/sinal, e usa
    // a convencao de que o Resultado em Aberto SEMPRE tem exatamente 2
    // casas decimais -- os 2 ultimos digitos reconhecidos sao sempre os
    // centavos, nao importa se a virgula apareceu ou nao.
    size_t inicio = reconhecido.find_first_not_of(' ');
    if (inicio == std::string::npos) return std::nullopt;
    reconhecido = reconhecido.substr(inicio);

    auto comecaComRS = [](const std::string& s) {
        if (s.size() < 2) return false;
        char a = std::toupper((unsigned char)s[0]);
        char b = std::toupper((unsigned char)s[1]);
        return a == 'R' && (b == 'S' || b == '$');
    };
    if (comecaComRS(reconhecido)) reconhecido = reconhecido.substr(2);

    bool negativo = false;
    size_t idx = reconhecido.find_first_not_of(' ');
    if (idx != std::string::npos && reconhecido[idx] == '-') { negativo = true; idx++; }

    std::string digitos;
    for (size_t i = idx; i < reconhecido.size(); ++i) {
        char c = reconhecido[i];
        if (c == 'O' || c == 'o') c = '0';
        else if (c == 'Z' || c == 'z') c = '2';
        else if (c == 'I' || c == 'l') c = '1';
        else if (c == 'S' || c == 's') c = '5'; // seguro aqui -- o "S" do "R$" ja' foi removido acima
        if (c >= '0' && c <= '9') digitos.push_back(c);
        // qualquer outra coisa (virgula, ponto, "r", espaco...) e' so' separador/ruido -- ignora
    }

    if (digitos.size() < 3) return std::nullopt; // precisa de pelo menos "0,00" (3 digitos)

    std::string parteCentavos = digitos.substr(digitos.size() - 2);
    std::string parteInteira = digitos.substr(0, digitos.size() - 2);

    long long valorInteiro = std::atoll(parteInteira.c_str());
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
