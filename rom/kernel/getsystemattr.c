#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>

#ifndef IRQ_TIMER
#define IRQ_TIMER -1
#endif

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1(intptr_t, KrnGetSystemAttr,

/*  SYNOPSIS */
	AROS_LHA(uint32_t, id, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 29, Kernel)

/*  FUNCTION
	Get value of internal system attributes.
	Currently defined attributes are:

	  KATTR_Architecture [.G] (char *)        - Name of architecture the kernel built for.
	  KATTR_VBlankEnable [SG] (unsigned char) - Enable or disable exec VBlank emulation by kernel.resource.
						    This can be needed for timer.device implementations for
						    systems with only one timer available. In this case timer.device
						    should shut off kernel's VBlank emulation before taking over the
						    timer and drive exec VBlank itself.
	  KATTR_TimerIRQ     [.G] (int)		  - Number of high precision periodic timer IRQ to be used by timer.device.
						    Needed if kernel.resource provides a timer which runs at frequency
						    which is multiple of VBlank. Generic timer.device may use this IRQ
						    for improved accuracy. Frequency of this timer is specified in
						    SysBase->ex_EClockFrequency.

    INPUTS
	id - ID of the attribute to get

    RESULT
	Value of the attribute

    NOTES
	These attributes (except KATTR_Architecture) are of very limited use for end user software.
	They are valid only in certain system states. For example KATTR_TimerPeriod goes meaningless
	if timer.device has taken over the system timer. These attributes are provided for system
	components themselves, so it's better to stay away from them.

    EXAMPLE

    BUGS

    SEE ALSO
	KrnSetSystemAttr()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    switch (id)
    {
    case KATTR_Architecture:
	return (intptr_t)AROS_ARCHITECTURE;

    case KATTR_VBlankEnable:
	return KernelBase->kb_VBlankEnable;

    case KATTR_TimerIRQ:
	return IRQ_TIMER;

    case KATTR_MinStack:
    	return AROS_STACKSIZE;

    default:
	return -1;
    }

    AROS_LIBFUNC_EXIT
}
