#include "devices.h"

device_access_result_t gic_handle_access(const device_io_t* io)
{
    uint64_t base_addr = 0;
    bool is_distributor = false;
    
    // Determinar se é acesso ao Distributor ou CPU Interface
    if (io->address >= GIC_DIST_BASE && io->address < GIC_DIST_BASE + 0x1000) {
        base_addr = GIC_DIST_BASE;
        is_distributor = true;
    } else if (io->address >= GIC_CPU_BASE && io->address < GIC_CPU_BASE + 0x1000) {
        base_addr = GIC_CPU_BASE;
        is_distributor = false;
    } else {
        return DEVICE_ACCESS_IGNORE;
    }
    
    uint64_t offset = io->address - base_addr;
    
    LOG_DEBUG("GIC %s access: offset=0x%llX, data=0x%llX, write=%d", 
              is_distributor ? "DIST" : "CPU", offset, io->data, io->is_write);
    
    if (is_distributor) {
        return gic_handle_distributor_access(offset, io);
    } else {
        return gic_handle_cpu_access(offset, io);
    }
}

device_access_result_t gic_handle_distributor_access(uint64_t offset, const device_io_t* io)
{
    switch (offset) {
        case 0x000:  // GICD_CTLR - Distributor Control Register
            if (io->is_write) {
                g_gic.distributor_ctrl = (uint32_t)io->data;
                LOG_DEBUG("GIC DIST CTRL write: 0x%X", g_gic.distributor_ctrl);
            } else {
                *(uint64_t*)&io->data = g_gic.distributor_ctrl;
            }
            break;
            
        case 0x004:  // GICD_TYPER - Interrupt Controller Type Register
            if (!io->is_write) {
                // Report: 1 CPU, 32 interrupt lines, no security extensions
                *(uint64_t*)&io->data = 0x0000001F;  // 32 interrupts (ITLinesNumber = 0)
                LOG_DEBUG("GIC DIST TYPER read: 0x%llX", io->data);
            }
            break;
            
        case 0x100:  // GICD_ISENABLER0 - Interrupt Set-Enable Register 0
            if (io->is_write) {
                g_gic.enabled_interrupts[0] |= (uint32_t)io->data;
                LOG_DEBUG("GIC DIST Enable IRQs: 0x%X", (uint32_t)io->data);
            } else {
                *(uint64_t*)&io->data = g_gic.enabled_interrupts[0];
            }
            break;
            
        case 0x180:  // GICD_ICENABLER0 - Interrupt Clear-Enable Register 0
            if (io->is_write) {
                g_gic.enabled_interrupts[0] &= ~(uint32_t)io->data;
                LOG_DEBUG("GIC DIST Disable IRQs: 0x%X", (uint32_t)io->data);
            }
            break;
            
        case 0x200:  // GICD_ISPENDR0 - Interrupt Set-Pending Register 0
            if (io->is_write) {
                g_gic.pending_interrupts[0] |= (uint32_t)io->data;
                LOG_DEBUG("GIC DIST Set pending IRQs: 0x%X", (uint32_t)io->data);
            } else {
                *(uint64_t*)&io->data = g_gic.pending_interrupts[0];
            }
            break;
            
        case 0x280:  // GICD_ICPENDR0 - Interrupt Clear-Pending Register 0
            if (io->is_write) {
                g_gic.pending_interrupts[0] &= ~(uint32_t)io->data;
                LOG_DEBUG("GIC DIST Clear pending IRQs: 0x%X", (uint32_t)io->data);
            }
            break;
            
        default:
            // Handle priority registers (0x400-0x4FF)
            if (offset >= 0x400 && offset < 0x500) {
                uint32_t irq = offset - 0x400;
                if (irq < 256) {
                    if (io->is_write) {
                        g_gic.priorities[irq] = (uint32_t)io->data & 0xFF;
                        LOG_DEBUG("GIC DIST Priority[%d] = 0x%X", irq, g_gic.priorities[irq]);
                    } else {
                        *(uint64_t*)&io->data = g_gic.priorities[irq];
                    }
                    break;
                }
            }
            
            LOG_DEBUG("GIC DIST: Registro não implementado offset=0x%llX", offset);
            return DEVICE_ACCESS_IGNORE;
    }
    
    return DEVICE_ACCESS_OK;
}

device_access_result_t gic_handle_cpu_access(uint64_t offset, const device_io_t* io)
{
    switch (offset) {
        case 0x00:  // GICC_CTLR - CPU Interface Control Register
            if (io->is_write) {
                g_gic.cpu_ctrl = (uint32_t)io->data;
                LOG_DEBUG("GIC CPU CTRL write: 0x%X", g_gic.cpu_ctrl);
            } else {
                *(uint64_t*)&io->data = g_gic.cpu_ctrl;
            }
            break;
            
        case 0x04:  // GICC_PMR - Interrupt Priority Mask Register
            if (io->is_write) {
                LOG_DEBUG("GIC CPU PMR write: 0x%llX", io->data);
                // Para simplificar, aceitar qualquer valor
            } else {
                *(uint64_t*)&io->data = 0xFF;  // Allow all priorities
            }
            break;
            
        case 0x0C:  // GICC_IAR - Interrupt Acknowledge Register
            if (!io->is_write) {
                uint32_t irq = gic_get_pending_interrupt();
                *(uint64_t*)&io->data = irq;
                if (irq != 1023) {  // 1023 = spurious interrupt
                    LOG_DEBUG("GIC CPU IAR read: IRQ %d", irq);
                    // Auto-acknowledge
                    gic_ack_interrupt(irq);
                }
            }
            break;
            
        case 0x10:  // GICC_EOIR - End of Interrupt Register
            if (io->is_write) {
                uint32_t irq = (uint32_t)io->data;
                LOG_DEBUG("GIC CPU EOIR write: IRQ %d", irq);
                // End of interrupt processing
                gic_ack_interrupt(irq);
            }
            break;
            
        default:
            LOG_DEBUG("GIC CPU: Registro não implementado offset=0x%llX", offset);
            return DEVICE_ACCESS_IGNORE;
    }
    
    return DEVICE_ACCESS_OK;
}

void gic_set_interrupt(uint32_t irq_num, bool pending)
{
    if (irq_num >= 256) return;
    
    uint32_t reg_idx = irq_num / 32;
    uint32_t bit_idx = irq_num % 32;
    
    if (pending) {
        g_gic.pending_interrupts[reg_idx] |= (1U << bit_idx);
        LOG_DEBUG("GIC: Set IRQ %d pending", irq_num);
    } else {
        g_gic.pending_interrupts[reg_idx] &= ~(1U << bit_idx);
        LOG_DEBUG("GIC: Clear IRQ %d pending", irq_num);
    }
}

uint32_t gic_get_pending_interrupt(void)
{
    // Verificar se GIC está habilitado
    if (!(g_gic.distributor_ctrl & 0x1) || !(g_gic.cpu_ctrl & 0x1)) {
        return 1023;  // Spurious interrupt
    }
    
    // Encontrar primeira interrupção pendente e habilitada
    for (int reg = 0; reg < 8; reg++) {
        uint32_t pending_and_enabled = g_gic.pending_interrupts[reg] & g_gic.enabled_interrupts[reg];
        if (pending_and_enabled) {
            for (int bit = 0; bit < 32; bit++) {
                if (pending_and_enabled & (1U << bit)) {
                    uint32_t irq = reg * 32 + bit;
                    LOG_DEBUG("GIC: Found pending IRQ %d", irq);
                    return irq;
                }
            }
        }
    }
    
    return 1023;  // No pending interrupts
}

void gic_ack_interrupt(uint32_t irq_num)
{
    if (irq_num >= 256 || irq_num == 1023) return;
    
    uint32_t reg_idx = irq_num / 32;
    uint32_t bit_idx = irq_num % 32;
    
    // Clear pending bit
    g_gic.pending_interrupts[reg_idx] &= ~(1U << bit_idx);
    LOG_DEBUG("GIC: Acknowledged IRQ %d", irq_num);
}