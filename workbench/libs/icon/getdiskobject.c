/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/dos.h>
#include <proto/aros.h>
#include "icon_intern.h"

extern const IPTR IconDesc[];

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>

	AROS_LH1(struct DiskObject *, GetDiskObject,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct Library *, IconBase, 13, Icon)

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
    struct DiskObject * dobj;
    char * iconname;
    BPTR   icon;

    /* Create the final filename */
    if (!(iconname = AllocVec (strlen (name) + 5 + 1, MEMF_ANY)) )
	return NULL;

    strcpy (iconname, name);
    strcat (iconname, ".info");

    /* Try to open that file */
    icon = Open (iconname, MODE_OLDFILE);

    FreeVec (iconname);

    if (!icon)
	return NULL;

    /* Read the file in */
    if (!ReadStruct (icon, (APTR *)&dobj, IconDesc))
	dobj = NULL;

    Close (icon);

    return dobj;
    AROS_LIBFUNC_EXIT
} /* GetDiskObject */
