/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/arossupport.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include "icon_intern.h"

#include <aros/debug.h>

extern const IPTR IconDesc[];

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>
#include <exec/types.h>

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    struct DiskObject * dobj, *dup_dobj;
    char * iconname;
    BPTR   icon;

    /* Name with correct extension ? */
    if (strrncasecmp (name, ".info", 5))
    {
	/* Create the final filename */
	if (!(iconname = AllocVec (strlen (name) + 5 + 1, MEMF_ANY)) )
	{
	    SetIoErr (ERROR_NO_FREE_STORE);
	    return NULL;
	}

	strcpy (iconname, name);
	strcat (iconname, ".info");

	/* Try to open that file */
	icon = Open (iconname, MODE_OLDFILE);

	FreeVec (iconname);
    }
    else
	icon = Open (name, MODE_OLDFILE);


    if (!icon)
	return NULL;

    /* Read the file in */
    if (!ReadStruct (&LB(IconBase)->dsh, (APTR *)&dobj, icon, IconDesc))
	dobj = NULL;

    /* Make the icon "native" so it can be free'd with FreeDiskObject() */
    if (dobj)
    {
    	struct TagItem dup_tags[] =
	{
	    {ICONDUPA_JustLoadedFromDisk, TRUE	},
	    {TAG_DONE	    	    	    	}
	};

	dup_dobj = DupDiskObjectA(dobj,dup_tags);
    }
    else
    {
 	dup_dobj = NULL;
    }
    
    FreeStruct ((APTR)dobj, IconDesc);

    Close (icon);

    return dup_dobj;
    AROS_LIBFUNC_EXIT
} /* GetDiskObject */
