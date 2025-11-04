/* Desenvolvido por: Escanearcpl */
#include "hypervisor.h"
#include "vm.h"
#include "devices.h"
#include "asm_functions.h"

// Buffer global para contexto do guest
guest_context_t guest_context_buffer = {0};

// Handlers de exceção do hypervisor
void handle_hypervisor_exception(uint32_t exception_type)
{
    LOG_ERROR("Exceção do hypervisor: tipo=%d", exception_type);
    
    switch (exception_type) {
        case 0: // Sync exception current EL SP0
            LOG_ERROR("Sync exception em EL2 com SP0");
            break;
        case 1: // IRQ current EL SP0
            LOG_DEBUG("IRQ em EL2 com SP0");
            break;
        case 2: // FIQ current EL SP0
            LOG_DEBUG("FIQ em EL2 com SP0");
            break;
        case 3: // SError current EL SP0
            LOG_ERROR("SError em EL2 com SP0");
            break;
        case 4: // Sync exception current EL SPx
            LOG_ERROR("Sync exception em EL2 com SPx");
            break;
        case 5: // IRQ current EL SPx
            LOG_DEBUG("IRQ em EL2 com SPx");
            break;
        case 6: // FIQ current EL SPx
            LOG_DEBUG("FIQ em EL2 com SPx");
            break;
        case 7: // SError current EL SPx
            LOG_ERROR("SError em EL2 com SPx");
            break;
        default:
            LOG_ERROR("Tipo de exceção desconhecido: %d", exception_type);
            break;
    }
}

// Handlers de exceção do guest
void handle_guest_exception(uint32_t exception_type, uint64_t esr, uint64_t far, uint64_t elr)
{
    LOG_DEBUG("Exceção do guest: tipo=%d, ESR=0x%llX, FAR=0x%llX, ELR=0x%llX", 
              exception_type, esr, far, elr);
    
    switch (exception_type) {
        case 8: // Sync exception from guest (AArch64)
            handle_guest_sync_exception(esr, far, elr);
            break;
        case 9: // IRQ from guest (AArch64)
            handle_guest_irq();
            break;
        case 10: // FIQ from guest (AArch64)
            handle_guest_fiq();
            break;
        case 11: // SError from guest (AArch64)
            handle_guest_serror(esr, far);
            break;
        case 12: // Sync exception from AArch32 guest
            handle_guest_sync_exception_aarch32(esr, far, elr);
            break;
        case 13: // IRQ from AArch32 guest
            handle_guest_irq();
            break;
        case 14: // FIQ from AArch32 guest
            handle_guest_fiq();
            break;
        case 15: // SError from AArch32 guest
            handle_guest_serror(esr, far);
            break;
        default:
            LOG_ERROR("Tipo de exceção do guest desconhecido: %d", exception_type);
            break;
    }
}

// Handler para exceções síncronas do guest
void handle_guest_sync_exception(uint64_t esr, uint64_t far, uint64_t elr)
{
    uint32_t ec = (esr >> 26) & 0x3F;  // Exception Class
    uint32_t iss = esr & 0x1FFFFFF;    // Instruction Specific Syndrome
    
    LOG_DEBUG("Guest sync exception: EC=0x%X, ISS=0x%X", ec, iss);
    
    switch (ec) {
        case 0x16: // HVC instruction (AArch64)
            handle_guest_hvc(iss, elr);
            break;
        case 0x24: // Data Abort from lower EL
            handle_guest_data_abort(iss, far, elr);
            break;
        case 0x20: // Instruction Abort from lower EL
            handle_guest_instruction_abort(iss, far, elr);
            break;
        case 0x18: // MSR/MRS/System instruction trap
            handle_guest_system_register_trap(iss, elr);
            break;
        case 0x01: // WFI/WFE instruction
            handle_guest_wfi_wfe(iss, elr);
            break;
        default:
            LOG_ERROR("Exception Class não tratada: 0x%X", ec);
            // Injetar exceção no guest
            inject_exception_to_guest(esr, far);
            break;
    }
}

// Handler para HVC (Hypervisor Call)
void handle_guest_hvc(uint32_t iss, uint64_t elr)
{
    uint16_t hvc_num = iss & 0xFFFF;
    LOG_INFO("Guest HVC #%d", hvc_num);
    
    // Simular hypercall context para compatibilidade
    WHV_HYPERCALL_CONTEXT hypercall = {0};
    
    // Ler registradores do guest para obter parâmetros
    guest_context_t* ctx = &guest_context_buffer;
    save_guest_context(ctx);
    
    hypercall.Rax = ctx->x[0];  // x0 = hypercall number
    hypercall.Rbx = ctx->x[1];  // x1 = parameter 1
    hypercall.Rcx = ctx->x[2];  // x2 = parameter 2
    hypercall.Rdx = ctx->x[3];  // x3 = parameter 3
    
    // Chamar handler existente
    handle_hypercall(&hypercall);
    
    // Avançar PC
    ctx->elr_el2 = elr + 4;  // HVC é instrução de 4 bytes
    restore_guest_context(ctx);
}

// Handler para Data Abort
void handle_guest_data_abort(uint32_t iss, uint64_t far, uint64_t elr)
{
    bool is_write = (iss >> 6) & 1;
    uint32_t sas = (iss >> 22) & 3;  // Size Access Size
    uint32_t size = 1 << sas;
    
    LOG_DEBUG("Guest data abort: FAR=0x%llX, write=%d, size=%d", far, is_write, size);
    
    // Verificar se é acesso a device
    if (far >= DEVICE_BASE && far < DEVICE_BASE + 0x100000) {
        uint64_t data = 0;
        
        if (is_write) {
            // Para writes, extrair dados do registrador
            guest_context_t* ctx = &guest_context_buffer;
            save_guest_context(ctx);
            
            // Determinar qual registrador baseado na instrução
            // Para simplificar, usar x0
            data = ctx->x[0];
        }
        
        device_access_result_t result = handle_device_access(far, &data, size, is_write);
        
        if (result == DEVICE_ACCESS_OK) {
            if (!is_write) {
                // Para reads, colocar dados no registrador
                guest_context_t* ctx = &guest_context_buffer;
                save_guest_context(ctx);
                ctx->x[0] = data;  // Simplificado
                restore_guest_context(ctx);
            }
            
            // Avançar PC
            guest_context_t* ctx = &guest_context_buffer;
            save_guest_context(ctx);
            ctx->elr_el2 = elr + 4;  // Assumir instrução de 4 bytes
            restore_guest_context(ctx);
            
            return;
        }
    }
    
    LOG_ERROR("Data abort não tratado: FAR=0x%llX", far);
    inject_exception_to_guest(0x96000000 | iss, far);  // Data abort ESR
}

// Handler para Instruction Abort
void handle_guest_instruction_abort(uint32_t iss, uint64_t far, uint64_t elr)
{
    LOG_ERROR("Guest instruction abort: FAR=0x%llX, ISS=0x%X", far, iss);
    inject_exception_to_guest(0x86000000 | iss, far);  // Instruction abort ESR
}

// Handler para System Register Trap
void handle_guest_system_register_trap(uint32_t iss, uint64_t elr)
{
    uint32_t op0 = (iss >> 20) & 3;
    uint32_t op2 = (iss >> 17) & 7;
    uint32_t op1 = (iss >> 14) & 7;
    uint32_t crn = (iss >> 10) & 15;
    uint32_t rt = (iss >> 5) & 31;
    uint32_t crm = (iss >> 1) & 15;
    uint32_t dir = iss & 1;  // 0=write, 1=read
    
    LOG_DEBUG("System register trap: op0=%d, op1=%d, crn=%d, crm=%d, op2=%d, rt=%d, dir=%d",
              op0, op1, crn, crm, op2, rt, dir);
    
    // Para demo, apenas avançar PC
    guest_context_t* ctx = &guest_context_buffer;
    save_guest_context(ctx);
    ctx->elr_el2 = elr + 4;
    restore_guest_context(ctx);
}

// Handler para WFI/WFE
void handle_guest_wfi_wfe(uint32_t iss, uint64_t elr)
{
    LOG_DEBUG("Guest WFI/WFE: ISS=0x%X", iss);
    
    // Para WFI, podemos implementar yield
    // Para demo, apenas avançar PC
    guest_context_t* ctx = &guest_context_buffer;
    save_guest_context(ctx);
    ctx->elr_el2 = elr + 4;
    restore_guest_context(ctx);
}

// Handler para IRQ do guest
void handle_guest_irq(void)
{
    LOG_DEBUG("Guest IRQ");
    
    // Verificar se há interrupts pendentes nos devices
    if (timer_has_interrupt()) {
        LOG_DEBUG("Timer interrupt para o guest");
        // Injetar interrupt no guest via GIC
        gic_set_interrupt(30, true);
    }
    
    if (uart_has_pending_rx()) {
        LOG_DEBUG("UART RX interrupt para o guest");
        gic_set_interrupt(33, true);  // UART interrupt
    }
}

// Handler para FIQ do guest
void handle_guest_fiq(void)
{
    LOG_DEBUG("Guest FIQ");
    // Similar ao IRQ mas com prioridade maior
    handle_guest_irq();
}

// Handler para SError do guest
void handle_guest_serror(uint64_t esr, uint64_t far)
{
    LOG_ERROR("Guest SError: ESR=0x%llX, FAR=0x%llX", esr, far);
    // SError é crítico - pode terminar o guest
    g_vm.running = false;
}

// Handler para exceções AArch32 (compatibilidade)
void handle_guest_sync_exception_aarch32(uint64_t esr, uint64_t far, uint64_t elr)
{
    LOG_DEBUG("Guest AArch32 sync exception: ESR=0x%llX, FAR=0x%llX", esr, far);
    // Para demo, tratar similar ao AArch64
    handle_guest_sync_exception(esr, far, elr);
}

// Função para injetar exceção no guest
void inject_exception_to_guest(uint64_t esr, uint64_t far)
{
    LOG_DEBUG("Injetando exceção no guest: ESR=0x%llX, FAR=0x%llX", esr, far);
    
    // Para implementação completa, configuraria:
    // - ELR_EL1 com PC atual do guest
    // - SPSR_EL1 com PSTATE atual
    // - ESR_EL1 com syndrome
    // - FAR_EL1 se relevante
    // - PC para vector de exceção apropriado
    
    // Para demo, apenas parar o guest
    LOG_ERROR("Exceção não tratada - parando guest");
    g_vm.running = false;
}

// Função principal do hypervisor (para modo assembly)
int hypervisor_main(void)
{
    LOG_INFO("Hypervisor ARM64 iniciado em EL2");
    
    // Inicializar devices
    if (devices_init() != 0) {
        LOG_ERROR("Falha na inicialização dos devices");
        return -1;
    }
    
    // Para demo, simular execução do guest
    LOG_INFO("Simulando execução do guest...");
    
    // Simular alguns VM-exits
    for (int i = 0; i < 5; i++) {
        LOG_INFO("Simulando VM-exit #%d", i + 1);
        
        // Simular HVC
        handle_guest_hvc(0, 0x40001000 + i * 4);
        
        // Simular data abort para device
        handle_guest_data_abort(0x40, UART_BASE, 0x40001000 + i * 4);
    }
    
    LOG_INFO("Demo do hypervisor concluída");
    devices_cleanup();
    return 0;
}
