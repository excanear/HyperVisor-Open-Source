// Guest simples ARM64 para teste
// Este código será carregado em GUEST_ENTRY_POINT

.section .text
.global _start

_start:
    // Setup stack pointer
    ldr x0, =0x44000000      // Stack top
    mov sp, x0
    
    // Hypercall 0 - Hello
    mov x0, #0               // Hypercall number 0
    hvc #0                   // Trigger hypercall
    
    // Hypercall 2 - Print 'H'
    mov x0, #2               // Hypercall number 2 (print char)
    mov x1, #0x48            // 'H'
    hvc #0
    
    // Hypercall 2 - Print 'i'
    mov x0, #2
    mov x1, #0x69            // 'i'
    hvc #0
    
    // Hypercall 2 - Print '!'
    mov x0, #2
    mov x1, #0x21            // '!'
    hvc #0
    
    // Hypercall 2 - Print newline
    mov x0, #2
    mov x1, #0x0A            // '\n'
    hvc #0
    
    // Hypercall 1 - Shutdown
    mov x0, #1               // Hypercall number 1
    hvc #0
    
    // Infinite loop (should not reach here)
loop:
    wfi                      // Wait for interrupt
    b loop