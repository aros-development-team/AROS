#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_intern.h>

#include <sys/mman.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH3I(void, KrnSetProtection,

/*  SYNOPSIS */
	AROS_LHA(void *, address, A0),
	AROS_LHA(uint32_t, length, D0),
        AROS_LHA(KRN_MapAttr, flags, D1),

/*  LOCATION */
	struct KernelBase *, KernelBase, 21, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    int flags_unix = 0;
    
    if (flags & MAP_Readable)
	flags_unix |= PROT_READ;
    if (flags & MAP_Writable)
	flags_unix |= PROT_WRITE;
    if (flags & MAP_Executable)
	flags_unix |= PROT_EXEC;

    KernelIFace.mprotect(address, length, flags_unix);

    AROS_LIBFUNC_EXIT
}
