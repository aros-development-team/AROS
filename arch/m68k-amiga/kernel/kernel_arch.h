/*
 * Machine-specific definitions for Amiga(tm) hardware.
 */

/* Number of IRQs used in the machine. Needed by kernel_base.h */
#define IRQ_COUNT 14

/* We don't need to emulate VBlank, it's real hardware interrupt here */
#define NO_VBLANK_EMU

/*
 * Interrupt controller functions. Actually have the following prototypes:
 *
 * void ictl_enable_irq(uint8_t num);
 * void ictl_disable_irq(uint8_t num);
 */

#define ictl_enable_irq(irq)
#define ictl_disable_irq(irq)

/* Amiga(tm) custom chips MMIO area */
#define _CUSTOM 0xDFF000
