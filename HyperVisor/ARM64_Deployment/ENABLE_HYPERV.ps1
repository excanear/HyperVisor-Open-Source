# ENABLE_HYPERV.ps1 - Habilitar Hyper-V e WHP

Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "                Windows Hypervisor Platform Setup" -ForegroundColor Cyan
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

# Verificar recursos atuais
Write-Host "Verificando recursos de virtualizacao..." -ForegroundColor Yellow

try {
    $hyperVFeature = Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All -ErrorAction Stop
    $whpFeature = Get-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform -ErrorAction Stop
    
    Write-Host "Hyper-V: $($hyperVFeature.State)" -ForegroundColor Gray
    Write-Host "Windows Hypervisor Platform: $($whpFeature.State)" -ForegroundColor Gray
    Write-Host ""
    
    $needsReboot = $false
    
    if ($hyperVFeature.State -ne "Enabled") {
        Write-Host "Habilitando Hyper-V..." -ForegroundColor Yellow
        $result = Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All -All -NoRestart
        if ($result.RestartNeeded) { $needsReboot = $true }
        Write-Host "Hyper-V habilitado com sucesso!" -ForegroundColor Green
    } else {
        Write-Host "Hyper-V ja esta habilitado" -ForegroundColor Green
    }
    
    if ($whpFeature.State -ne "Enabled") {
        Write-Host "Habilitando Windows Hypervisor Platform..." -ForegroundColor Yellow
        $result = Enable-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform -All -NoRestart
        if ($result.RestartNeeded) { $needsReboot = $true }
        Write-Host "Windows Hypervisor Platform habilitado com sucesso!" -ForegroundColor Green
    } else {
        Write-Host "Windows Hypervisor Platform ja esta habilitado" -ForegroundColor Green
    }
    
    # Configurar hypervisor no boot
    Write-Host "Configurando hypervisor no boot..." -ForegroundColor Yellow
    try {
        & bcdedit /set hypervisorlaunchtype auto | Out-Null
        Write-Host "Hypervisor configurado para iniciar automaticamente" -ForegroundColor Green
    } catch {
        Write-Host "Aviso: Nao foi possivel configurar hypervisor no boot" -ForegroundColor Yellow
    }
    
    Write-Host ""
    
    if ($needsReboot) {
        Write-Host "REINICIALIZACAO NECESSARIA" -ForegroundColor Yellow
        Write-Host "Os recursos foram habilitados, mas requer reinicializacao" -ForegroundColor Yellow
        Write-Host ""
        
        $reboot = Read-Host "Reiniciar agora? (Y/n)"
        if ($reboot -eq "" -or $reboot -eq "y" -or $reboot -eq "Y") {
            Write-Host "Reiniciando em 10 segundos..." -ForegroundColor Yellow
            Start-Sleep -Seconds 10
            Restart-Computer -Force
        } else {
            Write-Host "Lembre-se de reiniciar antes de usar o hypervisor!" -ForegroundColor Yellow
        }
    } else {
        Write-Host "Recursos habilitados e prontos para uso!" -ForegroundColor Green
    }
    
} catch {
    Write-Host "Erro ao verificar/habilitar recursos: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Verifique se a edicao do Windows suporta Hyper-V (Pro/Enterprise)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "                       Setup Concluido" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Proximos passos:" -ForegroundColor Yellow
Write-Host "1. Reiniciar o sistema (se necessario)" -ForegroundColor White
Write-Host "2. Executar SETUP_ARM64.ps1 para compilar" -ForegroundColor White
Write-Host ""

Read-Host "Pressione Enter para finalizar"