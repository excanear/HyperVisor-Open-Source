#ifndef VM_H
#define VM_H

#include "hypervisor.h"

// VM state structure
typedef struct {
    WHV_PARTITION_HANDLE partition;
    WHV_VPINDEX vpindex;
    void* guest_memory;
    uint64_t guest_memory_size;
    bool running;
} vm_state_t;

// Global VM state
extern vm_state_t g_vm;

// VM management functions
int vm_create(void);
void vm_destroy(void);
int vm_setup_memory(void);
int vm_setup_vcpu(void);
int vm_load_guest_code(const void* code, size_t code_size, uint64_t load_addr);

// vCPU functions  
int vcpu_run(void);
int vcpu_get_registers(WHV_REGISTER_NAME* reg_names, WHV_REGISTER_VALUE* reg_values, UINT32 count);
int vcpu_set_registers(WHV_REGISTER_NAME* reg_names, WHV_REGISTER_VALUE* reg_values, UINT32 count);

// Memory management
int vm_map_gpa_range(uint64_t guest_addr, uint64_t size, WHV_MAP_GPA_RANGE_FLAGS flags);
int vm_read_guest_memory(uint64_t guest_addr, void* buffer, size_t size);
int vm_write_guest_memory(uint64_t guest_addr, const void* buffer, size_t size);

// ARM64 register helpers
int vcpu_get_pc(uint64_t* pc);
int vcpu_set_pc(uint64_t pc);
int vcpu_get_sp(uint64_t* sp);
int vcpu_set_sp(uint64_t sp);

#endif // VM_H