/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, DisplayError,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, formatStr, A0),
	AROS_LHA(ULONG , flags    , D0),
	AROS_LHA(APTR  , args     , A1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 81, Dos)

/*  FUNCTION

    Displays an error message to and gets response from a user.

    INPUTS

    formatStr   --  printf-style formatted string
    flags       --  this is unimportant in the dos version of this function,
                    see INTERNALS below
    args        --  arguments to 'formatStr'

    RESULT

    Nothing

    NOTES

    This is a PRIVATE dos function.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    The purpose of this function is to put up a requester when an error has
    occurred that is connected to the filesystem. As dos is started before
    intuition, dos knows nothing about any requesters. Therefore, intuition
    has to setfunction this function when it's opened.
        This function may, however, have an implementation that is platform
    specific where the string is written out without intuition functions for
    startup error message displaying possibilities.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 1;

    AROS_LIBFUNC_EXIT
} /* DisplayError */
