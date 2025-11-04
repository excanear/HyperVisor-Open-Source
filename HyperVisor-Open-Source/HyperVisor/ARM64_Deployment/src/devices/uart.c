/* Desenvolvido por: Escanearcpl */
#include "devices.h"

device_access_result_t uart_handle_access(const device_io_t* io)
{
    uint64_t offset = io->address - UART_BASE;
    
    LOG_DEBUG("UART access: offset=0x%llX, data=0x%llX, write=%d", 
              offset, io->data, io->is_write);
    
    switch (offset) {
        case UART_DR:  // Data Register
            if (io->is_write) {
                char c = (char)(io->data & 0xFF);
                uart_write_char(c);
                LOG_INFO("UART TX: '%c' (0x%02X)", c, c);
            } else {
                // Read - return received character
                char c = uart_read_char();
                *(uint64_t*)&io->data = (uint64_t)c;
                LOG_DEBUG("UART RX: '%c' (0x%02X)", c, c);
            }
            break;
            
        case UART_FR:  // Flag Register
            if (!io->is_write) {
                uint32_t flags = 0;
                if (g_uart.tx_fifo_full) flags |= 0x20;    // TXFF
                if (!g_uart.tx_fifo_full) flags |= 0x80;   // TXFE  
                if (g_uart.rx_fifo_empty) flags |= 0x10;   // RXFE
                if (!g_uart.rx_fifo_empty) flags |= 0x40;  // RXFF
                
                g_uart.flag_reg = flags;
                *(uint64_t*)&io->data = g_uart.flag_reg;
                LOG_DEBUG("UART FR read: 0x%X", g_uart.flag_reg);
            }
            break;
            
        case UART_IBRD:  // Integer Baud Rate Divisor
            if (io->is_write) {
                // Ignore baud rate settings for simplicity
                LOG_DEBUG("UART IBRD write: 0x%llX (ignored)", io->data);
            } else {
                *(uint64_t*)&io->data = 0x1;  // Default value
            }
            break;
            
        case UART_FBRD:  // Fractional Baud Rate Divisor
            if (io->is_write) {
                LOG_DEBUG("UART FBRD write: 0x%llX (ignored)", io->data);
            } else {
                *(uint64_t*)&io->data = 0x0;
            }
            break;
            
        case UART_LCR_H:  // Line Control Register
            if (io->is_write) {
                g_uart.line_control = (uint32_t)io->data;
                LOG_DEBUG("UART LCR_H write: 0x%X", g_uart.line_control);
            } else {
                *(uint64_t*)&io->data = g_uart.line_control;
            }
            break;
            
        case UART_CR:  // Control Register
            if (io->is_write) {
                g_uart.control_reg = (uint32_t)io->data;
                LOG_DEBUG("UART CR write: 0x%X", g_uart.control_reg);
            } else {
                *(uint64_t*)&io->data = g_uart.control_reg;
            }
            break;
            
        case UART_IMSC:  // Interrupt Mask Set/Clear
            if (io->is_write) {
                g_uart.interrupt_mask = (uint32_t)io->data;
                LOG_DEBUG("UART IMSC write: 0x%X", g_uart.interrupt_mask);
            } else {
                *(uint64_t*)&io->data = g_uart.interrupt_mask;
            }
            break;
            
        case UART_ICR:  // Interrupt Clear Register
            if (io->is_write) {
                // Clear specified interrupts
                LOG_DEBUG("UART ICR write: 0x%llX", io->data);
                // For simplicity, clear all interrupts
            }
            break;
            
        default:
            LOG_DEBUG("UART: Registro não implementado offset=0x%llX", offset);
            return DEVICE_ACCESS_IGNORE;
    }
    
    return DEVICE_ACCESS_OK;
}

void uart_write_char(char c)
{
    // Para demo, apenas print no console host
    printf("%c", c);
    fflush(stdout);
    
    // Simular UART TX
    g_uart.tx_fifo_full = false;  // Always ready for next char
}

char uart_read_char(void)
{
    // Para demo, retornar caractere fixo ou do buffer
    // Em implementação real, isso viria de input do host
    static char demo_input[] = "Hello from UART!\n";
    static int input_pos = 0;
    
    if (input_pos < strlen(demo_input)) {
        char c = demo_input[input_pos++];
        g_uart.rx_fifo_empty = (input_pos >= strlen(demo_input));
        return c;
    }
    
    g_uart.rx_fifo_empty = true;
    return 0;
}

bool uart_has_pending_rx(void)
{
    return !g_uart.rx_fifo_empty;
}
