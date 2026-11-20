/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Calculate the size a text needs in a specific rastport.
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
        AROS_LHA(WORD               , count, D0),
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
        te_Extent covers the glyphs of all the characters, wherever
        kerning, a negative TxSpacing or a glyph wider than tf_XSize puts
        them. Where they reach past the range of a WORD, MinX and MaxX stop
        at -32768 and 32767; te_Width is what TextLength() gives.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TextFont *tf = rp->Font;
    QUAD minx, maxx;

    textExtent->te_Width = TextLength(rp, string, count);
    textExtent->te_Height = tf->tf_YSize;
    textExtent->te_Extent.MinY = -tf->tf_Baseline;
    textExtent->te_Extent.MaxY = textExtent->te_Height - 1 - tf->tf_Baseline;

    /* MinX/MaxX follow the glyphs, which kerning, a negative TxSpacing or a
     * glyph wider than tf_XSize can place outside te_Width; a font without
     * kerning or space tables works as one with kerning 0 and spaces of
     * tf_XSize, as Text() draws it. They are worked out in QUADs, since
     * 32767 characters can reach further than a LONG, and end up within
     * the range of a WORD. */

    if(count > 0) {
        WORD  idx;
        WORD  defaultidx = NUMCHARS(tf) - 1; /* Last glyph is default glyph */
        QUAD  x, x2;
        UBYTE c;

        minx = 0;
        maxx = 0;
        x = 0;

        while(count--) {
            c = *string++;

            if(c < tf->tf_LoChar || c > tf->tf_HiChar) {
                idx = defaultidx;
            } else {
                idx = c - tf->tf_LoChar;
            }

#define CHECK_MINMAX(x) \
                if ((x) < minx) \
                    minx = (x); \
                if ((x) > maxx) \
                    maxx = (x);

            if(tf->tf_CharKern) {
                x += ((WORD *)tf->tf_CharKern)[idx];
            }
            CHECK_MINMAX(x);

            x2 = x + ((((ULONG *)tf->tf_CharLoc)[idx]) & 0xFFFF);
            CHECK_MINMAX(x2);

            if(tf->tf_CharSpace) {
                x += ((WORD *)tf->tf_CharSpace)[idx];
            } else {
                x += tf->tf_XSize;
            }
            CHECK_MINMAX(x);

            x += rp->TxSpacing;
            CHECK_MINMAX(x);

        } /* while(count--) */

        maxx--;

    } /* if (count > 0) */
    else if((tf->tf_Flags & FPF_PROPORTIONAL) || tf->tf_CharKern
            || tf->tf_CharSpace) {
        minx = 0;
        maxx = 0;
    }
    else {
        /* Normal non-proportional Font */
        minx = 0;
        maxx = textExtent->te_Width - 1;
    }

    if(rp->AlgoStyle & FSF_BOLD) {
        maxx += tf->tf_BoldSmear;
    }

    if(rp->AlgoStyle & FSF_ITALIC) {
        /*  ######            ######
        **  ##  ##            ##  ##
        **  ##  ##           ##  ##
        **  ##  ##  ===>     ##  ##
        **  ##  ##          ##  ##
        **..##..##..      ..##..##..
        **  ##  ##         ##  ##
        **  ######         ######
        */

        maxx += tf->tf_Baseline / 2;
        minx -= (tf->tf_YSize - tf->tf_Baseline) / 2;
    }

    textExtent->te_Extent.MinX = minx < -32768 ? -32768 : minx > 32767 ? 32767 : minx;
    textExtent->te_Extent.MaxX = maxx < -32768 ? -32768 : maxx > 32767 ? 32767 : maxx;

    AROS_LIBFUNC_EXIT

} /* TextExtent */
