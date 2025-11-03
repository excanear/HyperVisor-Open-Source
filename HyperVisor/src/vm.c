#include "vm.h"

// Global VM state
vm_state_t g_vm = {0};

int vm_create(void)
{
    LOG_INFO("Criando partição VM...");
    
    // Criar partição VM
    HRESULT hr = WHvCreatePartition(&g_vm.partition);
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao criar partição: 0x%08X", hr);
        return -1;
    }
    
    // Configurar propriedades da partição para ARM64
    WHV_PARTITION_PROPERTY property;
    
    // Definir contadores de processador
    property.ProcessorCount = 1;
    hr = WHvSetPartitionProperty(g_vm.partition, WHvPartitionPropertyCodeProcessorCount,
                                &property, sizeof(property));
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao configurar contadores de processador: 0x%08X", hr);
        WHvDeletePartition(g_vm.partition);
        return -1;
    }
    
    // Configurar features ARM64
    memset(&property, 0, sizeof(property));
    property.ProcessorFeatures.AsUINT64 = 0;  // Features básicos ARM64
    hr = WHvSetPartitionProperty(g_vm.partition, WHvPartitionPropertyCodeProcessorFeatures,
                                &property, sizeof(property));
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao configurar features ARM64: 0x%08X", hr);
        WHvDeletePartition(g_vm.partition);
        return -1;
    }
    
    // Setup da partição
    hr = WHvSetupPartition(g_vm.partition);
    if (FAILED(hr)) {
        LOG_ERROR("Falha no setup da partição: 0x%08X", hr);
        WHvDeletePartition(g_vm.partition);
        return -1;
    }
    
    LOG_INFO("Partição VM criada com sucesso");
    
    // Setup memory and vCPU
    if (vm_setup_memory() != 0) {
        LOG_ERROR("Falha no setup de memória");
        vm_destroy();
        return -1;
    }
    
    if (vm_setup_vcpu() != 0) {
        LOG_ERROR("Falha no setup de vCPU");
        vm_destroy();
        return -1;
    }
    
    g_vm.running = true;
    LOG_INFO("VM criada e configurada com sucesso");
    return 0;
}

void vm_destroy(void)
{
    if (g_vm.partition != NULL) {
        LOG_INFO("Destruindo VM...");
        
        g_vm.running = false;
        
        // Liberar memória guest
        if (g_vm.guest_memory) {
            VirtualFree(g_vm.guest_memory, 0, MEM_RELEASE);
            g_vm.guest_memory = NULL;
        }
        
        // Deletar partição
        WHvDeletePartition(g_vm.partition);
        g_vm.partition = NULL;
        
        LOG_INFO("VM destruída");
    }
}

int vm_setup_memory(void)
{
    LOG_INFO("Configurando memória guest (%llu MB)...", GUEST_RAM_SIZE / (1024 * 1024));
    
    // Alocar memória para o guest
    g_vm.guest_memory = VirtualAlloc(NULL, GUEST_RAM_SIZE, 
                                    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!g_vm.guest_memory) {
        LOG_ERROR("Falha ao alocar memória guest: %lu", GetLastError());
        return -1;
    }
    
    g_vm.guest_memory_size = GUEST_RAM_SIZE;
    
    // Mapear memória guest na partição
    HRESULT hr = WHvMapGpaRange(g_vm.partition, g_vm.guest_memory, 
                               GUEST_RAM_BASE, GUEST_RAM_SIZE,
                               WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite | 
                               WHvMapGpaRangeFlagExecute);
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao mapear GPA range: 0x%08X", hr);
        return -1;
    }
    
    LOG_INFO("Memória guest mapeada: 0x%llX - 0x%llX", 
             GUEST_RAM_BASE, GUEST_RAM_BASE + GUEST_RAM_SIZE);
    
    return 0;
}

int vm_setup_vcpu(void)
{
    LOG_INFO("Configurando vCPU ARM64...");
    
    g_vm.vpindex = 0;
    
    // Criar vCPU
    HRESULT hr = WHvCreateVirtualProcessor(g_vm.partition, g_vm.vpindex, 0);
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao criar vCPU: 0x%08X", hr);
        return -1;
    }
    
    // Configurar registradores iniciais ARM64
    WHV_REGISTER_NAME reg_names[] = {
        WHvArm64RegisterX0,
        WHvArm64RegisterX1, 
        WHvArm64RegisterX2,
        WHvArm64RegisterSp,
        WHvArm64RegisterPc,
        WHvArm64RegisterPstateReg,
        WHvArm64RegisterElr,
        WHvArm64RegisterSpsr
    };
    
    WHV_REGISTER_VALUE reg_values[8];
    memset(reg_values, 0, sizeof(reg_values));
    
    // Configurar estado inicial ARM64
    reg_values[0].Reg64 = 0;  // X0
    reg_values[1].Reg64 = 0;  // X1
    reg_values[2].Reg64 = 0;  // X2
    reg_values[3].Reg64 = GUEST_RAM_BASE + GUEST_RAM_SIZE - 0x1000;  // SP
    reg_values[4].Reg64 = GUEST_ENTRY_POINT;  // PC
    reg_values[5].Reg64 = 0x3C5;  // PSTATE (EL1h, interrupts masked)
    reg_values[6].Reg64 = 0;  // ELR
    reg_values[7].Reg64 = 0;  // SPSR
    
    hr = WHvSetVirtualProcessorRegisters(g_vm.partition, g_vm.vpindex,
                                        reg_names, 8, reg_values);
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao configurar registradores: 0x%08X", hr);
        return -1;
    }
    
    LOG_INFO("vCPU ARM64 configurado com sucesso");
    return 0;
}

int vm_load_guest_code(const void* code, size_t code_size, uint64_t load_addr)
{
    if (!code || code_size == 0) {
        LOG_ERROR("Código guest inválido");
        return -1;
    }
    
    if (load_addr < GUEST_RAM_BASE || 
        load_addr + code_size > GUEST_RAM_BASE + GUEST_RAM_SIZE) {
        LOG_ERROR("Endereço de carregamento fora do range válido");
        return -1;
    }
    
    uint64_t offset = load_addr - GUEST_RAM_BASE;
    memcpy((char*)g_vm.guest_memory + offset, code, code_size);
    
    LOG_INFO("Código guest carregado: %zu bytes em 0x%llX", code_size, load_addr);
    return 0;
}

int vcpu_run(void)
{
    WHV_RUN_VP_EXIT_CONTEXT exit_context;
    
    HRESULT hr = WHvRunVirtualProcessor(g_vm.partition, g_vm.vpindex, 
                                       &exit_context, sizeof(exit_context));
    if (FAILED(hr)) {
        LOG_ERROR("Falha na execução do vCPU: 0x%08X", hr);
        return -1;
    }
    
    // Process exit context - implementado em exit_handler.c
    extern int handle_vm_exit(const WHV_RUN_VP_EXIT_CONTEXT* exit_context);
    return handle_vm_exit(&exit_context);
}

int vcpu_get_registers(WHV_REGISTER_NAME* reg_names, WHV_REGISTER_VALUE* reg_values, UINT32 count)
{
    HRESULT hr = WHvGetVirtualProcessorRegisters(g_vm.partition, g_vm.vpindex,
                                                reg_names, count, reg_values);
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao ler registradores: 0x%08X", hr);
        return -1;
    }
    return 0;
}

int vcpu_set_registers(WHV_REGISTER_NAME* reg_names, WHV_REGISTER_VALUE* reg_values, UINT32 count)
{
    HRESULT hr = WHvSetVirtualProcessorRegisters(g_vm.partition, g_vm.vpindex,
                                                reg_names, count, reg_values);
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao escrever registradores: 0x%08X", hr);
        return -1;
    }
    return 0;
}

int vcpu_get_pc(uint64_t* pc)
{
    WHV_REGISTER_NAME reg_name = WHvArm64RegisterPc;
    WHV_REGISTER_VALUE reg_value;
    
    if (vcpu_get_registers(&reg_name, &reg_value, 1) != 0) {
        return -1;
    }
    
    *pc = reg_value.Reg64;
    return 0;
}

int vcpu_set_pc(uint64_t pc)
{
    WHV_REGISTER_NAME reg_name = WHvArm64RegisterPc;
    WHV_REGISTER_VALUE reg_value;
    reg_value.Reg64 = pc;
    
    return vcpu_set_registers(&reg_name, &reg_value, 1);
}

int vcpu_get_sp(uint64_t* sp)
{
    WHV_REGISTER_NAME reg_name = WHvArm64RegisterSp;
    WHV_REGISTER_VALUE reg_value;
    
    if (vcpu_get_registers(&reg_name, &reg_value, 1) != 0) {
        return -1;
    }
    
    *sp = reg_value.Reg64;
    return 0;
}

int vcpu_set_sp(uint64_t sp)
{
    WHV_REGISTER_NAME reg_name = WHvArm64RegisterSp;
    WHV_REGISTER_VALUE reg_value;
    reg_value.Reg64 = sp;
    
    return vcpu_set_registers(&reg_name, &reg_value, 1);
}