#include "hypervisor.h"
#include "vm.h"
#include "devices.h"

int main(int argc, char* argv[])
{
    LOG_INFO("ARM64 Hypervisor Monitor iniciando...");
    
    // Verificar se está rodando como Administrator
    BOOL is_admin = FALSE;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminGroup;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &is_admin);
        FreeSid(adminGroup);
    }
    
    if (!is_admin) {
        LOG_ERROR("Este programa deve ser executado como Administrator");
        return EXIT_INIT_FAILED;
    }
    
    // Inicializar subsistemas
    if (hypervisor_init() != 0) {
        LOG_ERROR("Falha na inicialização do hypervisor");
        return EXIT_INIT_FAILED;
    }
    
    if (devices_init() != 0) {
        LOG_ERROR("Falha na inicialização dos devices");
        hypervisor_cleanup();
        return EXIT_INIT_FAILED;
    }
    
    if (vm_create() != 0) {
        LOG_ERROR("Falha na criação da VM");
        devices_cleanup();
        hypervisor_cleanup();
        return EXIT_VM_FAILED;
    }
    
    LOG_INFO("Sistema inicializado com sucesso. Iniciando guest...");
    
    // Executar o guest
    int result = run_guest();
    
    // Cleanup
    vm_destroy();
    devices_cleanup();
    hypervisor_cleanup();
    
    if (result == 0) {
        LOG_INFO("Guest executado com sucesso");
    } else {
        LOG_ERROR("Falha na execução do guest");
    }
    
    return result;
}

int hypervisor_init(void)
{
    LOG_INFO("Inicializando Windows Hypervisor Platform...");
    
    // Verificar se WHP está disponível
    WHV_CAPABILITY capability;
    UINT32 written_size;
    
    HRESULT hr = WHvGetCapability(WHvCapabilityCodeHypervisorPresent, 
                                 &capability, sizeof(capability), &written_size);
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao verificar suporte WHP: 0x%08X", hr);
        return -1;
    }
    
    if (!capability.HypervisorPresent) {
        LOG_ERROR("Hypervisor não está presente ou habilitado");
        LOG_ERROR("Certifique-se que Hyper-V está habilitado no Windows");
        return -1;
    }
    
    // Verificar suporte ARM64
    hr = WHvGetCapability(WHvCapabilityCodeProcessorFeatures,
                         &capability, sizeof(capability), &written_size);
    if (FAILED(hr)) {
        LOG_ERROR("Falha ao verificar recursos do processador: 0x%08X", hr);
        return -1;
    }
    
    LOG_INFO("WHP inicializado com sucesso");
    return 0;
}

void hypervisor_cleanup(void)
{
    LOG_INFO("Limpeza do hypervisor concluída");
}

int run_guest(void)
{
    LOG_INFO("Iniciando loop de execução do guest...");
    
    // Simple guest code que executa HVC (hypercall)
    // Isso vai gerar um VM-exit que podemos capturar
    uint32_t guest_code[] = {
        0xd4000002,  // hvc #0 - hypercall instruction
        0xd503207f,  // wfi - wait for interrupt  
        0x14000000   // b . - branch to self (infinite loop)
    };
    
    // Carregar o código guest na memória
    if (vm_load_guest_code(guest_code, sizeof(guest_code), GUEST_ENTRY_POINT) != 0) {
        LOG_ERROR("Falha ao carregar código guest");
        return EXIT_RUN_FAILED;
    }
    
    // Configurar PC inicial
    if (vcpu_set_pc(GUEST_ENTRY_POINT) != 0) {
        LOG_ERROR("Falha ao configurar PC inicial");
        return EXIT_RUN_FAILED;
    }
    
    // Configurar stack pointer
    if (vcpu_set_sp(GUEST_RAM_BASE + GUEST_RAM_SIZE - 0x1000) != 0) {
        LOG_ERROR("Falha ao configurar SP inicial");
        return EXIT_RUN_FAILED;
    }
    
    LOG_INFO("Guest carregado. PC=0x%llX, SP=0x%llX", 
             GUEST_ENTRY_POINT, GUEST_RAM_BASE + GUEST_RAM_SIZE - 0x1000);
    
    // Main execution loop
    int exit_count = 0;
    const int max_exits = 10;  // Limite para demo
    
    while (g_vm.running && exit_count < max_exits) {
        int result = vcpu_run();
        exit_count++;
        
        if (result != 0) {
            LOG_ERROR("Erro na execução do vCPU");
            break;
        }
        
        LOG_INFO("VM-exit #%d capturado", exit_count);
        
        // Para demo, parar após alguns exits
        if (exit_count >= max_exits) {
            LOG_INFO("Limite de exits atingido, parando demo");
            break;
        }
    }
    
    LOG_INFO("Execução do guest concluída (%d exits processados)", exit_count);
    return 0;
}