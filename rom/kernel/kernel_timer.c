#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_intr.h>

static int Timer_Init(struct KernelBase *KernelBase)
{
    /* Set up timer and VBlank. By default we enable VBlank emulation */
    KernelBase->kb_VBlankEnable = 1;
    /* Calculate timer ticks per VBlank */
    KernelBase->kb_VBlankTicks = SysBase->ex_EClockFrequency / SysBase->VBlankFrequency;
    /* Ensure that number of ticks is integer and greater than zero */
    if (!KernelBase->kb_VBlankTicks)
	KernelBase->kb_VBlankTicks = 1;
    /* Calculate fixed up value of timer frequency */
    SysBase->ex_EClockFrequency = SysBase->VBlankFrequency * KernelBase->kb_VBlankTicks;

    return TRUE;
}

ADD2INITLIB(Timer_Init, 0)

/* Handle periodic timer and drive exec VBlank */
void core_TimerTick()
{
    if (KernelBase->kb_VBlankEnable)
    {
	KernelBase->kb_TimerCount++;
	if (KernelBase->kb_TimerCount == KernelBase->kb_VBlankTicks)
	{
	    core_Cause(INTB_VERTB, 1L << INTB_VERTB);
	    KernelBase->kb_TimerCount = 0;
	}
    }
}
