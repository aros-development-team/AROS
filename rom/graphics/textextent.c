/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Calculate the size a text needs in a specific rastport.
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/graphics.h>

        AROS_LH4(void, TextExtent,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort   *, rp, A1),
        AROS_LHA(CONST_STRPTR       , string, A0),
        AROS_LHA(ULONG              , count, D0),
        AROS_LHA(struct TextExtent *, textExtent, A2),

/*  LOCATION */
        struct GfxBase *, GfxBase, 115, Graphics)

/*  FUNCTION
        This function determines the metric of the space that a text string
        would render into.

    INPUTS
        rp -     RastPort
        string - address of string
        count -  number of characters
        textExtent - storing place for the result
                     te_Width  - same as TextLength() result: the rp_cp_x
                                 advance that rendering this text would cause.
                     te_Height - same as tf_YSize.  The height of the
                                 font.
                     te_Extent.MinX - the offset to the left side of the
                                      rectangle this would render into.
                                      Often zero.
                     te_Extent.MinY - same as -tf_Baseline.  The offset
                                      from the baseline to the top of the
                                      rectangle this would render into.
                     te_Extent.MaxX - the offset of the left side of the
                                      rectangle this would render into.
                                      Often the same as te_Width-1.
                     te_Extent.MaxY - same as tf_YSize-tf_Baseline-1.
                                      The offset from the baseline to the
                                      bottom of the rectangle this would
                                      render into.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TextFont *tf = rp->Font;

    textExtent->te_Width = TextLength(rp, string, count);
    textExtent->te_Height = tf->tf_YSize;
    textExtent->te_Extent.MinY = -tf->tf_Baseline;
    textExtent->te_Extent.MaxY = textExtent->te_Height - 1 - tf->tf_Baseline;

    /* MinX/MaxX can be a bit more complicated if there are kerning/space
     * tables */

    if ((tf->tf_Flags & FPF_PROPORTIONAL) || tf->tf_CharKern
        || tf->tf_CharSpace)
    {
        WORD  idx;
        WORD  defaultidx = NUMCHARS(tf) - 1; /* Last glyph is default glyph */
        WORD  x, x2;
        UBYTE c;

        textExtent->te_Extent.MinX = 0;
        textExtent->te_Extent.MaxX = 0;
        x = 0;

        if (count)
        {
            while(count--)
            {
                c = *string++;

                if ( c < tf->tf_LoChar || c > tf->tf_HiChar)
                {
                    idx = defaultidx;
                }
                else
                {
                    idx = c - tf->tf_LoChar;
                }

                #define CHECK_MINMAX(x) \
                    if ((x) < textExtent->te_Extent.MinX) \
                        textExtent->te_Extent.MinX = (x); \
                    if ((x) > textExtent->te_Extent.MaxX) \
                        textExtent->te_Extent.MaxX = (x);

                x += ((WORD *)tf->tf_CharKern)[idx];
                CHECK_MINMAX(x);

                x2 = x + ( ( ((ULONG *)tf->tf_CharLoc)[idx] ) & 0xFFFF);
                CHECK_MINMAX(x2);

                x += ((WORD *)tf->tf_CharSpace)[idx];
                CHECK_MINMAX(x);

                x += rp->TxSpacing;
                CHECK_MINMAX(x);

            } /* while(count--) */

            textExtent->te_Extent.MaxX--;

        } /* if (count) */

    } /* if ((tf->tf_Flags & FPF_PROPORTIONAL) || tf->tf_CharKern
       * || tf->tf_CharSpace) */
    else
    {
        /* Normal non-proportional Font */
        textExtent->te_Extent.MinX = 0;
        textExtent->te_Extent.MaxX = textExtent->te_Width - 1;
    }

    if (rp->AlgoStyle & FSF_BOLD)
    {
        textExtent->te_Extent.MaxX += tf->tf_BoldSmear;
    }

    if (rp->AlgoStyle & FSF_ITALIC)
    {
        /*  ######            ######
        **  ##  ##            ##  ##
        **  ##  ##           ##  ##
        **  ##  ##  ===>     ##  ##
        **  ##  ##          ##  ##
        **..##..##..      ..##..##..
        **  ##  ##         ##  ##
        **  ######         ######
        */

        textExtent->te_Extent.MaxX += tf->tf_Baseline / 2;
        textExtent->te_Extent.MinX -= (tf->tf_YSize - tf->tf_Baseline) / 2;
    }

    AROS_LIBFUNC_EXIT

} /* TextExtent */
