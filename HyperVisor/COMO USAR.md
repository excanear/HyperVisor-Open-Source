# 🚀 Como Usar o ARM64 Hypervisor

## 📋 **Opções Disponíveis**

Você tem **3 maneiras** diferentes de usar o hypervisor:

### 1. 🎮 **GUI Hypervisor** (Recomendado - Com Gráficos)
### 2. 💻 **Hypervisor Básico** (Console)
### 3. 📦 **Deployment ARM64** (Para hardware ARM64)

---

## 🎮 **1. GUI Hypervisor - Real Graphics**

### **O que é:**
- Interface Windows moderna
- DirectX 11 graphics virtualization
- Virtual GPU com framebuffer
- Input devices virtualizados
- Carregamento de OS real (Linux/Windows/ISO)

### **Como usar:**

#### **Passo 1: Build e Deploy**
```powershell
# Execute como Administrator
.\build_gui_hypervisor.ps1 -Deploy
```

#### **Passo 2: Executar**
```powershell
cd deploy_gui
.\run_gui_hypervisor.bat
# OU
.\ARM64_GUI_Hypervisor.exe
```

#### **Passo 3: Usar a Interface**
1. **Load OS**: Clique para carregar imagem do SO
   - Linux ARM64 kernels (vmlinuz)
   - Windows PE images (.exe, .efi)
   - ISO 9660 boot images (.iso)
   - Raw binary files

2. **Start VM**: Inicia a máquina virtual
3. **Stop VM**: Para a execução
4. **Reset VM**: Reinicia a VM

#### **Recursos:**
- ✅ Display gráfico real do guest OS
- ✅ Entrada de teclado/mouse virtualizada
- ✅ Devices emulados (UART, Timer, GIC, VGA)
- ✅ Boot automático com device tree

---

## 💻 **2. Hypervisor Básico (Console)**

### **O que é:**
- Versão console simples
- VM-exit handling
- Device emulation básica
- Ideal para testes e desenvolvimento

### **Como usar:**

#### **Passo 1: Build**
```powershell
# Verificar sistema
.\VERIFY_SYSTEM.ps1

# Habilitar Hyper-V se necessário
.\ENABLE_HYPERV.ps1

# Build básico
cmake -S . -B build
cmake --build build --config Release
```

#### **Passo 2: Executar**
```powershell
# Execute como Administrator
cd build
.\hypervisor.exe
```

#### **O que faz:**
- Cria VM com 64MB RAM
- Configura vCPU ARM64
- Emula devices básicos
- Mostra VM-exits no console

---

## 📦 **3. Deployment ARM64 (Hardware Real)**

### **O que é:**
- Package completo para Windows 11 ARM64
- Scripts automatizados
- Build nativo ARM64
- Pronto para hardware real

### **Como usar:**

#### **Passo 1: Copiar para ARM64**
```powershell
# Copiar toda a pasta ARM64_Deployment para dispositivo ARM64
xcopy /E /I ARM64_Deployment C:\hypervisor_arm64\
```

#### **Passo 2: Setup no ARM64**
```powershell
# No dispositivo ARM64, execute como Administrator:
cd C:\hypervisor_arm64\
.\VERIFY_SYSTEM.ps1    # Verificar sistema
.\ENABLE_HYPERV.ps1    # Habilitar Hyper-V
.\SETUP_ARM64.ps1      # Setup toolchain
```

#### **Passo 3: Build e Execute**
```powershell
# Build nativo ARM64
.\DEPLOY_ARM64.bat

# Executar
.\EXECUTAR_DEMO.bat
```

---

## 🛠️ **Requisitos do Sistema**

### **Mínimos:**
- Windows 10/11 (ARM64 recomendado)
- Hyper-V suportado
- 4GB RAM disponível
- DirectX 11 (para GUI)

### **Recomendados:**
- Windows 11 ARM64
- 8GB+ RAM
- GPU com DirectX 11
- Privilégios Administrator

### **Para Development:**
- Visual Studio 2022 com ARM64 tools
- Windows SDK
- CMake 3.20+

---

## 📁 **Estrutura de Arquivos OS**

### **Linux ARM64:**
```
/boot/vmlinuz-5.x.x-arm64    # Kernel Linux
/boot/initrd.img-5.x.x       # Initial ramdisk
/boot/dtb/                   # Device tree blobs
```

### **Windows PE:**
```
winpe_arm64.wim              # Windows PE image
bootmgr.efi                  # Boot manager
BCD                          # Boot configuration
```

### **ISO Images:**
```
ubuntu-arm64.iso             # Ubuntu ARM64 ISO
windows11-arm64.iso          # Windows 11 ARM64 ISO
custom-boot.iso              # Custom bootable ISO
```

---

## 🎯 **Cenários de Uso**

### **1. Desenvolvimento:**
```powershell
# Use hypervisor básico para desenvolvimento
.\VERIFY_SYSTEM.ps1
cmake -S . -B build
cmake --build build
cd build && .\hypervisor.exe
```

### **2. Demo com Gráficos:**
```powershell
# Use GUI hypervisor para demonstrações
.\build_gui_hypervisor.ps1 -Deploy
cd deploy_gui && .\ARM64_GUI_Hypervisor.exe
```

### **3. Hardware ARM64:**
```powershell
# Use deployment package em hardware real
cd ARM64_Deployment
.\DEPLOY_ARM64.bat
.\EXECUTAR_DEMO.bat
```

---

## 🐛 **Troubleshooting**

### **Erro: "Hyper-V not available"**
```powershell
# Habilitar Hyper-V
.\ENABLE_HYPERV.ps1
# Reiniciar sistema
```

### **Erro: "Administrator required"**
```powershell
# Executar como Administrator
Right-click → "Run as administrator"
```

### **Erro: "DirectX not found"**
```powershell
# Instalar DirectX runtime
# Atualizar drivers da GPU
```

### **Build Error:**
```powershell
# Verificar toolchain
.\VERIFY_SYSTEM.ps1
# Instalar Visual Studio 2022 ARM64 tools
```

---

## 🏆 **Quick Start (Recomendado)**

**Para primeira execução:**

```powershell
# 1. Verificar sistema
.\VERIFY_SYSTEM.ps1

# 2. Build GUI hypervisor
.\build_gui_hypervisor.ps1 -Deploy

# 3. Executar (como Administrator)
cd deploy_gui
.\run_gui_hypervisor.bat
```

**Agora você tem um hypervisor real com gráficos rodando!** 🚀

---

## 📚 **Recursos Adicionais**

- **README.md** - Documentação técnica
- **ARM64_Deployment/README.md** - Guia de deployment
- **WORKSPACE_CLEAN.md** - Estrutura da workspace
- **Source code** em `src/` - Código completo comentado

**Divirta-se explorando a virtualização ARM64!** ⚡