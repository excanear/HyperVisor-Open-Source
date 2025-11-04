/* Desenvolvido por: Escanearcpl */
#ifndef ASM_FUNCTIONS_H
#define ASM_FUNCTIONS_H

#include <stdint.h>

// Estrutura para contexto do guest
typedef struct {
    uint64_t x[31];         // x0-x30
    uint64_t sp_el1;        // Stack pointer EL1
    uint64_t elr_el2;       // Exception Link Register
    uint64_t spsr_el2;      // Saved Program Status Register
    uint64_t esr_el2;       // Exception Syndrome Register
    uint64_t far_el2;       // Fault Address Register
} guest_context_t;

// Estrutura para contexto do hypervisor
typedef struct {
    uint64_t x[31];         // x0-x30
    uint64_t sp;            // Stack pointer
    uint64_t sctlr_el2;     // System Control Register
    uint64_t hcr_el2;       // Hypervisor Configuration Register
    uint64_t vbar_el2;      // Vector Base Address Register
    uint64_t vtcr_el2;      // Virtualization Translation Control Register
    uint64_t vttbr_el2;     // Virtualization Translation Table Base Register
} hypervisor_context_t;

// Funções assembly - entry.s
extern void setup_exception_vectors(void);
extern void enter_guest(uint64_t entry_point, uint64_t stack_pointer);
extern void save_guest_context(guest_context_t* ctx);
extern void restore_guest_context(const guest_context_t* ctx);

// Buffers globais para contexto
extern guest_context_t guest_context_buffer;

// Handlers de exceção implementados em C
void handle_hypervisor_exception(uint32_t exception_type);
void handle_guest_exception(uint32_t exception_type, uint64_t esr, uint64_t far, uint64_t elr);

// Funções específicas para tratamento de exceções
void handle_guest_sync_exception(uint64_t esr, uint64_t far, uint64_t elr);
void handle_guest_hvc(uint32_t iss, uint64_t elr);
void handle_guest_data_abort(uint32_t iss, uint64_t far, uint64_t elr);
void handle_guest_instruction_abort(uint32_t iss, uint64_t far, uint64_t elr);
void handle_guest_system_register_trap(uint32_t iss, uint64_t elr);
void handle_guest_wfi_wfe(uint32_t iss, uint64_t elr);
void handle_guest_irq(void);
void handle_guest_fiq(void);
void handle_guest_serror(uint64_t esr, uint64_t far);
void handle_guest_sync_exception_aarch32(uint64_t esr, uint64_t far, uint64_t elr);
void inject_exception_to_guest(uint64_t esr, uint64_t far);

// Função principal do hypervisor
int hypervisor_main(void);

#endif // ASM_FUNCTIONS_H
