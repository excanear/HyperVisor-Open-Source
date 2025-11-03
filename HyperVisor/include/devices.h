#ifndef DEVICES_H
#define DEVICES_H

#include "hypervisor.h"

// Device access result
typedef enum {
    DEVICE_ACCESS_OK,
    DEVICE_ACCESS_IGNORE,
    DEVICE_ACCESS_ERROR
} device_access_result_t;

// Device I/O structure
typedef struct {
    uint64_t address;
    uint64_t data;
    uint32_t size;
    bool is_write;
} device_io_t;

// UART device state
typedef struct {
    uint32_t data_reg;
    uint32_t flag_reg;
    uint32_t control_reg;
    uint32_t line_control;
    uint32_t interrupt_mask;
    bool tx_fifo_full;
    bool rx_fifo_empty;
} uart_state_t;

// Timer device state
typedef struct {
    uint64_t counter;
    uint64_t compare_value;
    uint32_t control;
    bool interrupt_pending;
} timer_state_t;

// GIC (interrupt controller) state
typedef struct {
    uint32_t distributor_ctrl;
    uint32_t cpu_ctrl;
    uint32_t pending_interrupts[8];  // Support up to 256 interrupts
    uint32_t enabled_interrupts[8];
    uint32_t priorities[256];
} gic_state_t;

// Global device states
extern uart_state_t g_uart;
extern timer_state_t g_timer;
extern gic_state_t g_gic;

// Device initialization
int devices_init(void);
void devices_cleanup(void);

// UART functions
device_access_result_t uart_handle_access(const device_io_t* io);
void uart_write_char(char c);
char uart_read_char(void);
bool uart_has_pending_rx(void);

// Timer functions
device_access_result_t timer_handle_access(const device_io_t* io);
void timer_tick(void);
bool timer_has_interrupt(void);
void timer_clear_interrupt(void);

// GIC functions
device_access_result_t gic_handle_access(const device_io_t* io);
device_access_result_t gic_handle_distributor_access(uint64_t offset, const device_io_t* io);
device_access_result_t gic_handle_cpu_access(uint64_t offset, const device_io_t* io);
void gic_set_interrupt(uint32_t irq_num, bool pending);
uint32_t gic_get_pending_interrupt(void);
void gic_ack_interrupt(uint32_t irq_num);

// Main device dispatcher
device_access_result_t handle_device_access(uint64_t guest_addr, uint64_t* data, 
                                          uint32_t size, bool is_write);

#endif // DEVICES_H