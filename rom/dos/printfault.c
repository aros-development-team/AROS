/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include "dos_intern.h"

#include <aros/debug.h>


/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, PrintFault,

/*  SYNOPSIS */
	AROS_LHA(LONG,   code,   D1),
	AROS_LHA(STRPTR, header, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 79, Dos)

/*  FUNCTION
	Prints the header and the text associated with the error code to
	the console (buffered), then sets the value returned by IoErr() to
	the error code given.

    INPUTS
	code    --  Error code.
	header  --  Text to print before the error message. This may be NULL
                    in which case only the error message is printed.

    RESULT
	!= 0 if all went well. 0 on failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	IoErr(), Fault(), SetIoErr()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct Process *me = (struct Process *)FindTask(NULL);
/*    BPTR            stream = me->pr_CES ? me->pr_CES : me->pr_COS; */
    BPTR            stream =  me->pr_COS; /* unfortunately AmigaOS programs expect
                                             this to be sent to Output() */
    UBYTE           buffer[80];
    BOOL            ret;

    ASSERT_VALID_PTR(stream);
    ASSERT_VALID_PTR_OR_NULL(header);

    /* Fault() will do all the formatting of the string */
    Fault(code, NULL, buffer, 80);

    if (code == 0)
    {
	ret = DOSTRUE;
    }
    else if (header != NULL)
    {
	if(!FPuts(stream, header) && !FPuts(stream, ": ") &&
	   !FPuts(stream, buffer) && !FPuts(stream, "\n"))
	{
	    ret = DOSTRUE;
	}
	else
	{
	    ret = DOSFALSE;
	}
    }
    else
    {
	if (!FPuts(stream, buffer) && !FPuts(stream,"\n"))
	{
	    ret = DOSTRUE;
	}
	else
	{
	    ret = DOSFALSE;
	}
    }

    /* All done. */
    SetIoErr(code);

    return ret;

    AROS_LIBFUNC_EXIT
} /* PrintFault */
