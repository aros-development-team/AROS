/* Machine-specific definitions for IBM PC hardware */

/* Currently we support only XT-PIC IRQs */
#define IRQ_COUNT 16

/* Interrupt controller functions */
void ictl_enable_irq(unsigned char irq, struct KernelBase *KernelBase);

/* Originally we didn't have disable function. Perhaps there was some reason. */
#define ictl_disable_irq(irq, base)
