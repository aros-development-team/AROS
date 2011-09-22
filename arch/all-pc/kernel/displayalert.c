#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/alerts.h>

#include <bootconsole.h>
#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include "alert_arch.h"

AROS_LH2(void, KrnDisplayAlert,
	 AROS_LHA(uint32_t, code, D0),
	 AROS_LHA(const char *, text, A0),
	 struct KernelBase *, KernelBase, 35, Kernel)
{
    AROS_LIBFUNC_INIT

    /* Display the alert */
    krnDisplayAlert(text, KernelBase);

    if ((code & AT_DeadEnd) && (scr_Type != SCR_UNKNOWN))
    {
    	/*
    	 * If we have a framebuffer, the user have seen the alert.
    	 * Unfortunately we have no input yet, so just stop.
    	 */
    	PrintString("\nSystem halted. Reset the machine.", KernelBase);
    	for(;;);
    }

    /* Recoverable alerts don't halt the machine. They are just dropped to debug log. */

    AROS_LIBFUNC_EXIT
}
