# VERIFY_SYSTEM.ps1 - Verificacao de Sistema ARM64

Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "                Verificacao do Sistema ARM64" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

# Verificar Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
if (-not $isAdmin) {
    Write-Host "ERRO: Execute como Administrator!" -ForegroundColor Red
    Read-Host "Pressione Enter para sair"
    exit 1
}

Write-Host "Executando como Administrator... OK" -ForegroundColor Green
Write-Host ""

# Verificar sistema
$osInfo = Get-ComputerInfo
$architecture = $env:PROCESSOR_ARCHITECTURE
$windowsVersion = $osInfo.WindowsVersion

Write-Host "Verificando hardware e sistema..." -ForegroundColor Yellow
Write-Host "Sistema: $($osInfo.WindowsProductName)" -ForegroundColor White
Write-Host "Versao: $windowsVersion" -ForegroundColor White
Write-Host "Arquitetura: $architecture" -ForegroundColor White
Write-Host ""

if ($architecture -eq "ARM64") {
    Write-Host "Sistema ARM64: OK" -ForegroundColor Green
} else {
    Write-Host "Sistema nao e ARM64: $architecture" -ForegroundColor Yellow
    Write-Host "Este sistema pode executar demo, mas nao hypervisor nativo ARM64" -ForegroundColor Yellow
}

# Verificar memoria
$totalRAM = [math]::Round($osInfo.TotalPhysicalMemory / 1GB, 1)
Write-Host "Memoria RAM: ${totalRAM}GB" -ForegroundColor White

if ($totalRAM -ge 16) {
    Write-Host "Memoria RAM: OK" -ForegroundColor Green
} elseif ($totalRAM -ge 8) {
    Write-Host "Memoria RAM: Suficiente (16GB recomendado)" -ForegroundColor Yellow
} else {
    Write-Host "Memoria RAM: Insuficiente (minimo 8GB)" -ForegroundColor Red
}

Write-Host ""

# Verificar recursos de virtualizacao
Write-Host "Verificando recursos de virtualizacao..." -ForegroundColor Yellow

try {
    $hyperVFeature = Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All -ErrorAction SilentlyContinue
    $whpFeature = Get-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform -ErrorAction SilentlyContinue
    
    if ($hyperVFeature.State -eq "Enabled") {
        Write-Host "Hyper-V: Habilitado" -ForegroundColor Green
    } else {
        Write-Host "Hyper-V: Desabilitado" -ForegroundColor Red
        Write-Host "Execute ENABLE_HYPERV.ps1 para habilitar" -ForegroundColor Yellow
    }
    
    if ($whpFeature.State -eq "Enabled") {
        Write-Host "Windows Hypervisor Platform: Habilitado" -ForegroundColor Green
    } else {
        Write-Host "Windows Hypervisor Platform: Desabilitado" -ForegroundColor Red
        Write-Host "Execute ENABLE_HYPERV.ps1 para habilitar" -ForegroundColor Yellow
    }
} catch {
    Write-Host "Nao foi possivel verificar recursos de virtualizacao" -ForegroundColor Yellow
}

Write-Host ""

# Verificar Visual Studio 2022
Write-Host "Verificando ferramentas de desenvolvimento..." -ForegroundColor Yellow

$vsPaths = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community"
)

$vsFound = $false
foreach ($path in $vsPaths) {
    if (Test-Path "$path\Common7\IDE\devenv.exe") {
        $vsFound = $true
        $edition = Split-Path $path -Leaf
        Write-Host "Visual Studio 2022: $edition encontrado" -ForegroundColor Green
        break
    }
}

if (-not $vsFound) {
    Write-Host "Visual Studio 2022: Nao encontrado" -ForegroundColor Red
    Write-Host "Instale Visual Studio 2022 com workload C++" -ForegroundColor Yellow
}

# Verificar CMake
try {
    $cmakeVersion = & cmake --version 2>$null
    if ($LASTEXITCODE -eq 0) {
        $version = ($cmakeVersion | Select-Object -First 1) -replace "cmake version ", ""
        Write-Host "CMake: $version" -ForegroundColor Green
    }
} catch {
    Write-Host "CMake: Nao encontrado" -ForegroundColor Red
    Write-Host "Instale CMake via Visual Studio Installer" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "                    Verificacao Concluida" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

if ($architecture -eq "ARM64" -and $vsFound) {
    Write-Host "Sistema pronto para desenvolvimento ARM64!" -ForegroundColor Green
    Write-Host "Proximo passo: Execute SETUP_ARM64.ps1" -ForegroundColor Cyan
} else {
    Write-Host "Sistema requer ajustes para desenvolvimento ARM64" -ForegroundColor Yellow
    Write-Host "Consulte o guia DEPLOY_GUIDE.md para requisitos completos" -ForegroundColor Cyan
}

Write-Host ""
Read-Host "Pressione Enter para continuar"