/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, DisplayError,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, formatStr , A1),
	AROS_LHA(ULONG , IDCMPFlags, D0),
	AROS_LHA(APTR  , args      , A2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 0, Intuition)

/*  FUNCTION

    Display an error message via a system requester and get response from
    the user.

    INPUTS

    formatStr   --  The message to display in printf style
    IDCMPFlags  --  Argument to EasyRequestArgs()
    args        --  Arguments to 'formatStr'

    RESULT

    0 if user chose "Retry"
    1 if user chose "Cancel"

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    This function is SetFunction()ed into dos.library by intuition as dos
    is up and running long before any intuition library is opened.

    HISTORY

    23.10.99  SDuvan  --  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Window *window;	/* The window to put up the requester in */

    char   gadTexts[128];	/* Space for localized "Retry|Cancel" */
    char  *gtPtr = (char *)&gadTexts;
    ULONG  idcmp = IDCMPFlags;
    ULONG  res;

    struct EasyStruct es = 
    {
	sizeof(struct EasyStruct),
	0,			/* Flags */
	DosGetString(STRING_REQUESTTITLE), /* "System requester" */
	formatStr,
	&gadTexts		/* "Retry|Cancel" */
    };

    window = (struct Window *)((struct Process *)FindTask(NULL))->pr_WindowPtr;

    /* Supress requesters? */
    if((IPTR)window == (IPTR)-1L)
	return 0;

    /* Create localized "Retry|Cancel" gadget texts. */
    strcpy(gtPtr, DosGetString(STRING_RETRY));

    while(*gtPtr++);

    gtPtr--;
    *gtPtr++ = '|';
    strcpy(gtPtr, DosGetString(STRING_CANCEL));

    res = EasyRequestArgs(window, &es, &idcmp, args);

    if(res == 0)
	return 1;
    else
	return 0;

    AROS_LIBFUNC_EXIT
} /* DisplayError */
