REM Desenvolvido por: Escanearcpl
@echo off
echo ============================================
echo    ARM64 Hypervisor - Quick Start Guide
echo ============================================
echo.

echo Escolha uma opcao:
echo.
echo 1. GUI Hypervisor (Recomendado - Com Graficos)
echo 2. Hypervisor Basico (Console)
echo 3. Setup ARM64 (Para hardware ARM64)
echo 4. Verificar Sistema
echo 5. Sair
echo.

set /p choice="Digite sua escolha (1-5): "

if "%choice%"=="1" goto GUI_HYPERVISOR
if "%choice%"=="2" goto BASIC_HYPERVISOR
if "%choice%"=="3" goto ARM64_SETUP
if "%choice%"=="4" goto VERIFY_SYSTEM
if "%choice%"=="5" goto EXIT
goto INVALID_CHOICE

:GUI_HYPERVISOR
echo.
echo ============================================
echo    Iniciando GUI Hypervisor...
echo ============================================
echo.
echo Este e o hypervisor REAL com graficos!
echo Recursos:
echo   - DirectX 11 graphics virtualization
echo   - Virtual GPU com framebuffer
echo   - Input devices virtualizados
echo   - Carregamento de OS real
echo.
echo Verificando privilegios de Administrator...
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [OK] Executando como Administrator
) else (
    echo [AVISO] Nao esta executando como Administrator
    echo Algumas funcionalidades podem nao funcionar
    echo.
    echo Recomendacao: Clique com botao direito e "Executar como administrador"
    pause
)

echo.
echo Construindo GUI Hypervisor...
powershell -ExecutionPolicy Bypass -File "build_gui_hypervisor.ps1" -Deploy

if exist "deploy_gui\ARM64_GUI_Hypervisor.exe" (
    echo.
    echo [SUCESSO] GUI Hypervisor construido!
    echo.
    echo Iniciando interface grafica...
    cd deploy_gui
    start ARM64_GUI_Hypervisor.exe
    echo.
    echo GUI Hypervisor iniciado!
    echo Verifique a janela do hypervisor que foi aberta.
) else (
    echo.
    echo [ERRO] Falha ao construir GUI Hypervisor
    echo Verifique se tem Visual Studio ou MinGW instalado
    echo.
    echo Para instalar ferramentas:
    echo   - Visual Studio 2022 com C++ tools
    echo   - OU MinGW-w64 GCC
)
pause
goto MENU

:BASIC_HYPERVISOR
echo.
echo ============================================
echo    Iniciando Hypervisor Basico...
echo ============================================
echo.
echo Verificando privilegios de Administrator...
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [OK] Executando como Administrator
) else (
    echo [ERRO] Administrator requerido para hypervisor basico
    echo.
    echo Execute como Administrator:
    echo   Clique com botao direito -> "Executar como administrador"
    pause
    goto MENU
)

echo.
echo Verificando sistema...
powershell -ExecutionPolicy Bypass -File "VERIFY_SYSTEM.ps1"

echo.
echo Construindo hypervisor basico...
if not exist "build" mkdir build
cd build
cmake -S .. -B .
cmake --build . --config Release

if exist "hypervisor.exe" (
    echo.
    echo [SUCESSO] Hypervisor basico construido!
    echo.
    echo Iniciando hypervisor...
    hypervisor.exe
) else (
    echo.
    echo [ERRO] Falha ao construir hypervisor basico
    cd ..
)
pause
goto MENU

:ARM64_SETUP
echo.
echo ============================================
echo    Setup para Hardware ARM64
echo ============================================
echo.
echo Este setup e para dispositivos Windows 11 ARM64 reais.
echo.
echo Passos para usar em hardware ARM64:
echo.
echo 1. Copie a pasta ARM64_Deployment para o dispositivo ARM64
echo 2. Execute como Administrator no dispositivo ARM64:
echo    - VERIFY_SYSTEM.ps1 (verificar sistema)
echo    - ENABLE_HYPERV.ps1 (habilitar Hyper-V)
echo    - SETUP_ARM64.ps1 (setup toolchain)
echo    - DEPLOY_ARM64.bat (build e executar)
echo.
echo Pressione qualquer tecla para abrir a pasta ARM64_Deployment...
pause
explorer ARM64_Deployment
goto MENU

:VERIFY_SYSTEM
echo.
echo ============================================
echo    Verificando Sistema...
echo ============================================
echo.
powershell -ExecutionPolicy Bypass -File "VERIFY_SYSTEM.ps1"
echo.
pause
goto MENU

:INVALID_CHOICE
echo.
echo [ERRO] Opcao invalida! Digite um numero de 1 a 5.
echo.
pause
goto MENU

:EXIT
echo.
echo Saindo...
exit /b 0

:MENU
echo.
echo Voltando ao menu principal...
echo.
goto :eof
