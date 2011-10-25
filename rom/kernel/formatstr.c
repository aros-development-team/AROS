#include <aros/kernel.h>
#include <aros/libcall.h>

#include <stdarg.h>
#include <stdio.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH3(int, KrnFormatStr,

/*  SYNOPSIS */
	AROS_LHA(void *, putch, A0),
	AROS_LHA(const char *, format, A1),
        AROS_LHA(va_list, args, A2),

/*  LOCATION */
        APTR, KernelBase, 30, Kernel)

/*  FUNCTION
	Format a string using C printf() convention, using 'putch'
	as character output function.

    INPUTS
    	putch  - A pointer to the output function
	format - A format string
	args   - A list of arguments

	A character output function needs to be declared as:
	
	int myPutCh(int char, void *KernelBase)

	It is expected to return 1 on success and 0 on failure.

    RESULT
	Number of succesfully printed characters

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return __vcformat(KernelBase, putch, format, args);

    AROS_LIBFUNC_EXIT
}
