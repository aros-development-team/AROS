/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "icon_intern.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH1(struct DiskObject *, GetDefDiskObject,

/*  SYNOPSIS */
	AROS_LHA(long, type, D0),

/*  LOCATION */
	struct Library *, IconBase, 20, Icon)

/*  FUNCTION
	Gets the default icon for the supplied type of icon.

    INPUTS
	type  -  type of icon to get default diskobject for.


    RESULT
	DiskObject structure or NULL if an error occured. The error may
	be obtained by IoErr().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	PutDefDiskObject(), GetDiskObjectNew()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    UBYTE deficonname[MAX_DEFICON_FILEPATH]; /* Will contain path and name
				    of a defaulticon "ENVARC:Sys/..." */

    GetDefIconName (type, deficonname);

    return GetDiskObject (deficonname);
    AROS_LIBFUNC_EXIT
} /* GetDefDiskObject */
