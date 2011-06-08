/* Our IRQs are UNIX signals */
#define IRQ_COUNT 32
/*
 * A little hack to get correct SIGALRM number.
 * This definition is used by base code in rom/kernel. We can't simply #define it
 * to SIGALRM because this would require including host OS headers and we want to
 * avoid it in the base code.
 * So we #define it to a small subroutine implemented in UNIX-specific part. This
 * subroutine simply returns SIGALRM number.
 */
#define IRQ_TIMER krnTimerIRQ()

/* UNIX virtualizer needs large stack. Signal handling demands it. */
#undef AROS_STACKSIZE
#define AROS_STACKSIZE 40960

/* We have no interrupt controller */
#define ictl_enable_irq(irq, base)
#define ictl_disable_irq(irq, base)

/* Service functions needed by us */
int krnTimerIRQ(void);
