#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <proto/hostlib.h>

AROS_LH3(IPTR, HostLib_SysCallA,

/*  SYNOPSIS */
        AROS_LHA(IPTR, Number, D0),
	AROS_LHA(ULONG, NArgs, D1),
        AROS_LHA(IPTR *, Args, A0),

/*  LOCATION */
        APTR, HostLibBase, 7, HostLib)

/*  FUNCTION
	Performs system call on the host OS

    INPUTS
	code - System call number
	num  - Number of arguments in the array
	args - Pointer to array of arguments

    RESULT
	Value returned by the actual system call.

    NOTES
	The exact semantics of system calls depends on the host operating system.
	Some systems (like Windows) do not have semantics of system calls at all.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/	 
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is host-specific */
    return -1;

    AROS_LIBFUNC_EXIT
}
