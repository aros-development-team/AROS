/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    GetTagData()
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <proto/utility.h>

	AROS_LH3(IPTR, GetTagData,

/*  SYNOPSIS */
	AROS_LHA(Tag                   , tagValue, D0),
	AROS_LHA(IPTR                  , defaultVal, D1),
	AROS_LHA(const struct TagItem *, tagList, A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 6, Utility)

/*  FUNCTION
	Searches the TagList for the Tag specified, if it exists, then
	returns the ti_Data field of that Tag, otherwise returns the
	supplied default value.

    INPUTS
	tagValue    -	Tag to search for.
	defaultVal  -	Default value for the Tag.
	tagList     -	Pointer to first TagItem in the list.

    RESULT
	The data value if the Tag exists, or the default value if it
	doesn't.

    NOTES
	If the input TagList doesn't exist (eg for some reason equals
	NULL), then the return value will be NULL. This way you can
	check for broken code, whereas returing the default would allow
	code that is possibly buggy to still seem to work. (Until you
	tried to do anything special at least).

    EXAMPLE

	struct Window *window;	    \* The Window we are creating *\
	struct TagItem *wintags;    \* Tags for this window *\

	\* Find out the value for the WA_Left tag *\
	window->Left = GetTagData( WA_Left, 320, wintags )

    BUGS

    SEE ALSO
	utility/tagitem.h

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti = NULL;

    /*
	If we can find the Tag in the supplied list, then return the
	ti_Data fields value.

	If the tag is not in the list, just return the default value.
    */
    if (tagList && (ti = FindTagItem(tagValue, tagList)))
	return ti->ti_Data;

    return defaultVal;
    AROS_LIBFUNC_EXIT
} /* GetTagData */
