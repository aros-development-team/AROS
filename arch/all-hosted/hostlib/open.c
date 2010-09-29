#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <proto/hostlib.h>

AROS_LH2(void *, HostLib_Open,

/*  SYNOPSIS */
         AROS_LHA(const char *, filename, A0),
         AROS_LHA(char **,      error,    A1),

/*  LOCATION */
         APTR, HostLibBase, 1, HostLib)

/*  FUNCTION
	Opens a host operating system's shared library.

    INPUTS
	filename - File name of the shared library.
	error    - Optional pointer to a location where error string
		   will be placed (should an error happen). Can be NULL,
		   in this case it is ignored.

    RESULT
	Opaque library handle or NULL in case of some error.

    NOTES
	The exact semantics of library opening depends on host operating
	system.

    EXAMPLE

    BUGS

    SEE ALSO
	HostLib_Close()

    INTERNALS

*****************************************************************************/	 
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is host-specific */
    if (error)
	*error = "Function is not implemented";
    return NULL;

    AROS_LIBFUNC_EXIT
}
