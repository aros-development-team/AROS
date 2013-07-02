#include "kernel_base.h"
#include "kernel_intern.h"
#include "xtpic.h"

void ictl_enable_irq(unsigned char irq, struct KernelBase *KernelBase)
{
    if (irq < 0x20)
    {
	/* XT-PIC IRQ */
        XTPIC_EnableIRQ(irq, &KernelBase->kb_PlatformData->xtpic_mask);
    }
}
