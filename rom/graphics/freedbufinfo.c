/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "graphics_intern.h"

/*****************************************************************************

    NAME */

#include <graphics/view.h>
#include <proto/exec.h>

    AROS_LH1(VOID, FreeDBufInfo,

/*  SYNOPSIS */

	AROS_LHA(struct DBufInfo *, db, A1),

/*  LOCATION */

	struct GfxBase *, GfxBase, 162, Graphics)

/*  FUNCTION

    Frees structure allocated with AllocDBufInfo().

    INPUTS

    RESULTS

    Frees memory occupied by 'db'; ('db' may be NULL, in that case nothing
    is done).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    AllocDBufInfo(), ChangeVPBitMap()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if(db == NULL) return;

    FreeMem(db, sizeof(struct DBufInfo));

    AROS_LIBFUNC_EXIT
} /* FreeDBufInfo */

