/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

	struct ReqToolsBase *, ReqToolsBase, 10, ReqTools)

/*  FUNCTION

	Frees a filelist returned by rtFileRequest() when the FREQF_MULTISELECT
	flag was set.  Call this after you have scanned the filelist and you no
	longer need it.

    INPUTS

	filelist  --  pointer to rtFileList structure, returned by rtFileRequest();
                      may be NULL.

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS
	none known

    SEE ALSO

        rtFileRequestA()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeFileList(selfile);
    
    AROS_LIBFUNC_EXIT
    
} /* rtFreeFileList */
