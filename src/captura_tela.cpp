#include "captura_tela.h"
#include <cstring>
#include <climits>
#include <cstdlib>

CapturaRegiao::CapturaRegiao(RegiaoTela regiao) : regiao_(regiao) {
    size_t bytes = (size_t)regiao_.largura * regiao_.altura * 4;
    atual_.resize(bytes, 0);
    anterior_.resize(bytes, 0);
}

bool CapturaRegiao::capturar() {
    HDC hdcTela = GetDC(nullptr);
    if (!hdcTela) return false;

    HDC hdcMem = CreateCompatibleDC(hdcTela);
    HBITMAP hbmp = CreateCompatibleBitmap(hdcTela, regiao_.largura, regiao_.altura);
    HBITMAP hbmpAntigo = (HBITMAP)SelectObject(hdcMem, hbmp);

    BOOL ok = BitBlt(hdcMem, 0, 0, regiao_.largura, regiao_.altura,
                      hdcTela, regiao_.x, regiao_.y, SRCCOPY | CAPTUREBLT);

    if (ok) {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = regiao_.largura;
        bmi.bmiHeader.biHeight = -regiao_.altura; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        std::swap(atual_, anterior_);
        ok = GetDIBits(hdcMem, hbmp, 0, regiao_.altura, atual_.data(), &bmi, DIB_RGB_COLORS) != 0;
    }

    SelectObject(hdcMem, hbmpAntigo);
    DeleteObject(hbmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcTela);

    if (!ok) return false;

    mudou_ = (std::memcmp(atual_.data(), anterior_.data(), atual_.size()) != 0);
    return true;
}

void CapturaRegiao::corMedia(BYTE& b, BYTE& g, BYTE& r) const {
    uint64_t sb = 0, sg = 0, sr = 0;
    size_t n = atual_.size() / 4;
    for (size_t i = 0; i < n; ++i) {
        sb += atual_[i * 4 + 0];
        sg += atual_[i * 4 + 1];
        sr += atual_[i * 4 + 2];
    }
    if (n == 0) { b = g = r = 0; return; }
    b = (BYTE)(sb / n);
    g = (BYTE)(sg / n);
    r = (BYTE)(sr / n);
}

long long CapturaRegiao::diferencaPara(const std::vector<BYTE>& referencia) const {
    if (referencia.size() != atual_.size()) return LLONG_MAX;
    long long soma = 0;
    for (size_t i = 0; i < atual_.size(); ++i) {
        soma += std::abs((int)atual_[i] - (int)referencia[i]);
    }
    return soma;
}
