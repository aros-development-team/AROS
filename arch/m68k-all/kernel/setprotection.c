#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_intern.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH3(void, KrnSetProtection,

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

    BOOL invalid = (flags & (MAP_Readable | MAP_Writable | MAP_Executable)) == 0;
    BOOL readonly = (flags & MAP_Writable) == 0;
    BOOL supervisor = (flags & MAP_Supervisor) != 0;
    UBYTE cm = (flags & MAP_CacheInhibit) ? CM_SERIALIZED : ((flags & MAP_WriteThrough) ? CM_WRITETHROUGH : CM_COPYBACK);

    /* We need MAP_Supervisor and MAP_CacheInhibitSerialized */
    
    if ((ULONG)address < 0x00200000 && cm == CM_SERIALIZED)
	cm = CM_NONCACHEABLE; /* Chip RAM does not need to be non-cacheable + serialized, only noncacheable */
    
    map_region(KernelBase, address, NULL, length, invalid, readonly, supervisor, cm);

    AROS_LIBFUNC_EXIT
}
