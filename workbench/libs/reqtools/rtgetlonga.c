
/*
    (C) 1999 - 2000 AROS - The Amiga Research OS
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
#include <libraries/reqtools.h>
#include <aros/libcall.h>

#include "reqtools_intern.h"
#include "general.h"

/*****************************************************************************

    NAME */

    AROS_LH4(ULONG, rtGetLongA,

/*  SYNOPSIS */

	AROS_LHA(ULONG *, longptr, A1),
	AROS_LHA(char *, title, A2),
	AROS_LHA(struct rtReqInfo *, reqinfo, A3),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 13, ReqTools)

/*  FUNCTION
   
    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
   
    return GetString(NULL,
    		     0,
		     title,
		     0,
		     longptr,
		     ENTER_NUMBER,
		     reqinfo,
		     taglist);
		     
    AROS_LIBFUNC_EXIT
  
} /* rtGetLongA */
