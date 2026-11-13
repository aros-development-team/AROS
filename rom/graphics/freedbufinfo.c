/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
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

    db - DBufInfo structure allocated by AllocDBufInfo(), or NULL.
         Passing NULL has no effect.

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

