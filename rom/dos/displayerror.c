/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/intuition.h>
#include "dos_intern.h"
#include <aros/debug.h>
#include <string.h>

/*****i***********************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, DisplayError,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, formatStr, A0),
	AROS_LHA(ULONG       , flags    , D0),
	AROS_LHA(APTR        , args     , A1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 81, Dos)

/*  FUNCTION
	Displays an error message to and gets response from a user.

    INPUTS
	formatStr   --  printf-style formatted string
	flags       --  arguments to EasyRequestArgs()
	args        --  arguments to 'formatStr'

    RESULT
	Nothing

    NOTES
	This is a PRIVATE dos function.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	The purpose of this function is to put up a requester when an error
	has occurred that is connected to the filesystem.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntuitionBase *IntuitionBase = NULL;
    struct Window *window;	/* The window to put the requester in */
    char	   gadTexts[128];
    char          *gtPtr = (char *)gadTexts;
    ULONG	   idcmp = flags;
    ULONG	   res;

    struct EasyStruct es =
    {
	sizeof(struct EasyStruct),
	0,			    	    /* flags */
	DosGetString(STRING_REQUESTTITLE),  /* "System Requester" */
	(STRPTR)formatStr,
	gadTexts			    /* "Retry|Cancel" */
    };

    window = (struct Window *)((struct Process *)FindTask(NULL))->pr_WindowPtr;

    /* Supress requesters? */
    if ((IPTR)window == (IPTR)-1L)
    {
	return 0;
    }

    if (DOSBase->dl_IntuitionBase == NULL)
    {
	DOSBase->dl_IntuitionBase = OpenLibrary("intuition.library", 37L);
    }

    if (DOSBase->dl_IntuitionBase == NULL)
    {
	/* XXX Do something else here please */
    }
    else
    {
	IntuitionBase = (struct IntuitionBase *)DOSBase->dl_IntuitionBase;
    }

    /* Create localised gadget texts */
    strcpy(gtPtr, DosGetString(STRING_RETRY));
    strcat(gtPtr, "|");
    strcat(gtPtr, DosGetString(STRING_CANCEL));

    res = EasyRequestArgs(window, &es, &idcmp, args);

    if (res == 0)
    {
	return 1;
    }
    else
    {
	return 0;
    }

    AROS_LIBFUNC_EXIT
} /* DisplayError */
