/*
 *  Copyright (c) 2010-2011 Matthias Rustler
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included
 *  in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 *   
 *  $Id$
 */

#include <proto/utility.h>

#include <string.h>

#include "identify_intern.h"
#include "identify.h"

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH2(LONG, IdAlert,

/*  SYNOPSIS */
        AROS_LHA(ULONG           , id     , D0),
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 7, Identify)

/*  FUNCTION
        Get a human readable description of the alert ('Guru') code.

    INPUTS
        Code    -- (ULONG) alert code, as defined in exec/alerts.h

        TagList -- (struct TagItem *) tags that describe further
                   options.

    RESULT
        Error   -- (LONG) error code, or 0 if everything went fine.

    TAGS
        IDTAG_DeadStr   -- (STRPTR) Alert type string (deadend or
                           recoverable). You may skip this tag if you do not
                           want to get the string.

        IDTAG_SubsysStr -- (STRPTR) String of the subsystem that caused
                           the alert (CPU, exec.library, ...). You may skip this tag
                           if you do not want to get the string.

        IDTAG_GeneralStr-- (STRPTR) General alert cause. You  may skip
                           this tag if you do not want to get the string.

        IDTAG_SpecStr   -- (STRPTR) Specified alert cause. You may skip
                           this tag if you do not want to get the string.

        IDTAG_StrLength -- (UWORD) Maximum length of the string buffer,
                           including termination. Defaults to 50.

        IDTAG_Localize  -- [V8] (BOOL) FALSE to get English strings
                           only, TRUE for localized strings. This is useful for
                           applications with English as only language. Defaults to TRUE.

    NOTES
        This function isn't implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR deadstr = NULL;
    STRPTR subsysstr = NULL;
    STRPTR generalstr = NULL;
    STRPTR specstr = NULL;
    BOOL localize = TRUE;
    ULONG strlength = 50;

    struct TagItem *tag;
    const struct TagItem *tags;

    for (tags = taglist; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case IDTAG_DeadStr:
                deadstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_SubsysStr:
                subsysstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_GeneralStr:
                generalstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_SpecStr:
                specstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_StrLength:
                strlength = tag->ti_Data;
                break;

            case IDTAG_Localize:
                localize = tag->ti_Data ? TRUE : FALSE;
                break;
        }
    }

    if (strlength <= 0)
    {
        return IDERR_NOLENGTH;
    }

    // return something to avoid crashes when function is called
    if (deadstr)
    {
        strlcpy(deadstr, "Unknown", strlength);
    }
    if (subsysstr)
    {
        strlcpy(subsysstr, "Unknown", strlength);
    }
    if (generalstr)
    {
        strlcpy(generalstr, "Unknown", strlength);
    }
    if (specstr)
    {
        strlcpy(specstr, "Unknown", strlength);
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdAlert */
