#include "leitura_valor.h"
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.Streams.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Media::Ocr;
using namespace winrt::Windows::Security::Cryptography;
using namespace winrt::Windows::Storage::Streams;

namespace {

OcrEngine g_engine{ nullptr };

// OCR de fonte pequena de UI fica bem mais confiavel com o texto
// ampliado antes de mandar pro motor -- 6x deixa uma letra de ~12px de
// altura virar ~72px, bem dentro da faixa confortavel do OcrEngine.
constexpr int FATOR_AMPLIACAO = 6;

std::vector<BYTE> ampliar(const std::vector<BYTE>& origem, int largura, int altura, int fator,
                           int& larguraOut, int& alturaOut) {
    larguraOut = largura * fator;
    alturaOut = altura * fator;
    std::vector<BYTE> dst((size_t)larguraOut * alturaOut * 4);
    for (int y = 0; y < alturaOut; ++y) {
        int ySrc = y / fator;
        const BYTE* linhaSrc = origem.data() + (size_t)ySrc * largura * 4;
        BYTE* linhaDst = dst.data() + (size_t)y * larguraOut * 4;
        for (int x = 0; x < larguraOut; ++x) {
            int xSrc = x / fator;
            const BYTE* p = linhaSrc + (size_t)xSrc * 4;
            BYTE* q = linhaDst + (size_t)x * 4;
            q[0] = p[0]; q[1] = p[1]; q[2] = p[2]; q[3] = 255;
        }
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

std::optional<long long> lerResultadoEmCentavos(const CapturaRegiao& cap) {
    if (!g_engine) return std::nullopt;

    const RegiaoTela& r = cap.regiao();
    const std::vector<BYTE>& buf = cap.pixelsBrutos();
    if (r.largura <= 0 || r.altura <= 0 || buf.size() < (size_t)r.largura * r.altura * 4) return std::nullopt;

    int largAmpliada = 0, altAmpliada = 0;
    std::vector<BYTE> ampliada = ampliar(buf, r.largura, r.altura, FATOR_AMPLIACAO, largAmpliada, altAmpliada);

    std::string reconhecido;
    try {
        IBuffer buffer = CryptographicBuffer::CreateFromByteArray(
            winrt::array_view<uint8_t const>(ampliada.data(), ampliada.data() + ampliada.size()));

        SoftwareBitmap bitmap = SoftwareBitmap::CreateCopyFromBuffer(
            buffer, BitmapPixelFormat::Bgra8, largAmpliada, altAmpliada, BitmapAlphaMode::Ignore);

        OcrResult resultado = g_engine.RecognizeAsync(bitmap).get();
        std::wstring texto{ resultado.Text().c_str() };

        // filtra so' o que interessa pro valor -- descarta "R$", espacos e
        // qualquer artefato de reconhecimento (letras soltas etc.), igual
        // a' filosofia de "nunca adivinha" ja' usada em todo o projeto.
        for (wchar_t wc : texto) {
            if ((wc >= L'0' && wc <= L'9') || wc == L'-' || wc == L',' || wc == L'.') {
                reconhecido.push_back((char)wc);
            }
        }
    } catch (...) {
        return std::nullopt;
    }

    if (reconhecido.empty()) return std::nullopt;

    // exige exatamente 1 virgula (a dos centavos) -- se o OCR reconheceu
    // 0 ou 2+ virgulas, a leitura esta' ambigua/errada, descarta em vez
    // de adivinhar qual e' a certa.
    if (std::count(reconhecido.begin(), reconhecido.end(), ',') != 1) return std::nullopt;

    bool negativo = false;
    size_t idx = 0;
    if (reconhecido[0] == '-') { negativo = true; idx = 1; }

    size_t posVirgula = reconhecido.find(',', idx);
    std::string parteInteira = reconhecido.substr(idx, posVirgula - idx);
    std::string parteCentavos = reconhecido.substr(posVirgula + 1);
    if (parteInteira.empty() || parteCentavos.size() != 2) return std::nullopt;
    for (char c : parteCentavos) if (c < '0' || c > '9') return std::nullopt;

    // "." e' o separador de milhar do formato BR (ex. "1.000") -- so'
    // marca agrupamento, nao entra no valor.
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
