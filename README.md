# ARM64 Hypervisor Monitor

Um hypervisor educativo para Windows ARM64 usando Windows Hypervisor Platform (WHP) APIs.

## Características

- **Arquitetura**: ARM64 (AArch64)
- **Host**: Windows 11 ARM64 
- **API**: Windows Hypervisor Platform (WHP)
- **Devices emulados**: UART, Timer, GIC básico
- **Guest support**: Bare-metal ARM64 binaries
- **Exception handling**: Native ARM64 assembly com exception vectors
- **VM-exit handling**: Hypercalls, data/instruction aborts, system register traps

## Requisitos

- Windows 11 ARM64
- Hyper-V habilitado
- Visual Studio 2022 ou Build Tools para C++
- SDK do Windows 11
- Permissões de Administrator

## Estrutura do Projeto

```
HyperVisor/
├── src/
│   ├── main.c                  # Entry point e loop principal
│   ├── vm.c                    # Gerenciamento de VM e vCPU  
│   ├── exit_handler.c          # Tratamento de VM-exits (WHP)
│   ├── exception_handlers.c    # Tratamento nativo ARM64
│   ├── asm/
│   │   └── entry.s             # Exception vectors ARM64
│   ├── devices/
│   │   ├── devices_main.c      # Device dispatcher
│   │   ├── uart.c              # Emulação UART PL011
│   │   ├── timer.c             # Timer genérico
│   │   └── gic.c               # GIC (interrupt controller)
│   └── guest/
│       └── hello.s             # Guest code de exemplo
├── include/
│   ├── hypervisor.h            # Definições principais
│   ├── vm.h                    # VM/vCPU structures
│   ├── devices.h               # Device interfaces
│   └── asm_functions.h         # Assembly function declarations
├── build/                      # Arquivos de build
└── README.md
```

## Componentes Principais

### 1. VM Management (`vm.c`)
- Criação e configuração de VM via WHP
- Mapeamento de memória guest
- Configuração de vCPU ARM64
- Registradores e estado do processador

### 2. Exception Handling (`exception_handlers.c` + `entry.s`)
- Exception vectors ARM64 nativos
- Tratamento de HVC (hypercalls)
- Data/Instruction aborts
- System register traps
- IRQ/FIQ handling

### 3. Device Emulation (`devices/`)
- **UART PL011**: Console I/O, registradores padrão
- **Timer**: Generic timer com compare, interrupts
- **GIC**: ARM Generic Interrupt Controller básico
- Memory-mapped I/O com ranges apropriados

### 4. VM-Exit Processing (`exit_handler.c`)
- Interface WHP para captura de exits
- Tradução entre WHP e ARM64 nativo
- Dispatch para handlers específicos

## Build

```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Executar

```cmd
# Executar como Administrator
.\build\Release\hypervisor.exe                                                                                                                                  ```

## Como Funciona

1. **Inicialização**: 
   - Verifica suporte WHP e permissões
   - Cria partição VM e configura vCPU ARM64
   - Inicializa devices emulados

2. **Guest Loading**:
   - Carrega código guest na memória
   - Configura registradores iniciais (PC, SP)
   - Mapeia devices na região MMIO

3. **Execution Loop**:
   - Executa guest via `WHvRunVirtualProcessor`
   - Captura VM-exits (hypercalls, memory access)
   - Processa via device emulation ou exception injection
   - Retorna ao guest ou termina

4. **Exception Handling**:
   - Assembly ARM64 para context switching
   - Exception vectors para diferentes tipos
   - Integration com C handlers

## Exemplo de Saída

```
[INFO] ARM64 Hypervisor Monitor iniciando...
[INFO] WHP inicializado com sucesso
[INFO] Partição VM criada com sucesso
[INFO] Memória guest mapeada: 0x40000000 - 0x44000000
[INFO] vCPU ARM64 configurado com sucesso
[INFO] Devices inicializados com sucesso
[INFO] Sistema inicializado com sucesso. Iniciando guest...
[INFO] Guest carregado. PC=0x40001000, SP=0x43FFF000
[INFO] Simulando VM-exit #1
[INFO] Guest HVC #0
[INFO] Guest disse: Hello Hypervisor!
[INFO] UART TX: 'H' (0x48)
```

## Arquitetura ARM64 

### Exception Levels
- **EL2**: Hypervisor mode (nosso código)
- **EL1**: Guest kernel mode
- **EL0**: Guest user mode

### Memory Layout
```
0x40000000 - 0x44000000: Guest RAM (64MB)
0x09000000 - 0x09100000: Device MMIO
  ├── 0x09000000: UART PL011
  ├── 0x09010000: Timer
  ├── 0x09020000: GIC Distributor
  └── 0x09030000: GIC CPU Interface
```

### Exception Types Handled
- **HVC**: Hypercalls do guest
- **Data Abort**: Memory access (MMIO devices)
- **Instruction Abort**: Execution faults
- **System Register**: MSR/MRS traps
- **WFI/WFE**: Power management
- **IRQ/FIQ**: External interrupts

## Extensões Sugeridas

### 1. Memory Management
- Stage-2 translation tables
- Page fault handling
- Memory protection

### 2. Mais Devices
- Storage (virtio-blk)
- Network (virtio-net)
- Graphics básico
- RTC (Real Time Clock)

### 3. Guest Support
- Bootloader (U-Boot)
- Linux kernel boot
- Device tree generation
- SMP support

### 4. Performance
- VFIO passthrough
- Nested virtualization
- CPU affinity
- Memory ballooning

### 5. Debugging
- GDB stub integration
- Instruction tracing
- Performance counters
- Memory dump utilities

## Troubleshooting

### Erro: "Hypervisor não está presente"
- Verificar se Hyper-V está habilitado
- Executar `bcdedit /set hypervisorlaunchtype auto`
- Reiniciar o sistema

### Erro: "Deve ser executado como Administrator"
- Clicar direito → "Executar como Administrador"
- Ou usar terminal administrativo

### Compilation Issues
- Verificar Visual Studio 2022 instalado
- SDK do Windows 11 disponível
- ARM64 toolchain configurado

## Referências

- [ARM Architecture Reference Manual](https://developer.arm.com/documentation/ddi0487/)
- [Windows Hypervisor Platform APIs](https://docs.microsoft.com/en-us/virtualization/api/)
- [ARM GIC Architecture](https://developer.arm.com/documentation/ihi0069/)
- [UART PL011 Technical Reference](https://developer.arm.com/documentation/ddi0183/)

## Licença

Este projeto é educativo e open-source para aprendizado de hypervisors ARM64.
