/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:22  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2I(BOOL, PrintFault,

/*  SYNOPSIS */
	__AROS_LA(LONG,   code,   D1),
	__AROS_LA(STRPTR, header, D2),

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
