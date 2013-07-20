#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <proto/hostlib.h>

	AROS_LH1(void, HostLib_FreeErrorStr,

/*  SYNOPSIS */
	AROS_LHA(char *, error, A0),

/*  LOCATION */
	APTR, HostLibBase, 4, HostLib)

/*  FUNCTION
	Release error description string.
	In some dynamic library loaders (like in Windows) error strings
	are allocated dynamically. They need to be explicitly freed
	when not used any more.

    INPUTS
	error  - Pointer to a string ro free.

    RESULT
	None.

    NOTES
	For portability sake, it is recommended to call this function on any
	error string returned by hostlib.resource. However you can omit this
	if you exactly know that your code runs only on UNIX-alike operating
	system (it's UNIX-specific library or driver).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Our error strings are static, there's nothing to do here */

    AROS_LIBFUNC_EXIT
}
