#include <hardware/intbits.h>

#include <kernel_base.h>
#include <kernel_scheduler.h>

/* Handle periodic timer and drive exec VBlank */
void core_TimerTick()
{
    if (KernelBase->kb_VBlankEnable)
    {
	KernelBase->kb_TimerCount++;
	if (KernelBase->kb_TimerCount == KernelBase->kb_VBlankTicks)
	{
	    core_Cause(INTB_VERTB);
	    KernelBase->kb_TimerCount = 0;
	}
    }
}
