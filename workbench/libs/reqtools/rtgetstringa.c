
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

    AROS_LH5(ULONG, rtGetStringA,

/*  SYNOPSIS */

	AROS_LHA(UBYTE *, buffer, A1),
	AROS_LHA(ULONG, maxchars, D0),
	AROS_LHA(char *, title, A2),
	AROS_LHA(struct rtReqInfo *, reqinfo, A3),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct Library *, RTBase, 12, ReqTools)

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
    
    return GetString(buffer,
    		     maxchars,
		     title,
		     0,
		     NULL,
		     ENTER_STRING,
		     reqinfo,
		     taglist);
		     
    AROS_LIBFUNC_EXIT
    
} /* rtgetstringa */
