/* Machine-specific definitions for IBM PC hardware */

/* IRQs are CPU exceptions vectors starting from 0x20 */
#define IRQ_COUNT (255 - 32)

/* We have several types of IRQ handlers and distinguish them by node type */
#define KBL_INTERNAL 0
#define KBL_XTPIC    1
#define KBL_APIC     2

/* Interrupt controller functions */
void ictl_enable_irq(unsigned char irq, struct KernelBase *KernelBase);

/* Originally we didn't have disable function. Perhaps there was some reason. */
#define ictl_disable_irq(irq, base)
