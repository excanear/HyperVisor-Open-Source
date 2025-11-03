#ifndef HYPERVISOR_H
#define HYPERVISOR_H

#include <windows.h>
#include <winhvplatform.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Logging macros
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Constants
#define GUEST_RAM_SIZE      (64 * 1024 * 1024)  // 64MB
#define GUEST_RAM_BASE      0x40000000           // ARM64 typical RAM base
#define GUEST_ENTRY_POINT   0x40001000           // Entry point offset

// ARM64 specific constants
#define ARM64_PAGE_SIZE     4096
#define ARM64_PAGE_MASK     (ARM64_PAGE_SIZE - 1)

// Device memory map (ARM64 typical layout)
#define DEVICE_BASE         0x09000000
#define UART_BASE           (DEVICE_BASE + 0x00000000)
#define TIMER_BASE          (DEVICE_BASE + 0x00010000) 
#define GIC_DIST_BASE       (DEVICE_BASE + 0x00020000)
#define GIC_CPU_BASE        (DEVICE_BASE + 0x00030000)

// UART registers (PL011)
#define UART_DR             0x000
#define UART_FR             0x018
#define UART_IBRD           0x024
#define UART_FBRD           0x028
#define UART_LCR_H          0x02C
#define UART_CR             0x030
#define UART_IMSC           0x038
#define UART_ICR            0x044

// Exit codes
#define EXIT_SUCCESS        0
#define EXIT_INIT_FAILED    1
#define EXIT_VM_FAILED      2
#define EXIT_RUN_FAILED     3

// Function declarations
int hypervisor_init(void);
void hypervisor_cleanup(void);
int run_guest(void);

#endif // HYPERVISOR_H