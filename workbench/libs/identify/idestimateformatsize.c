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

#include "identify_intern.h"

static STRPTR finddollar(TEXT *t);

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH2(ULONG, IdEstimateFormatSize,

/*  SYNOPSIS */
        AROS_LHA(STRPTR          , string, A0),
        AROS_LHA(struct TagItem *, tags  , A1),

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 12, Identify)

/*  FUNCTION
        Estimates the size of the buffer that will contain the output
        of the format string when used on IdFormatString().

    INPUTS
        String  -- (STRPTR) Format string
        Tags    -- (struct TagItem *) Tags, currently NULL or TAG_DONE.

    RESULT
        Length  -- (ULONG) Length of the buffer size that will
                   be able to hold the entire result.

    TAGS
        None yet.

    NOTES
        The returned size will be large enough to contain the result
        of a IdFormatString(). It is not necessarily the size of the
        resulting buffer (the result length of IdFormatString()).

    EXAMPLE

    BUGS

    SEE ALSO
        IdHardware(), IdFormatString()

    INTERNALS

    HISTORY


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    // no tags

    TEXT *from = string;
    ULONG result = 1;

    if (from == NULL)
    {
        return 0;
    }

    while (*from)
    {
        if (*from == '$')
        {
            from++;
            if (*from == '$')
            {
                from++;
                result++;
            }
            else
            {
                from = finddollar(from);
                result += STRBUFSIZE;
            }
        }
        else
        {
            result++;
            from++;
        }
    }

    return result;

    AROS_LIBFUNC_EXIT
} /* IdEstimateFormatSize */


static STRPTR finddollar(TEXT *t)
{
    while (*t && *t != '$')
    {
        t++;
    }

    if (*t) // '$'
    {
        return t + 1;
    }
    else
    {
        return t;
    }
}
