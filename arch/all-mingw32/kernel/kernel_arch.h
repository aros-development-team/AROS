/*
 * In Windows-hosted kernel IRQs are used to receive events from emulated
 * hardware. Hardware is mostly emulated using Windows threads running
 * asynchronously to AROS. When the thread finishes its job it calls host-side
 * KrnCauseIRQ() function in order to initiate an IRQ in AROS.
 *
 * IRQs are managed dynamically using host-side KrnAllocIRQ() and KrnFreeIRQ() functions
 * except the following static allocations:
 *
 * IRQ 0 - main system periodic timer (50 Hz). Used internally by kernel.resource
 *         for task switching and VBlank emulation. Exec uses it as a VBLANK source.
 *         In current implementation it can not be caused manually using KrnCauseIRQ().
 *
 * The whole described thing is experimental and subject to change.
 */
#define IRQ_TIMER 0

/* We use only one list for all IRQs */
#define IRQ_COUNT 1

/* We have no interrupt controller */
#define ictl_enable_irq(irq, base)
#define ictl_disable_irq(irq, base)
