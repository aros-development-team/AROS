/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
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
