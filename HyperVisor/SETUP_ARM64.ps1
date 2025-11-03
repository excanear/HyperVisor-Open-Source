# ============================================================================
# SETUP_ARM64.ps1 - Build Automático para ARM64 Hypervisor (Versão Principal)
# Execute em Windows 11 ARM64 com Visual Studio 2022
# ============================================================================

param(
    [string]$BuildType = "Release",
    [switch]$Clean = $false,
    [switch]$Verbose = $false,
    [switch]$RunAfterBuild = $false
)

# Verificar se está no diretório correto
$currentDir = Get-Location
Write-Host "Diretório atual: $currentDir"

# Procurar pelo diretório ARM64_Deployment
$deploymentDir = Join-Path $currentDir "ARM64_Deployment"
if (Test-Path $deploymentDir) {
    Write-Host "✅ Encontrado diretório ARM64_Deployment" -ForegroundColor Green
    Set-Location $deploymentDir
    
    # Executar o script principal do deployment
    if (Test-Path "SETUP_ARM64.ps1") {
        Write-Host "🚀 Executando script de deployment..." -ForegroundColor Cyan
        & ".\SETUP_ARM64.ps1" -BuildType $BuildType -Clean:$Clean -Verbose:$Verbose -RunAfterBuild:$RunAfterBuild
    } else {
        Write-Host "❌ Script SETUP_ARM64.ps1 não encontrado no diretório de deployment!" -ForegroundColor Red
    }
    
    Set-Location $currentDir
} else {
    Write-Host "❌ Diretório ARM64_Deployment não encontrado!" -ForegroundColor Red
    Write-Host "   Execute este script do diretório principal do projeto." -ForegroundColor Yellow
    Write-Host "   Ou use os scripts diretamente em ARM64_Deployment/" -ForegroundColor Yellow
    
    # Tentar criar uma versão simplificada para demonstração
    Write-Host ""
    Write-Host "🔧 Tentando build de demonstração no x86-64..." -ForegroundColor Yellow
    
    if (Test-Path "src\demo_main.c") {
        Write-Host "   Compilando demo com gcc..." -ForegroundColor Cyan
        try {
            $result = & gcc "src\demo_main.c" -o "hypervisor_demo.exe" -lkernel32 -luser32 -ladvapi32 2>&1
            if ($LASTEXITCODE -eq 0) {
                Write-Host "   ✅ Demo compilada com sucesso!" -ForegroundColor Green
                
                if ($RunAfterBuild) {
                    Write-Host "   🎯 Executando demo..." -ForegroundColor Cyan
                    & ".\hypervisor_demo.exe"
                } else {
                    Write-Host "   💡 Execute: .\hypervisor_demo.exe" -ForegroundColor Yellow
                }
            } else {
                Write-Host "   ❌ Erro na compilação: $result" -ForegroundColor Red
            }
        } catch {
            Write-Host "   ❌ gcc não encontrado. Instale MinGW ou use Visual Studio." -ForegroundColor Red
        }
    }
}

Read-Host "Pressione Enter para continuar"