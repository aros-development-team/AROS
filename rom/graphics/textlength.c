/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function TextLength()
    Lang: english
*/
#include "graphics_intern.h"

#undef NUMCHARS
#define NUMCHARS(tf) ((tf->tf_HiChar - tf->tf_LoChar) + 2)

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

        AROS_LH3(WORD, TextLength,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp, A1),
        AROS_LHA(CONST_STRPTR     , string, A0),
        AROS_LHA(ULONG            , count, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 9, Graphics)

/*  FUNCTION
        Determines the length of a string in pixels.

    INPUTS
        rp     - RastPort
        string - address of string
        count  - number of characters of string

    RESULT
        Length of string in pixels.

    NOTES
        Use the newer TextExtent() to get more information.

    EXAMPLE

    BUGS

    SEE ALSO
        Text(), TextExtent()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TextFont *tf = rp->Font;
    WORD strlen;

    if ((tf->tf_Flags & FPF_PROPORTIONAL) || tf->tf_CharKern
        || tf->tf_CharSpace)
    {
        WORD  idx;
        WORD  defaultidx = NUMCHARS(tf) - 1; /* Last glyph is default glyph */
        UBYTE c;

        for(strlen = 0; count; count--)
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

            strlen += ((WORD *)tf->tf_CharKern)[idx];
            strlen += ((WORD *)tf->tf_CharSpace)[idx];
            strlen += rp->TxSpacing;
        }
    }
    else
    {
        strlen = count * (tf->tf_XSize + rp->TxSpacing);
    }

    return strlen;

    AROS_LIBFUNC_EXIT

} /* TextLength */
