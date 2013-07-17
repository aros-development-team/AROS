/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#include LC_LIBDEFS_FILE

/*****************************************************************************

    NAME */
#include <libraries/nonvolatile.h>

        AROS_LH1(VOID, FreeNVData,

/*  SYNOPSIS */

	AROS_LHA(APTR, data, A0),

/*  LOCATION */

	struct Library *, nvBase, 6, Nonvolatile)

/*  FUNCTION

    Free data allocated by nonvolatile.library (GetCopyNV(), GetNVInfo(),
    GetNVList()).

    INPUTS

    data  --  data got from a nonvolatile.library function; may be NULL in
              which case this function does nothing

    RESULT
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    GetCopyNV(), GetNVInfo(), GetNVList()

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    FreeVec(data);

    AROS_LIBFUNC_EXIT
} /* FreeNVData */
