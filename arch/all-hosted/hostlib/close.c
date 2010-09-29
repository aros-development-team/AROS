#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <proto/hostlib.h>

AROS_LH2(BOOL, HostLib_Close,

/*  SYNOPSIS */
        AROS_LHA(void *,  handle, A0),
        AROS_LHA(char **, error,  A1),

/*  LOCATION */
        APTR, HostLibBase, 2, HostLib)

/*  FUNCTION
	Closes a host operating system's shared library.

    INPUTS
	handle - Library handle provided by HostLib_Open()
	error  - Optional pointer to a location where error string
		 will be placed (should an error happen). Can be NULL,
		 in this case it is ignored.

    RESULT
	TRUE on success or FALSE in case of error.

    NOTES
	The exact semantics of library close depends on host operating
	system.

    EXAMPLE

    BUGS

    SEE ALSO
	HostLib_Open()

    INTERNALS

*****************************************************************************/	 
{
    AROS_LIBFUNC_INIT

    /* The implementation is host-specific */
    if (error)
	*error = "Function is not implemented";
    return FALSE;

    AROS_LIBFUNC_EXIT
}
