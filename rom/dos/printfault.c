/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 13:52:49  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:40:55  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2(BOOL, PrintFault,

/*  SYNOPSIS */
	__AROS_LHA(LONG,   code,   D1),
	__AROS_LHA(STRPTR, header, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 79, Dos)

/*  FUNCTION
	Prints the header and the text associated with the error code to
	the console (buffered), then sets the value returned by IoErr() to
	the error code given.

    INPUTS
	code   - Error code.
	header - Text to print before the error message.

    RESULT
	!=0 if all went well. 0 on failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct Process *me=(struct Process *)FindTask(NULL);
    BPTR stream=me->pr_CES?me->pr_CES:me->pr_COS;
    struct EString *es=EString;
    BOOL ret=0;

    /* First find error string */
    while(es->Number)
    {
	if(es->Number==code)
	    break;
	es++;
    }

    /* Print everything */
    if(!FPuts(stream,header)&&
       !FPuts(stream,": ")&&
       !FPuts(stream,es->String)&&
       !FPuts(stream,"\n"))
	ret=1;

    /* All done. */
    me->pr_Result2=code;
    return ret;

    __AROS_FUNC_EXIT
} /* PrintFault */
