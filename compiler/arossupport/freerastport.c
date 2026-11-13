/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: AROS Graphics function FreeRastPort
*/

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
#include <graphics/rastport.h>
#include <proto/arossupport.h>

        void FreeRastPort(

/*  SYNOPSIS */
        struct RastPort *rp)

/*  FUNCTION
        This frees a RastPort obtained with CloneRastPort() or
        CreateRastPort().

    INPUTS
        rp - The result of CloneRastPort() or CreateRastPort().

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CloneRastPort(), CreateRastPort()

    INTERNALS

*****************************************************************************/
{
    if (RP_EXTRA_OWNED(rp))
        FreeVec(rp->RP_Extra);

    FreeMem (rp, sizeof (struct RastPort));

} /* FreeRastPort */
