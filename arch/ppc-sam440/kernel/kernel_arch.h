/*
 * Machine-specific definitions.
 *
 * This file needs to be replaced for every machine. Hosted ports
 * may share the same file in arch/all-$(ARCH)/kernel/kernel_arch.h
 *
 * This file is just a sample providing necessary minimum.
 */
#include <asm/amcc440.h>

struct KernelBase;

/* Number of IRQs used in the machine. Needed by kernel_base.h */
#define IRQ_COUNT 256

/*
 * Interrupt controller functions. Actually have the following prototypes:
 *
 * void ictl_enable_irq(uint8_t num);
 * void ictl_disable_irq(uint8_t num);
 */

static inline void ictl_enable_irq(int irq, struct KernelBase *base)
{
    if (irq < 32) {
        wrdcr(UIC0_ER, rddcr(UIC0_ER) | (0x80000000 >> irq));
    } else {
        wrdcr(UIC1_ER, rddcr(UIC1_ER) | (0x80000000 >> (irq - 32)));
        wrdcr(UIC0_ER, rddcr(UIC0_ER) | 0x00000003);
    }
}

static inline void ictl_disable_irq(int irq, struct KernelBase *base)
{
    if (irq < 30) {
        wrdcr(UIC0_ER, rddcr(UIC0_ER) & ~(0x80000000 >> irq));
    } else if (irq > 31) {
        wrdcr(UIC1_ER, rddcr(UIC0_ER) & ~(0x80000000 >> (irq - 32)));
    }
}
