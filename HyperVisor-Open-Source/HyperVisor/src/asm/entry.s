@ Desenvolvido por: Escanearcpl
# ARM64 Assembly para entrada do hypervisor e manipulação de contexto
# Arquivo: entry.s

.section .text
.align 4

# Tabela de vetores de exceção ARM64
.global exception_vector_table
exception_vector_table:
    # Current EL with SP0
    .align 7
    b sync_exception_current_el_sp0
    .align 7
    b irq_exception_current_el_sp0
    .align 7
    b fiq_exception_current_el_sp0
    .align 7
    b serror_exception_current_el_sp0

    # Current EL with SPx
    .align 7
    b sync_exception_current_el_spx
    .align 7
    b irq_exception_current_el_spx
    .align 7
    b fiq_exception_current_el_spx
    .align 7
    b serror_exception_current_el_spx

    # Lower EL using AArch64
    .align 7
    b sync_exception_lower_el_aarch64
    .align 7
    b irq_exception_lower_el_aarch64
    .align 7
    b fiq_exception_lower_el_aarch64
    .align 7
    b serror_exception_lower_el_aarch64

    # Lower EL using AArch32
    .align 7
    b sync_exception_lower_el_aarch32
    .align 7
    b irq_exception_lower_el_aarch32
    .align 7
    b fiq_exception_lower_el_aarch32
    .align 7
    b serror_exception_lower_el_aarch32

# Macro para salvar registradores
.macro save_registers
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x28, x29, [sp, #-16]!
    str x30, [sp, #-8]!
.endm

# Macro para restaurar registradores
.macro restore_registers
    ldr x30, [sp], #8
    ldp x28, x29, [sp], #16
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x18, x19, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
.endm

# Handlers de exceção
sync_exception_current_el_sp0:
    save_registers
    mov x0, #0  # Exception type
    bl handle_hypervisor_exception
    restore_registers
    eret

irq_exception_current_el_sp0:
    save_registers
    mov x0, #1  # IRQ type
    bl handle_hypervisor_exception
    restore_registers
    eret

fiq_exception_current_el_sp0:
    save_registers
    mov x0, #2  # FIQ type
    bl handle_hypervisor_exception
    restore_registers
    eret

serror_exception_current_el_sp0:
    save_registers
    mov x0, #3  # SError type
    bl handle_hypervisor_exception
    restore_registers
    eret

sync_exception_current_el_spx:
    save_registers
    mov x0, #4  # Sync current EL SPx
    bl handle_hypervisor_exception
    restore_registers
    eret

irq_exception_current_el_spx:
    save_registers
    mov x0, #5  # IRQ current EL SPx
    bl handle_hypervisor_exception
    restore_registers
    eret

fiq_exception_current_el_spx:
    save_registers
    mov x0, #6  # FIQ current EL SPx
    bl handle_hypervisor_exception
    restore_registers
    eret

serror_exception_current_el_spx:
    save_registers
    mov x0, #7  # SError current EL SPx
    bl handle_hypervisor_exception
    restore_registers
    eret

# Handlers para guest (Lower EL)
sync_exception_lower_el_aarch64:
    save_registers
    mov x0, #8  # Sync from guest
    mrs x1, esr_el2     # Exception Syndrome Register
    mrs x2, far_el2     # Fault Address Register
    mrs x3, elr_el2     # Exception Link Register
    bl handle_guest_exception
    restore_registers
    eret

irq_exception_lower_el_aarch64:
    save_registers
    mov x0, #9  # IRQ from guest
    bl handle_guest_exception
    restore_registers
    eret

fiq_exception_lower_el_aarch64:
    save_registers
    mov x0, #10  # FIQ from guest
    bl handle_guest_exception
    restore_registers
    eret

serror_exception_lower_el_aarch64:
    save_registers
    mov x0, #11  # SError from guest
    bl handle_guest_exception
    restore_registers
    eret

sync_exception_lower_el_aarch32:
    save_registers
    mov x0, #12  # Sync from AArch32 guest
    bl handle_guest_exception
    restore_registers
    eret

irq_exception_lower_el_aarch32:
    save_registers
    mov x0, #13  # IRQ from AArch32 guest
    bl handle_guest_exception
    restore_registers
    eret

fiq_exception_lower_el_aarch32:
    save_registers
    mov x0, #14  # FIQ from AArch32 guest
    bl handle_guest_exception
    restore_registers
    eret

serror_exception_lower_el_aarch32:
    save_registers
    mov x0, #15  # SError from AArch32 guest
    bl handle_guest_exception
    restore_registers
    eret

# Função para configurar VBAR_EL2 (Vector Base Address Register)
.global setup_exception_vectors
setup_exception_vectors:
    adrp x0, exception_vector_table
    add x0, x0, :lo12:exception_vector_table
    msr vbar_el2, x0
    isb
    ret

# Função para entrar no guest (EL1)
.global enter_guest
enter_guest:
    # x0 = guest entry point
    # x1 = guest stack pointer
    
    # Configurar ELR_EL2 para apontar para o guest
    msr elr_el2, x0
    
    # Configurar SPSR_EL2 para EL1h mode
    mov x2, #0x5    # EL1h, interrupts masked
    msr spsr_el2, x2
    
    # Configurar SP_EL1
    msr sp_el1, x1
    
    # Limpar registradores do guest
    mov x0, #0
    mov x1, #0
    mov x2, #0
    mov x3, #0
    mov x4, #0
    mov x5, #0
    mov x6, #0
    mov x7, #0
    mov x8, #0
    mov x9, #0
    mov x10, #0
    mov x11, #0
    mov x12, #0
    mov x13, #0
    mov x14, #0
    mov x15, #0
    mov x16, #0
    mov x17, #0
    mov x18, #0
    mov x19, #0
    mov x20, #0
    mov x21, #0
    mov x22, #0
    mov x23, #0
    mov x24, #0
    mov x25, #0
    mov x26, #0
    mov x27, #0
    mov x28, #0
    mov x29, #0
    mov x30, #0
    
    # Entrar no guest
    eret

# Função para salvar contexto do guest
.global save_guest_context
save_guest_context:
    # x0 = ponteiro para estrutura de contexto
    
    # Salvar registradores gerais
    stp x1, x2, [x0, #8]
    stp x3, x4, [x0, #24]
    stp x5, x6, [x0, #40]
    stp x7, x8, [x0, #56]
    stp x9, x10, [x0, #72]
    stp x11, x12, [x0, #88]
    stp x13, x14, [x0, #104]
    stp x15, x16, [x0, #120]
    stp x17, x18, [x0, #136]
    stp x19, x20, [x0, #152]
    stp x21, x22, [x0, #168]
    stp x23, x24, [x0, #184]
    stp x25, x26, [x0, #200]
    stp x27, x28, [x0, #216]
    stp x29, x30, [x0, #232]
    
    # Salvar registradores de sistema
    mrs x1, sp_el1
    str x1, [x0, #248]
    mrs x1, elr_el2
    str x1, [x0, #256]
    mrs x1, spsr_el2
    str x1, [x0, #264]
    mrs x1, esr_el2
    str x1, [x0, #272]
    mrs x1, far_el2
    str x1, [x0, #280]
    
    ret

# Função para restaurar contexto do guest
.global restore_guest_context
restore_guest_context:
    # x0 = ponteiro para estrutura de contexto
    
    # Restaurar registradores de sistema
    ldr x1, [x0, #248]
    msr sp_el1, x1
    ldr x1, [x0, #256]
    msr elr_el2, x1
    ldr x1, [x0, #264]
    msr spsr_el2, x1
    
    # Restaurar registradores gerais
    ldp x1, x2, [x0, #8]
    ldp x3, x4, [x0, #24]
    ldp x5, x6, [x0, #40]
    ldp x7, x8, [x0, #56]
    ldp x9, x10, [x0, #72]
    ldp x11, x12, [x0, #88]
    ldp x13, x14, [x0, #104]
    ldp x15, x16, [x0, #120]
    ldp x17, x18, [x0, #136]
    ldp x19, x20, [x0, #152]
    ldp x21, x22, [x0, #168]
    ldp x23, x24, [x0, #184]
    ldp x25, x26, [x0, #200]
    ldp x27, x28, [x0, #216]
    ldp x29, x30, [x0, #232]
    
    ret
