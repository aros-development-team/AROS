
/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/reqtools.h>
#include <proto/intuition.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include "reqtools_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH1(VOID, rtFreeFileList,

/*  SYNOPSIS */

	AROS_LHA(struct rtFileList *, selfile, A0),

/*  LOCATION */

	struct Library *, RTBase, 11, ReqTools)

/*  FUNCTION

    Frees a filelist returned by rtFileRequest() when the FREQF_MULTISELECT
    flag was set.  Call this after you have scanned the filelist and you no
    longer need it.

    INPUTS

    filelist  --  pointer to rtFileList structure, returned by rtFileRequest();
                  may be NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    rtFileRequest()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct rtFileList *last;

    while(selfile != NULL)
    {
	last    = selfile;
	selfile = selfile->Next;
	FreeVec(last);
    }

    AROS_LIBFUNC_EXIT
} /* rtFreeFileList */
