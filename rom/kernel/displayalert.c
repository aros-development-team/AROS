#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(void, KrnDisplayAlert,

/*  SYNOPSIS */
	AROS_LHA(uint32_t, code, D0),
	AROS_LHA(const char *, text, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 35, Kernel)

/*  FUNCTION
	Inform the user about critical system failure.

    INPUTS
    	code - Corresponding alert code.
	text - A NULL-terminated text to print out.

	First three lines are assumed to be a header. Some implementations
	may print them centered inside a frame.

    RESULT
	None. This function is not guaranteed to return.

    NOTES
	This function exists for system internal purposes. Please do not
	call it from within regular applications! In 99% of cases this function
	will halt or reboot the machine. Certain structures in RAM, as well as
	video hardware state, will be irreversibly destroyed.

	'code' parameter is passed for convenience. Based on it, the system
	can make a decision to log the alert in debug output and continue,
	instead of displaying a message and halting.

	This function is currently experimental. Its definition may change.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * This is a generic version that simply formats the text into debug log.
     * It can be replaced with machine-specific implementation which can do more.
     */
    krnDisplayAlert(text, KernelBase);

    AROS_LIBFUNC_EXIT
}
