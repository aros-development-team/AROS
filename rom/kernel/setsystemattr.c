#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(int, KrnSetSystemAttr,

/*  SYNOPSIS */
	AROS_LHA(uint32_t, id, D0),
	AROS_LHA(intptr_t, val, D1),

/*  LOCATION */
	struct KernelBase *, KernelBase, 30, Kernel)

/*  FUNCTION
	Set values of internal system attributes

    INPUTS
	id  - ID of the attribute to set. See KrnGetAttr() description for the
	      list of attributes and their meaning.
	val - New value of the attribute

    RESULT
	Zero for success and nonzero for failure (unknown or read-only attribute or
	bad value).

    NOTES
	Kernel attributes control various aspects of kernel's low-level functions.
	The exact semantics of each attribute may vary from system to system.
	Not all systems support all attributes.

	Please do not alter these attributes from within user software, this will
	not do any good things. This function is provided mainly for use by
	other system components which exactly know what they do.

    EXAMPLE

    BUGS

    SEE ALSO
	KrnGetSystemAttr()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    switch (id)
    {
#ifndef NO_VBLANK_EMU
    case KATTR_VBlankEnable:
	KernelBase->kb_VBlankEnable = val;
	break;
#endif

    default:
	return -1;
    }

    return 0;

    AROS_LIBFUNC_EXIT
}
