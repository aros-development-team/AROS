/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/dos.h>
#include <proto/workbench.h>
#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH1(BOOL, DeleteDiskObject,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct IconBase *, IconBase, 23, Icon)

/*  FUNCTION
	Deletes an icon description file.

    INPUTS
	name  -  name of the icon file without the ".info".

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
    UBYTE * infofilename;
    BOOL    success = FALSE;
    BPTR lock, parent;

    if (!WorkbenchBase)
        WorkbenchBase = TaggedOpenLibrary(TAGGEDOPEN_WORKBENCH);

    if (!(infofilename = (UBYTE*)AllocVec (strlen(name) + 6,
	MEMF_ANY | MEMF_CLEAR)
    ) )
       return (FALSE);

    /* Construct the icon's name */
    strcpy (infofilename, name);
    strcat (infofilename, ".info");

    lock = Lock(infofilename, SHARED_LOCK);
    if (lock) {
        parent = ParentDir(lock);
        if (parent) {
            success = DeleteFile (infofilename);
            if (success && WorkbenchBase) {
                UpdateWorkbench(FilePart(name), parent, UPDATEWB_ObjectRemoved);
            }
            UnLock(parent);
        }
        UnLock(lock);
    }

    FreeVec (infofilename);

    return success;
    AROS_LIBFUNC_EXIT
} /* DeleteDiskObject */
