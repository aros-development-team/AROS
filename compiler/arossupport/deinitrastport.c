/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$        $Log

    Desc: AROS Graphics function DeinitRastPort()
*/

#include <aros/debug.h>
#include <graphics/rastport.h>
#include <proto/exec.h>

/*
 * RP_Extra is graphics.library's private data of a RastPort; its first
 * field is the RastPort it belongs to. A copy of a RastPort (CloneRastPort(),
 * or one copied by hand) shares the pointer but not the data.
 */
#define RP_EXTRA_OWNED(rp)  ((rp)->RP_Extra && (*(struct RastPort **)(rp)->RP_Extra == (rp)))

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

        void DeinitRastPort(

/*  SYNOPSIS */
        struct RastPort *rp)

/*  FUNCTION

        Frees the extra contents associated with a RastPort structure.
        The structure itself is not freed.

    INPUTS

        rp - The RastPort which contents are to be freed.

    RESULT

        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

        graphics.library/InitRastPort()

    INTERNALS

    HISTORY
        29-10-95    digulla automatically created from
                            graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    D(bug("DeInitRastPort()\n"));

    if (RP_EXTRA_OWNED(rp))
        FreeVec(rp->RP_Extra);
    rp->RP_Extra = NULL;

} /* DeinitRastPort */
