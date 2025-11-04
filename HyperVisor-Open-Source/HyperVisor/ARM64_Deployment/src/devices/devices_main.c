/* Desenvolvido por: Escanearcpl */
#include "devices.h"

// Global device states
uart_state_t g_uart = {0};
timer_state_t g_timer = {0};
gic_state_t g_gic = {0};

int devices_init(void)
{
    LOG_INFO("Inicializando devices...");
    
    // Initialize UART (PL011)
    g_uart.flag_reg = 0x90;  // TXFE (TX FIFO empty) + RXFE (RX FIFO empty) 
    g_uart.control_reg = 0x300;  // TXE + RXE (TX/RX enabled)
    g_uart.line_control = 0x70;  // 8 bits, FIFO enabled
    g_uart.interrupt_mask = 0;
    g_uart.tx_fifo_full = false;
    g_uart.rx_fifo_empty = true;
    
    // Initialize Timer
    g_timer.counter = 0;
    g_timer.compare_value = 0xFFFFFFFFFFFFFFFF;
    g_timer.control = 0;
    g_timer.interrupt_pending = false;
    
    // Initialize GIC
    g_gic.distributor_ctrl = 0;
    g_gic.cpu_ctrl = 0;
    memset(g_gic.pending_interrupts, 0, sizeof(g_gic.pending_interrupts));
    memset(g_gic.enabled_interrupts, 0, sizeof(g_gic.enabled_interrupts));
    memset(g_gic.priorities, 0, sizeof(g_gic.priorities));
    
    LOG_INFO("Devices inicializados com sucesso");
    return 0;
}

void devices_cleanup(void)
{
    LOG_INFO("Limpeza dos devices concluída");
}

device_access_result_t handle_device_access(uint64_t guest_addr, uint64_t* data, 
                                          uint32_t size, bool is_write)
{
    device_io_t io = {
        .address = guest_addr,
        .data = data ? *data : 0,
        .size = size,
        .is_write = is_write
    };
    
    LOG_DEBUG("Device access: addr=0x%llX, data=0x%llX, size=%d, write=%d",
              guest_addr, io.data, size, is_write);
    
    device_access_result_t result = DEVICE_ACCESS_IGNORE;
    
    // UART range
    if (guest_addr >= UART_BASE && guest_addr < UART_BASE + 0x1000) {
        result = uart_handle_access(&io);
    }
    // Timer range  
    else if (guest_addr >= TIMER_BASE && guest_addr < TIMER_BASE + 0x1000) {
        result = timer_handle_access(&io);
    }
    // GIC Distributor range
    else if (guest_addr >= GIC_DIST_BASE && guest_addr < GIC_DIST_BASE + 0x1000) {
        result = gic_handle_access(&io);
    }
    // GIC CPU Interface range
    else if (guest_addr >= GIC_CPU_BASE && guest_addr < GIC_CPU_BASE + 0x1000) {
        result = gic_handle_access(&io);
    }
    else {
        LOG_DEBUG("Acesso a endereço não mapeado: 0x%llX", guest_addr);
        result = DEVICE_ACCESS_IGNORE;
    }
    
    // Update data for reads
    if (!is_write && result == DEVICE_ACCESS_OK && data) {
        *data = io.data;
    }
    
    return result;
}
