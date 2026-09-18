@echo off
rem build.bat -- compila Miracle.exe com o MSVC Build Tools instalado.
rem Uso: build.bat  (gera Miracle.exe nesta pasta)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /nologo /EHsc /std:c++17 /O2 /DNOMINMAX /Fe:"%~dp0Miracle.exe" ^
   "%~dp0src\main.cpp" ^
   "%~dp0src\atalho.cpp" ^
   "%~dp0src\janela_alvo.cpp" ^
   "%~dp0src\captura_tela.cpp" ^
   "%~dp0src\config.cpp" ^
   "%~dp0src\calibracao.cpp" ^
   "%~dp0src\entrada.cpp" ^
   "%~dp0src\leitura_valor.cpp" ^
   user32.lib gdi32.lib ole32.lib oleaut32.lib uuid.lib /link /MANIFEST:EMBED
del "%~dp0*.obj" >nul 2>&1
echo.
echo build ok:
echo   calibrar: Miracle.exe calibrar
echo   debug:    Miracle.exe debug   (le' e mostra o que faria, nao manda nada)
echo   rodar:    Miracle.exe rodar   (manda ordem de verdade -- so' na conta SIMULADORA)
