/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/arossupport.h>
#include <proto/dos.h>
#include "icon_intern.h"

extern const IPTR IconDesc[];

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>

	AROS_LH2(BOOL, PutDiskObject,

/*  SYNOPSIS */
	AROS_LHA(UBYTE             *, name, A0),
	AROS_LHA(struct DiskObject *, diskobj, A1),

/*  LOCATION */
	struct Library *, IconBase, 14, Icon)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    icon_lib.fd and clib/icon_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    char * iconname;
    BPTR   icon;
    BOOL   success;

    /* Name with correct extension ? */
    if (strrncasecmp (name, ".info", 5))
    {
	/* Create the final filename */
	if (!(iconname = AllocVec (strlen (name) + 5 + 1, MEMF_ANY)) )
	    return NULL;

	strcpy (iconname, name);
	strcat (iconname, ".info");

	/* Try to open that file */
	icon = Open (iconname, MODE_NEWFILE);

	FreeVec (iconname);
    }
    else
	icon = Open (name, MODE_NEWFILE);

    if (!icon)
	return FALSE;

    /* Read the file in */
    success = WriteStruct (&LB(IconBase)->dsh, (APTR)diskobj, icon, IconDesc);

    Close (icon);

    return success;
    AROS_LIBFUNC_EXIT
} /* PutDiskObject */
