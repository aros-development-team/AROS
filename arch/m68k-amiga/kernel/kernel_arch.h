/*
 * Machine-specific definitions for Amiga(tm) hardware.
 */

/* Number of IRQs used in the machine. Needed by kernel_base.h */
#define IRQ_COUNT 14

/* We don't need to emulate VBlank, it's real hardware interrupt here */
#define NO_VBLANK_EMU

#define ictl_enable_irq(irq, base)
#define ictl_disable_irq(irq, base)

/* Amiga(tm) custom chips MMIO area */
#define _CUSTOM 0xDFF000
