/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Draw a line
    Lang: english
*/

#include <math.h>

#include <exec/types.h>

#include "gfxhidd_intern.h"
#define DEBUG 1
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>

        AROS_LH5(VOID, HIDD_Graphics_DrawLine_Q,

/*  SYNOPSIS */
        AROS_LHA(APTR            , gc          , A2),
        AROS_LHA(WORD            , x1          , D2),
        AROS_LHA(WORD            , y1          , D3),
        AROS_LHA(WORD            , x2          , D4),
        AROS_LHA(WORD            , y2          , D5),

/*  LOCATION */
        struct Library *, GfxHiddBase, 21, GfxHidd)

/*  FUNCTION
        Draws a line from (x1,y1) to (x2,y2) in the specified gc.
        The function does not check gc != NULL and does not clipp
        the line against the drawing are.

    INPUTS
        gc    - valid pointer to a graphics context that was created with
                HIDD_Graphics_CreateGC()
        x1,y1 - start point of the line in hidd units
        x2,y2 - end point of the line in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Pixel

    INTERNALS
        Uses midpoint line ("Bresenham") algorithm([FOL90] 3.2.2)
        TODO Support for line pattern
             Optimize remove if t == 1 ...

    HISTORY
        06-04-98    drieling created
***************************************************************************/

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    WORD dx, dy, incrE, incrNE, d, x, y, s1, s2, t, i;

    D(bug("HIDD_Graphics_DrawLine_Q\n"));
    /* D(bug("  sorry, not yet implemented\n")); */

    /* Calculate slope */
    dx = abs(x2 - x1);
    dy = abs(y2 - y1);

    /* which direction? */
    if((x2 - x1) > 0) s1 = 1; else s1 = - 1;
    if((y2 - y1) > 0) s2 = 1; else s2 = - 1;

    /* change axes if dx < dy */
    if(dx < dy)
    {
        d = dx; dx = dy; dy = d; t = 0;
    }
    else
    {
       t = 1;
    }


    d  = 2 * dy - dx;        /* initial value of d */

    incrE  = 2 * dy;         /* Increment use for move to E  */
    incrNE = 2 * (dy - dx);  /* Increment use for move to NE */

    x = x1; y = y1;
    HIDD_Graphics_WritePixel_Q(gc, x, y); /* The start pixel */

    for(i = 0; i <= dx; i++)
    {
        if(d <= 0)
        {
            if(t == 1)
            {
                x = x + s1;
            }
            else
            {
                y = y + s2;
            }

            d = d + incrE;
        }
        else
        {
            if(t == 1)
            {
                x = x + s1;
                y = y + s2;
            }
            else
            {
                x = x + s1;
                y = y + s2;
            }

            d = d + incrNE;
        }
        HIDD_Graphics_WritePixel_Q(gc, x, y);
    }

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_DrawLine_Q */
