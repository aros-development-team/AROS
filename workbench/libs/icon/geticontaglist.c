/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 1
#include <aros/debug.h>

#include <stdio.h>
#include <workbench/icon.h>

#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH2(struct DiskObject *, GetIconTagList,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, A0),
	AROS_LHA(struct TagItem *, tags, A1),
/*  LOCATION */
	struct Library *, IconBase, 30, Icon)

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
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)

    struct TagItem  *tag, *tagp;
    struct DiskObject *dob = NULL;
    int type = -1;
    char *def_name = NULL;
    int fail = 1;

    /* parse tags */
    for (tagp = tags; (tag = NextTagItem(&tagp)); )
    {
	switch (tag->ti_Tag)
	{
	    case    ICONGETA_GetDefaultType:
		    type = tag->ti_Data;
		    break;

	    case    ICONGETA_GetDefaultName:
		    def_name = (char*)tag->ti_Data;
		    break;

	    case    ICONGETA_FailIfUnavailable:
		    fail = tag->ti_Data;
		    break;
	}
    }

   /* TODO: We use the old GetDiskObjectXXX() calls here,
    * but it would be better if those functions use
    * GetIconTagList() because this supersets all */

    if (type != -1 || def_name)
    {
	/* the name argument have to be ignored */

	if (def_name)
	{
	    char buf[360];
	    /* TODO: has aros snprintf()? */
	    sprintf(buf,"ENV:Sys/def_%s",def_name);
	    dob = GetDiskObject(def_name);
	}

	if (!dob && type != -1)
	{
	    dob = GetDefDiskObject(type);
	}
    }   else
    {
	/* icon.library v44 uses a global identification hook
	 * to determine the filetype. We just call GetDiskObjectNew() for now */

	if (fail)
	{
	    dob = GetDiskObject(name);
	} else
	{
	    dob = GetDiskObjectNew(name);
	}
    }

    return dob;

    AROS_LIBFUNC_EXIT
} /* GetIconRectangle */
