@echo off
setlocal enabledelayedexpansion

echo ===============================================
echo     ARM64 Hypervisor - Deployment Rapido
echo ===============================================
echo.

REM Verificar Administrator
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERRO: Execute como Administrator!
    echo.
    echo Clique direito neste arquivo e selecione:
    echo "Executar como administrador"
    echo.
    pause
    exit /b 1
)

echo [INFO] Executando como Administrator... OK
echo.

echo Selecione uma opcao:
echo.
echo 1. Verificar sistema ARM64
echo 2. Habilitar Hyper-V/WHP
echo 3. Build automatico
echo 4. Verificar + Habilitar + Build (completo)
echo 5. Sair
echo.

set /p choice="Digite sua escolha (1-5): "

if "%choice%"=="1" (
    echo.
    echo [INFO] Verificando sistema...
    powershell -ExecutionPolicy Bypass -File "VERIFY_SYSTEM.ps1"
    goto end
)

if "%choice%"=="2" (
    echo.
    echo [INFO] Habilitando Hyper-V e WHP...
    powershell -ExecutionPolicy Bypass -File "ENABLE_HYPERV.ps1"
    goto end
)

if "%choice%"=="3" (
    echo.
    echo [INFO] Iniciando build automatico...
    powershell -ExecutionPolicy Bypass -File "SETUP_ARM64.ps1"
    goto end
)

if "%choice%"=="4" (
    echo.
    echo [INFO] Executando processo completo...
    echo.
    echo === PASSO 1: Verificacao ===
    powershell -ExecutionPolicy Bypass -File "VERIFY_SYSTEM.ps1" -Detailed
    echo.
    echo === PASSO 2: Habilitando recursos ===
    powershell -ExecutionPolicy Bypass -File "ENABLE_HYPERV.ps1" -Quiet
    echo.
    echo === PASSO 3: Build ===
    powershell -ExecutionPolicy Bypass -File "SETUP_ARM64.ps1" -RunAfterBuild
    goto end
)

if "%choice%"=="5" (
    echo Saindo...
    exit /b 0
)

echo Opcao invalida! Tente novamente.
pause
goto start

:end
echo.
echo ===============================================
echo           PROCESSO CONCLUIDO
echo ===============================================
pause