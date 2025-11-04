# Desenvolvido por: Escanearcpl
# ============================================================================
# ENABLE_HYPERV.ps1 - Habilitar Hyper-V (Versão Principal)
# ============================================================================

# Verificar se está no diretório correto
$currentDir = Get-Location
Write-Host "⚙️  Configurando Hyper-V..." -ForegroundColor Cyan

# Procurar pelo diretório ARM64_Deployment
$deploymentDir = Join-Path $currentDir "ARM64_Deployment"
if (Test-Path $deploymentDir) {
    Write-Host "✅ Encontrado diretório ARM64_Deployment" -ForegroundColor Green
    Set-Location $deploymentDir
    
    # Executar o script principal do deployment
    if (Test-Path "ENABLE_HYPERV.ps1") {
        Write-Host "🚀 Executando configuração de Hyper-V..." -ForegroundColor Cyan
        & ".\ENABLE_HYPERV.ps1"
    } else {
        Write-Host "❌ Script ENABLE_HYPERV.ps1 não encontrado!" -ForegroundColor Red
    }
    
    Set-Location $currentDir
} else {
    Write-Host "⚠️  Diretório ARM64_Deployment não encontrado!" -ForegroundColor Yellow
    Write-Host ""
    
    # Verificar Administrator
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
    if (-not $isAdmin) {
        Write-Host "❌ ERRO: Execute como Administrator!" -ForegroundColor Red
        Write-Host "   Clique direito no PowerShell → 'Executar como administrador'" -ForegroundColor Yellow
        Read-Host "Pressione Enter para sair"
        return
    }
    
    Write-Host "📋 Configuração básica de Hyper-V:" -ForegroundColor Cyan
    
    try {
        # Verificar recursos atuais
        $hyperVFeature = Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All -ErrorAction SilentlyContinue
        $whpFeature = Get-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform -ErrorAction SilentlyContinue
        
        Write-Host "   Status atual:" -ForegroundColor White
        Write-Host "   - Hyper-V: $($hyperVFeature.State)" -ForegroundColor Gray
        Write-Host "   - WHP: $($whpFeature.State)" -ForegroundColor Gray
        
        $needsChanges = $false
        
        if ($hyperVFeature.State -ne "Enabled") {
            Write-Host "   🔧 Habilitando Hyper-V..." -ForegroundColor Yellow
            Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All -All -NoRestart
            $needsChanges = $true
        }
        
        if ($whpFeature.State -ne "Enabled") {
            Write-Host "   🔧 Habilitando Windows Hypervisor Platform..." -ForegroundColor Yellow
            Enable-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform -All -NoRestart
            $needsChanges = $true
        }
        
        if ($needsChanges) {
            Write-Host "   ✅ Recursos habilitados!" -ForegroundColor Green
            Write-Host "   🔄 REINICIALIZAÇÃO NECESSÁRIA" -ForegroundColor Yellow
            
            $reboot = Read-Host "   Reiniciar agora? (Y/n)"
            if ($reboot -eq "" -or $reboot -eq "y" -or $reboot -eq "Y") {
                Write-Host "   🔄 Reiniciando em 10 segundos..." -ForegroundColor Yellow
                Start-Sleep -Seconds 10
                Restart-Computer -Force
            }
        } else {
            Write-Host "   ✅ Recursos já estão habilitados!" -ForegroundColor Green
        }
        
    } catch {
        Write-Host "   ❌ Erro: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "   💡 Use o pacote ARM64_Deployment em dispositivo ARM64 para configuração completa" -ForegroundColor Cyan
    }
}

Write-Host ""
Read-Host "Pressione Enter para continuar"
