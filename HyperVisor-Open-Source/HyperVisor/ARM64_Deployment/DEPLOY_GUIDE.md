<!-- Desenvolvido por: Escanearcpl -->
# 🚀 ARM64 Hypervisor - Deployment para Windows 11 ARM64

## 📋 CHECKLIST DE PRÉ-REQUISITOS

Antes de começar, certifique-se de ter:

### Hardware & Sistema
- [ ] **Dispositivo Windows 11 ARM64** (Snapdragon, Apple M1/M2 via Parallels, etc.)
- [ ] **16GB+ RAM** (recomendado)
- [ ] **SSD com 50GB+ livres**
- [ ] **Conexão com internet** (para downloads)

### Software Necessário
- [ ] **Windows 11 versão 22H2 ou superior**
- [ ] **Visual Studio 2022** (Community/Professional/Enterprise)
- [ ] **CMake 3.20+**
- [ ] **Git para Windows**
- [ ] **Permissões de Administrator**

---

## 🔧 PASSOS DE DEPLOYMENT

### 1️⃣ PREPARAÇÃO DO SISTEMA

#### Verificar Windows 11 ARM64
```powershell
# Execute no PowerShell (como Administrator)
Get-ComputerInfo | Select-Object WindowsProductName, WindowsVersion, TotalPhysicalMemory
$env:PROCESSOR_ARCHITECTURE
```
✅ **Deve mostrar:** ARM64

#### Verificar Recursos de Virtualização
```powershell
# Verificar suporte a Hyper-V
Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All
systeminfo | findstr /C:"Hyper-V"
```

### 2️⃣ INSTALAÇÃO DE FERRAMENTAS

#### Visual Studio 2022 com ARM64
1. **Download:** https://visualstudio.microsoft.com/downloads/
2. **Workloads necessários:**
   - ✅ Desktop development with C++
   - ✅ MSVC v143 - VS 2022 C++ ARM64 build tools
   - ✅ Windows 11 SDK (latest)
   - ✅ CMake tools for Visual Studio

#### CMake Standalone (se necessário)
```powershell
# Via winget
winget install Kitware.CMake

# Ou download: https://cmake.org/download/
```

### 3️⃣ HABILITAR HYPER-V

#### Método 1: PowerShell (Recomendado)
```powershell
# Execute como Administrator
Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All -All

# Habilitar Windows Hypervisor Platform
Enable-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform -All

# Reiniciar quando solicitado
Restart-Computer
```

#### Método 2: DISM
```cmd
# Como Administrator
dism /online /enable-feature /featurename:Microsoft-Hyper-V-All /all
dism /online /enable-feature /featurename:HypervisorPlatform /all
shutdown /r /t 0
```

#### Método 3: Interface Gráfica
1. **Painel de Controle** → **Programas** → **Recursos do Windows**
2. ✅ **Hyper-V** (todos os sub-itens)
3. ✅ **Windows Hypervisor Platform**
4. **Reiniciar**

### 4️⃣ VALIDAÇÃO PÓS-REINÍCIO

```powershell
# Verificar se Hyper-V está ativo
Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All | Select-Object State
Get-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform | Select-Object State

# Verificar hypervisor
bcdedit /enum | findstr hypervisorlaunchtype

# Deve mostrar: hypervisorlaunchtype    Auto
```

### 5️⃣ BUILD E EXECUÇÃO

#### Copiar Arquivos
1. **Copie** toda a pasta `ARM64_Deployment` para o dispositivo ARM64
2. **Extraia** em local sem espaços: `C:\HyperVisor_ARM64`

#### Build Automático
```powershell
# Como Administrator
cd C:\HyperVisor_ARM64
.\SETUP_ARM64.ps1
```

#### Build Manual
```powershell
# Como Administrator
cd C:\HyperVisor_ARM64
mkdir build
cd build

# Configurar para ARM64
cmake .. -G "Visual Studio 17 2022" -A ARM64

# Compilar
cmake --build . --config Release

# Executar
.\Release\hypervisor.exe
```

---

## 🎯 SAÍDA ESPERADA EM ARM64 REAL

```
[INFO] ARM64 Hypervisor Monitor iniciando...
[INFO] WHP inicializado com sucesso
[INFO] Partição VM criada: Handle=0x...
[INFO] Memória guest mapeada: 0x40000000-0x44000000
[INFO] vCPU ARM64 configurado: VCPU#0
[INFO] Exception vectors carregados em 0x40000000
[INFO] Devices inicializados:
  - UART PL011 @ 0x09000000
  - ARM Generic Timer
  - GICv2 @ 0x08000000
[INFO] Guest carregado. PC=0x40001000, SP=0x43FFF000
[INFO] Iniciando guest ARM64 nativo...
[INFO] VM-exit capturado: Reason=0x1 (HLT)
[INFO] Guest HVC #0 interceptado
[INFO] UART write interceptado: data=0x48 ('H')
[INFO] Timer interrupt: currentCount=0x12345678
[INFO] Execução bem-sucedida!
```

---

## ⚠️ TROUBLESHOOTING

### "Hyper-V não disponível"
```powershell
# Verificar BIOS/UEFI
# Habilitar: Virtualization Technology, VT-d/IOMMU

# Verificar edição do Windows
Get-WindowsEdition -Online
# Home não suporta Hyper-V - precisa Pro/Enterprise
```

### "WHP não encontrado"
```powershell
# Reinstalar Windows Hypervisor Platform
Disable-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform
Enable-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform
Restart-Computer
```

### "Visual Studio não encontra ARM64"
- ✅ Verificar se instalou **MSVC ARM64 build tools**
- ✅ Usar **Visual Studio Installer** para adicionar workloads
- ✅ Reiniciar VS após instalação

### "CMake não configura"
```powershell
# Limpar cache
Remove-Item build -Recurse -Force
mkdir build
cd build

# Configurar explicitamente
cmake .. -G "Visual Studio 17 2022" -A ARM64 -DCMAKE_GENERATOR_PLATFORM=ARM64
```

---

## 📁 ESTRUTURA DE ARQUIVOS PARA TRANSFER

```
ARM64_Deployment/
├── src/                     # Código fonte completo
├── include/                 # Headers
├── CMakeLists.txt          # Build ARM64 nativo
├── SETUP_ARM64.ps1         # Setup automático
├── ENABLE_HYPERV.ps1       # Habilitar Hyper-V
├── BUILD_ARM64.bat         # Build script
├── DEPLOY_GUIDE.md         # Este guia
└── VERIFY_SYSTEM.ps1       # Verificação de sistema
```

---

## 🎉 PRÓXIMOS PASSOS

1. **Execute `VERIFY_SYSTEM.ps1`** para checar pré-requisitos
2. **Execute `ENABLE_HYPERV.ps1`** para habilitar virtualização
3. **Reinicie** o sistema
4. **Execute `SETUP_ARM64.ps1`** para build automático
5. **Execute `.\build\Release\hypervisor.exe`** como Administrator

**Agora você terá um hypervisor ARM64 real funcionando! 🚀**
