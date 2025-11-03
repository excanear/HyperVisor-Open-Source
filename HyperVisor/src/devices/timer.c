#include "devices.h"

device_access_result_t timer_handle_access(const device_io_t* io)
{
    uint64_t offset = io->address - TIMER_BASE;
    
    LOG_DEBUG("Timer access: offset=0x%llX, data=0x%llX, write=%d", 
              offset, io->data, io->is_write);
    
    switch (offset) {
        case 0x00:  // Timer Control Register
            if (io->is_write) {
                g_timer.control = (uint32_t)io->data;
                LOG_DEBUG("Timer control write: 0x%X", g_timer.control);
                
                // Bit 0: Timer enabled
                if (g_timer.control & 0x1) {
                    LOG_INFO("Timer habilitado");
                } else {
                    LOG_INFO("Timer desabilitado");
                }
            } else {
                *(uint64_t*)&io->data = g_timer.control;
            }
            break;
            
        case 0x04:  // Timer Counter Register (lower 32 bits)
            if (io->is_write) {
                g_timer.counter = (g_timer.counter & 0xFFFFFFFF00000000ULL) | (io->data & 0xFFFFFFFF);
                LOG_DEBUG("Timer counter low write: 0x%llX", io->data);
            } else {
                *(uint64_t*)&io->data = g_timer.counter & 0xFFFFFFFF;
            }
            break;
            
        case 0x08:  // Timer Counter Register (upper 32 bits)
            if (io->is_write) {
                g_timer.counter = (g_timer.counter & 0x00000000FFFFFFFFULL) | ((io->data & 0xFFFFFFFF) << 32);
                LOG_DEBUG("Timer counter high write: 0x%llX", io->data);
            } else {
                *(uint64_t*)&io->data = (g_timer.counter >> 32) & 0xFFFFFFFF;
            }
            break;
            
        case 0x0C:  // Timer Compare Register (lower 32 bits)
            if (io->is_write) {
                g_timer.compare_value = (g_timer.compare_value & 0xFFFFFFFF00000000ULL) | (io->data & 0xFFFFFFFF);
                LOG_DEBUG("Timer compare low write: 0x%llX", io->data);
            } else {
                *(uint64_t*)&io->data = g_timer.compare_value & 0xFFFFFFFF;
            }
            break;
            
        case 0x10:  // Timer Compare Register (upper 32 bits)
            if (io->is_write) {
                g_timer.compare_value = (g_timer.compare_value & 0x00000000FFFFFFFFULL) | ((io->data & 0xFFFFFFFF) << 32);
                LOG_DEBUG("Timer compare high write: 0x%llX", io->data);
            } else {
                *(uint64_t*)&io->data = (g_timer.compare_value >> 32) & 0xFFFFFFFF;
            }
            break;
            
        case 0x14:  // Timer Status Register
            if (!io->is_write) {
                uint32_t status = 0;
                if (g_timer.interrupt_pending) {
                    status |= 0x1;  // Interrupt pending bit
                }
                *(uint64_t*)&io->data = status;
                LOG_DEBUG("Timer status read: 0x%X", status);
            }
            break;
            
        case 0x18:  // Timer Interrupt Clear Register
            if (io->is_write) {
                if (io->data & 0x1) {
                    g_timer.interrupt_pending = false;
                    LOG_DEBUG("Timer interrupt cleared");
                }
            }
            break;
            
        default:
            LOG_DEBUG("Timer: Registro não implementado offset=0x%llX", offset);
            return DEVICE_ACCESS_IGNORE;
    }
    
    return DEVICE_ACCESS_OK;
}

void timer_tick(void)
{
    // Simular tick do timer
    if (g_timer.control & 0x1) {  // Timer enabled
        g_timer.counter++;
        
        // Verificar se atingiu valor de comparação
        if (g_timer.counter >= g_timer.compare_value) {
            g_timer.interrupt_pending = true;
            LOG_DEBUG("Timer interrupt triggered at counter=0x%llX", g_timer.counter);
            
            // Trigger interrupt via GIC
            gic_set_interrupt(30, true);  // Timer interrupt (IRQ 30)
        }
    }
}

bool timer_has_interrupt(void)
{
    return g_timer.interrupt_pending;
}

void timer_clear_interrupt(void)
{
    g_timer.interrupt_pending = false;
    gic_set_interrupt(30, false);
}