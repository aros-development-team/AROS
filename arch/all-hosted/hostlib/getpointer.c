#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <proto/hostlib.h>

AROS_LH3(void *, HostLib_GetPointer,

/*  SYNOPSIS */
        AROS_LHA(void *,       handle, A0),
        AROS_LHA(const char *, symbol, A1),
        AROS_LHA(char **,      error,  A2),

/*  LOCATION */
        APTR, HostLibBase, 3, HostLib)

/*  FUNCTION
	Resolve a named symbol in host operating system's shared library.

    INPUTS
	handle - Library handle provided by HostLib_Open()
	symbol - Name of the symbol to resolve.
	error  - Optional pointer to a location where error string
		 will be placed (should an error happen). Can be NULL,
		 in this case it is ignored.

    RESULT
	Symbol value or NULL in case of error.

    NOTES
	The exact semantics of this operation depends on host operating
	system. A system with AmigaOS-alike libraries where entry points
	are accessed by offsets, and not by names, would not have this
	function implemented.

	Values obtained with this function are valid as long as the library
	is open.

    EXAMPLE

    BUGS

    SEE ALSO
	HostLib_GetInterface()

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
