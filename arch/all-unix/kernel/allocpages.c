#include <kernel_base.h>
#include <kernel_intern.h>

#include <sys/mman.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(void *, KrnAllocPages,

/*  SYNOPSIS */
	AROS_LHA(uint32_t, length, D0),
	AROS_LHA(KRN_MapAttr, flags, D1),

/*  LOCATION */
	struct KernelBase *, KernelBase, 27, Kernel)

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
    void *map = 0;
    
    if (flags & MAP_Readable)
	flags_unix |= PROT_READ;
    if (flags & MAP_Writable)
	flags_unix |= PROT_WRITE;
    if (flags & MAP_Executable)
	flags_unix |= PROT_EXEC;

    map = KernelIFace.mmap(NULL, length, flags_unix, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    /* Just a reserved function for now */
    return map;

    AROS_LIBFUNC_EXIT
}
