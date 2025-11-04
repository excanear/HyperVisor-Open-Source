# Desenvolvido por: Escanearcpl
# SETUP_ARM64.ps1 - Build Automatico ARM64

Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "                ARM64 Hypervisor - Build Automatico" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

# Verificar Administrator
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "ERRO: Execute como Administrator!" -ForegroundColor Red
    Read-Host "Pressione Enter para sair"
    exit 1
}

Write-Host "Executando como Administrator... OK" -ForegroundColor Green
Write-Host ""

# Verificar sistema
$osInfo = Get-ComputerInfo
$architecture = $env:PROCESSOR_ARCHITECTURE

Write-Host "Sistema: $($osInfo.WindowsProductName)" -ForegroundColor White
Write-Host "Arquitetura: $architecture" -ForegroundColor White
Write-Host ""

if ($architecture -ne "ARM64") {
    Write-Host "AVISO: Sistema nao e ARM64 ($architecture)" -ForegroundColor Yellow
    Write-Host "Build pode nao funcionar corretamente em hardware nao-ARM64" -ForegroundColor Yellow
    Write-Host ""
    
    # Tentar demo local
    Write-Host "Tentando compilar demo local..." -ForegroundColor Cyan
    $parentDir = Split-Path (Get-Location) -Parent
    $demoPath = Join-Path $parentDir "src\demo_main.c"
    
    if (Test-Path $demoPath) {
        try {
            Write-Host "Compilando demo com gcc..." -ForegroundColor Yellow
            $result = & gcc $demoPath -o "hypervisor_demo.exe" -lkernel32 -luser32 -ladvapi32 2>&1
            if ($LASTEXITCODE -eq 0 -and (Test-Path "hypervisor_demo.exe")) {
                Write-Host "Demo compilada com sucesso!" -ForegroundColor Green
                Write-Host "Executando demo..." -ForegroundColor Cyan
                & ".\hypervisor_demo.exe"
            } else {
                Write-Host "Erro na compilacao: $result" -ForegroundColor Red
            }
        } catch {
            Write-Host "gcc nao encontrado. Instale MinGW ou use Visual Studio." -ForegroundColor Red
        }
    } else {
        Write-Host "Arquivo demo_main.c nao encontrado" -ForegroundColor Red
    }
    
    Read-Host "Pressione Enter para continuar"
    return
}

# Sistema ARM64 - build completo
Write-Host "Sistema ARM64 detectado! Iniciando build completo..." -ForegroundColor Green
Write-Host ""

# Procurar Visual Studio 2022
Write-Host "Procurando Visual Studio 2022..." -ForegroundColor Yellow

$vsPaths = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community"
)

$vsPath = $null
foreach ($path in $vsPaths) {
    if (Test-Path "$path\Common7\IDE\devenv.exe") {
        $vsPath = $path
        $edition = Split-Path $path -Leaf
        Write-Host "Visual Studio 2022 $edition encontrado!" -ForegroundColor Green
        break
    }
}

if (-not $vsPath) {
    Write-Host "Visual Studio 2022 nao encontrado!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Instale Visual Studio 2022 com:" -ForegroundColor Yellow
    Write-Host "- Desktop development with C++" -ForegroundColor White
    Write-Host "- MSVC v143 - VS 2022 C++ ARM64 build tools" -ForegroundColor White
    Write-Host "- Windows 11 SDK" -ForegroundColor White
    Read-Host "Pressione Enter para sair"
    return
}

# Verificar CMake
Write-Host "Verificando CMake..." -ForegroundColor Yellow

$cmakePath = $null
$cmakePaths = @(
    "cmake",
    "$vsPath\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)

foreach ($path in $cmakePaths) {
    try {
        $version = & $path --version 2>$null
        if ($LASTEXITCODE -eq 0) {
            $cmakePath = $path
            $versionLine = ($version | Select-Object -First 1) -replace "cmake version ", ""
            Write-Host "CMake $versionLine encontrado!" -ForegroundColor Green
            break
        }
    } catch { continue }
}

if (-not $cmakePath) {
    Write-Host "CMake nao encontrado!" -ForegroundColor Red
    Write-Host "Instale CMake via Visual Studio Installer" -ForegroundColor Yellow
    Read-Host "Pressione Enter para sair"
    return
}

# Preparar build
$buildDir = "build"
if (Test-Path $buildDir) {
    Write-Host "Limpando build anterior..." -ForegroundColor Yellow
    Remove-Item $buildDir -Recurse -Force
}

New-Item -ItemType Directory -Path $buildDir | Out-Null
Set-Location $buildDir

# Configurar com CMake
Write-Host ""
Write-Host "Configurando build ARM64 com CMake..." -ForegroundColor Cyan

try {
    $configOutput = & $cmakePath .. -G "Visual Studio 17 2022" -A ARM64 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Erro na configuracao: $configOutput" -ForegroundColor Red
        return
    }
    Write-Host "Configuracao concluida!" -ForegroundColor Green
} catch {
    Write-Host "Erro na configuracao: $($_.Exception.Message)" -ForegroundColor Red
    return
}

# Compilar
Write-Host ""
Write-Host "Compilando hypervisor ARM64..." -ForegroundColor Cyan

try {
    $buildOutput = & $cmakePath --build . --config Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Erro na compilacao: $buildOutput" -ForegroundColor Red
        return
    }
    Write-Host "Compilacao concluida com sucesso!" -ForegroundColor Green
} catch {
    Write-Host "Erro na compilacao: $($_.Exception.Message)" -ForegroundColor Red
    return
}

# Verificar executavel
$execPaths = @("Release\hypervisor_arm64.exe", "Release\hypervisor.exe", "hypervisor_arm64.exe", "hypervisor.exe")
$execPath = $null

foreach ($path in $execPaths) {
    if (Test-Path $path) {
        $execPath = $path
        break
    }
}

if ($execPath) {
    $fileInfo = Get-Item $execPath
    Write-Host ""
    Write-Host "Executavel criado: $($fileInfo.Name)" -ForegroundColor Green
    Write-Host "Tamanho: $([math]::Round($fileInfo.Length / 1KB, 1)) KB" -ForegroundColor White
    Write-Host "Caminho: $($fileInfo.FullName)" -ForegroundColor Gray
    
    Write-Host ""
    $run = Read-Host "Executar hypervisor agora? (Y/n)"
    if ($run -eq "" -or $run -eq "y" -or $run -eq "Y") {
        Write-Host ""
        Write-Host "Executando hypervisor ARM64..." -ForegroundColor Cyan
        & $execPath
    }
} else {
    Write-Host "Executavel nao encontrado nos locais esperados" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "                        Build Concluido" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

Read-Host "Pressione Enter para finalizar"
