/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
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
	AROS_LHA(STRPTR,           name, A0),
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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)

    struct TagItem *tstate = tags,
                   *tag;
                   
    struct DiskObject *dob = NULL;
    
    LONG    defaultType       = -1;
    STRPTR  defaultName       = NULL;
    BOOL    failIfUnavailable = TRUE;
    LONG   *isDefaultIcon     = NULL;
    
    /* Parse taglist -------------------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case ICONGETA_GetDefaultType:
                defaultType = tag->ti_Data; 
                break;
                
            case ICONGETA_GetDefaultName:
                defaultName = (STRPTR) tag->ti_Data; 
                break;
                
            case ICONGETA_FailIfUnavailable:
                failIfUnavailable = tag->ti_Data;
                break;
                
            case ICONGETA_IsDefaultIcon:
                isDefaultIcon  = (LONG *) tag->ti_Data;
                *isDefaultIcon = FALSE;
                // FIXME: implement
                break;
                
                
            case ICONGETA_GetPaletteMappedIcon:
                // FIXME: implement
                break;
                
            case ICONGETA_RemapIcon:
                // FIXME: implement
                break;
                
            case ICONGETA_GenerateImageMasks:
                // FIXME: implement
                break;
                
            case ICONGETA_Screen:
                // FIXME: implement
                break;
            
            case ICONGETA_Label:
                // FIXME: implement
                break;
                
            case ICONA_ErrorCode:
                // FIXME: implement
                break;
        }
    }
    

   /* TODO: We use the old GetDiskObjectXXX() calls here,
    * but it would be better if those functions use
    * GetIconTagList() because this supersets all */

    if (defaultType != -1 || defaultName != NULL)
    {
	*isDefaultIcon = TRUE;
        
        /* the name argument have to be ignored */

	if (defaultName)
	{
	    char buf[360];
	    /* TODO: has aros snprintf()? */
	    sprintf(buf,"ENV:Sys/def_%s",defaultName);
	    dob = GetDiskObject(defaultName);
	}

	if (!dob && defaultType != -1)
	{
	    dob = GetDefDiskObject(defaultType);
	}
    }
    else
    {
	/* icon.library v44 uses a global identification hook
	 * to determine the filetype. We just call GetDiskObjectNew() for now */

	dob = GetDiskObject(name);
        
        if (dob == NULL && !failIfUnavailable)
        {
            *isDefaultIcon = TRUE;
	    dob = GetDiskObjectNew(name);
	}
    }

    return dob;

    AROS_LIBFUNC_EXIT
} /* GetIconRectangle */
