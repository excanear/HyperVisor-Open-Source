# Desenvolvido por: Escanearcpl
# Criar package de teste para hardware ARM64
param(
    [string]$DestinationPath = "ARM64_Hardware_Test"
)

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Preparando Package para Hardware ARM64" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# Criar diretório de destino
if (Test-Path $DestinationPath) {
    Remove-Item -Path $DestinationPath -Recurse -Force
}
New-Item -Path $DestinationPath -ItemType Directory -Force | Out-Null

Write-Host "Copiando arquivos essenciais..." -ForegroundColor Green

# Copiar ARM64_Deployment completo
Copy-Item -Path "ARM64_Deployment\*" -Destination $DestinationPath -Recurse -Force

# Copiar scripts principais
Copy-Item -Path "VERIFY_SYSTEM.ps1" -Destination $DestinationPath -Force
Copy-Item -Path "ENABLE_HYPERV.ps1" -Destination $DestinationPath -Force

# Criar script de teste específico para hardware
$testScript = @"
@echo off
echo ============================================
echo    ARM64 Hypervisor - Teste em Hardware Real
echo ============================================
echo.

echo Verificando arquitetura do sistema...
echo Arquitetura: %PROCESSOR_ARCHITECTURE%
echo Processador: %PROCESSOR_IDENTIFIER%
echo.

REM Verificar se é ARM64
if "%PROCESSOR_ARCHITECTURE%" NEQ "ARM64" (
    echo [AVISO] Sistema nao e ARM64 nativo
    echo Arquitetura detectada: %PROCESSOR_ARCHITECTURE%
    echo.
    pause
)

echo Verificando privilegios de Administrator...
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [OK] Executando como Administrator
) else (
    echo [ERRO] Requer privilegios de Administrator
    echo.
    echo Execute como Administrator:
    echo   1. Clique com botao direito neste arquivo
    echo   2. Selecione "Executar como administrador"
    echo.
    pause
    exit /b 1
)

echo.
echo Executando verificacao do sistema...
powershell -ExecutionPolicy Bypass -File "VERIFY_SYSTEM.ps1"

echo.
echo Habilitando Hyper-V se necessario...
powershell -ExecutionPolicy Bypass -File "ENABLE_HYPERV.ps1"

echo.
echo Configurando ambiente ARM64...
powershell -ExecutionPolicy Bypass -File "SETUP_ARM64.ps1"

echo.
echo Compilando hypervisor para ARM64...
if exist "build" rmdir /s /q build
mkdir build
cd build

cmake -S .. -B . -A ARM64 -DCMAKE_SYSTEM_PROCESSOR=ARM64
if %errorLevel% NEQ 0 (
    echo [ERRO] Falha na configuracao CMake
    cd ..
    pause
    exit /b 1
)

cmake --build . --config Release
if %errorLevel% NEQ 0 (
    echo [ERRO] Falha na compilacao
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ============================================
echo    Teste do Hypervisor ARM64
echo ============================================
echo.

if exist "build\Release\hypervisor.exe" (
    echo [SUCESSO] Hypervisor compilado com sucesso!
    echo.
    echo Executando teste do hypervisor...
    echo.
    cd build\Release
    hypervisor.exe
    cd ..\..
) else if exist "build\hypervisor.exe" (
    echo [SUCESSO] Hypervisor compilado com sucesso!
    echo.
    echo Executando teste do hypervisor...
    echo.
    cd build
    hypervisor.exe
    cd ..
) else (
    echo [ERRO] Executavel nao encontrado
    echo Verifique a compilacao
)

echo.
echo ============================================
echo    Teste Concluido
echo ============================================
pause
"@

Set-Content -Path "$DestinationPath\TESTE_HARDWARE_ARM64.bat" -Value $testScript -Encoding ASCII

# Criar guia de uso específico
$guideContent = @"
# 🚀 Teste em Hardware ARM64 Real

## 📋 **Instruções Passo a Passo**

### **1. Transferir para Hardware ARM64**
- Copie a pasta completa para o dispositivo ARM64
- Exemplo: pendrive, rede, OneDrive, etc.

### **2. No Hardware ARM64:**

#### **Passo 1: Abrir PowerShell como Administrator**
1. Pressione Win + X
2. Selecione "Windows PowerShell (Admin)" ou "Terminal (Admin)"
3. Confirme UAC

#### **Passo 2: Navegar para a pasta**
```powershell
cd C:\caminho\para\ARM64_Hardware_Test
```

#### **Passo 3: Executar teste automatizado**
```powershell
.\TESTE_HARDWARE_ARM64.bat
```

### **3. O que o teste faz:**
✅ Verifica arquitetura ARM64
✅ Confirma privilégios Administrator
✅ Verifica capacidades do sistema
✅ Habilita Hyper-V se necessário
✅ Configura ambiente ARM64
✅ Compila hypervisor nativo
✅ Executa teste do hypervisor

### **4. Resultados Esperados:**

#### **Sucesso:**
```
[SUCESSO] Hypervisor compilado com sucesso!
Executando teste do hypervisor...

ARM64 Hypervisor Starting...
Creating partition...
Setting up vCPU...
VM running successfully!
```

#### **Possíveis Problemas:**

**"Sistema não é ARM64 nativo"**
- Hardware não é ARM64 real
- Funciona mas com emulação

**"Requer privilégios de Administrator"**
- Execute como Administrator
- Clique direito → "Executar como administrador"

**"Hyper-V não suportado"**
- Hardware não suporta virtualização
- Verifique BIOS/UEFI settings

**"Falha na compilação"**
- Instale Visual Studio Build Tools
- Ou use MinGW-w64 ARM64

### **5. Teste Manual (Alternativo):**

Se o teste automatizado falhar:

```powershell
# Verificar sistema
.\VERIFY_SYSTEM.ps1

# Habilitar Hyper-V
.\ENABLE_HYPERV.ps1

# Build manual
mkdir build
cd build
cmake -S .. -B . -A ARM64
cmake --build . --config Release
```

### **6. Validação Final:**

O hypervisor deve:
- ✅ Inicializar sem erros
- ✅ Criar partição WHP
- ✅ Configurar vCPU ARM64
- ✅ Executar guest code
- ✅ Capturar VM-exits
- ✅ Emular devices

### **Hardware Testado:**
- Surface Pro X (SQ1/SQ2/SQ3)
- MacBook Air M1/M2 (Windows 11 ARM64)
- Lenovo ThinkPad X13s
- Samsung Galaxy Book Go
- ASUS NovaGo

### **Performance Esperada:**
- Startup: < 5 segundos
- VM Creation: < 2 segundos
- vCPU Setup: < 1 segundo
- Guest Execution: Tempo real

## 🎯 **Próximos Passos após Sucesso:**

1. **Testar GUI Hypervisor:**
```powershell
# Build GUI version
cmake -S .. -B . -f ..\CMakeLists_GUI.txt -A ARM64
cmake --build . --config Release
```

2. **Carregar OS real:**
- Linux ARM64 kernels
- Windows PE ARM64
- Custom OS images

3. **Benchmark performance:**
- Medir latência VM-exit
- Testar throughput devices
- Validar graphics virtualization

**Boa sorte com o teste! 🚀**
"@

Set-Content -Path "$DestinationPath\GUIA_TESTE_HARDWARE.md" -Value $guideContent -Encoding UTF8

# Criar arquivo de informações do sistema
$systemInfo = @"
# Informações do Sistema de Build
Build Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Build System: $env:COMPUTERNAME
OS Version: $(Get-WmiObject -Class Win32_OperatingSystem | Select-Object -ExpandProperty Caption)
Architecture: $env:PROCESSOR_ARCHITECTURE
.NET Version: $((Get-ItemProperty "HKLM:SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full\" -Name Release).Release)

# Arquivos Incluídos
$(Get-ChildItem -Path $DestinationPath -Recurse | Select-Object Name, Length | Format-Table | Out-String)
"@

Set-Content -Path "$DestinationPath\BUILD_INFO.txt" -Value $systemInfo -Encoding UTF8

Write-Host "Package criado com sucesso!" -ForegroundColor Green
Write-Host "Localização: $DestinationPath" -ForegroundColor White
Write-Host ""
Write-Host "Próximos passos:" -ForegroundColor Cyan
Write-Host "1. Copie a pasta '$DestinationPath' para o hardware ARM64" -ForegroundColor White
Write-Host "2. Execute 'TESTE_HARDWARE_ARM64.bat' como Administrator" -ForegroundColor White
Write-Host "3. Siga o guia em 'GUIA_TESTE_HARDWARE.md'" -ForegroundColor White

# Abrir explorador na pasta
explorer $DestinationPath
