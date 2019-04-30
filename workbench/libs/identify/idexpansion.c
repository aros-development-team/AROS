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
#include <clib/alib_protos.h>

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
        Gets the name and class of the expansion and it's manufacturer.

    INPUTS
        TagList -- (struct TagItem *) tags that describe further options.

    RESULT
        Error   -- (LONG) error code, or 0 if everything went fine.

                   IDERR_NOLENGTH  -- IDTAG_StrLength has been set to 0!
                   IDERR_BADID     -- IDTAG_ManufID and IDTAG_ProdID were
                                      out of range or one of them was missing.
                   IDERR_DONE      -- Checked all expansions using the
                                      IDTAG_Expansion tag. This is not really an error.
                   IDERR_SECONDARY -- This expansion is secondary to a primary
                                      expansion entry.

    TAGS
        IDTAG_ConfigDev -- (struct ConfigDev *) ConfigDev structure
                           containing all information. You should use this tag if ever
                           possible, since there are more possibilities to recognize and
                           distinguish between a board.

        IDTAG_ManufID   -- (UWORD) Manufacturer ID if ConfigDev is not
                           provided. You must also provide IDTAG_ProdID!

        IDTAG_ProdID    -- (UBYTE) Product ID if ConfigDev is not
                           provided. You must also provide IDTAG_ManufID!

        IDTAG_ManufStr  -- (STRPTR) Pointer to a buffer space for the
                           manufacturer name. You may skip this tag if you do not want
                           to get this string.

        IDTAG_ProdStr   -- (STRPTR) Pointer to a buffer space for the
                           product name. You may skip this tag if you do not want
                           to get this string.

        IDTAG_ClassStr  -- (STRPTR) Pointer to a buffer space for the
                           product class. You may skip this tag if you do not want to get
                           this string.

        IDTAG_StrLength -- (UWORD) Buffer length, including
                           termination. Defaults to 50.

        IDTAG_Expansion -- [V6] (struct ConfigDev **) Use this tag to
                           easily traverse through the expansion board list. Init the
                           pointed variable with NULL. After each call, you will find
                           the current ConfigDev pointer in this variable. If you are
                           done, this function returns IDERR_DONE and the variable is set
                           to NULL. See example.

        IDTAG_Secondary -- [V7] (BOOL) If set to TRUE, identify will
                           warn about secondary expansions. E.g. some graphic boards
                           create more than one entry in the expansion list. Then, one
                           entry is the primary entry, and any additional are secondary.
                           This tag does only make sense when checking all mounted
                           expansions. Defaults to FALSE. (See Bugs)

        IDTAG_ClassID   -- [V8] (ULONG *) The ULONG field will be filled
                           with a numerical class ID of the expansion (see include file:
                           IDCID_...). IMPORTANT: You MUST be prepared to get a number
                           that does not match to any IDCID value. In this case, assume
                           IDCID_MISC.

        IDTAG_Localize  -- [V8] (BOOL) FALSE to get English strings
                           only, TRUE for localized strings. This is useful for applications
                           with English as only language. Defaults to TRUE.

    EXAMPLE
        To check all expansion boards, you may use this code:

        void PrintExpansions(void)
        {
          struct ConfigDev *expans = NULL;
          char manuf[IDENTIFYBUFLEN];
          char prod[IDENTIFYBUFLEN];
          char pclass[IDENTIFYBUFLEN];

          while(!IdExpansionTags(
                  IDTAG_ManufStr ,manuf,
                  IDTAG_ProdStr  ,prod,
                  IDTAG_ClassStr ,pclass,
                  IDTAG_Expansion,&expans,
                  TAG_DONE))
          {
            Printf("Current ConfigDev = 0x%08lx\n",expans);
            Printf("  Manufacturer    = %s\n",manuf);
            Printf("  Product         = %s\n",prod);
            Printf("  Expansion class = %s\n\n",class);
          }
        }

    NOTES
        This function isn't implemented yet.

        If the manufacturer or the product is not known, the string will be
        filled with its number.

        This call is guaranteed to preserve all registers except D0.

    BUGS
        You must also provide IDTAG_ProdStr if you want to use IDTAG_Secondary.

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
    struct TagItem *tags;

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

    if (localize) {
        /* FIXME: Do we need to do something here? */
    }

    if (secondary) {
        /* FIXME: Do we need to do something here? */
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
