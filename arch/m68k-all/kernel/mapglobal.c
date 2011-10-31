#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_intern.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH4(int, KrnMapGlobal,

/*  SYNOPSIS */
	AROS_LHA(void *, virtual, A0),
	AROS_LHA(void *, physical, A1),
	AROS_LHA(uint32_t, length, D0),
	AROS_LHA(KRN_MapAttr, flags, D1),

/*  LOCATION */
	struct KernelBase *, KernelBase, 16, Kernel)

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

    BOOL invalid = (flags & (MAP_Readable | MAP_Writable | MAP_Executable)) == 0;
    BOOL readonly = (flags & MAP_Writable) == 0;
    BOOL supervisor = (flags & MAP_Supervisor) != 0;
    UBYTE cm = (flags & MAP_CacheInhibit) ? CM_SERIALIZED : ((flags & MAP_WriteThrough) ? CM_WRITETHROUGH : CM_COPYBACK);

    return map_region(KernelBase, virtual, physical, length, invalid, readonly, supervisor, cm);

    AROS_LIBFUNC_EXIT
}
