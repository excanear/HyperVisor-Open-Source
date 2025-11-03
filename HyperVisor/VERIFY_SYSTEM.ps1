# VERIFY_SYSTEM.ps1 - Verificacao do Sistema

$currentDir = Get-Location
Write-Host "Verificando sistema..." -ForegroundColor Cyan
Write-Host "Diretorio atual: $currentDir" -ForegroundColor White

$deploymentDir = Join-Path $currentDir "ARM64_Deployment"
if (Test-Path $deploymentDir) {
    Write-Host "Encontrado diretorio ARM64_Deployment" -ForegroundColor Green
    Set-Location $deploymentDir
    
    if (Test-Path "VERIFY_SYSTEM.ps1") {
        Write-Host "Executando verificacao completa..." -ForegroundColor Cyan
        & ".\VERIFY_SYSTEM.ps1"
    } else {
        Write-Host "Script VERIFY_SYSTEM.ps1 nao encontrado!" -ForegroundColor Red
    }
    
    Set-Location $currentDir
} else {
    Write-Host "Diretorio ARM64_Deployment nao encontrado!" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Verificacao basica do sistema atual:" -ForegroundColor Cyan
    
    $osInfo = Get-ComputerInfo
    $architecture = $env:PROCESSOR_ARCHITECTURE
    
    Write-Host "Sistema: $($osInfo.WindowsProductName)" -ForegroundColor White
    Write-Host "Arquitetura: $architecture" -ForegroundColor White
    Write-Host "Versao: $($osInfo.WindowsVersion)" -ForegroundColor White
    
    if ($architecture -eq "ARM64") {
        Write-Host "Sistema ARM64 detectado!" -ForegroundColor Green
    } else {
        Write-Host "Sistema nao e ARM64 ($architecture)" -ForegroundColor Yellow
        Write-Host "Use o pacote ARM64_Deployment em dispositivo ARM64 real" -ForegroundColor Cyan
    }
    
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
    if ($isAdmin) {
        Write-Host "Executando como Administrator" -ForegroundColor Green
    } else {
        Write-Host "NAO esta executando como Administrator" -ForegroundColor Red
        Write-Host "Clique direito no PowerShell e selecione Executar como administrador" -ForegroundColor Cyan
    }
    
    Write-Host ""
    Write-Host "Para deployment completo em ARM64:" -ForegroundColor Yellow
    Write-Host "1. Copie a pasta ARM64_Deployment para dispositivo ARM64" -ForegroundColor White
    Write-Host "2. Execute DEPLOY_ARM64.bat como Administrator" -ForegroundColor White
    Write-Host "3. Escolha opcao 4 (processo completo)" -ForegroundColor White
}

Write-Host ""
Read-Host "Pressione Enter para continuar"