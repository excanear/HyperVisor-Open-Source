#include "vm.h"
#include "devices.h"

// Forward declarations
int handle_hypercall(const WHV_HYPERCALL_CONTEXT* hypercall);
int handle_memory_access(const WHV_MEMORY_ACCESS_CONTEXT* memory_access);
int handle_io_port_access(const WHV_X64_IO_PORT_ACCESS_CONTEXT* io_port);
int handle_exception(const WHV_VP_EXCEPTION_CONTEXT* exception);

int handle_vm_exit(const WHV_RUN_VP_EXIT_CONTEXT* exit_context)
{
    if (!exit_context) {
        LOG_ERROR("Exit context é NULL");
        return -1;
    }
    
    LOG_DEBUG("VM-Exit: Reason=%d", exit_context->ExitReason);
    
    switch (exit_context->ExitReason) {
        case WHvRunVpExitReasonHypercall:
            return handle_hypercall(&exit_context->Hypercall);
            
        case WHvRunVpExitReasonMemoryAccess:
            return handle_memory_access(&exit_context->MemoryAccess);
            
        case WHvRunVpExitReasonIoPortAccess:
            return handle_io_port_access(&exit_context->IoPortAccess);
            
        case WHvRunVpExitReasonException:
            return handle_exception(&exit_context->VpException);
            
        case WHvRunVpExitReasonCanceled:
            LOG_INFO("VM-Exit cancelado");
            return 0;
            
        case WHvRunVpExitReasonUnsupportedFeature:
            LOG_ERROR("Feature não suportada");
            return -1;
            
        default:
            LOG_ERROR("VM-Exit não reconhecido: %d", exit_context->ExitReason);
            return -1;
    }
}

int handle_hypercall(const WHV_HYPERCALL_CONTEXT* hypercall)
{
    LOG_INFO("Hypercall capturada: Input=0x%llX", hypercall->Rax);
    
    // Para ARM64, os hypercalls normalmente usam a instrução HVC
    // O número do hypercall vem do registrador X0
    uint64_t hypercall_num = hypercall->Rax & 0xFFFF;
    
    switch (hypercall_num) {
        case 0:  // Hypercall 0 - Hello from guest
            LOG_INFO("Guest disse: Hello Hypervisor!");
            uart_write_char('H');
            uart_write_char('\n');
            break;
            
        case 1:  // Hypercall 1 - Request shutdown
            LOG_INFO("Guest solicitou shutdown");
            g_vm.running = false;
            break;
            
        case 2:  // Hypercall 2 - Print character
            {
                char c = (char)(hypercall->Rbx & 0xFF);
                LOG_INFO("Guest print: '%c' (0x%02X)", c, c);
                uart_write_char(c);
            }
            break;
            
        default:
            LOG_INFO("Hypercall desconhecido: %llu", hypercall_num);
            break;
    }
    
    // Avançar PC após a instrução HVC
    uint64_t pc;
    if (vcpu_get_pc(&pc) == 0) {
        vcpu_set_pc(pc + 4);  // HVC é uma instrução de 4 bytes
    }
    
    return 0;
}

int handle_memory_access(const WHV_MEMORY_ACCESS_CONTEXT* memory_access)
{
    uint64_t gpa = memory_access->Gpa;
    uint8_t instruction_bytes[16];
    uint32_t instruction_byte_count = memory_access->InstructionByteCount;
    
    LOG_DEBUG("Memory Access: GPA=0x%llX, Size=%d, Write=%d", 
              gpa, memory_access->AccessInfo.AccessSize,
              memory_access->AccessInfo.IsWrite);
    
    // Verificar se é acesso a device
    if (gpa >= DEVICE_BASE && gpa < DEVICE_BASE + 0x100000) {
        uint64_t data = 0;
        bool is_write = memory_access->AccessInfo.IsWrite;
        uint32_t size = 1 << memory_access->AccessInfo.AccessSize;  // 0=1byte, 1=2bytes, 2=4bytes, 3=8bytes
        
        if (is_write) {
            // Para writes, obter dados dos registradores
            // Isso requer decodificação da instrução
            data = memory_access->Rax;  // Simplificado - normalmente seria decodificado
        }
        
        device_access_result_t result = handle_device_access(gpa, &data, size, is_write);
        
        if (result == DEVICE_ACCESS_OK) {
            if (!is_write) {
                // Para reads, colocar dados no registrador apropriado
                WHV_REGISTER_NAME reg_name = WHvArm64RegisterX0;  // Simplificado
                WHV_REGISTER_VALUE reg_value;
                reg_value.Reg64 = data;
                vcpu_set_registers(&reg_name, &reg_value, 1);
            }
            
            // Avançar PC
            uint64_t pc;
            if (vcpu_get_pc(&pc) == 0) {
                vcpu_set_pc(pc + 4);  // Assumir instrução de 4 bytes
            }
            
            return 0;
        } else if (result == DEVICE_ACCESS_ERROR) {
            LOG_ERROR("Erro no acesso ao device");
            return -1;
        }
    }
    
    LOG_ERROR("Acesso de memória não tratado: GPA=0x%llX", gpa);
    return -1;
}

int handle_io_port_access(const WHV_X64_IO_PORT_ACCESS_CONTEXT* io_port)
{
    // ARM64 normalmente não usa I/O ports como x86
    // Mas podemos tratar para compatibilidade
    LOG_DEBUG("I/O Port Access: Port=0x%X, Size=%d, Write=%d", 
              io_port->PortNumber, io_port->AccessSize, io_port->IsWrite);
    
    // Mapear I/O ports para memory-mapped devices
    uint64_t mapped_addr = DEVICE_BASE + io_port->PortNumber;
    uint64_t data = io_port->IsWrite ? io_port->Rax : 0;
    
    device_access_result_t result = handle_device_access(mapped_addr, &data, 
                                                        io_port->AccessSize, 
                                                        io_port->IsWrite);
    
    if (result == DEVICE_ACCESS_OK && !io_port->IsWrite) {
        // Para reads, atualizar RAX
        WHV_REGISTER_NAME reg_name = WHvArm64RegisterX0;
        WHV_REGISTER_VALUE reg_value;
        reg_value.Reg64 = data;
        vcpu_set_registers(&reg_name, &reg_value, 1);
    }
    
    return (result == DEVICE_ACCESS_ERROR) ? -1 : 0;
}

int handle_exception(const WHV_VP_EXCEPTION_CONTEXT* exception)
{
    LOG_INFO("Exception: Type=%d, ErrorCode=0x%X", 
             exception->ExceptionType, exception->ErrorCode);
    
    switch (exception->ExceptionType) {
        case WHvArm64ExceptionTypeDataAbortLowerEl:
        case WHvArm64ExceptionTypeDataAbortSameEl:
            LOG_INFO("Data Abort - possivelmente acesso a device");
            // Tentar tratar como acesso a device
            // Isso requer análise mais detalhada da instrução
            break;
            
        case WHvArm64ExceptionTypeInstructionAbortLowerEl:
        case WHvArm64ExceptionTypeInstructionAbortSameEl:
            LOG_INFO("Instruction Abort");
            break;
            
        case WHvArm64ExceptionTypeSystemRegisterTrap:
            LOG_INFO("System Register Trap");
            break;
            
        default:
            LOG_ERROR("Exception não tratada: %d", exception->ExceptionType);
            return -1;
    }
    
    // Para demo, apenas avançar PC
    uint64_t pc;
    if (vcpu_get_pc(&pc) == 0) {
        vcpu_set_pc(pc + 4);
    }
    
    return 0;
}