/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include <libraries/nonvolatile.h>
#include <proto/exec.h>
#include "nonvolatile_intern.h"

AROS_LH1(VOID, FreeNVData,

/*  SYNOPSIS */

	AROS_LHA(APTR, data, A0),

/*  LOCATION */

	struct Library *, nvBase, 6, Nonvolatile)

/*  FUNCTION

    Free data allocated by nonvolatile.library (GetCopyNV(), GetNVInfo(),
    GetNVList()).

    INPUTS

    data  --  data got from a nonvolatile.library function

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

    FreeVec(data);

    AROS_LIBFUNC_EXIT
}
