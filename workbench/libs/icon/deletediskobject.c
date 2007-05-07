/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/dos.h>
#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH1(BOOL, DeleteDiskObject,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct Library *, IconBase, 23, Icon)

/*  FUNCTION
	Deletes an icon description file.

    INPUTS
	name  -  name of the icon file without the ".info".

    RESULT

    NOTES

    EXAMPLE

    BUGS
	Does not yet notify workbench about the deletion.

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE * infofilename;
    BOOL    success;

    if (!(infofilename = (UBYTE*)AllocVec (strlen(name) + 6,
	MEMF_ANY | MEMF_CLEAR)
    ) )
       return (FALSE);

    /* Construct the icon's name */
    strcpy (infofilename, name);
    strcat (infofilename, ".info");

    success = DeleteFile (infofilename);

    FreeVec (infofilename);

    return success;
    AROS_LIBFUNC_EXIT
} /* DeleteDiskObject */
