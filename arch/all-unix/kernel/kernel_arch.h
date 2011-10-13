/* Our IRQs are UNIX signals */
#define IRQ_COUNT 32

/* Disable old kernel software timer. Obsolete and will be removed. */
#define NO_VBLANK_EMU

/* We have no interrupt controller */
#define ictl_enable_irq(irq, base)
#define ictl_disable_irq(irq, base)

/* Service functions needed by us */
int krnTimerIRQ(void);
