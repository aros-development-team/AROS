/*
 * Generic support code for software emulated system timer.
 *
 * The main idea is: we have a single timer or arbitrary frequence (eclock),
 * which is used for generating timer interrupts. Its frequency can be higher
 * than reasonable VBlank frequency, so we emulate VBlank by dividing EClock
 * frequency by some integer factor.
 *
 * This code uses two variables in ExecBase, which describe the way in which the machine
 * measures time:
 *
 * VBlankFrequency    - this is the frequency with which exec VBlank handler is called.
 *                      Task scheduler's Quantum is measured in these intervals.
 * ex_EClockFrequency - this is the machine's master clock frequency. It specifies
 *                      how many EClock units is in one second. This value is provided
 *			here to be used by generic software implementation of timer.device,
 *			which simply counts ticks of our system timer. Real hardware
 *			implementations of timer.device are expected to update ex_EClockFrequency
 *			to the unit in which machine's hardware measures time.
 *
 * On native ports this file will likely be replaced by hardware-probing code.
 */

#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/alib.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_intr.h>

#include <stdlib.h>
#include <string.h>

static int Timer_Init(struct KernelBase *KernelBase)
{
    char *args;

    /*
     * This will make EClock frequency to be equal to VBlank by default.
     * Reasonable Amiga(tm)-compatible VBlank is already provided by exec.library.
     */
    SysBase->ex_EClockFrequency = 0;

    /*
     * Since we are software-driven, we can just ask the user which
     * frequencies he wishes to use.
     */
    args = (char *)LibGetTagData(KRN_CmdLine, 0, BootMsg);
    if (args)
    {
    	char *s;

	/* Set VBlank and EClock frequencies if specified */
	s = strstr(args, "vblank=");
	if (s)
	    SysBase->VBlankFrequency = atoi(&s[7]);

	s = strstr(args, "eclock=");
	if (s)
	    SysBase->ex_EClockFrequency = atoi(&s[7]);
    }

    /* By default we enable VBlank emulation */
    KernelBase->kb_VBlankEnable = 1;

    /*
     * Calculate number of timer ticks per VBlank and ensure that it is nonzero.
     * Note that if eclock= argument was not supplied this will set ticks
     * number to 1 (because ex_EClockFrequency is 0 by default).
     */
    KernelBase->kb_VBlankTicks = SysBase->ex_EClockFrequency / SysBase->VBlankFrequency;
    if (!KernelBase->kb_VBlankTicks)
	KernelBase->kb_VBlankTicks = 1;

    /*
     * Calculate fixed up value of timer frequency. We need to set ex_EClockFrequency
     * in order to provide correct information to the software.
     * If timer.device provides better clock, it should update ex_EClockFrequency with
     * the value which reflects its time granularity.
     */
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
