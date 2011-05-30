/*
 * Copyright (c) 2010-2011 Matthias Rustler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *   
 * $Id$
 */

#include <proto/utility.h>

#include <string.h>

#include "identify_intern.h"
#include "identify.h"

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH1(LONG, IdExpansion,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 5, Identify)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR manufstr = NULL;
    STRPTR prodstr = NULL;
    STRPTR classstr = NULL;
    ULONG strlength = 50;
    struct ConfigDev **expansion = NULL;
    BOOL secondary = FALSE;
    ULONG *classid = NULL;
    BOOL localize = TRUE;

    struct TagItem *tag;
    const struct TagItem *tags;

    for (tags = taglist; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case IDTAG_ConfigDev:
                // doesn't return anything
                break;

            case IDTAG_ManufID:
                // doesn't return anything
                break;

            case IDTAG_ProdID:
                // doesn't return anything
                break;

            case IDTAG_ManufStr:
                manufstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_ProdStr:
                prodstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_ClassStr:
                classstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_StrLength:
                strlength = tag->ti_Data;
                break;

            case IDTAG_Expansion:
                expansion = (struct ConfigDev **)tag->ti_Data;
                break;

            case IDTAG_Secondary:
                secondary = tag->ti_Data ? TRUE : FALSE;
                break;

            case IDTAG_ClassID:
                classid = (ULONG *)tag->ti_Data;
                break;

            case IDTAG_Localize:
                localize = tag->ti_Data ? TRUE : FALSE;
                break;

        }
    }

    if (strlength <=0)
    {
        return IDERR_NOLENGTH;
    }

    // return something to avoid crashes when function is called
    if (manufstr)
    {
        strlcpy(manufstr, "Unknown", strlength);
    }
    if (prodstr)
    {
        strlcpy(prodstr, "Unknown", strlength);
    }
    if (classstr)
    {
        strlcpy(classstr, "Unknown", strlength);
    }
    if (expansion)
    {
        *expansion = NULL;
    }
    if (classid)
    {
        *classid = IDCID_UNKNOWN;
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdExpansion */
